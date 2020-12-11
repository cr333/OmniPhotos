#include "PreprocessingApp.hpp"

#ifdef USE_CERES
	#include "SphereFitting.hpp"
#endif

#include "3rdParty/fs_std.hpp"

#include "Core/GUI/Dialog.hpp"
#include "Core/Loaders/ColmapLoader.hpp"
#include "Core/Loaders/OpenVSLAMLoader.hpp"
#include "Core/OpticalFlow/OpticalFlowApp.hpp"

#include "Utils/Exceptions.hpp"
#include "Utils/IOTools.hpp"
#include "Utils/Logger.hpp"
#include "Utils/Timer.hpp"
#include "Utils/Utils.hpp"

#include <thread>


using namespace std;


PreprocessingApp::PreprocessingApp(const std::string& pathToConfigYaml)
{
	appDataset = new CameraSetupDataset();
	appDataset->pathToConfigYAML = pathToConfigYaml;
}


PreprocessingApp::~PreprocessingApp()
{
	if (appDataset)
	{
		delete appDataset;
		appDataset = nullptr;
	}

	if (appActiveDataset)
	{
		delete appActiveDataset;
		appActiveDataset = nullptr;
	}
}


int PreprocessingApp::init()
{
	appSettings.readSettingsFromFile(appDataset->pathToConfigYAML, appDataset);
	appDataset->setCameraSetup(new CameraSetup(appSettings));

	// Initialise the multi-view geometry data loader object.
	switch (appSettings.sfmFormat)
	{
		case SfmFormat::Colmap:
		{
			sfmLoader = std::make_shared<ColmapLoader>();
			VLOG(1) << "Loading multi-view geometry data in COLMAP format.";
			break;
		}
		case SfmFormat::OpenVSLAM:
		{
			sfmLoader = std::make_shared<OpenVSLAMLoader>();
			VLOG(1) << "Loading multi-view geometry data in OpenVSLAM format.";
			break;
		}
		default:
			RUNTIME_EXCEPTION("SfM format not implemented.");
	}

	// Read SphereFitting settings.
	ConfigReader config;
	config.open(appDataset->pathToConfigYAML);
	sphereFittingSettings.readSettings(config.readNode("Preprocessing", "SphereFitting"));

	initDataset();
	processDataset();
	return -1;
}


void PreprocessingApp::rescalePointCloud()
{
	auto points = appActiveDataset->getWorldPointCloud()->getPoints();
	float scale = appActiveDataset->getCameraSetup()->getSettings()->physicalScale;
	VLOG(1) << "Rescaling point cloud by factor " << scale;

	Eigen::Matrix3f rotation = Eigen::Matrix3f::Identity();

	if (appSettings.changeBasis)
	{
		for (int j = 0; j < svdBases.size(); j++)
			rotation = svdBases[j] * rotation;
	}

	for (auto& p : *points)
	{
		p->pos = scale * rotation * p->pos;
	}
}


void PreprocessingApp::initDataset()
{
	appDataset->setSfmLoader(sfmLoader);
	bool sfmLoad = appDataset->load(&appSettings);

	if (!sfmLoad)
		RUNTIME_EXCEPTION("SfM loading failed");

	// Make sure the cache directory exists.
	if (fs::exists(appDataset->pathToCacheFolder))
	{
		string question = "The following cache folder already exists:\n" + appDataset->pathToCacheFolder + "\n\nDo you want to overwrite it?";
		QuestionDialog qDialog = QuestionDialog(question);
		qDialog.title = "Overwrite cache folder?";
		(void)qDialog.run();
		bool answer = qDialog.getBResult();
		if (answer)
		{
			LOG(INFO) << "Deleting cache directory: " << appDataset->pathToCacheFolder;

			// It turns out that asking Windows to delete a directory is but a polite request and will not always
			// be carried out immediately. This is a problem when we try to recreate the directory immediately below.
			// So we shall rename the directory first and then ask for deletion while we create a new cache directory.
			auto temp_name = appDataset->pathToCacheFolder + "_TO_BE_DELETED";
			fs::rename(appDataset->pathToCacheFolder, temp_name);
			removeFolder(temp_name);

			//// Wait for completion.
			//// This fixes a race condition between deleting the directory here and creating it again below.
			//while (fs::exists(appDataset->pathToCacheFolder))
			//	std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
		else
		{
			RUNTIME_EXCEPTION("Not overwriting existing cache directory. Choose a different name.");
		}
	}

	if (!fs::create_directories(appDataset->pathToCacheFolder))
		LOG(FATAL) << "Cache directory '" << appDataset->pathToCacheFolder << "' could not be created";

	appActiveDataset = new CameraSetupDataset(appDataset);

	// Change basis.
	updateCircle();
	rescaleDataset();

	// Set up TextureLoader.
	textureLoader = make_shared<TextureLoader>(new ImageLoader(appActiveDataset->getCameraSetup()->getCameras()));
}


void PreprocessingApp::rescaleDataset()
{
	std::vector<Camera*>* _cams = appActiveDataset->getCameraSetup()->getCameras();

	float avgDistance = 0;
	for (int i = 0; i < _cams->size(); i++)
	{
		avgDistance += (_cams->at(i)->getCentre()).norm();
	}
	avgDistance /= _cams->size();

	float radius = 100 * appSettings.circleRadius; // from m to cm
	float scaleFactor = radius / avgDistance;
	for (int i = 0; i < _cams->size(); i++)
	{
		Camera* cam = _cams->at(i);
		Eigen::Vector3f C = cam->getCentre();
		cam->setCentre(scaleFactor * C);
	}
	appSettings.physicalScale = scaleFactor;
}


void PreprocessingApp::processDataset() //automatic preprocessing executes this
{
	// Sorting and subsampling of cameras.
	updateNumberOfCameras();

#ifdef USE_CERES
	// Scene-adaptive proxy goemetry fitting.
	if (sphereFittingSettings.enabled)
		fitSphereMesh();
#endif

	// Compute optical flow between pairs of images.
	computeOpticalFlow();

	// Save dataset.
	appActiveDataset->save(&appSettings);
}


void PreprocessingApp::computeOpticalFlow()
{
	if (appSettings.computeOpticalFlow == 0)
	{
		LOG(INFO) << "Skipping optical flow computation";
		return;
	}

	FlowMethod _method = appSettings.opticalFlowMethod;

	int preset = 0;
	if (_method == FlowMethod::DIS)
		preset = 2;

	OpticalFlowApp* opticalFlow = new OpticalFlowApp(_method, preset, appSettings.downsampleFlow > 0);

	if (_method == FlowMethod::BroxCUDA)
		opticalFlow->init(_method, 0, &appSettings.broxFlowParams);

	opticalFlow->outputDirectory = appDataset->pathToCacheFolder;
	opticalFlow->readyToComputeFlowFields = false;
	opticalFlow->flowFieldsComputed = false;
	opticalFlow->shouldShutdown = false;
	opticalFlow->writeFlowIntoFile = true;
	opticalFlow->equirectWraparound = appSettings.useEquirectCamera;

	if (appSettings.downsampleFlow == 1)
		opticalFlow->downsampleFlow = true;
	else
		opticalFlow->downsampleFlow = false;

	int size = appActiveDataset->getCameraSetup()->getNumberOfCameras();

	Timer t1;
	try
	{
		ScopedTimer timer;
		std::vector<Camera*>& cameras = *appActiveDataset->getCameraSetup()->getCameras();

		for (int i = 0; i < size; i++)
		{
			LOG(INFO) << "Computing optical flow (" << (i + 1) << " of " << size << ")";
			opticalFlow->setPair(*cameras[i], *cameras[(i + 1) % size]);
			opticalFlow->run();
			opticalFlow->flowFieldsComputed = false;
		}

		LOG(INFO) << "Computed " << (2 * size) << " flow fields in "
		          << std::fixed << std::setprecision(2) << timer.getElapsedSeconds() << "s";

		appActiveDataset->forwardFlows = opticalFlow->getForwardFlows();
		appActiveDataset->backwardFlows = opticalFlow->getBackwardFlows();

		// The backward flows are from the next to the current frame, but need
		// them to be the backward flows from the current to the previous frames.
		// To fix that, we need to shift them along by one in a ring buffer fashion.
		std::vector<string> tmpBWFlows;
		tmpBWFlows.push_back(appActiveDataset->backwardFlows[size - 1]);
		for (int k = 0; k < size - 1; k++)
			tmpBWFlows.push_back(appActiveDataset->backwardFlows[k]);
		appActiveDataset->backwardFlows = tmpBWFlows;

		opticalFlow->shouldShutdown = true;
	}
	catch (const std::exception& e)
	{
		LOG(WARNING) << "Optical flow crashed: " << e.what();
	}
}


void PreprocessingApp::updateNumberOfCameras() //sub-sample camera setup
{
	LOG(INFO) << "Entering updateNumberOfCameras().";

	Eigen::Vector2i clip = appSettings.videoParams.getClip();
	int requestedNumberOfCameras = appSettings.numberOfCameras;

	int firstFrame = clip(0);
	int lastFrame = clip(1);

	//sorted by frame number
	//I have to be careful with the rescaling etc.
	//The originally loaded images should always remain in appDataset.
	//appActiveDataset must "deep copy" points from the appDataset.

	//Deep copy the appDataset camera vector
	//std::vector<Camera*>* sortedCameras = appDataset->camSetup->getCameras();
	std::vector<Camera*>* sortedCameras = new std::vector<Camera*>();

	//This is always the whole bunch loaded from the reconstruction
	std::vector<Camera*>* baseCameras = appDataset->getCameraSetup()->getCameras();
	for (size_t i = 0; i < baseCameras->size(); i++)
		sortedCameras->push_back(new Camera(*baseCameras->at(i)));

	std::vector<Camera*> _cams;

	//The subset must be taken according to frame number.
	//The sampling of the subset should happen according to phi
	//Now extract the subset
	for (int i = 0; i < sortedCameras->size(); i++)
	{
		// just get the data of the selected range of images
		int frameNumber = i; // sortedCameras->at(i)->frame; // TODO FIXME: this is confusing as it renumbers frames
		if (frameNumber < firstFrame || frameNumber > lastFrame)
			continue;
		_cams.push_back(sortedCameras->at(i));
	}

	bool load3DPoints = appSettings.load3DPoints;
	appActiveDataset->setCameraSetup(new CameraSetup(_cams, appSettings));

	// sorted by polar angle
	appActiveDataset->sortCameras();

	//full dataset size
	int datasetSize = (int)_cams.size();
	//I want to clip before further decision making
	bool loadAll = false;
	//special case, consider all images
	if (requestedNumberOfCameras >= datasetSize)
	{
		requestedNumberOfCameras = datasetSize;
		loadAll = true;
	}

	std::vector<Camera*> newCameras;
	if (loadAll)
	{
		for (int i = 0; i < _cams.size(); i++)
			newCameras.push_back(_cams.at(i));
	}
	else
	{
		// Sample a subset of cameras, ideally with the following properties:
		//   - smooth change in orientation between neighbours
		//   - more images for close-by objects preferable
		//   - maintain minimal angular resolution.

		// Generate equidistant points on the circle
		float* phis = new float[requestedNumberOfCameras];
		for (int i = 0; i < requestedNumberOfCameras; i++)
			phis[i] = float(i) * 360.f / requestedNumberOfCameras;

		// Now search for each point the closest camera.
		switch (appSettings.shapeSampling)
		{
			case 0:
			{
				sampleCameraCirclePhi(&newCameras, requestedNumberOfCameras, phis);
				break;
			}
			case 1:
			{
				sampleCameraCirclePhiAndEuclideanDistance(&newCameras, requestedNumberOfCameras, phis, 25, 50);
				break;
			}
			case 2:
			{
				sampleCameraCircleCreatePath(&newCameras, requestedNumberOfCameras, phis, 20, 0.1f);
				break;
			}
		}

		delete[] phis;

		if (newCameras.size() == 0)
			RUNTIME_EXCEPTION("newCameras vector empty!");

		if (newCameras.size() != requestedNumberOfCameras)
			LOG(WARNING) << "Number of sampled cameras (" << newCameras.size() << ") does not match requested number (" << requestedNumberOfCameras << ")";
	} // else from "if (loadAll)"

	appActiveDataset->setCameraSetup(new CameraSetup(newCameras, appSettings));

	appActiveDataset->sortCameras();

	if (load3DPoints)
		rescalePointCloud();

	// Load images.
	textureLoader->setCameras(appActiveDataset->getCameraSetup()->getCameras());
	if (!textureLoader->load())
		RUNTIME_EXCEPTION("Couldn't load all images.");
}


void PreprocessingApp::computeEigenspace(Eigen::Matrix3f* base, Eigen::Vector3f* values)
{
	int size = appActiveDataset->getCameraSetup()->getNumberOfCameras();

	// data matrix
	Eigen::MatrixXf mat(size, 3);
	Eigen::Vector3f avgUp = Eigen::Vector3f(0, 0, 0);
	//Eigen::Vector3f avgForward = Eigen::Vector3f(0, 0, 0);

	std::vector<Camera*>* _cams = appActiveDataset->getCameraSetup()->getCameras();
	for (int i = 0; i < _cams->size(); i++)
	{
		Camera* cam = _cams->at(i);
		avgUp += cam->getYDir();
		//avgForward += cam->getViewDir();
		mat.row(i) = cam->getCentre(); // for SVD
	}
	avgUp /= (float)_cams->size();
	avgUp.normalize();


	Eigen::Vector3f globalUp = Eigen::Vector3f(0, 1, 0);
	Eigen::Vector3f globalForward = Eigen::Vector3f(0, 0, -1);
	Eigen::Vector3f globalLeft = Eigen::Vector3f(-1, 0, 0);

	switch (appSettings.shapeFit)
	{
		case 0:
		{
			if (avgUp.dot(globalUp) < 0)
				avgUp *= -1.0f;

			Eigen::Vector3f left = avgUp.cross(globalForward);
			if (left.dot(globalLeft) < 0)
				left *= -1.0f;

			Eigen::Vector3f forward = avgUp.cross(left);
			if (forward.dot(globalForward) < 0)
				forward *= -1.0f;

			*base = concatenateColumnVectors(left.normalized(), avgUp.normalized(), forward.normalized());
			//base->transposeInPlace();
			(*values)(0) = 0;
			(*values)(1) = 0;
			(*values)(2) = 0;

			break;
		}
		case 1:
		{
			////////////////
			////SVD
			Eigen::JacobiSVD<Eigen::MatrixXf> m_svd(mat, Eigen::ComputeFullV | Eigen::ComputeFullU);
			Eigen::VectorXf ev = m_svd.singularValues();
			//eigenvalues
			//sorted descending from big to small
			(*values)(0) = ev(0);
			(*values)(1) = ev(1);
			(*values)(2) = ev(2);

			Eigen::MatrixXf U = m_svd.matrixU();
			Eigen::MatrixXf V = m_svd.matrixV();
			Eigen::MatrixXf SV = m_svd.singularValues().asDiagonal();
			Eigen::MatrixXf res(mat.rows(), mat.cols());
			Eigen::Matrix3f eigenvectors(V);

			Eigen::Vector3f svd_normal = eigenvectors.col(2);
			Eigen::Vector3f svd_forward = eigenvectors.col(1);
			Eigen::Vector3f svd_left = eigenvectors.col(0);


			if (svd_normal.dot(globalUp) < 0)
				svd_normal *= -1;
			svd_left = svd_normal.cross(globalForward);
			svd_forward = svd_left.cross(svd_normal);
			if (svd_forward.dot(globalForward) < 0)
				svd_forward *= -1;
			svd_left = svd_normal.cross(svd_forward);
			*base = concatenateColumnVectors(svd_left.normalized(), svd_normal.normalized(), svd_forward.normalized());
			break;
		}
	}

	LOG(INFO) << "Eigenspace Basis (det=" << base->determinant() << "):\n " << *base;
}


void PreprocessingApp::changeBasis() //depends on Eigenspace
{
	Eigen::Matrix3f base = svdBase;
	svdBases.push_back(base);
	centroids.push_back(appActiveDataset->originalCentroid);
	std::vector<Camera*>* _cams = appActiveDataset->getCameraSetup()->getCameras();
	//ConstantMatrices m;
	for (int i = 0; i < _cams->size(); i++)
	{
		Camera* cam = _cams->at(i);
		Eigen::Matrix3f oldRotation = cam->getRotation();
		//		Eigen::Vector3f oldCentre = cam->getCentre();
		Eigen::Vector3f translation = cam->getTranslation();
		//printf("Determinant of svdBase is: %f \n", base.determinant());
		//printf("Determinant of oldRotation is: %f \n", oldRotation.determinant());

		Eigen::Matrix3f newRotation = oldRotation * base.inverse();
		//Eigen::Matrix3f newRotation = /*oldRotation**/base*oldRotation;
		LOG(INFO) << "Determinant of newRotation is: " << newRotation.determinant();
		Eigen::Vector3f newCentre = -newRotation.transpose() * translation;
		cam->setExtrinsics(newRotation, newCentre);
	}
}


void PreprocessingApp::fitCircleToCameras()
{
	computeEigenspace(&svdBase, &svdValues);

	Eigen::Vector3f svdUp = svdBase.col(1);
	Eigen::Vector3f svdForward = svdBase.col(2);
	LOG(INFO) << "Up vector of fitted circle is ( " << svdUp.transpose() << ").";

	*appActiveDataset->circle = Circle(
	    appActiveDataset->getCameraSetup()->getCentroid(), svdUp.normalized(),
	    svdForward.normalized(),
	    100 * appSettings.circleRadius // from m to cm
	);
}


void PreprocessingApp::updateCircle()
{
	appActiveDataset->centreDatasetAtOrigin();

	if (initialRun)
	{
		originalCentroid = appActiveDataset->originalCentroid;
		initialRun = false;
	}
	else
	{
		originalCentroid -= appActiveDataset->originalCentroid;
	}
	//std::cout << "new setup centroid: " << originalCentroid << std::endl;

	fitCircleToCameras();
	Eigen::Vector3f c = appActiveDataset->getCameraSetup()->getCentroid();
	LOG(INFO) << "Centroid before basis change: (" << c << ").";
	if (appSettings.changeBasis > 0)
	{
		changeBasis();
		fitCircleToCameras();
	}
	//fitShapeToCameraManifold();
	Eigen::Matrix3f orientation = appActiveDataset->circle->getBase();
	*appActiveDataset->cylinder = Cylinder(c, orientation, 100 * appSettings.cylinderRadius /* from m to cm */, 10000.0);
}


/*
phis: contains camera's phis (in ascending order since the cameras were sorted according to their phi value)
M: is the requested number of new cameras
*/
void PreprocessingApp::sampleCameraCirclePhi(std::vector<Camera*>* _cameras, int M, float* phis)
{
	int datasetSize = appActiveDataset->getCameraSetup()->getNumberOfCameras();

	// Cameras are not allowed to be taken multiple times.
	bool* camerasTaken = new bool[datasetSize];
	for (int i = 0; i < datasetSize; i++)
		camerasTaken[i] = false;

	//whenever I got a camera which is further away than the last one, STOP
	//choose the next camera to the points generated on the circle
	for (int j = 0; j < M; j++)
	{
		int i = 0;
		bool found = false;
		float newSign = 1.0;

		while ((!found) && (i < datasetSize))
		{
			Camera* cam = appActiveDataset->getCameraSetup()->getCameras()->at(i);
			if (!camerasTaken[i])
			{
				float lastSign = newSign;
				float newDistance = (phis[j] - cam->getPhi());
				if (newDistance < 0.1f)
					newSign = -1.0;
				else
					newSign = 1.0;

				// Sign change: we passed the ideal one
				if (lastSign * newSign < 0)
				{
					found = true;
					camerasTaken[i] = true;
					_cameras->push_back(cam);
				}
			}
			i++;
		}
	}

	delete[] camerasTaken;
}


void PreprocessingApp::sampleCameraCirclePhiAndEuclideanDistance(std::vector<Camera*>* _cameras, int M, float* phis, int neighbourhood, float maxDist)
{
	int datasetSize = appDataset->getCameraSetup()->getNumberOfCameras();

	//cameras are not allowed to be taken multiple times.
	bool* camerasTaken = new bool[datasetSize];
	for (int i = 0; i < datasetSize; i++)
		camerasTaken[i] = false;

	float eps = 0.001f;
	//whenever I got a camera which is further away than the last one, STOP
	//choose the next camera to the points generated on the circle
	for (int j = 0; j < M; j++)
	{
		int i = 0;
		float pointPhi = phis[j];
		bool found = false;
		float newSign = 1.0;
		float lastSign = 1.0;
		while ((!found) && (i < datasetSize))
		{
			Camera* cam = appDataset->getCameraSetup()->getCameras()->at(i);
			if (!camerasTaken[i])
			{
				lastSign = newSign;
				float newDistance = (pointPhi - cam->getPhi());
				if (newDistance < eps)
					newSign = -1.0;
				else
				{
					if (newDistance < 0)
						newSign = -1.0;
					else
						newSign = 1.0;
				}
				//sign change
				//we passed the ideal one
				if (lastSign * newSign < 0)
				{
					//now search for the best one in a neighbourhood
					int radius = neighbourhood / 2;
					float closestCam = 1000000.f;
					int bestCam = -1;

					Eigen::Vector3f circleVertPos = appActiveDataset->circle->generateVertex(degToRad(pointPhi));
					for (int k = -radius; k < radius; k++)
					{
						int l = (i + k + datasetSize) % datasetSize;
						if (!camerasTaken[l])
						{
							Camera* cam = appDataset->getCameraSetup()->getCameras()->at(l);

							Eigen::Vector3f c = cam->getCentre();
							float dist = abs(circleVertPos.y() - c.y());
							//float dist = (circleVertPos - c).norm();
							if (dist < closestCam)
							{
								closestCam = dist;
								bestCam = l;
							}
						}
					}
					if (closestCam < maxDist)
					{
						cam = appDataset->getCameraSetup()->getCameras()->at(bestCam);
						//bestCamPhi = cam->phi;
						found = true;
						camerasTaken[bestCam] = true;
						_cameras->push_back(cam);
					}
					else
					{
						LOG(WARNING) << "Should not happen.";
						found = true;
					}
				}
			}
			i++;
		}
	}

	delete[] camerasTaken;
}


inline float evaluateGaussian(float x, float sigma, float mu)
{
	return expf(-((x - mu) * (x - mu)) / (2.0f * sigma * sigma));
}



//Think of a worm
float computePathWeight(Camera& candidate, Camera& last, Eigen::Vector3f lastDir, float avgSegmentLength)
{
	//candidates are always in front of us
	//we can't accept big difference in y
	Eigen::Vector3f dir = (candidate.getCentre() - last.getCentre()).normalized();

	//float diffDirY = abs(dir.y() - lastDir.y());
	float diffDir = 1.0f - dir.dot(lastDir);
	float muDir = 0.0f;
	float sigmaDir = 10000.0f;
	float weightDir = evaluateGaussian(diffDir, sigmaDir, muDir);
	//
	float len = dir.norm();
	float muLen = avgSegmentLength;
	float sigmaLen = 100000.0f;
	float weightLen = evaluateGaussian(len, sigmaLen, muLen);

	return weightDir * weightLen;
	//return weightLen;
}


/*
Most importantly:
Look in the same direction
almost the same importance has the same up vector
*/
float computeOrientationWeight(Camera& candidate, Camera& last)
{
	Eigen::Vector3f cV = candidate.getZDir();
	Eigen::Vector3f cU = candidate.getYDir();

	Eigen::Vector3f lV = last.getZDir();
	Eigen::Vector3f lU = last.getYDir();

	float diffForward = 1.0f - cV.dot(lV); //mu = 0
	float muV = 0.0f;
	float sigmaV = 10000.0f;
	float weightV = evaluateGaussian(diffForward, sigmaV, muV);


	float diffUp = 1.0f - cU.dot(lU); //mu = 0
	float muU = 0.0f;
	float sigmaU = 10000.0f;
	float weightU = evaluateGaussian(diffUp, sigmaU, muU);

	return weightV * weightU;
}


void PreprocessingApp::sampleCameraCircleCreatePath(std::vector<Camera*>* _cameras, int M, float* phis, int neighbourhood, float minScore)
{
	int datasetSize = appActiveDataset->getCameraSetup()->getNumberOfCameras();

	//cameras are not allowed to be taken multiple times.
	bool* camerasTaken = new bool[datasetSize];
	//camerasTaken[0] = true;
	for (int i = 0; i < datasetSize; i++)
		camerasTaken[i] = false;

	float eps = 0.001f;
	Camera* lastCam = nullptr;
	//circle circumference / M
	float avgSegmentLength = appActiveDataset->circle->getRadius() * 2.0f * 3.1415f / M;
	//whenever I got a camera which is further away than the last one, STOP
	//choose the next camera to the points generated on the circle
	for (int j = 0; j < M; j++)
	{
		int i = 0;
		float pointPhi = phis[j];
		bool found = false;
		float newSign = 1.0;
		float lastSign = 1.0;

		while ((!found) && (i < datasetSize))
		{
			Camera* cam = appDataset->getCameraSetup()->getCameras()->at(i);
			if (!camerasTaken[i])
			{
				lastSign = newSign;
				float newDistance = (pointPhi - cam->getPhi());
				if (newDistance < eps)
					newSign = -1.0;
				else
				{
					if (newDistance < 0)
						newSign = -1.0;
					else
						newSign = 1.0;
				}
				//sign change
				//we passed the ideal one
				if (lastSign * newSign < 0)
				{
					//now search for the best one in a neighbourhood
					float bestCamScore = 0.0f;
					int bestCamIndex = -1;

					float weightDistance;
					Eigen::Vector3f lastDir = Eigen::Vector3f(1, 0, 0);
					float weightOrientation;
					float candidateScore;
					int radius = neighbourhood / 2;
					for (int k = -radius; k <= radius; k++) // just look ahead
					{
						int l = (i + k + datasetSize) % datasetSize;
						if (!camerasTaken[l])
						{
							Camera* cam = appDataset->getCameraSetup()->getCameras()->at(l);
							if (lastCam) //init -> Take the first one
							{
								//compute weights
								//weightDistance = computeDistanceWeight(*cam, *lastCam);

								weightDistance = computePathWeight(*cam, *lastCam, lastDir, avgSegmentLength);
								weightOrientation = computeOrientationWeight(*cam, *lastCam);
								candidateScore = weightDistance * weightOrientation;

								if (candidateScore > bestCamScore) // keeping track of the "best" around
								{
									bestCamScore = candidateScore;
									bestCamIndex = l;
								}
							}
							else //init
							{
								bestCamScore = 1.0;
								bestCamIndex = l;
							}
						}
					}
					if (bestCamScore > 0.0f) //maxDist = 0.5, accept everything greater than that (goes to 1)
					{
						cam = appDataset->getCameraSetup()->getCameras()->at(bestCamIndex);

						found = true;
						camerasTaken[bestCamIndex] = true;
						_cameras->push_back(cam);
						if (lastCam)
						{
							lastDir = cam->getCentre() - lastCam->getCentre();
							lastDir.normalize();
						}
						lastCam = cam;
					}
					//else
					//{
					//	printf("Should not happen.\n");
					//	found = true;
					//}
				}
			}
			i++;
		}
	}

	delete[] camerasTaken;
}


#ifdef USE_CERES

void PreprocessingApp::fitSphereMesh()
{
	if (appActiveDataset == nullptr || appActiveDataset->getWorldPointCloud().get() == nullptr)
	{
		LOG(WARNING) << "Cannot fit sphere mesh -- no point cloud loaded";
		return;
	}

	// Get world points.
	std::vector<Point3D*>& points3D = *appActiveDataset->getWorldPointCloud()->getPoints();
	std::vector<Eigen::Vector3f> points;
	std::vector<Eigen::Vector3f> points_disparity;
	int points_skipped = 0;
	for (auto& point : points3D)
	{
		float distance = point->pos.norm();
		if (distance < 10 || distance > 10000) // [cm]
		{
			points_skipped++;
			continue;
		}
		points.push_back(point->pos);
		points_disparity.push_back(point->pos / pow(distance, 2));
	}

	if (points_skipped > 0)
		LOG(INFO) << "Skipped " << points_skipped << " points ("
		          << std::fixed << std::setprecision(2) << (100. * points_skipped) / points3D.size() << "%)";

	// Run sphere fitting for all combinations of settings.
	for (auto robust_loss : sphereFittingSettings.robust_data_loss)
	{
		for (double robust_scale : sphereFittingSettings.robust_data_loss_scale)
		{
			for (double data_weight : sphereFittingSettings.data_weight)
			{
				for (double smoothness_weight : sphereFittingSettings.smoothness_weight)
				{
					for (double prior_weight : sphereFittingSettings.prior_weight)
					{
						// Apply sphere fitting settings.
						SphereFitting fit(sphereFittingSettings.polar_steps, sphereFittingSettings.azimuth_steps);
						fit.robust_data_loss = static_cast<SphereFitting::RobustLoss>(robust_loss);
						fit.robust_data_loss_scale = robust_scale;
						fit.data_weight = data_weight;
						fit.smoothness_weight = smoothness_weight;
						fit.prior_weight = prior_weight;

						// Encode all settings in filename.
						stringstream suffix;
						//suffix << "-" << sphereFittingSettings.azimuth_steps << "x" << sphereFittingSettings.polar_steps;
						suffix << "-loss" << robust_loss;
						suffix << "-scale" << robust_scale;
						suffix << "-data" << data_weight;
						suffix << "-sm" << smoothness_weight;
						suffix << "-pr" << prior_weight;

						//{
						//	// First solve for points directly.
						//	fit.points = points;
						//	fit.solveProblem();

						//	string filename_prefix = (fs::path(appDataset->pathToCacheFolder) / ("spherefit-depth" + suffix.str())).generic_string();
						//	fit.exportSphereMeshAndPoints(fit.est_depth_map, fit.points, filename_prefix, 50, 2000);
						//}

						{
							// Then solve using inverse depth.
							fit.points = points_disparity;
							fit.solveProblem();

							//string filename_prefix = (fs::path(appDataset->pathToCacheFolder) / ("spherefit-disparity" + params)).generic_string();
							//fit.exportSphereMeshAndPoints(fit.est_depth_map, fit.points, filename_prefix);

							// Convert from disparity (inverse depth) to depth map.
							auto depth_map = fit.est_depth_map.cwiseInverse();
							string filename_prefix = (fs::path(appDataset->pathToCacheFolder) / ("spherefit-d2d" + suffix.str())).generic_string();
							fit.exportSphereMeshAndPoints(depth_map, points, filename_prefix, 50, 2000);

							//// Clamp depth to 1..20 metres.
							//auto clamped_depth_map = depth_map.cwiseMax(100).cwiseMin(2000);
							//filename_prefix = fs::path(appDataset->pathToCacheFolder) / ("spherefit-disparity2depth-clamped" + params);
							//fit.exportSphereMeshAndPoints(depth_map, points, filename_prefix);
						}
					}
				}
			}
		}
	}
}

#endif // USE_CERES

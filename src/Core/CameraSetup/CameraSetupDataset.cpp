#include "CameraSetupDataset.hpp"

#include "3rdParty/fs_std.hpp"
#include "3rdParty/json/json.h"

#include "Core/CameraSetup/CameraSetup.hpp"
#include "Core/GUI/Dialog.hpp"

#include "Utils/Exceptions.hpp"
#include "Utils/Logger.hpp"
#include "Utils/STLutils.hpp"
#include "Utils/Utils.hpp"

#include <opencv2/opencv.hpp>

#include <fstream>


using namespace Eigen;
using namespace std;


//TODO: Cleanup way how point cloud is initialized. Smells like duplicated code -> use new interface.


CameraSetupDataset::CameraSetupDataset()
{
	camera_setup = new CameraSetup();

	name = "CameraSetupDataset";

	//definitely dataset
	circle = make_shared<Circle>();
	//should all adapt to the "up" direction of the circle.
	cylinder = make_shared<Cylinder>();
	sphere = make_shared<Sphere>(60, 30, 1000);
}


CameraSetupDataset::CameraSetupDataset(CameraSetup* _camSetup) :
    CameraSetupDataset()
{
	camera_setup = _camSetup;
}


CameraSetupDataset::CameraSetupDataset(CameraSetupDataset* _dataset) :
    CameraSetupDataset()
{
	camera_setup = new CameraSetup(*_dataset->camera_setup->getCameras(), *_dataset->camera_setup->getSettings());

	//TODO:
	//I don't like that
	circle = _dataset->circle;
	cylinder = _dataset->cylinder;
	sphere = _dataset->sphere;
	setWorldPointCloud(_dataset->getWorldPointCloud());

	pathToConfigYAML = _dataset->pathToConfigYAML;

	name = _dataset->name;
	workingDirectory = _dataset->workingDirectory;
	pathToInputFolder = _dataset->pathToInputFolder;
	pathToConfigFolder = _dataset->pathToConfigFolder;
	pathToCacheFolder = _dataset->pathToCacheFolder;
}


CameraSetupDataset::~CameraSetupDataset()
{
	// Clear cameras.
	std::vector<Camera*>& cam_list = *camera_setup->getCameras();
	for (int idx = 0; idx < cam_list.size(); idx++)
	{
		delete cam_list[idx];
		cam_list[idx] = nullptr;
	}
	cam_list.clear();

	// Clear flows.
	forwardFlows.clear();
	backwardFlows.clear();

	//TODO: point cloud?
}


CameraSetup* CameraSetupDataset::getCameraSetup()
{
	return camera_setup;
}


void CameraSetupDataset::setCameraSetup(CameraSetup* _camera_setup)
{
	if (!camera_setup)
		camera_setup = new CameraSetup(*_camera_setup);

	*camera_setup = *_camera_setup;
}


bool CameraSetupDataset::loadFromCache(CameraSetupSettings* settings)
{
	// Open the config.yaml and read settings.
	cv::FileStorage fs(settings->configFile, cv::FileStorage::READ);

	fs["General"]["CacheFolder"] >> pathToCacheFolder;

	fs["Camera"]["Equirectangular"] >> settings->useEquirectCamera;

	fs["Geometry"]["MaxNumber3DPoints"] >> settings->maxLoad3Dpoints;

	fs["Viewer"]["UseOpticalFlow"] >> settings->useOpticalFlow;
	settings->opticalFlowLoaded = (settings->useOpticalFlow > 0);
	fs["Viewer"]["UsePointCloud"] >> settings->usePointCloud;
	if (!fs["Viewer"]["LookAtDirection"].empty()) fs["Viewer"]["LookAtDirection"] >> settings->lookAtDirection;
	if (!fs["Viewer"]["LookAtDistance"].empty())
	{
		fs["Viewer"]["LookAtDistance"] >> settings->lookAtDistance;
		settings->lookAtDistance *= 100; // convert from [m] to [cm] since value is in meters in config file.
	}
	// Set up paths to different directories.
	auto datasetDirectory = fs::path(settings->configFile).parent_path().parent_path();
	workingDirectory = datasetDirectory.generic_string();
	name = datasetDirectory.filename().generic_string();

	pathToCacheFolder = ltrim(pathToCacheFolder, "\\/"); // remove leading slashes/backslashes
	pathToInputFolder = (datasetDirectory / "Input").generic_string();
	pathToCacheFolder = (datasetDirectory / "Cache" / pathToCacheFolder).generic_string();
	pathToConfigFolder = (datasetDirectory / "Config").generic_string();

	if (fs::is_directory(pathToCacheFolder) == false)
	{
		string msg = "Cache folder '" + pathToCacheFolder + "' not found.";
		RUNTIME_EXCEPTION(msg);
	}

	if (fs::is_empty(pathToCacheFolder))
	{
		string msg = "The cache folder '" + pathToCacheFolder + "' is empty.";
		RUNTIME_EXCEPTION(msg);
	}

	// Search for the first Preprocessing*.json file.
	for (auto& itr : fs::directory_iterator(pathToCacheFolder))
	{
		string filename = itr.path().filename().string();
		if (startsWith(filename, "Preprocessing"))
		{
			settings->preprocessingSetupFilename = pathToCacheFolder + "/" + filename;
			break;
		}
	}

	if (settings->preprocessingSetupFilename.empty())
	{
		string msg = "Preprocessing*.json file not found in '" + pathToCacheFolder + "'.";
		RUNTIME_EXCEPTION(msg);
	}

	// Parse the Preprocessing*-nnnn.json file.
	string filename = settings->preprocessingSetupFilename;
	LOG(INFO) << "Read data from cache file: " << filename;

	// Extract the number of cameras from its filename.
	settings->numberOfCameras = extractLastNDigitsFromFilenameAsInt(filename, 4);

	// Parse the JSON.
	Json::Value root;
	Json::Reader reader;
	string fileContent = readFile(filename);
	bool parsedSuccess = reader.parse(fileContent, root, false);
	if (!parsedSuccess)
	{
		LOG(WARNING) << "Reading from JSON file '" << filename << "' failed.\n"
		             << "Error message: " << reader.getFormattedErrorMessages();
		return false;
	}

	// Utility function to read Eigen vectors from JSON.
	auto json_to_eigen_vec = [](Json::Value value, string x, string y, string z) {
		return Vector3f(value[x].asFloat(), value[y].asFloat(), value[z].asFloat());
	};

	settings->downsampleFlow = root["Dataset"]["Flow"]["Downsampled"].asInt();

	// Read the fitted camera circle.
	auto& camera_circle_json = root["Dataset"]["CameraCircle"];
	Vector3f centroid = json_to_eigen_vec(camera_circle_json["Centroid"],  "X",  "Y",  "Z");
	Vector3f up       = json_to_eigen_vec(camera_circle_json["Up"],       "Ux", "Uy", "Uz");
	Vector3f forward  = json_to_eigen_vec(camera_circle_json["Forward"],  "Fx", "Fy", "Fz");
	settings->circleRadius = camera_circle_json["Radius"].asFloat();

	VLOG(1) << "Setting up fitted circle.";
	circle->setCentre(centroid);
	circle->setNormal(up);
	circle->setForward(forward);
	circle->setRadius(settings->circleRadius);
	circle->computeTangentPlane(up, forward);

	// Read the cylinder.
	auto& cylinder_json = root["Dataset"]["ProjectionCylinder"];
	centroid = json_to_eigen_vec(cylinder_json["Centroid"],  "X",  "Y",  "Z");
	up       = json_to_eigen_vec(cylinder_json["Up"],       "Ux", "Uy", "Uz");
	settings->cylinderRadius = cylinder_json["Radius"].asFloat();

	VLOG(1) << "Setting up cylinder.";
	cylinder->setCentre(centroid);
	cylinder->setUp(up);
	cylinder->setForward(forward);
	cylinder->setHeight(100000);
	cylinder->init(settings->circle_resolution);
	cylinder->setRadius(settings->cylinderRadius);

	setCameraSetup(new CameraSetup());
	std::vector<Camera*>* cameras = getCameraSetup()->getCameras();
	readCamerasCSV(filename, cameras, settings);


	//TODO: Wanna keep this?
	///////////////////////////////////////////////////////////////////////////////
	//check whether image paths make sense
	//if not do all the dialog mumbo jumbo and write new files

	string imgName = cameras->at(0)->imageName;
	bool change = updateCSV(imgName, cameras, settings);
	if (change)
	{
		cameras->clear();
		forwardFlows.clear();
		backwardFlows.clear();
		readCamerasCSV(filename, cameras, settings);
	}
	///////////////////////////////////////////////////////////////////////////////


	// Read in point cloud.
	int pointCloudExists = root["Dataset"]["PointCloud"]["Exists"].asInt();
	settings->load3DPoints = false;
	if (pointCloudExists > 0 && settings->usePointCloud > 0)
	{
		VLOG(1) << "Setting up pointCloud.";
		string cloudFilename = pathToCacheFolder + "/PointCloud.csv";
		setWorldPointCloud(std::make_shared<PointCloud>(cloudFilename));
		settings->load3DPoints = true;
	}

	VLOG(1) << "Finished reading json file.";
	return 0;
}


void CameraSetupDataset::save(CameraSetupSettings* settings)
{
	std::string filename = generateCacheFilename();
	LOG(INFO) << "Serialising dataset into file: " << filename;
	Json::Value root;

	// Save cameras to CSV
	std::vector<Camera*>* cameras = getCameraSetup()->getCameras();
	writeCamerasCSV(pathToCacheFolder + "/", cameras, settings->computeOpticalFlow > 0);

	// Save point cloud to CSV
	auto pointCloud = getWorldPointCloud();
	if (pointCloud)
	{
		root["Dataset"]["PointCloud"]["Exists"] = 1;
		root["Dataset"]["PointCloud"]["Size"] = pointCloud->numberOfVertices();

		const string cloudFilename = pathToCacheFolder + "/PointCloud.csv";
		std::ofstream file(cloudFilename.c_str(), std::ios::trunc);
		for (auto p : *(pointCloud->getPoints()))
		{
			file << p->id << "," << p->pos.x() << "," << p->pos.y() << "," << p->pos.z() << "," << p->colour.x() << "," << p->colour.y() << "," << p->colour.z() << "," << p->error << "\n";
		}
		file.close();
	}
	else
	{
		root["Dataset"]["PointCloud"]["Exists"] = 0;
	}

	// Relation between image resolutions used in SfM and in the actual applications
	root["Dataset"]["IntrinsicScale"] = settings->intrinsicScale;
	root["Dataset"]["Scaling"] = getCameraSetup()->getSettings()->physicalScale;

	// Cylinder
	root["Dataset"]["ProjectionCylinder"]["Centroid"]["X"] = cylinder->getCentre()(0);
	root["Dataset"]["ProjectionCylinder"]["Centroid"]["Y"] = cylinder->getCentre()(1);
	root["Dataset"]["ProjectionCylinder"]["Centroid"]["Z"] = cylinder->getCentre()(2);
	root["Dataset"]["ProjectionCylinder"]["Up"]["Ux"] = cylinder->getUp()(0);
	root["Dataset"]["ProjectionCylinder"]["Up"]["Uy"] = cylinder->getUp()(1);
	root["Dataset"]["ProjectionCylinder"]["Up"]["Uz"] = cylinder->getUp()(2);
	root["Dataset"]["ProjectionCylinder"]["Radius"] = cylinder->getRadius();
	root["Dataset"]["ProjectionCylinder"]["Height"] = cylinder->getHeight();

	// Circle
	root["Dataset"]["CameraCircle"]["Centroid"]["X"] = circle->getCentre()(0);
	root["Dataset"]["CameraCircle"]["Centroid"]["Y"] = circle->getCentre()(1);
	root["Dataset"]["CameraCircle"]["Centroid"]["Z"] = circle->getCentre()(2);
	root["Dataset"]["CameraCircle"]["Up"]["Ux"] = circle->getNormal().x();
	root["Dataset"]["CameraCircle"]["Up"]["Uy"] = circle->getNormal().y();
	root["Dataset"]["CameraCircle"]["Up"]["Uz"] = circle->getNormal().z();
	root["Dataset"]["CameraCircle"]["Radius"] = circle->getRadius();
	root["Dataset"]["CameraCircle"]["Forward"]["Fx"] = circle->getForward().x();
	root["Dataset"]["CameraCircle"]["Forward"]["Fy"] = circle->getForward().y();
	root["Dataset"]["CameraCircle"]["Forward"]["Fz"] = circle->getForward().z();

	// Optical flow parameters
	root["Dataset"]["Flow"]["Brox"]["Alpha"] = settings->broxFlowParams.alpha;
	root["Dataset"]["Flow"]["Brox"]["Gamma"] = settings->broxFlowParams.gamma;
	root["Dataset"]["Flow"]["Brox"]["InnerIterations"] = settings->broxFlowParams.innerIterations;
	root["Dataset"]["Flow"]["Brox"]["OuterIterations"] = settings->broxFlowParams.outerIterations;
	root["Dataset"]["Flow"]["Brox"]["ScaleFactor"] = settings->broxFlowParams.scaleFactor;
	root["Dataset"]["Flow"]["Brox"]["SolverIterations"] = settings->broxFlowParams.solverIterations;
	root["Dataset"]["Flow"]["Downsampled"] = settings->downsampleFlow;

	// Write JSON
	std::ofstream file(filename.c_str(), std::ios::trunc);
	file << root;
	file.close();
}


bool CameraSetupDataset::updateCSV(std::string imgName, std::vector<Camera*>* cameras,
                                   CameraSetupSettings* settings)
{
	bool change = false;
	std::string expectedWorkingDirectory;

	//Dataset was moved! It was created somewhere else.
	if (!pathStartsWith(imgName, workingDirectory))
	{
		change = true;
		string path, file;
		//See whether there are images in the working Directory
		splitFilename(imgName, &path, &file);
		expectedWorkingDirectory = path;

		//Try to build a new filepath:
		string newInputPath = workingDirectory + "/Input";
		string newImgName = newInputPath + "/" + file;

		if (fs::exists(newImgName))
		{
			//located input images.
			LOG(INFO) << "Found new image location at: " << newInputPath;
		}
		else
		{
			//couldn't located images. Where are they? -> dialog
			LOG(INFO) << "Couldn't find new image location at: " << newInputPath;
			ChooseFolderDialog* cfDialog = new ChooseFolderDialog("Where are the input images located for this dataset?");
			bool validFolderFound = false;
			while (!validFolderFound)
			{
				newInputPath = cfDialog->run();
				newImgName = newInputPath + file;
				if (fs::exists(newImgName))
					validFolderFound; // TODO(TB): What is this meant to do?
			}
			delete cfDialog;
		}
		//Got the new input directory here or I am trapped in Limbo -> images tick

		//I should locate other stuff as well, what about flow?
		string flowName = forwardFlows[0];
		//Dataset was moved! It was created somewhere else.
		if (!startsWith(flowName, pathToCacheFolder))
		{
			splitFilename(flowName, &path, &file);
			//Try to build a new filepath:
			string newFlowPath = pathToCacheFolder;
			string newFlowName = newFlowPath + "/" + file;
			if (fs::exists(newFlowName))
			{
				//located input images.
				LOG(INFO) << "Found new float location at: " << newFlowPath;
			}
			else
			{
				//couldn't located images. Where are they? -> dialog
				LOG(WARNING) << "Couldn't find new image location at: " << newFlowPath;
				ChooseFolderDialog* cfDialog = new ChooseFolderDialog("Where are the input images located for this dataset?");
				bool validFolderFound = false;
				while (!validFolderFound)
				{
					newFlowPath = cfDialog->run();
					newFlowName = newFlowPath + file;
					if (fs::exists(newFlowName))
						validFolderFound; // TODO(TB): What is this meant to do?
				}
				delete cfDialog;
			}
		}
		//flows tick.

		//I will let the user know that I want overwrite existing files

		//Do I want to make a backup?
		string awesomeQuestion = "Dataset was created here: " + expectedWorkingDirectory + ".\n";
		awesomeQuestion += "I will update the dataset files to welcome the dataset at its new location.\n";
		awesomeQuestion += "Do you wish to backup the original files?";
		QuestionDialog* qDialog = new QuestionDialog(awesomeQuestion);
		qDialog->run();
		bool bResult = qDialog->getBResult();
		delete qDialog;

		if (bResult)
		{
			//copy "cameras.csv" to "cameras-origin.csv"
			string sourcePath = pathToCacheFolder + "/Cameras.csv";
			string oldPath;
			string oldFileName;
			splitFilename(sourcePath, &oldPath, &oldFileName);
			string oldFile = oldPath + oldFileName;

			string extension = extractFileExtension(oldFile);
			string nameWithoutExtension = stripExtensionFromFilename(oldFileName);
			string newFileName = nameWithoutExtension + "-origin" + extension;

			string newFile = pathToCacheFolder + "/" + newFileName;

			fs::copy_file(oldFile, newFile, fs::copy_options::overwrite_existing);
		}

		//We go line-wise over cameras.csv and change image and flow paths
		string newFlowPath = pathToCacheFolder;
		//-> NO. I will adapt the cameras vector and write a new cameras.csv!
		for (int i = 0; i < cameras->size(); i++)
		{
			string imgName = cameras->at(i)->imageName;
			string path, file;
			splitFilename(imgName, &path, &file);
			//newInputPath = workingDirectory + "/Input";
			string newFileName = newInputPath + "/" + file;
			cameras->at(i)->imageName = newFileName;

			string forwardFlowNameI = forwardFlows[i];
			string backwardFlowNameI = backwardFlows[i];

			string flowName = forwardFlowNameI;
			splitFilename(flowName, &path, &file);
			string newFlowName = newFlowPath + "/" + file;
			forwardFlows[i] = newFlowName;

			flowName = backwardFlowNameI;
			splitFilename(flowName, &path, &file);
			newFlowName = newFlowPath + "/" + file;
			backwardFlows[i] = newFlowName;
		}
		writeCamerasCSV(pathToCacheFolder + "/", cameras, settings->useOpticalFlow > 0);

		//todo: We could generally store locally and build the paths everytime we run.
		//On the other hand, a dataset moves once and is then used hundreds of times.
		//So having the paths hardcoded is nice and fast, especially in debug mode.
	}
	return change;
}


int CameraSetupDataset::scanDatasetDirectory(const std::string& datasetDir, CameraSetupDataset& datasetInfo)
{
	if (!fs::is_directory(datasetDir))
	{
		LOG(ERROR) << "Specified dataset root directory is not a directory: " << datasetDir;
		return -1;
	}

	// 0) Set basic dataset information.
	fs::path datasetRoot(datasetDir);
	datasetInfo.name = datasetRoot.filename().string();
	datasetInfo.workingDirectory = fs::absolute(datasetRoot).generic_string() + "/"; // absolute path
	datasetInfo.pathToInputFolder = datasetInfo.workingDirectory + "Input/";
	datasetInfo.pathToCacheFolder = datasetInfo.workingDirectory + "Cache/";
	datasetInfo.pathToConfigFolder = datasetInfo.workingDirectory + "Config/";
	datasetInfo.pathToConfigYAML.clear();

	// 1) Check if dataset is preprocessed, i.e. the cache folder is not empty.
	if (fs::is_directory(datasetInfo.pathToCacheFolder) == false)
	{
		VLOG(1) << "Dataset '" << datasetInfo.name << "' has no cache folder at: " << datasetInfo.pathToCacheFolder;
		return -1;
	}
	if (fs::is_empty(datasetInfo.pathToCacheFolder))
	{
		VLOG(1) << "Dataset '" << datasetInfo.name << "' cache folder is empty: " << datasetInfo.pathToCacheFolder;
		return -1;
	}

	// 2) Check for a dataset YAML configuration file.
	for (const auto& configEntry : fs::directory_iterator(datasetInfo.pathToConfigFolder))
	{
		if (configEntry.path().extension() == ".yaml" && configEntry.path().string().find("-viewer") != std::string::npos)
		{
			datasetInfo.pathToConfigYAML = configEntry.path().generic_string();
			break;
		}
	}

	if (datasetInfo.pathToConfigYAML.empty())
	{
		VLOG(1) << "Cannot find '*viewer*.yaml' in '" << datasetInfo.pathToConfigFolder << "'";
		return -1;
	}

	return scanDatasetConfig(datasetInfo.pathToConfigYAML, datasetInfo);
}


int CameraSetupDataset::scanDatasetConfig(const std::string& configPath, CameraSetupDataset& datasetInfo)
{
	if (!fs::exists(configPath))
	{
		LOG(ERROR) << "Specified config file does not exist: " << configPath;
		return -1;
	}

	// 0) Set basic dataset information.
	fs::path datasetRoot = fs::path(configPath).parent_path().parent_path();
	datasetInfo.name = datasetRoot.filename().string();
	datasetInfo.workingDirectory = fs::absolute(datasetRoot).generic_string() + "/"; // absolute path
	datasetInfo.pathToInputFolder = datasetInfo.workingDirectory + "Input/";
	datasetInfo.pathToCacheFolder = datasetInfo.workingDirectory + "Cache/";
	datasetInfo.pathToConfigFolder = datasetInfo.workingDirectory + "Config/";
	datasetInfo.pathToConfigYAML = configPath;

	// 1) Check if dataset is preprocessed, i.e. the cache folder is not empty.
	if (fs::is_directory(datasetInfo.pathToCacheFolder) == false)
	{
		VLOG(1) << "Dataset '" << datasetInfo.name << "' has no cache folder at: " << datasetInfo.pathToCacheFolder;
		return -1;
	}
	if (fs::is_empty(datasetInfo.pathToCacheFolder))
	{
		VLOG(1) << "Dataset '" << datasetInfo.name << "' cache folder is empty: " << datasetInfo.pathToCacheFolder;
		return -1;
	}

	// 2) Load thumbnail image.
	for (const auto& inputEntry : fs::directory_iterator(datasetInfo.pathToInputFolder))
	{
		const auto& ext = inputEntry.path().extension();
		if (ext == ".jpg" || ext == ".jpeg" || ext == ".png")
		{
			std::string image_path = inputEntry.path().generic_string();
			datasetInfo.thumbImage = cv::imread(image_path);

			if (datasetInfo.thumbImage.empty())
			{
				LOG(WARNING) << "Failed to load thumbnail image from '" << image_path << "'";
				continue;
			}
			else // image loaded successfully
			{
				cv::resize(datasetInfo.thumbImage, datasetInfo.thumbImage, cv::Size(240, 120), 0, 0, cv::INTER_AREA);
				break;
			}
		}
	}

	return 0;
}


std::vector<CameraSetupDataset> CameraSetupDataset::scanDatasets(const string& rootFolder)
{
	std::vector<CameraSetupDataset> datasetInfoList;
	for (const auto& entry : fs::directory_iterator(rootFolder))
	{
		if (!entry.is_directory())
			continue;

		std::string datasetPath = entry.path().string();
		CameraSetupDataset datasetInfo;
		if (scanDatasetDirectory(datasetPath, datasetInfo) < 0)
			continue;

		// Add dataset to list.
		VLOG(1) << "Found available dataset '" << datasetInfo.name << "'";
		datasetInfoList.push_back(datasetInfo);
	}

	// Sort datasets alphabetically by name.
	sort(datasetInfoList.begin(), datasetInfoList.end(),
	     [](const CameraSetupDataset& dataset1, const CameraSetupDataset& dataset2) {
		     return dataset1.name.compare(dataset2.name) < 0;
	     });

	return datasetInfoList;
}


bool CameraSetupDataset::load(CameraSetupSettings* settings)
{
	int ret = -1;
	sfmLoader->setImageNameDigits(settings->digits);
	ret = sfmLoader->loadData(
	    this,
	    settings->cameraGeometryFile,
	    pathToInputFolder,
	    settings->intrinsicScale,
	    settings->load3DPoints,
	    settings->loadSparsePointCloud);

	return (ret > -1);
}


std::string CameraSetupDataset::generateCacheFilename()
{
	std::stringstream ss;
	ss << std::setw(4) << std::setfill('0') << getCameraSetup()->getNumberOfCameras();
	std::string jsonFilename = pathToCacheFolder + "/PreprocessingSetup-" + ss.str() + ".json";
	return jsonFilename;
}


void CameraSetupDataset::centreDatasetAtOrigin()
{
	Eigen::IOFormat CleanFmt(4, 0, ", ", "\n", "[", "]");

	// Find the centroid of the camera circle.
	originalCentroid = getCameraSetup()->getCentroid();
	LOG(INFO) << "Camera centroid before centering: " << originalCentroid.transpose().format(CleanFmt);

	// Centre camera circle on origin.
	vector<Camera*>* _cams = getCameraSetup()->getCameras();
	for (Camera* cam : *_cams)
	{
		cam->setCentre(cam->getCentre() - originalCentroid);
	}
	PointCloud* world = getWorldPointCloud().get();
	// Move point cloud similarly.
	if (world)
	{
		std::vector<Point3D*>* points = world->getPoints();
		for (auto point : *points)
		{
			point->pos = point->pos - originalCentroid;
		}
	}
	else
	{
		LOG(INFO) << "No point cloud available, so no centering";
	}

	const Eigen::Vector3f newCentroid = getCameraSetup()->getCentroid();
	LOG(INFO) << "Camera centroid after centering: " << newCentroid.transpose().format(CleanFmt);
	if (newCentroid.norm() > 0.001f)
		RUNTIME_EXCEPTION("Camera centroid not in world origin!");
}


void CameraSetupDataset::sortCameras()
{
	LOG(INFO) << "Sorting cameras by azimuth angle phi";
	auto& cameras = *getCameraSetup()->getCameras();
	sort(cameras.begin(), cameras.end(), [](Camera* cam1, Camera* cam2) {
		return (cam1->getPhi() < cam2->getPhi());
	});
}


void CameraSetupDataset::readCamerasCSV(std::string filename, std::vector<Camera*>* cameras, CameraSetupSettings* settings)
{
	//Reading cameras
	string folder, name;
	splitFilename(filename, &folder, &name);
	string camerasFilename = folder + "Cameras.csv";

	// Check if the file can be read.
	std::ifstream ifsCameras(camerasFilename);
	if (!ifsCameras.is_open())
	{
		string msg = "File '" + camerasFilename + "' could not be opened.";
		RUNTIME_EXCEPTION(msg);
	}

	// Read and parse the file line by line.
	//	int count = 0;
	std::string line;
	while (getline(ifsCameras, line))
	{
		Camera* cam = new Camera();

		// Parse camera line.
		std::vector<string> tokens = split(line, ',');

		if (tokens.size() == 23) //without flow
		{
			cam->frame = stoi(tokens[0]);
			Eigen::Matrix3f K;
			K << stof(tokens[1]), stof(tokens[2]), stof(tokens[3]),
			     stof(tokens[4]), stof(tokens[5]), stof(tokens[6]),
			     stof(tokens[7]), stof(tokens[8]), stof(tokens[9]);
			cam->setIntrinsics(K);

			Eigen::Matrix3f R;
			R << stof(tokens[10]), stof(tokens[11]), stof(tokens[12]),
			     stof(tokens[13]), stof(tokens[14]), stof(tokens[15]),
			     stof(tokens[16]), stof(tokens[17]), stof(tokens[18]);
			Eigen::Vector3f C;
			C << stof(tokens[19]), stof(tokens[20]), stof(tokens[21]);
			cam->setExtrinsics(R, C);

			// Handle relative paths as relative to the cache directory.
			const auto cacheDir = fs::path(pathToCacheFolder);
			cam->imageName = (cacheDir / tokens[22]).string();

			cameras->push_back(cam);
		}

		// old style camera.csv
		if (tokens.size() == 24)
		{
			cam->frame = 0;
			Eigen::Matrix3f K;
			K << stof(tokens[0]), stof(tokens[1]), stof(tokens[2]),
			     stof(tokens[3]), stof(tokens[4]), stof(tokens[5]),
			     stof(tokens[6]), stof(tokens[7]), stof(tokens[8]);
			cam->setIntrinsics(K);

			Eigen::Matrix3f R;
			R << stof(tokens[9]), stof(tokens[10]), stof(tokens[11]),
			     stof(tokens[12]), stof(tokens[13]), stof(tokens[14]),
			     stof(tokens[15]), stof(tokens[16]), stof(tokens[17]);
			Eigen::Vector3f C;
			C << stof(tokens[18]), stof(tokens[19]), stof(tokens[20]);
			cam->setExtrinsics(R, C);

			// Handle relative paths as relative to the cache directory.
			const auto cacheDir = fs::path(pathToCacheFolder);
			cam->imageName = (cacheDir / tokens[21]).string();
			forwardFlows.push_back((cacheDir / tokens[22]).string());
			backwardFlows.push_back((cacheDir / tokens[23]).string());

			cameras->push_back(cam);
		}

		//added frame nr for keeping stuff in order
		if (tokens.size() == 25)
		{
			cam->frame = stoi(tokens[0]);
			Eigen::Matrix3f K;
			K << stof(tokens[1]), stof(tokens[2]), stof(tokens[3]),
			     stof(tokens[4]), stof(tokens[5]), stof(tokens[6]),
			     stof(tokens[7]), stof(tokens[8]), stof(tokens[9]);
			cam->setIntrinsics(K);

			Eigen::Matrix3f R;
			R << stof(tokens[10]), stof(tokens[11]), stof(tokens[12]),
			     stof(tokens[13]), stof(tokens[14]), stof(tokens[15]),
			     stof(tokens[16]), stof(tokens[17]), stof(tokens[18]);
			Eigen::Vector3f C;
			C << stof(tokens[19]), stof(tokens[20]), stof(tokens[21]);
			cam->setExtrinsics(R, C);

			// Handle relative paths as relative to the cache directory.
			const auto cacheDir = fs::path(pathToCacheFolder);
			cam->imageName = (cacheDir / tokens[22]).string();
			forwardFlows.push_back((cacheDir / tokens[23]).string());
			backwardFlows.push_back((cacheDir / tokens[24]).string());

			cameras->push_back(cam);
		}
	}
	ifsCameras.close();

	// Set camera intrinsics to identity for equirectangular cameras.
	if (settings->useEquirectCamera)
	{
		for (auto& cam : *cameras)
		{
			cam->setIntrinsics(Eigen::Matrix3f::Identity());
		}
	}
}


void CameraSetupDataset::writeCamerasCSV(std::string folder, std::vector<Camera*>* cameras, bool computeOpticalFlow)
{
	string camerasFilename = folder + "Cameras.csv";
	if (fs::exists(camerasFilename))
		std::remove(camerasFilename.c_str());

	std::ofstream camerasFile(camerasFilename.c_str(), std::fstream::out);

	// Cameras.csv file format -- one line per camera:
	// frame, K(0,0), K(0,1), K(0,2), ..., K(2,2), R(0,0), R(0,1), R(0,2), ..., R(2,2), C(0), C(1), C(2), imageName, FlowPrevious, FlowNext
	for (int i = 0; i < cameras->size(); i++)
	{
		Camera* cam = cameras->at(i);

		// Frame number
		camerasFile << cam->frame << ",";

		// Camera intrinsics -- K
		Eigen::Matrix3f K = cam->getIntrinsics();
		for (int j = 0; j < 3; j++)
			for (int k = 0; k < 3; k++)
				camerasFile << K(j, k) << ",";

		// Camera rotation -- R
		Eigen::Matrix3f R = cam->getRotation();
		for (int j = 0; j < 3; j++)
			for (int k = 0; k < 3; k++)
				camerasFile << R(j, k) << ",";

		// Camera centre -- C
		Eigen::Vector3f C = cam->getCentre();
		camerasFile << C(0) << "," << C(1) << "," << C(2) << ",";

		// Image filename
		camerasFile << createFilenameRelativeToFolder(cam->imageName, folder);

		// Optical flow fields
		if (computeOpticalFlow)
			camerasFile << "," << createFilenameRelativeToFolder(forwardFlows[i], folder)
			            << "," << createFilenameRelativeToFolder(backwardFlows[i], folder);

		camerasFile << "\n";
	}
	camerasFile.close();
}

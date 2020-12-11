#include "ColmapLoader.hpp"

#include "3rdParty/fs_std.hpp"

#include "Core/CameraSetup/CameraSetupDataset.hpp"

#include "Utils/Exceptions.hpp"
#include "Utils/Logger.hpp"
#include "Utils/STLutils.hpp"
#include "Utils/Timer.hpp"
#include "Utils/Utils.hpp"

#include <fstream>

using namespace std;


ColmapLoader::ColmapLoader()
{
	ConstantMatrices constMatrices;
	colmapTransform = constMatrices.invYZ;
}


void ColmapLoader::readPathsToColmapModelFiles(const string& sfmFile)
{
	LOG(INFO) << "Reading COLMAP model files from '" << sfmFile << "'";

	ifstream reader(sfmFile);
	if (!reader.is_open())
	{
		RUNTIME_EXCEPTION(string("SfM file '") + sfmFile + "' could not be opened.");
	}

	// Read the paths of the different COLMAP files.
	// If paths are given (non-empty), then interpret relative paths w.r.t. the config directory.
	fs::path configDirectory = fs::path(sfmFile).parent_path();
	try
	{
		getline(reader, pathToCameras);
		if (!pathToCameras.empty())
			pathToCameras = fs::canonical(configDirectory / pathToCameras).generic_string();

		getline(reader, pathToImages);
		if (!pathToImages.empty())
			pathToImages = fs::canonical(configDirectory / pathToImages).generic_string();

		getline(reader, pathTo3DPoints);
		if (!pathTo3DPoints.empty())
			pathTo3DPoints = fs::canonical(configDirectory / pathTo3DPoints).generic_string();
	}
	catch (const exception& e)
	{
		// fs::canonical will throw a FileNotFound error if the path does not exist and therefore
		// this is a useful exception to catch and elaborate on.

		string errormsg = e.what();
		RUNTIME_EXCEPTION(errormsg + "\nCheck " + sfmFile);
	}

	reader.close();
}


// Fiddling filenames together
void ColmapLoader::updateCameraImagePaths(CameraSetup* camSetup, const string& pathToInputFolder)
{
	for (int i = 0; i < camSetup->getNumberOfCameras(); i++)
	{
		Camera* cam = camSetup->getCameras()->at(i);
		cam->imageName = pathToInputFolder + "/" + cam->imageName;
	}
}


// interface, called by Application
int ColmapLoader::loadData(
    CameraSetupDataset* dataset,
    const string& sfmFile,
    const string& pathToInputFolder,
    float intrinsicScale,
    bool load3DPoints,
    bool loadSparsePointCloud)
{
	CameraSetup* camSetup = dataset->getCameraSetup();
	readPathsToColmapModelFiles(sfmFile);
	frameInterval = camSetup->getSettings()->videoParams.frameInterval;

	Timer t1;
	t1.startTiming();
	readCameraGeometryColmap(camSetup, intrinsicScale);
	t1.endTiming();
	//read the direction of capture +1 CW -1 CCW

	LOG(INFO) << "Finished computation after " << t1.getElapsedTime().count() << "s";
	updateCameraImagePaths(camSetup, pathToInputFolder);

	if (load3DPoints)
	{
		maxLoad3DPoints = camSetup->getSettings()->maxLoad3Dpoints; //not good. readColmapFiles() might have an optionFile or something.
		Timer t2;
		t2.startTiming();
		vector<Colmap3DPoint> points = readColmap3DPoints(loadSparsePointCloud, camSetup->getSettings()->max3DPointError);
		t2.endTiming();
		LOG(INFO) << "Finished computation after " << t2.getElapsedTime().count() << "s.";
		vector<Point3D*> convertedPoints = convert3DPoints(points); //my internal format

		dataset->setWorldPointCloud(make_shared<PointCloud>(convertedPoints));
	}

	LOG(INFO) << "Loading COLMAP files finished!";
	return 0;
}


//colour channel converts 0..255 -> 0..1
vector<Point3D*> ColmapLoader::convert3DPoints(vector<Colmap3DPoint> colmapPoints)
{
	vector<Point3D*> convertedPoints;
	for (int i = 0; i < colmapPoints.size(); i++)
	{
		Colmap3DPoint cp = colmapPoints[i];

		Point3D* p = new Point3D(cp.pointID,
		                         Eigen::Vector3f(cp.x, cp.y, cp.z),
		                         Eigen::Vector3f(cp.r / 255.0f, cp.g / 255.0f, cp.b / 255.0f),
		                         cp.error);

		convertedPoints.push_back(p);
	}
	return convertedPoints;
}


void ColmapLoader::readCameraGeometryColmap(CameraSetup* camSetup, float intrinsicScale)
{
	vector<Camera*>* sfmCameras = camSetup->getCameras();

	// Read camera intrinsics (should be just a single camera).
	vector<ColmapCamera> colmapCams = readColmapCameras();

	// Read camera extrinsics for each image.
	vector<ColmapImage> colmapImages = readColmapImages();

	// Shared intrinsics for all images -> same camera, optionally scaled using "intrinsicScale"
	ColmapCamera intr = colmapCams[0]; // only one camera per dataset
	Eigen::Matrix3f K = Eigen::Matrix3f::Identity();
	float focalLength = intrinsicScale * intr.focal;
	Eigen::Vector2f principalPoint = intrinsicScale * Eigen::Vector2f(intr.pX, intr.pY);
	K(0, 0) = focalLength;
	K(1, 1) = focalLength;
	K(0, 2) = principalPoint(0);
	K(1, 2) = principalPoint(1);

	LOG(INFO) << "Mapping COLMAP to internal data structures.";

	for (int i = 0; i < (int)colmapImages.size(); i++)
	{
		ColmapImage image = colmapImages[i];

		// COLMAP camera orientation: x-right, y-down, z-forward.
		Eigen::Matrix3f R = Eigen::Quaternionf(image.qw, image.qx, image.qy, image.qz).toRotationMatrix();

		// Compute camera centre from rotation + translation (standard H&Z stuff).
		Eigen::Vector3f translation(image.tx, image.ty, image.tz);
		Eigen::Vector3f C = -R.transpose() * translation;

		// Next, convert the world coordinate system from COLMAP to OpenGL.
		R = R * colmapTransform;
		C = colmapTransform * C;

		Camera* cam = new Camera(K, R, C);
		setCamNames(cam, image.name, imageNameDigits);
		sfmCameras->push_back(cam);
	}
}


vector<ColmapCamera> ColmapLoader::readColmapCameras()
{
	LOG(INFO) << "Reading cameras from COLMAP ... ";

	// Collect all intrinsic information -> usually just one camera
	vector<ColmapCamera> sfmCameras;

	// Check if the file can be read.
	ifstream ifsCameras(pathToCameras);
	if (!ifsCameras.is_open())
		RUNTIME_EXCEPTION(string("File '") + pathToCameras + " could not be opened.");

	// Read and parse the file line by line.
	string line;
	while (getline(ifsCameras, line))
	{
		// Skip comments.
		if (line[0] == '#')
			continue;

		// Parse camera line.
		vector<string> tokens = split(line, ' ');

		ColmapCamera camera;
		camera.type = tokens[1]; // skip the first token (camera ID)
		if (camera.type == string("SIMPLE_PINHOLE"))
		{
			camera.dimX = stoi(tokens[2]);
			camera.dimY = stoi(tokens[3]);
			camera.focal = stof(tokens[4]);
			camera.pX = stof(tokens[5]);
			camera.pY = stof(tokens[6]);
			sfmCameras.push_back(camera);
		}
		else
		{
			LOG(ERROR) << "Error in ColmapLoader::readColmapCameras: "
			           << "Camera type '" << camera.type << "' is not implemented (only 'SIMPLE_PINHOLE' at the moment).";
		}
	}
	ifsCameras.close();

	return sfmCameras;
}


vector<ColmapImage> ColmapLoader::readColmapImages()
{
	LOG(INFO) << "Loading COLMAP images from '" << pathToImages << "'";

	vector<ColmapImage> images;

	// Check if the file can be read.
	ifstream ifsImages(pathToImages);
	if (!ifsImages.is_open())
		RUNTIME_EXCEPTION(string("File '") + pathToImages + "' could not be opened.");

	ColmapImage* image = NULL;

	// Read and parse the file line by line.
	int lineCount = 0;
	int entryNumber = -1;

	string line;
	while (getline(ifsImages, line))
	{
		// Skip comments.
		if (line[0] == '#')
			continue;

		// I just care about poses, not feature points
		if (lineCount % 2 == 0)
		{
			entryNumber++;
			if ((entryNumber % frameInterval) == 0)
			{
				// Parse camera line.
				vector<string> tokens = split(line, ' ');
				image = new ColmapImage();
				image->imageID = stoi(tokens[0]);
				image->qw = stof(tokens[1]);
				image->qx = stof(tokens[2]);
				image->qy = stof(tokens[3]);
				image->qz = stof(tokens[4]);
				image->tx = stof(tokens[5]);
				image->ty = stof(tokens[6]);
				image->tz = stof(tokens[7]);
				image->camID = stoi(tokens[8]);
				image->name = tokens[9];
			}
		}

		if (image)
		{
			images.push_back(*image);
			image = NULL;
		}

		lineCount++;
	}
	ifsImages.close();

	return images;
}


vector<Colmap3DPoint> ColmapLoader::readColmap3DPoints(bool loadSparsePointCloud, float maxError)
{
	LOG(INFO) << "Reading 3D points from COLMAP.";
	string pathTo3DPointsTmp = pathTo3DPoints;
	if (!loadSparsePointCloud)
	{
		string cloudFile = stripExtensionFromFilename(pathTo3DPoints);
		cloudFile = cloudFile + "-dense.txt";
		pathTo3DPointsTmp = cloudFile;
	}

	// Check if the file can be read.
	ifstream ifsPoints(pathTo3DPointsTmp);
	if (!ifsPoints.is_open())
	{
		RUNTIME_EXCEPTION("ColmapLoader::readColmap3DPoints: File '" + pathTo3DPointsTmp + "' could not be opened.");
	}

	// Read and parse the file line by line.
	string line;
	int count = 0;
	vector<Colmap3DPoint> points;
	while (getline(ifsPoints, line))
	{
		// Stop after maximum number of points.
		if (count >= maxLoad3DPoints)
			break;

		// Skip comments.
		if (line[0] == '#')
			continue;

		// Parse camera line.
		Colmap3DPoint point;

		vector<string> tokens = split(line, ' ');
		point.error = stof(tokens[7]);
		if (point.error >= maxError)
			continue;

		point.pointID = stoi(tokens[0]);

		point.x = stof(tokens[1]);
		point.y = stof(tokens[2]);
		point.z = stof(tokens[3]);

		// TODO: Move colmapTransform to convert3DPoints, so that this function just reads the raw data
		//       and can be made static (easier to reuse elsewhere).
		// apply same transform as when loading geometry
		Eigen::Vector3f p = Eigen::Vector3f(point.x, point.y, point.z);
		p = colmapTransform * p;
		point.x = p.x();
		point.y = p.y();
		point.z = p.z();

		point.r = stof(tokens[4]);
		point.g = stof(tokens[5]);
		point.b = stof(tokens[6]);

		points.push_back(point);
		count++;
	}

	ifsPoints.close();
	return points;
}


// extracting frame number from filename and sets filename itself
void ColmapLoader::setCamNames(Camera* cam, string name, int len)
{
	cam->imageName = name;
	size_t pos = name.find("/");

	// Stripping folder from image name
	// I wonder why it's actually there...
	if (pos != string::npos)
		cam->imageName = name.substr(pos + 1, name.length() - pos - 1);

	if (contains(name, ".jpg"))
	{
		pos = name.find(".jpg");
	}
	else if (contains(name, ".png"))
	{
		pos = name.find(".png");
	}
	else
	{
		RUNTIME_EXCEPTION("Only jpg and png as input image");
	}

	name = name.substr(pos - len, len);
	cam->frame = stoi(name);
}

#include "OpenVSLAMLoader.hpp"

#include "3rdParty/fs_std.hpp"

#include "Core/CameraSetup/CameraSetupDataset.hpp"

#include "Utils/Exceptions.hpp"
#include "Utils/Logger.hpp"
#include "Utils/STLutils.hpp"
#include "Utils/Timer.hpp"
#include "Utils/Utils.hpp"

#include <nlohmann/json.hpp>

#include <fstream>

using namespace std;


OpenVSLAMLoader::OpenVSLAMLoader()
{
	ConstantMatrices constMatrices;
	openVSLAMTransform = constMatrices.invYZ;
}


void OpenVSLAMLoader::setCamNames(Camera* cam, string name, const int len)
{
	cam->imageName = name;
	size_t pos = name.find("/");

	// Stripping folder from image name, I wonder why it's actually there
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
		std::string msg = "Only *.jpg and *.png supported for input images.";
		LOG(WARNING) << msg;
		RUNTIME_EXCEPTION(msg);
	}

	name = name.substr(pos - len, len);
	cam->frame = std::stoi(name);
}


void OpenVSLAMLoader::loadCameraIntrinsicParameters(const std::string& pathToCameras,
                                                    int& camera_dimX,
                                                    int& camera_dimY,
                                                    float& camera_focal,
                                                    float& camera_pX,
                                                    float& camera_pY,
                                                    bool& useEquirectImages)
{
	LOG(INFO) << "Reading OpenVSLAM cameras from '" << pathToCameras << "'";

	// Check if the file can be read.
	std::ifstream ifsCameras(pathToCameras);
	if (!ifsCameras.is_open())
	{
		std::string msg = "File " + pathToCameras + " could not be opened.";
		LOG(WARNING) << msg;
		RUNTIME_EXCEPTION(msg);
	}

	// Read and parse the file line by line to collect all intrinsic information -> usually just one camera
	std::string line;
	int cameraNumber = 0;
	while (getline(ifsCameras, line))
	{
		// Skip comments.
		if (line[0] == '#')
			continue;

		// Parse camera line.
		std::vector<string> tokens = split(line, ' ');
		if (tokens.size() == 0)
			continue;

		std::string camera_type = tokens[1]; // skip the first token (camera ID)
		if (camera_type == "SIMPLE_PINHOLE" || camera_type == "EQUIRECTANGULAR")
		{
			VLOG(1) << "The Camera" << cameraNumber << "'s type is " << camera_type;
			cameraNumber++;
			camera_dimX = stoi(tokens[2]);
			camera_dimY = stoi(tokens[3]);
			camera_focal = stof(tokens[4]);
			camera_pX = stof(tokens[5]);
			camera_pY = stof(tokens[6]);

			if (camera_type == "EQUIRECTANGULAR")
				useEquirectImages = true;
			else
				useEquirectImages = false;
		}
		else
		{
			std::string msg = "Camera type " + camera_type + " is not implemented (only SIMPLE_PINHOLE or EQUIRECTANGULAR).";
			LOG(WARNING) << msg;
			RUNTIME_EXCEPTION(msg);
		}
	}
	ifsCameras.close();

	if (cameraNumber != 1)
		LOG(ERROR) << "There are " << cameraNumber << " camera intrinsic parameter in the file " << pathToCameras;
}


void OpenVSLAMLoader::loadCameraParameters(CameraSetup* camSetup,
                                           const float intrinsicScale,
                                           const std::string& pathToCameras,
                                           const std::string& pathToImages,
                                           bool& useEquirectImages)
{
	// 0) Read camera intrinsics (should be just a single camera).
	// Only set intrinsics for normal cameras. Shared intrinsics for all images -> same camera
	int camera_dimX;
	int camera_dimY;
	float camera_focal;
	float camera_pX;
	float camera_pY;
	loadCameraIntrinsicParameters(pathToCameras, camera_dimX, camera_dimY, camera_focal, camera_pX, camera_pY, useEquirectImages);

	Eigen::Matrix3f K = Eigen::Matrix3f::Identity();
	if (!useEquirectImages)
	{
		// pin-hole image
		// Scale intrinsics (optional).
		float focalLength = intrinsicScale * camera_focal;
		Eigen::Vector2f principalPoint = intrinsicScale * Eigen::Vector2f(camera_pX, camera_pY);

		K(0, 0) = focalLength;
		K(1, 1) = focalLength;
		K(0, 2) = principalPoint(0);
		K(1, 2) = principalPoint(1);
	}

	// 2) Read camera extrinsic parameters for each image.
	std::ifstream ifsImages(pathToImages);
	if (!ifsImages.is_open()) // Check if the file can be read.
	{
		std::string msg = "File '" + pathToImages + "' could not be opened. File";
		LOG(WARNING) << msg;
		RUNTIME_EXCEPTION(msg);
	}

	// Read and parse the file line by line.
	std::string line;
	std::vector<Camera*>* sfmCameras = camSetup->getCameras();
	while (getline(ifsImages, line))
	{
		std::vector<string> tokens = split(line, ' ');

		// Skip comments.
		if (line[0] == '#')
			continue;

		int imageID = stoi(tokens[0]);
		if (imageID < 0)
			continue;

		std::string image_name = tokens[1];

		// load each image's camera pose information
		float image_tx = stof(tokens[2]);
		float image_ty = stof(tokens[3]);
		float image_tz = stof(tokens[4]);
		float image_qx = stof(tokens[5]);
		float image_qy = stof(tokens[6]);
		float image_qz = stof(tokens[7]);
		float image_qw = stof(tokens[8]);

		// We use OpenVSLAM's "TUM" format export, which is not given in the same [R t] format
		// as COLMAP's world-to-camera transform, but as its inverse, the camera-to-world
		// transform [R t]^-1 = [R^T C]. So we need to invert this to match our conventions (R, C).
		Eigen::Matrix3f R = Eigen::Quaternionf(image_qw, image_qx, image_qy, image_qz).toRotationMatrix().transpose();
		Eigen::Vector3f C(image_tx, image_ty, image_tz);

		// Next, convert the world coordinate system from OpenVSLAM to OpenGL.
		R = R * openVSLAMTransform;
		C = openVSLAMTransform * C;

		Camera* cam;
		cam = new Camera(K, R, C);
		setCamNames(cam, image_name, imageNameDigits);
		sfmCameras->push_back(cam);
	}
	ifsImages.close();
}


int OpenVSLAMLoader::loadData(
    CameraSetupDataset* dataset,
    const std::string& sfmFile,
    const std::string& pathToInputFolder,
    const float intrinsicScale,
    const bool load3DPoints,
    const bool loadSparsePointCloud)
{
	// 0) Load OpenVSLAM output file path list.
	std::string pathToCameras;
	std::string pathToImages;
	std::string pathTo3DPoints;

	CameraSetup* camSetup = dataset->getCameraSetup();
	readPathsFromModelFiles(sfmFile, pathToCameras, pathToImages, pathTo3DPoints);

	// 1) Load each image's camera pose & camera parameters.
	bool useEquirectImages;
	loadCameraParameters(camSetup, intrinsicScale, pathToCameras, pathToImages, useEquirectImages);
	// update color image path
	for (int i = 0; i < camSetup->getNumberOfCameras(); i++)
	{
		Camera* cam = camSetup->getCameras()->at(i);
		cam->imageName = pathToInputFolder + "/" + cam->imageName; // update color image path
	}

	// 2) Load point cloud.
	if (load3DPoints && loadSparsePointCloud)
	{
		std::vector<Point3D*> points;
		int maxLoad3DPoints = camSetup->getSettings()->maxLoad3Dpoints;
		this->load3DPoints(pathTo3DPoints, points, maxLoad3DPoints);
		dataset->setWorldPointCloud(std::make_shared<PointCloud>(points));
	}

	return 0;
}


void OpenVSLAMLoader::readPathsFromModelFiles(const std::string& slamResultListPath,
                                              std::string& pathToCameras,
                                              std::string& pathToImages,
                                              std::string& pathTo3DPoints)
{
	LOG(INFO) << "Reading OpenVSLAM model files from '" << slamResultListPath << "'";

	ifstream reader(slamResultListPath);
	if (!reader.is_open())
	{
		RUNTIME_EXCEPTION(string("SfM file '") + slamResultListPath + "' could not be opened.");
	}

	// Read the paths of the different OpenVSLAM files.
	// If paths are given (non-empty), then interpret relative paths w.r.t. the config directory.
	fs::path configDirectory = fs::path(slamResultListPath).parent_path();
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

		std::string errormsg = e.what();
		RUNTIME_EXCEPTION(errormsg + "\nCheck " + slamResultListPath);
	}

	reader.close();
}


void OpenVSLAMLoader::load3DPoints(const std::string& filePath, std::vector<Point3D*>& pointCloud, int maxLoad3Dpoints)
{
	LOG(INFO) << "Reading OpenVSLAM points from '" << filePath << "'";

	// 1. Load binary bytes.
	std::ifstream ifs(filePath, std::ios::in | std::ios::binary);
	if (!ifs.is_open())
	{
		RUNTIME_EXCEPTION("Cannot load the file at " + filePath);
	}

	std::vector<uint8_t> msgpack;
	while (true)
	{
		uint8_t buffer;
		ifs.read(reinterpret_cast<char*>(&buffer), sizeof(uint8_t));
		if (ifs.eof())
		{
			break;
		}
		msgpack.push_back(buffer);
	}
	ifs.close();

	// 2. Parse into JSON.
	const auto json = nlohmann::json::from_msgpack(msgpack);

	// 3. Load database.
	int count = 0;
	const auto json_landmarks = json.at("landmarks");
	for (const auto& json_id_landmark : json_landmarks.items())
	{
		// Stop after maximum number of points.
		count++;
		if (maxLoad3Dpoints > 0 && count >= maxLoad3Dpoints)
			break;

		const auto id = std::stoi(json_id_landmark.key());
		assert(0 <= id);
		const auto json_landmark = json_id_landmark.value();

		float point_x = json_landmark.at("pos_w")[0];
		float point_y = json_landmark.at("pos_w")[1];
		float point_z = json_landmark.at("pos_w")[2];

		// Apply the same transform as when loading geometry.
		Eigen::Vector3f p = openVSLAMTransform * Eigen::Vector3f(point_x, point_y, point_z);

		Point3D* point = new Point3D(id, p, Eigen::Vector3f(1, 1, 1), -1);
		pointCloud.push_back(point);
	}
}

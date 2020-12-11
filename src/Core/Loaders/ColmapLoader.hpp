#pragma once

#include "Core/Geometry/Point3D.hpp"
#include "Core/Loaders/MultiViewDataLoader.hpp"

#include <string>
#include <vector>


class Camera;
class CameraSetup;


struct ColmapCamera
{
	std::string type; // e.g. "SIMPLE_PINHOLE"
	int dimX;         // image width
	int dimY;         // image height
	float focal;      // focal length in pixels
	float pX;         // principal point in x
	float pY;         // principal point in y
};


struct ColmapImage
{
	int imageID; // These are important for reproducing COLMAP feature tracks

	// Quaternion
	float qw;
	float qx;
	float qy;
	float qz;

	// Translation
	float tx;
	float ty;
	float tz;

	int camID;        // same for all since we use only one camera per dataset
	std::string name; // e.g. "undistorted-%04d.jpg"
};


struct Colmap3DPoint
{
	int pointID;

	// position
	float x;
	float y;
	float z;

	// colour
	float r;
	float g;
	float b;

	// reprojection error
	float error;
};


class ColmapLoader : public MultiViewDataLoader
{
public:
	ColmapLoader();

	/*!
	 * Loads COLMAP multi-view geometry data from disk.
	 * The data includes camera pose of each frame and optionally reconstructed 3D points.
	 * 
	 * @param  dataset The dataset object to store the loaded data in.
	 * @param  sfmFile Path of file storing the path of image, camera, 3D points and depth map folders.
	 * @param  pathToInputFolder Path to the input data files.
	 * @param  intrinsicScale Scale factor for the intrinsic parameters of the camera.
	 * @param  load3DPoints Whether to load 3D points.
	 * @param  loadSparsePointCloud Whether to load the reconstructed sparse 3D points.
	 * @return The status of the loading process.
	 */
	int loadData(CameraSetupDataset* dataset,
	             const std::string& sfmFile,
	             const std::string& pathToInputFolder,
	             const float intrinsicScale,
	             const bool load3DPoints = false,
	             const bool loadSparsePointCloud = true) override;

	/*!
	 * Sets the number of digit characters in the image file name postfix.
	 * 
	 * @param n The number of digits.
	 */
	void setImageNameDigits(int n) override { imageNameDigits = n; }

private:
	void readPathsToColmapModelFiles(const std::string& sfmFile);
	std::vector<ColmapCamera> readColmapCameras();
	std::vector<ColmapImage> readColmapImages();
	std::vector<Colmap3DPoint> readColmap3DPoints(bool loadSparsePointCloud, float maxError);
	std::vector<Point3D*> convert3DPoints(std::vector<Colmap3DPoint> colmapPoints);

	void readCameraGeometryColmap(CameraSetup* camSetup, float intrinsicScale);
	void updateCameraImagePaths(CameraSetup* camSetup, const std::string& pathToInputFolder);

	void setCamNames(Camera* cam, std::string name, int len);

	int maxLoad3DPoints;
	int frameInterval = 1;
	int imageNameDigits = 4;

	std::string pathToCameras = "";
	std::string pathToImages = "";
	std::string pathTo3DPoints = "";

	// Convert from COLMAP to OpenGL coordinate system.
	Eigen::Matrix3f colmapTransform;
};


// COLMAP quick reference
// ======================
//
// Reading COLMAP files:
//
// 1. cameras.txt
//    contains the intrinsics of the reconstructed cameras
//    TYPE|WIDTH|HEIGHT|FOCAL|PRINCIPALX|PRINCIPALY
//
// 2. images.txt
//    contains the pose and the keypoints of all reconstructed images using two lines per image
//    IMAGE_ID|QW|QX|QY|QZ|TX|TY|TZ|CAM_ID|NAME
//    the optical centre is defined as c = -R^t * T, same as in MVG, Bundler
//
// 3. points3D.txt
//    POINT3D_ID|X|Y|Z|R|G|B|ERROR

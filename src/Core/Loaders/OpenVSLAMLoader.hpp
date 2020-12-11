#pragma once

#include "Core/Loaders/MultiViewDataLoader.hpp"

#include <Eigen/Core>

#include <string>
#include <vector>

class Camera;
class CameraSetup;
class CameraSetupDataset;
struct Point3D;


class OpenVSLAMLoader : public MultiViewDataLoader
{
public:
	OpenVSLAMLoader();

	/*!
	 * Loads OpenVSLAM multi-view geometry data from disk.
	 * The data includes camera pose of each frame and optionally reconstructed 3D points.
	 * @see [OpenVSLAM official website](https://openvslam.readthedocs.io/en/master/)
	 * 
	 * @param  dataset The dataset object to store the loaded data in.
	 * @param  sfmFile Path of file storing the path of image, camera, 3D points and depth map folders.
	 * @param  pathToInputFolder Path to the input data files.
	 * @param  intrinsicScale Scale factor for the intrinsic parameters of the camera.
	 * @param  load3DPoints Whether to load 3D points.
	 * @param  loadSparsePointCloud Whether to load the reconstructed sparse 3D points.
	 * @return The status of the loading process.
	 */
	int loadData(
	    CameraSetupDataset* dataset,
	    const std::string& sfmFile,
	    const std::string& pathToInputFolder,
	    const float intrinsicScale,
	    const bool load3DPoints = false,
	    const bool loadSparsePointCloud = false) override;

	/*!
	 * Sets the number of digit characters in the image file name postfix.
	 * 
	 * @param n The number of digits.
	 */
	void setImageNameDigits(int n) override { imageNameDigits = n; }

private:
	// the digit characters number in the postfix of the image file
	int imageNameDigits;

	// transform matrix from OpenVSLAM coordinate to system coordinate
	Eigen::Matrix3f openVSLAMTransform;

	/*!
	 * Sets camera pose to each frame (@see class: Camera).
	 */
	void loadCameraParameters(CameraSetup* camSetup,
	                          const float intrinsicScale,
	                          const std::string& pathToCameras,
	                          const std::string& pathToImages,
	                          bool& useEquirectImages);

	/*!
	 * Sets the image file name for each frame, extracting frame number from filename.
	 */
	void setCamNames(Camera* cam, std::string name, const int len);

	/*!
	 * Loads camera intrinsics from file.
	 */
	void loadCameraIntrinsicParameters(const std::string& pathToCameras, int& camera_dimX,
	                                   int& camera_dimY,
	                                   float& camera_focal,
	                                   float& camera_pX,
	                                   float& camera_pY,
	                                   bool& useEquirectImages);

	/*!
	 * Loads point cloud from OpenVSLAM *.msg file.
	 */
	void load3DPoints(const std::string& filePath, std::vector<Point3D*>& pointCloud, int maxLoad3Dpoints = -1);

	/*!
	* Loads the resource path from "modelFiles.openvslam".
	*/
	void readPathsFromModelFiles(const std::string& slamResultListPath,
	                             std::string& pathToCameras,
	                             std::string& pathToImages,
	                             std::string& pathTo3DPoints);
};

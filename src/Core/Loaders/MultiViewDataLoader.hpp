#pragma once

#include <string>

class CameraSetupDataset;


enum class SfmFormat
{
	VisualSFM = 0,
	Bundler = 1,
	Colmap = 2,
	OpenVSLAM = 3,
};


/**
* The interface of the multi-view geometry data loader.
*/
class MultiViewDataLoader
{
public:
	/*!
	 * Loads multi-view geometry data from disk.
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
	virtual int loadData(CameraSetupDataset* dataset,
	                     const std::string& sfmFile,
	                     const std::string& pathToInputFolder,
	                     const float intrinsicScale,
	                     const bool load3DPoints,
	                     const bool loadSparsePointCloud) = 0;

	/*!
	 * Sets the number of digit characters in the image file name postfix.
	 * 
	 * @param n The number of digits.
	 */
	virtual void setImageNameDigits(int n) = 0;
};

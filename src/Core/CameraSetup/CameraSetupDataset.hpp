#pragma once

#include "Core/CameraSetup/CameraSetup.hpp"
#include "Core/Geometry/Circle.hpp"
#include "Core/Geometry/Cylinder.hpp"
#include "Core/Geometry/PointCloud.hpp"
#include "Core/Geometry/Sphere.hpp"


/** 
 * @brief       Represents an OmniPhotos dataset. 
 * @description Preprocessing creates a dataset and writes the representation into a cache directory.
 *            The viewer loads the cache and passes the dataset to the visualization.
 */
class CameraSetupDataset
{
public:
	CameraSetupDataset();

	CameraSetupDataset(CameraSetup* _camSetup);
	CameraSetupDataset(CameraSetupDataset* _dataset);
	~CameraSetupDataset();


	std::string pathToConfigYAML = "";

	// Dataset stuff -> TODO: is a setting as well
	std::string name;
	std::string workingDirectory = "NULL";
	std::string pathToInputFolder = "NULL";
	std::string pathToCacheFolder = "NULL";
	std::string pathToConfigFolder = "NULL";

	//TODO: Only used in Preprocessing
	Eigen::Vector3f originalCentroid;

	CameraSetup* getCameraSetup();
	void setCameraSetup(CameraSetup* _camera_setup);


	//The one fitted in the Preprocessing
	std::shared_ptr<Circle> circle;
	//standard proxies independent of CameraSetup
	std::shared_ptr<Cylinder> cylinder;
	std::shared_ptr<Sphere> sphere;


	//correspondences between neighbouring pairs of images.
	//we always assume 1D camera trajectories.
	//paths to the files, serves as input for the FlowLoader
	std::vector<std::string> forwardFlows;
	std::vector<std::string> backwardFlows;

	inline void setSfmLoader(std::shared_ptr<MultiViewDataLoader> _sfmLoader) { sfmLoader = _sfmLoader; }

	inline std::shared_ptr<PointCloud> getWorldPointCloud() const { return worldPointCloud; }
	inline void setWorldPointCloud(std::shared_ptr<PointCloud> wpc) { worldPointCloud = wpc; }

	std::string generateCacheFilename();
	void sortCameras();
	void centreDatasetAtOrigin();

	bool load(CameraSetupSettings* settings);
	void readCamerasCSV(std::string filename, std::vector<Camera*>* cameras, CameraSetupSettings* settings);
	void writeCamerasCSV(std::string folder, std::vector<Camera*>* cameras, bool computeOpticalFlow);

	void save(CameraSetupSettings* settings);
	bool loadFromCache(CameraSetupSettings* settings);

	/** Scans 'rootFolder' for valid datasets (i.e. preprocessed & with cache folder).
	* And the load the first *-viewer-*.yaml file as the default configuration file.
	* @param  rootFolder  The root path of datasets.
	* @return List of datasets storing basic information for UI dataset selection.
	*         The datasets do not allocate dataset resources.
	*         All paths are relative to workingDirectory.
	*/
	static std::vector<CameraSetupDataset> scanDatasets(const std::string& rootFolder);

	/** Scans for a valid dataset config in the specified directory.
	* @param datasetDir  The root path of a dataset.
	* @param datasetInfo  The dataset information to be populated.
	* @return 0 if okay, <0 in case of error.
	*/
	static int scanDatasetDirectory(const std::string& datasetDir, CameraSetupDataset& datasetInfo);

	/** Scans the given config file for a dataset.
	* @param configPath  Path to a YAML config file.
	* @param datasetInfo  The dataset information to be populated.
	* @return 0 if okay, <0 in case of error.
	*/
	static int scanDatasetConfig(const std::string& configPath, CameraSetupDataset& datasetInfo);

	// Thumbnail image from UI preview, the image size 240 x 480.
	cv::Mat thumbImage;

private:
	CameraSetup* camera_setup = nullptr;

	// Multi-view geometry data loader (COLMAP/OpenVSLAM).
	std::shared_ptr<MultiViewDataLoader> sfmLoader;

	// Multi-view geometry point cloud.
	std::shared_ptr<PointCloud> worldPointCloud;

	//TODO: Who uses it?
	bool updateCSV(std::string imgName, std::vector<Camera*>* cameras, CameraSetupSettings* settings);
};

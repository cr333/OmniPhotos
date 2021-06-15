#pragma once

#include "SphereFittingSettings.hpp"

#include "Core/Application.hpp"
#include "Core/CameraSetup/CameraSetupDataset.hpp"
#include "Core/CameraSetup/CameraSetupSettings.hpp"
#include "Core/Loaders/ImageLoader.hpp"
#include "Core/Loaders/TextureLoader.hpp"

class MultiViewDataLoader;


/**
 * Preprocessing application that prepares a dataset before it can be viewed in the ViewerApp (Viewer.exe).
 */
class PreprocessingApp : public Application
{
public:
	/**
	 * @brief Construct a new PreprocessingApp object
	 * 
	 * This string is passed to appDataset in oder to encapsulate both the
	 * dataset on which the preprocessing is going to be performed, as well
	 * as the config.yaml file where the user inputs are stored.
	 * 
	 * @param pathToConfigYaml used to set appDataset->pathToconfigYaml
	 */
	PreprocessingApp(const std::string& pathToConfigYaml);
	~PreprocessingApp();

	/**
	 * @brief init() initialises the PreprocessingApp
	 * 
	 * After construction with app = new PreprocessingApp(pathToYaml),
	 * the initialization now takes the path and passes it to appDataset
	 * (initialized as null pointer in the constructor) as well as appSettings.
	 * 
	 * initDataset is then called from where appDataset loads the data using appSettings.
	 *
	 * initOpenGL is called to set appWindow. initOpenGL also calls initTextures to construct
	 * textureLoader (set in GLApplication.hpp).
	 *
	 * Shaders are then initialized in initPrograms.
	 * 
	 * @return int error code when it feels like it.
	 */
	int init() override;
	void run() override {};

	void initDataset();
	void rescalePointCloud();
	void rescaleDataset();
	void processDataset();
	void updateNumberOfCameras(); // sub-sample camera setup
	void computeEigenspace(Eigen::Matrix3f* base, Eigen::Vector3f* values);
	void changeBasis(); // depends on Eigenspace
	void fitCircleToCameras();
	void updateCircle();
	void computeOpticalFlow();

#ifdef USE_CERES
	void fitSphereMesh();
#endif


private:
	CameraSetupSettings appSettings;
	SphereFittingSettings sphereFittingSettings;

	CameraSetupDataset* appDataset = nullptr;
	CameraSetupDataset* appActiveDataset = nullptr;

	std::shared_ptr<MultiViewDataLoader> sfmLoader;
	std::shared_ptr<TextureLoader> textureLoader;

	bool initialRun = true;
	std::vector<Eigen::Vector3f> centroids;
	Eigen::Vector3f originalCentroid;
	std::vector<Eigen::Matrix3f> svdBases;
	Eigen::Matrix3f svdBase;
	Eigen::Vector3f svdValues;

	void sampleCameraCirclePhi(std::vector<Camera*>* _cameras, int M, float* phis);
	void sampleCameraCirclePhiAndEuclideanDistance(std::vector<Camera*>* _cameras, int M, float* phis, int neighbourhood, float maxDist);
	void sampleCameraCircleCreatePath(std::vector<Camera*>* _cameras, int M, float* phis, int neighbourhood, float minScore);
};

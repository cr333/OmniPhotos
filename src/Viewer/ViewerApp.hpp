/******************************************************************************
* @file   ViewerApp.hpp
*
* @authors Tobias Bertel, Mingze Yuan, Reuben Lindroos, Christian Richardt
******************************************************************************/
#pragma once

#include "Core/CameraSetup/CameraSetupDataset.hpp"
#include "Core/CameraSetup/CameraSetupSettings.hpp"
#include "Core/CameraSetup/CameraSetupVisualization.hpp"
#include "Core/GL/GLApplication.hpp"
#include "Core/GL/GLRenderModel.hpp"
#include "Core/Geometry/Mesh.hpp"

#include "Viewer/ViewerGLProgram.hpp"
#include "Viewer/ViewerGUI.hpp"


class ViewerApp : public GLApplication
{
public:
	/**
	 * Default constructor.
	 */
	ViewerApp();

	/**
	 * Initialises the Viewer within a directory of datasets.
	 * Standard usage from main file.
	 *
	 * @param _datasetPath Path to a config file or the dataset root folder.
	 * @param _enableVR Flag to enable the VR rendering.
	 */
	ViewerApp(const std::string& _datasetPath, bool _enableVR);

	/**
	 * Destroys the imgui context (if applicable) and termninates glfw 
	 */
	~ViewerApp();

	/**
	 * This function does the following: 
	 * 		- loads the dataset
	 * 		- initialises the glfw Window (and thereby an OpenGL context), 
	 * 		- initialises the GUI/VR rendering
	 * 		- loads textures/images
	 * 		- loads geometries/point clouds
	 *
	 * @returns	error code. 
	 */
	int init() override; // from Application

	/**
	 * Initializes the rendering systems and loads shaders.
	 *
	 * @returns	error code.
	 */
	int initPrograms() override; // from Application

	/**
	 * Sets up the recording session. When user presses F9 in the viewer, the camera will
	 * follow the path specified in this function. This is useful for demonstrations of
	 * the software. (for videos and presentations).
	 * 
	 * Also initialises the camera path method.
	 */
	void setupRecordingSession() override; // from GLApplication

	void initGLCamera() override;
	void initGLCameraToCentre() override;

	/**
	 * updates GLProgram visibilities.
	 * overridden from GLApplication
	 * overridden from GLApplication
	 */
	void updateProgramDisplay() override;

	/**
	 * Handles the user input (keyboard, mouse etc).
	 */
	void handleUserInput() override; // from Application

	std::string getScreenShotName() override; // from GLApplication

	/**
	 * Runs the main render loop and handles the user input.
	 */
	void run() override; // from Application

	/**
	* Load a dataset asynchronously from disk to CPU, store in the back dataset object.
	*
	* @param dataset_idx Index of the new dataset
	*/
	void loadDatasetCPU(const int dataset_idx);

	/**
	* Load a dataset from disk to CPU asynchronously.
	*
	* @param dataset_idx Index of the new dataset
	*/
	void loadDatasetCPUAsync(const int dataset_idx);

	/** Upload the CPU resources to the GPU, and reinitialize the dataset GL resources. */
	void loadDatasetGPU();

	/** Release all dataset GL resources on the GPU, just maintain the GUI. */
	void unloadDatasetGPU();


	CameraSetupDataset* getDataset() const;
	void setDataset(CameraSetupDataset* dataset, CameraSetupSettings* _settings);

	CameraSetupVisualization* getVisualization();

	inline int numberOfDatasets() { return (int)datasetInfoList.size(); }
	CameraSetupDataset& getDatasetFromList(int index);


	/** Path to the dataset config or directory. */
	std::string datasetPath;


	/** Index of the current dataset in datasetInfoList. */
	int currentDatasetIdx = -1;

	/** The thumbnails image of each dataset for GUI visualization. */
	std::vector<cv::Mat> thumbnails;

	/** the RGB texture format in GPU. If specify DXT1 or DTX5, the image loader will compress the texture. */
	std::string imageTextureFormat = "GL_RGB";

private:
	bool checkForDatasets();
	void updateCamPhiDirTexture();

	ViewerGUI* gui = nullptr;
	CameraSetupDataset* appDataset = nullptr;
	CameraSetupVisualization* appVisualization = nullptr;

	/** List of found datasets. */
	std::vector<CameraSetupDataset> datasetInfoList;

	/** List of all found proxy meshes. */
	std::vector<Mesh*> loaded_meshes;
	//used for asynchronuous loading
	std::vector<Mesh*> loaded_meshes_back;

	CameraSetupSettings appSettings;

	/** Back-buffer dataset storage and semaphore for background dataset loading. */
	enum class DatasetStatus
	{
		Empty = 0,   // background dataset empty
		Loading = 1, // background dataset is loading
		Loaded = 2,  // background dataset loaded to CPU, ready for GPU loading
	};
	DatasetStatus datasetBackStatus = DatasetStatus::Empty;
	CameraSetupDataset* datasetBack = nullptr;
	CameraSetupSettings datasetBackSetting;

	//TODO: We should re-use the texture loader, but update its buffers!
	//I think it's fine to "back" it, the important bit is to "update" the render models in the Visualization
	TextureLoader* textureLoaderBack = nullptr;

	ViewerGLProgram* megastereo_prog = nullptr;
	ViewerGLProgram* megaparallax_prog = nullptr;

	GLTexture* camDirPhiTexture = nullptr;
};

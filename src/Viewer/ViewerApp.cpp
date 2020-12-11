#include "ViewerApp.hpp"

#include "3rdParty/fs_std.hpp"

#include "Core/CameraSetup/CameraSetupVisualization.hpp"
#include "Core/GL/GLDefaultPrograms.hpp"
#include "Core/GL/GLFormats.hpp"
#include "Core/GL/GLRenderModel.hpp"

#ifdef WITH_OPENVR
	#include "Core/GUI/VRInterface.hpp"
#endif

#include "Utils/Exceptions.hpp"
#include "Utils/Logger.hpp"
#include "Utils/Timer.hpp"
#include "Utils/Utils.hpp"

#include <GitVersion.hpp>

#include <set>

using namespace std;


ViewerApp::ViewerApp()
{
}


ViewerApp::ViewerApp(const std::string& _datasetPath, bool _enableVR) :
    ViewerApp()
{
	datasetPath = _datasetPath;
	enableVR = _enableVR;
}


ViewerApp::~ViewerApp()
{
	// We don't use GUI in VR, so only clean it up if not in VR.
#ifdef WITH_OPENVR
	if (settings->enableVR == true)
	{
		delete hmd;
		hmd = nullptr;
	}
	else
#endif
	{
		delete gui;
		gui = nullptr;
	}

	glfwTerminate();
}


void ViewerApp::setDataset(CameraSetupDataset* dataset, CameraSetupSettings* _settings)
{
	appDataset = dataset;
	appSettings = *_settings;
	appVisualization->setDataset(appDataset);
}


CameraSetupDataset* ViewerApp::getDataset() const
{
	return appDataset;
}


CameraSetupDataset& ViewerApp::getDatasetFromList(int index)
{
	return datasetInfoList[index];
}


CameraSetupVisualization* ViewerApp::getVisualization()
{
	if (!appVisualization)
		appVisualization = new CameraSetupVisualization(appDataset, settings);
	return appVisualization;
}


void ViewerApp::setupRecordingSession()
{
	recordingSessionAvailable = true;
	getGLCamera()->cameraControl = make_shared<GLCameraControl>(getGLCamera(), inputHandler);

	// Initialise the camera path to the circle swing.
	// The path can later be changed in the GUI, in function ViewerGUI::showCameraPathAnimation().
	auto swing = make_shared<CircleSwingCommand>();
	swing->dataset_name = appDataset->name;
	swing->radius = appDataset->circle->getRadius() / 2;
	swing->basis = appDataset->circle->getBase();
	float dir_rad = degToRad(appSettings.lookAtDirection);
	swing->lookAt = appSettings.lookAtDistance * Eigen::Vector3f(sinf(dir_rad), 0.f, -cosf(dir_rad));

	vector<shared_ptr<Command>> recordingSession;
	recordingSession.push_back(swing);
	getGLCamera()->cameraControl->init(recordingSession);
}


void ViewerApp::updateProgramDisplay()
{
	//TODO: What if the Visualization was separated from the dataset?
	//For isntance next to the dataset in the app?
	getVisualization()->circle_prog->display = appSettings.showFittedCircle;
	getVisualization()->radial_directions_prog->display = appSettings.showRadialLines;

	//This reflects the choices from the method and proxy combo boxes in the GUI
	//TODO: renaming switches.
	GLApplication::updateProgramDisplay(appSettings.switchGLProgram);
	//Sets RenderModel for active GLProgram
	GLApplication::updateRenderModel(appSettings.switchGLRenderModel);

	// visibility can change according to settings
	getVisualization()->updateProgramDisplay();
}


bool ViewerApp::checkForDatasets()
{
	if (fs::is_directory(datasetPath))
	{
		// Scan for all datasets in the given directory.
		datasetInfoList = CameraSetupDataset::scanDatasets(datasetPath);
	}
	else
	{
		// Loading a single dataset, specified by a config file.
		CameraSetupDataset datasetInfo;
		if (CameraSetupDataset::scanDatasetConfig(datasetPath, datasetInfo) < 0)
		{
			LOG(ERROR) << "Config file '" << datasetPath << "' is not preprocessed";
			return false;
		}
		datasetInfoList.push_back(datasetInfo);
	}

	if (datasetInfoList.size() == 0)
	{
		LOG(ERROR) << "No datasets found in folder:\n"
		           << datasetPath;
		return false;
	}

	return true;
}


int ViewerApp::init()
{
	if (!checkForDatasets())
	{
		LOG(ERROR) << "Could not find any datasets -> terminating:";
		return -1;
	}

	// Set the first dataset as the default dataset and load its configuration from disk.
	// Use its dataset configuration to initialise the OpenGL context.
	int defaultDatasetIndex = 0;
	appSettings.configFile = datasetInfoList[defaultDatasetIndex].pathToConfigYAML;
	appDataset = &(datasetInfoList[defaultDatasetIndex]);
	appDataset->loadFromCache(&appSettings);
	settings = &appSettings;

	// Setup OpenGL context & GUI
	windowTitle = string("OmniPhotos Viewer (") + g_GIT_VERSION + ")";

	// UI, Mouse and keyhandler
	GLApplication::init();

#ifdef WITH_OPENVR
	if (settings->enableVR)
	{
		Eigen::Matrix4f initRot = createRotationTransform44(Eigen::Vector3f::UnitY(), settings->lookAtDirection);
		hmd = new VRInterface(0.1f, 1000.0f, initRot, this); // near/far clip distance [m]
		hmd->init(windowTitle, hmd->companionWindowDims, &maintenance);
		setGLwindow(hmd->companionWindow);
	}
	else
#endif //OPENVR
	{
		GLWindow* appWindow = new GLWindow(windowTitle, appSettings.windowDims, &maintenance, true, NULL);
		glfwSwapInterval(1); // Enable vsync
		setGLwindow(appWindow);
	}

	// Register UI callbacks.
	GLFWwindow* window = getGLwindow()->getGLFWwindow();
	if (getInputHandler())
		getInputHandler()->registerCallbacks(window);
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_FALSE);
	glfwMakeContextCurrent(getGLwindow()->getGLFWwindow());

	// GUI is only used outside the HMD.
#ifdef WITH_OPENVR
	if (settings->enableVR == false)
#endif
	{
		if (!guiInit)
		{
			gui = new ViewerGUI(this);
			gui->init();
			guiInit = true;
		}
	}

	appDataset = nullptr;

	// Load the default dataset from disk to CPU.
	loadDatasetCPU(defaultDatasetIndex);

	// Load data to GPU memory and render.
	loadDatasetGPU();

	if (gui)
		gui->datasetLoading = false;

	setupRecordingSession();

	VLOG(1) << "Leaving ViewerApp::init().";
	return 0;
}


int ViewerApp::initPrograms()
{
	VLOG(1) << "ViewerApp::initPrograms()";

#ifdef WITH_OPENVR
	if (settings->enableVR)
	{
		hmd->initGLPrograms(&app_gl_programs);

		getVisualization()->initPrograms(hmd->companionWindow, &app_gl_programs);
		for (int i = 0; i < app_gl_programs.size(); i++)
			app_gl_programs[i]->setContext(hmd->companionWindow->getGLFWwindow());
	}
	else
#endif // WITH_OPENVR
	{
		getVisualization()->initPrograms(getGLwindow(), &app_gl_programs);
		for (int i = 0; i < app_gl_programs.size(); i++)
			app_gl_programs[i]->setContext(getGLwindow()->getGLFWwindow());
	}

	ErrorChecking::checkGLError();

	string projectSrcDir = determinePathToSource();

	proxies_str.push_back("None"); // special black method
	int _method_id = 0;

	//"None" proxy still handy though to render only static visualization without IBR (black background).
	int _proxy_id = 0;
	proxies.push_back(std::pair<int, GLRenderModel*>(_proxy_id++, nullptr));


	// Megastereo-----------------------------------------------------------------------------------------------
	megastereo_prog = new ViewerGLProgram(getVisualization()->cylinder_model, &appSettings, appDataset);
	megastereo_prog->job = "Megastereo (Richardt et al. 2013)";
	megastereo_prog->setFragmentShaderFilename(projectSrcDir + "Shaders/General/Megastereo.fragment.glsl");
	megastereo_prog->setVertexShaderFilename(projectSrcDir + "Shaders/General/PassWorldPositions.vertex.glsl");
	megastereo_prog->addTexture(textureLoader->getProjectionMatrixTexture(), 0, "projectionMatrices");
	megastereo_prog->addTexture(textureLoader->getImageTexture(), 1, "cameraImages");
	megastereo_prog->addTexture(textureLoader->getPosViewTexture(), 2, "cameraPositionsAndViewingDirections");
	megastereo_prog->addTexture(camDirPhiTexture, 4, "cameraActualPhisArray");
	if (appSettings.useOpticalFlow)
	{
		megastereo_prog->addTexture(textureLoader->getForwardFlowTexture(), 8, "forwardFlows");
		megastereo_prog->addTexture(textureLoader->getBackwardFlowTexture(), 9, "backwardFlows");
	}
	megastereo_prog->setContext(getGLwindow()->getGLFWwindow());
	addProgram(megastereo_prog);
	//methods
	methods_str.push_back("Megastereo");
	methods.push_back(std::pair<int, GLProgram*>(_method_id++, megastereo_prog)); // 0 is reserved in the flag to show a black screen
	//proxy
	proxies_str.push_back("Cylinder");
	proxies.push_back(std::pair<int, GLRenderModel*>(_proxy_id++, getVisualization()->cylinder_model));


	// MegaParallax-----------------------------------------------------------------------------------------------
	megaparallax_prog = new ViewerGLProgram(nullptr, &appSettings, appDataset);
	megaparallax_prog->job = "MegaParallax (Bertel et al. 2019)";
	megaparallax_prog->setFragmentShaderFilename(projectSrcDir + "Shaders/General/MegaParallax.fragment.glsl");
	megaparallax_prog->setVertexShaderFilename(projectSrcDir + "Shaders/General/PassWorldPositions.vertex.glsl");
	megaparallax_prog->addTexture(textureLoader->getProjectionMatrixTexture(), 0, "projectionMatrices");
	megaparallax_prog->addTexture(textureLoader->getImageTexture(), 1, "cameraImages");
	megaparallax_prog->addTexture(textureLoader->getPosViewTexture(), 2, "cameraPositionsAndViewingDirections");
	megaparallax_prog->addTexture(camDirPhiTexture, 4, "cameraActualPhisArray");
	if (appSettings.useOpticalFlow)
	{
		megaparallax_prog->addTexture(textureLoader->getForwardFlowTexture(), 8, "forwardFlows");
		megaparallax_prog->addTexture(textureLoader->getBackwardFlowTexture(), 9, "backwardFlows");
	}
	megaparallax_prog->setContext(getGLwindow()->getGLFWwindow());
	addProgram(megaparallax_prog);
	methods_str.push_back("OmniPhotos");
	methods.push_back(std::pair<int, GLProgram*>(_method_id++, megaparallax_prog));

	// Ignore the plane in VR.
	// TODO: update the plane pose in VR.
	if (settings->enableVR == false)
	{
		proxies_str.push_back("Plane");
		proxies.push_back(std::pair<int, GLRenderModel*>(_proxy_id++, getGLCamera()->getPlane()->getRenderModel()));
	}

	// Add more proxies
	proxies_str.push_back("Sphere");
	proxies.push_back(std::pair<int, GLRenderModel*>(_proxy_id++, getVisualization()->sphere_model));

	for (Mesh* mesh : loaded_meshes)
	{
		proxies_str.push_back(mesh->name);
		mesh->createRenderModel(mesh->name);
		proxies.push_back(std::pair<int, GLRenderModel*>(_proxy_id++, mesh->getRenderModel()));
	}

	maintenance.addPrograms(app_gl_programs);
	maintenance.updatePrograms(true);

	// Default settings
	setActiveMethod(megaparallax_prog);
	appSettings.switchGLProgram = _method_id - 1;
	appSettings.switchGLRenderModel = min(4, _proxy_id - 1); // first loaded mesh or sphere
	setActiveProxy(proxies[appSettings.switchGLRenderModel].second);

	return 0;
}


// The camera viewpoint phis are passed as a 1D texture.
// It's still very useful when performing searches involving polar coordinates.
void ViewerApp::updateCamPhiDirTexture()
{
	// Set up or update GLTexture.
	int size = appSettings.numberOfCameras;
	ErrorChecking::checkGLError();
	if (camDirPhiTexture)
	{
		camDirPhiTexture->layout.mem.elements = size;
	}
	else
	{
		GLMemoryLayout memLayout = GLMemoryLayout(size, 1, "GL_RED", "GL_FLOAT", "GL_R16F");
		GLTextureLayout texLayout = GLTextureLayout(memLayout, Eigen::Vector2i(size, 0), "", -1);
		camDirPhiTexture = new GLTexture(texLayout, "GL_TEXTURE_1D");
	}
	ErrorChecking::checkGLError();

	if (!camDirPhiTexture->gl_ID)
		glGenTextures(1, &camDirPhiTexture->gl_ID);
	ErrorChecking::checkGLError();

	glBindTexture(GL_TEXTURE_1D, camDirPhiTexture->gl_ID);
	ErrorChecking::checkGLError();

	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexStorage1D(GL_TEXTURE_1D, 1,
	               getGLInternalFormat(camDirPhiTexture->layout.mem.internalFormat), // GL_R16F
	               camDirPhiTexture->layout.mem.elements);
	ErrorChecking::checkGLError();

	// Create 1D texture by setting phi angles for each camera, one pixel per camera.
	GLfloat data[3];
	auto& cameras = *appDataset->getCameraSetup()->getCameras();
	for (int i = 0; i < size; i++)
	{
		float val = cameras[i]->getPhi();
		data[0] = val;
		data[1] = val;
		data[2] = val;

		glBindTexture(GL_TEXTURE_1D, camDirPhiTexture->gl_ID);
		glTexSubImage1D(GL_TEXTURE_1D, 0, i, 1,
		                getGLFormat(camDirPhiTexture->layout.mem.format),
		                getGLType(camDirPhiTexture->layout.mem.type),
		                data);
	}
	ErrorChecking::checkGLError();
}


void ViewerApp::loadDatasetCPU(const int dataset_idx)
{
	// 0-0) Skip if current dataset.
	if (dataset_idx == currentDatasetIdx)
	{
		LOG(WARNING) << "Selected dataset is currently loaded. Skipping the dataset loading.";
		return;
	}

	// 0-1) Skip if dataset is loading.
	if (datasetBackStatus == DatasetStatus::Loading)
	{
		LOG(WARNING) << "Another dataset is loading! Please be patient.";
		return;
	}
	else if (datasetBackStatus == DatasetStatus::Loaded)
	{
		LOG(WARNING) << "Another dataset is uploading to GPU! Please be patient.";
		return;
	}
	datasetBackStatus = DatasetStatus::Loading;

	if (gui)
		gui->datasetLoading = true;

	LOG(INFO) << "Loading dataset '" << datasetInfoList[dataset_idx].name << "'";
	currentDatasetIdx = dataset_idx;

	if (datasetBack == nullptr)
	{
		VLOG(1) << "Creating initial 'datasetBack'";
		datasetBack = new CameraSetupDataset();
	}

	// 0-2) Load new dataset and settings from disk.
	CameraSetupDataset& dataset = datasetInfoList[dataset_idx];
	datasetBackSetting.configFile = dataset.pathToConfigYAML;
	datasetBackSetting.enableVR = enableVR;
	LOG(INFO) << "Loading the dataset from:\n"
	          << datasetBackSetting.configFile;
	datasetBack->loadFromCache(&datasetBackSetting);
	settings = &datasetBackSetting;

	// 1) Load all .obj files we can find in the cache dir as potential proxy geometry.
	for (const auto& entry : fs::directory_iterator(datasetBack->pathToCacheFolder))
	{
		auto& filepath = entry.path();
		if (entry.is_regular_file() && filepath.extension() == ".obj")
		{
			// Skip points.obj files (debug output of SphereFitting).
			if (endsWith(filepath.string(), "points.obj"))
				continue;

			Mesh* m = Mesh::load(filepath.generic_string());
			loaded_meshes_back.push_back(m);
		}
	}

	// 2) Load images, flows and depth maps.
	ImageLoader* imgLoader = new ImageLoader(datasetBack->getCameraSetup()->getCameras(), true);
	if (imageTextureFormat.empty())
	{
		LOG(WARNING) << "The RGB image texture format not set. Use the default option GL_RGB." << imageTextureFormat;
		imageTextureFormat = "GL_RGB";
	}
	imgLoader->imageTextureFormat = imageTextureFormat;

	FlowLoader* flowLoader = nullptr;
	if (datasetBackSetting.useOpticalFlow)
	{
		flowLoader = new FlowLoader((datasetBackSetting.downsampleFlow > 0), imgLoader->getImageDims(),
		                            &datasetBack->forwardFlows, &datasetBack->backwardFlows);
		if (!flowLoader->checkAvailability())
		{
			delete flowLoader;
			flowLoader = nullptr;
			datasetBackSetting.useOpticalFlow = false;
			LOG(WARNING) << "Optical flow files are not complete. Flow-based blending will not work.";
		}
	}

	textureLoaderBack = new TextureLoader(imgLoader, flowLoader);
	textureLoaderBack->loadTextures();

	// 3) Load 3D point cloud
	//don't want to delete, I want to update
	//	delete datasetBack->camVis;
	//visualizationBack = new CameraSetupVisualization(datasetBack, &datasetBackSetting);
	if (datasetBackSetting.load3DPoints)
		datasetBack->setWorldPointCloud(datasetBack->getWorldPointCloud());

	// 4) Inform the main thread the back-buffer dataset is ready.
	datasetBackStatus = DatasetStatus::Loaded;
}


void ViewerApp::loadDatasetCPUAsync(const int dataset_idx)
{
	std::thread load([this, dataset_idx]() { loadDatasetCPU(dataset_idx); });
	load.detach();
}


void ViewerApp::loadDatasetGPU()
{
	// 0) Clear previous dataset.
	if (textureLoader)
	{
		textureLoader->releaseCPUMemory();
		delete textureLoader;
	}

	if (appDataset)
		delete appDataset;

	// 1) Switch over to the new dataset.
	appSettings = std::move(datasetBackSetting);
	appDataset = new CameraSetupDataset(datasetBack);

	textureLoader = textureLoaderBack;

	loaded_meshes = std::move(loaded_meshes_back);

	datasetBack = nullptr;
	textureLoaderBack = nullptr;

	// 2) Set up new GL runtime data + upload the proxy data to GPU.
	textureLoader->initTextures();
	// Upload the data to GPU.
	textureLoader->fillTextures();

	// Update runtime dataset and dataset setting.
	//When are settings used? For checking visibility of programs for instance, right?
	settings = &appSettings;
	//appDataset->camVis->setSettings(&appSettings);
	//It should combine a few things
	getVisualization()->setDataset(appDataset);
	// Upload the data to GPU memory.
	updateCamPhiDirTexture();
	///////////////////////////////////////////////////
	initGLCamera();
	initPrograms();

	datasetBackStatus = DatasetStatus::Empty;
}


void ViewerApp::unloadDatasetGPU()
{
	// TODO: only update the textures of the GLProgram and update the GLRenderModel

	// Release GPU resource loaders, buffers and textures.
	textureLoader->releaseGPUMemory();

	// clang-format off
	delete camDirPhiTexture; camDirPhiTexture = nullptr;
	delete megastereo_prog; megastereo_prog = nullptr;
	delete megaparallax_prog; megaparallax_prog = nullptr;
	// clang-format on

	// Release previous data.
	app_gl_programs.clear();
	loaded_meshes.clear();
	maintenance.clear();
	methods.clear();
	methods_str.clear();
	proxies.clear();
	proxies_str.clear();
}


void ViewerApp::run()
{
	double time_at_start = glfwGetTime();
	updateProgramDisplay();
	handleUserInput();
	GLApplication::render();

#ifdef WITH_OPENVR
	if (settings->enableVR)
		hmd->handleInput(&app_gl_programs);
	///////////////////////////////////////////////////////////
	//This belongs all to rendering...

	if (settings->enableVR == false)
#endif
	{
		if (showImGui)
			gui->draw();

		// Timing and buffer swap
		GLApplication::postRender();
	}

	ErrorChecking::checkGLError();

	getInputHandler()->deltaTime = glfwGetTime() - lastRunTime;
	shouldShutdown |= (glfwWindowShouldClose(getGLwindow()->getGLFWwindow()) > 0);
	lastRunTime = glfwGetTime();

	// if back dataset ready, exchange the dataset in runtime & GPU
	if (datasetBackStatus == DatasetStatus::Loaded)
	{
		unloadDatasetGPU();
		loadDatasetGPU();
		setupRecordingSession();
		if (gui) gui->datasetLoading = false;
	}
}


string ViewerApp::getScreenShotName()
{
	// clang-format off
	string mode;
	switch (appSettings.displayMode)
	{
		case DisplayMode::Left:           { mode = "Left";        break; }
		case DisplayMode::Right:          { mode = "Right";       break; }
		case DisplayMode::LinearBlending: { mode = "Blend";       break; }
		case DisplayMode::CameraPair:     { mode = "CameraPair";  break; }
		case DisplayMode::TexLeft:        { mode = "xL";          break; }
		case DisplayMode::TexRight:       { mode = "xR";          break; }
		case DisplayMode::FlowLeft:       { mode = "Flow_LR";     break; }
		case DisplayMode::FlowRight:      { mode = "Flow_RL";     break; }
		case DisplayMode::CompFlowLeft:   { mode = "compFlow_LR"; break; }
		case DisplayMode::CompFlowRight:  { mode = "compFlow_RL"; break; }
		case DisplayMode::WorldPositions: { mode = "WorldPos";    break; }
		case DisplayMode::WorldLines:     { mode = "WorldLines";  break; }
	}
	// clang-format on

	string name = methods_str[appSettings.switchGLProgram];
	name = name + "-" + mode;
	string rpp = "rpp_" + std::to_string(settings->raysPerPixel);
	name = name + "-" + rpp;
	string flow = "flow_" + std::to_string(appSettings.useOpticalFlow);
	name = name + "-" + flow;
	name = windowTitle + "-" + name + ".jpg";

	return name;
}


void ViewerApp::handleUserInput()
{
	// Ignore input if it's meant for ImGUI.
	if (showImGui)
	{
		if (*ignoreMouseInput)
			getInputHandler()->resetMouseInputs();

		if (*ignoreKeyboardInput)
			getInputHandler()->resetKeyboardInputs();
	}

	// Change point size
	if (getInputHandler()->p_key_pressed)
	{
		Eigen::Vector2f r = appSettings.pointSizeRange;
		float p = appSettings.pointSize;
		if (getInputHandler()->ctrl_pressed)
			p = max(r.x(), min(p - 1.0f, r.y()));
		else
			p = max(r.x(), min(p + 1.0f, r.y()));
		appSettings.pointSize = p;
		LOG(INFO) << "Point size: " << p;
		getInputHandler()->p_key_pressed = false;
		return;
	}


	float change_factor = 0.5f;

	// GLCamera, decrease distance to proxy plane
	if (getInputHandler()->num2_pressed)
	{
		std::string proxy_str = proxies_str[settings->switchGLRenderModel];
		if (proxy_str.compare("Plane") == 0)
		{
			getGLCamera()->changePlaneDistance(-change_factor * mouse->zoomSpeed);
			getGLCamera()->update();
		}
		//TODO: We lose real-time interactivity for Cylinder and Sphere since it consists of many more vertices than a Plane.
		if (proxy_str.compare("Cylinder") == 0)
		{
			getDataset()->cylinder->changeRadius(-change_factor * mouse->zoomSpeed);
			getVisualization()->updateCylinderModel(settings->circle_resolution);
		}
		if (proxy_str.compare("Sphere") == 0)
		{
			//changeSphereRadius() as an App-method?
			getDataset()->sphere->changeRadius(-change_factor * mouse->zoomSpeed);
			getVisualization()->updateSphereModel();
		}
		return;
	}

	// GLCamera, increase distance to proxy plane
	if (getInputHandler()->num8_pressed)
	{
		std::string proxy_str = proxies_str[settings->switchGLRenderModel];

		if (proxy_str.compare("Plane") == 0)
		{
			getGLCamera()->changePlaneDistance(change_factor * mouse->zoomSpeed);
			getGLCamera()->update();
		}

		if (proxy_str.compare("Cylinder") == 0)
		{
			getDataset()->cylinder->changeRadius(change_factor * mouse->zoomSpeed);
			getVisualization()->updateCylinderModel(settings->circle_resolution);
		}

		if (proxy_str.compare("Sphere") == 0)
		{
			getDataset()->sphere->changeRadius(change_factor * mouse->zoomSpeed);
			getVisualization()->updateSphereModel();
		}

		return;
	}

	// Toggle show/hide ImGUI.
	if (getInputHandler()->h_key_pressed)
	{
		showImGui = !showImGui;
		getInputHandler()->h_key_pressed = false;
		return;
	}

	GLApplication::sharedUserInput();
}


void ViewerApp::initGLCamera()
{
	// Initialise camera at origin + looking along -z axis.
	Eigen::Point3f cameraPos = appDataset->circle->getCentre();
	Eigen::Matrix3f rot = appDataset->circle->getBase();

	GLCamera* _cam = getGLCamera();
	if (!_cam)
	{
		_cam = new GLCamera(cameraPos, rot, 60.0f, getGLwindow()->getAspect(), 1.0f, 50000.0f,
		                    appDataset->circle->getBase(), Eigen::Vector2i(4096, 4096), getGLwindow()->wInfo);
		setGLCamera(_cam);
	}
	else
	{
		_cam->placeToCentre(cameraPos, rot); // reset camera at runtime
	}

	// Apply initial viewing direction. Need to flip sign as camera-up is negative y-axis.
	_cam->yaw(-appSettings.lookAtDirection);

	_cam->setPlaneDistance(appDataset->cylinder->getRadius());
	mouse->freeTranslate = false;

#ifdef WITH_OPENVR
	if (hmd)
	{
		Eigen::Matrix4f initRot = createRotationTransform44(Eigen::Vector3f::UnitY(), settings->lookAtDirection);
		hmd->setStartPose(initRot);
	}
#endif
}


void ViewerApp::initGLCameraToCentre()
{
	getGLCamera()->placeToCentre(appDataset->circle->getCentre(), appDataset->circle->getBase());

	// Apply initial viewing direction. Need to flip sign as camera-up is negative y-axis.
	getGLCamera()->yaw(-appSettings.lookAtDirection);

	mouse->freeTranslate = false;
}

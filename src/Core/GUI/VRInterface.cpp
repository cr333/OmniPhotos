#include "VRInterface.hpp"

#include "3rdParty/fs_std.hpp"

#include "Utils/Utils.hpp"

#ifndef _countof
	#define _countof(x) (sizeof(x) / sizeof((x)[0]))
#endif


using namespace std;


struct VertexDataWindow
{
	OpenVR::Vector2 position;
	OpenVR::Vector2 texCoord;

	VertexDataWindow(const OpenVR::Vector2& pos, const OpenVR::Vector2 tex) :
	    position(pos), texCoord(tex) {}
};


ControllerInfo_t::~ControllerInfo_t()
{
	if (sphere)
		delete sphere;
	if (sphere_program)
		delete sphere_program;
	if (mesh)
		delete mesh;
	if (controller_program)
		delete controller_program;
}


std::string getTrackedDeviceString(
    vr::TrackedDeviceIndex_t unDevice,
    vr::TrackedDeviceProperty prop,
    vr::TrackedPropertyError* peError)
{
	uint32_t unRequiredBufferLen = vr::VRSystem()->GetStringTrackedDeviceProperty(unDevice, prop, nullptr, 0, peError);
	if (unRequiredBufferLen == 0)
		return "";

	char* pchBuffer = new char[unRequiredBufferLen];
	vr::VRSystem()->GetStringTrackedDeviceProperty(unDevice, prop, pchBuffer, unRequiredBufferLen, peError);
	std::string sResult = pchBuffer;
	delete[] pchBuffer;
	return sResult;
}


bool getDigitalActionRisingEdge(vr::VRActionHandle_t action, vr::VRInputValueHandle_t* pDevicePath)
{
	vr::InputDigitalActionData_t actionData;
	vr::VRInput()->GetDigitalActionData(action, &actionData, sizeof(actionData), vr::k_ulInvalidInputValueHandle);
	if (pDevicePath)
	{
		*pDevicePath = vr::k_ulInvalidInputValueHandle;
		if (actionData.bActive)
		{
			vr::InputOriginInfo_t originInfo;
			if (vr::VRInputError_None == vr::VRInput()->GetOriginTrackedDeviceInfo(actionData.activeOrigin, &originInfo, sizeof(originInfo)))
			{
				*pDevicePath = originInfo.devicePath;
			}
		}
	}
	return actionData.bActive && actionData.bChanged && actionData.bState;
}


bool getDigitalActionFallingEdge(vr::VRActionHandle_t action, vr::VRInputValueHandle_t* pDevicePath)
{
	vr::InputDigitalActionData_t actionData;
	vr::VRInput()->GetDigitalActionData(action, &actionData, sizeof(actionData), vr::k_ulInvalidInputValueHandle);
	if (pDevicePath)
	{
		*pDevicePath = vr::k_ulInvalidInputValueHandle;
		if (actionData.bActive)
		{
			vr::InputOriginInfo_t originInfo;
			if (vr::VRInputError_None == vr::VRInput()->GetOriginTrackedDeviceInfo(actionData.activeOrigin, &originInfo, sizeof(originInfo)))
			{
				*pDevicePath = originInfo.devicePath;
			}
		}
	}
	return actionData.bActive && actionData.bChanged && !actionData.bState;
}


bool getDigitalActionState(vr::VRActionHandle_t action, vr::VRInputValueHandle_t* pDevicePath)
{
	vr::InputDigitalActionData_t actionData;
	vr::VRInput()->GetDigitalActionData(action, &actionData, sizeof(actionData), vr::k_ulInvalidInputValueHandle);
	if (pDevicePath)
	{
		*pDevicePath = vr::k_ulInvalidInputValueHandle;
		if (actionData.bActive)
		{
			vr::InputOriginInfo_t originInfo;
			if (vr::VRInputError_None == vr::VRInput()->GetOriginTrackedDeviceInfo(actionData.activeOrigin, &originInfo, sizeof(originInfo)))
			{
				*pDevicePath = originInfo.devicePath;
			}
		}
	}
	return actionData.bActive && actionData.bState;
}


void VRInterface::processVREvent(const vr::VREvent_t& event)
{
	switch (event.eventType)
	{
		case vr::VREvent_TrackedDeviceDeactivated:
			LOG(INFO) << "Device " << event.trackedDeviceIndex << " detached.";
			break;

		case vr::VREvent_TrackedDeviceUpdated:
			LOG(INFO) << "Device " << event.trackedDeviceIndex << " updated;";
			break;
	}
}


void VRInterface::pollChanges()
{
	if (trackedControllerCount != trackedControllerCount_Last || validPoseCount != validPoseCount_Last)
	{
		validPoseCount_Last = validPoseCount;
		trackedControllerCount_Last = trackedControllerCount;

		LOG(INFO) << "PoseCount:" << validPoseCount << "(" << poseClasses << ") Controllers:" << trackedControllerCount;
	}
}


bool VRInterface::initOpenVR()
{
	LOG(INFO) << "Loading the SteamVR Runtime ...";
	vr::EVRInitError eError = vr::VRInitError_None;
	m_pHMD = vr::VR_Init(&eError, vr::VRApplication_Scene);

	if (eError != vr::VRInitError_None)
		LOG(FATAL) << "Unable to initialise SteamVR Runtime: " << vr::VR_GetVRInitErrorAsEnglishDescription(eError);

	LOG(INFO) << "Found HMD of type " << getTrackedDeviceString(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_ModelNumber_String);

	vr::EVRInitError peError = vr::VRInitError_None;

	if (!vr::VRCompositor())
	{
		LOG(FATAL) << "Compositor initialisation failed. See log file for details";
		return false;
	}

	std::string pathToJson = (fs::absolute(determinePathToSource()) / "Core/GUI/vr_actions.json").string();
	vr::VRInput()->SetActionManifestPath(pathToJson.c_str());

	vr::VRInput()->GetActionHandle("/actions/omniphotos/in/ToggleMethod", &actionToggleMethod);
	vr::VRInput()->GetActionHandle("/actions/omniphotos/in/ToggleDisplayMode", &actionToggleDisplayMode);
	vr::VRInput()->GetActionHandle("/actions/omniphotos/in/RecentreHeadset", &actionRecentreHeadset);
	vr::VRInput()->GetActionHandle("/actions/omniphotos/in/NextDataset", &actionNextDataset);
	vr::VRInput()->GetActionHandle("/actions/omniphotos/in/PreviousDataset", &actionPreviousDataset);
	vr::VRInput()->GetActionHandle("/actions/omniphotos/in/NextProxy", &actionNextProxy);
	vr::VRInput()->GetActionHandle("/actions/omniphotos/in/PreviousProxy", &actionPreviousProxy);
	vr::VRInput()->GetActionHandle("/actions/omniphotos/in/TriggerHaptic", &actionTriggerHaptic);

	vr::VRInput()->GetActionSetHandle("/actions/omniphotos", &actionsetOmniphotos);

	vr::VRInput()->GetActionHandle("/actions/omniphotos/out/Haptic_Left", &handControllerInfo[LeftHand].m_actionHaptic);
	vr::VRInput()->GetInputSourceHandle("/user/hand/left", &handControllerInfo[LeftHand].m_source);
	vr::VRInput()->GetActionHandle("/actions/omniphotos/in/Hand_Left", &handControllerInfo[LeftHand].m_actionPose);

	vr::VRInput()->GetActionHandle("/actions/omniphotos/out/Haptic_Right", &handControllerInfo[RightHand].m_actionHaptic);
	vr::VRInput()->GetInputSourceHandle("/user/hand/right", &handControllerInfo[RightHand].m_source);
	vr::VRInput()->GetActionHandle("/actions/omniphotos/in/Hand_Right", &handControllerInfo[RightHand].m_actionPose);

	return true;
}


VRInterface::VRInterface(
    float _eyeNearClip,
    float _eyeFarClip,
    Eigen::Matrix4f initRot,
    ViewerApp* _app) :
    eyeNearClip(_eyeNearClip),
    eyeFarClip(_eyeFarClip)
{
	initOpenVR();
	app = _app;
	companionWindowDims = Eigen::Vector2i(1280, 640);

	// Initial pose and scale factor (convert from OpenVR [m] to ours [cm]).
	startPose = initRot;
	setScaleFactor(100);

	//WIP we want to get rid of that.
	//The point is that we need to pass some HMD render model to the GLApplication::initPrograms().
	//Currently, we always return controller_render_models[0]

	//We use this to hack our own code. We currently assume to render models produced by the HMD, namely both controllers
	//We will update this baby to render our HMD-related geometries, e.g. controller and plane.
	//TODO: We want a render model for each geometry and just execute the same program several times.
	//controller_render_models.push_back(new GLRenderModel("HMD left controller"));
	//controller_render_models.push_back(new GLRenderModel("HMD right controller"));
}


//WIP
Mesh* VRInterface::findOrLoadControllerMesh(std::vector<Mesh*>& _hmd_ui_meshes, std::string pchRenderModelName)
{
	Mesh* mesh = nullptr;
	for (std::vector<Mesh*>::iterator i = _hmd_ui_meshes.begin(); i != _hmd_ui_meshes.end(); i++)
	{
		std::string name = (*i)->name;
		if (!_stricmp(name.c_str(), pchRenderModelName.c_str()))
		{
			mesh = *i;
			break;
		}
	}

	// load the model if we didn't find one
	if (!mesh)
	{
		vr::RenderModel_t* pModel;
		vr::EVRRenderModelError error;
		while (1)
		{
			error = vr::VRRenderModels()->LoadRenderModel_Async(pchRenderModelName.c_str(), &pModel);
			if (error != vr::VRRenderModelError_Loading)
				break;

			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}

		if (error != vr::VRRenderModelError_None)
		{
			DLOG(INFO) << "Unable to load render model '" << pchRenderModelName << "': " << vr::VRRenderModels()->GetRenderModelErrorNameFromEnum(error);
			return nullptr; // move on to the next tracked device
		}

		vr::RenderModel_TextureMap_t* pTexture;
		while (1)
		{
			error = vr::VRRenderModels()->LoadTexture_Async(pModel->diffuseTextureId, &pTexture);
			if (error != vr::VRRenderModelError_Loading)
				break;

			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}

		if (error != vr::VRRenderModelError_None)
		{
			LOG(WARNING) << "Unable to load render texture id:" << pModel->diffuseTextureId << " for render model '" << pchRenderModelName << "'";
			vr::VRRenderModels()->FreeRenderModel(pModel);
			return nullptr; // move on to the next tracked device
		}


		mesh = loadRiftControllerMesh(pchRenderModelName, *pModel, *pTexture);
		mesh->createRenderModel("Rift controller");
		if (!mesh)
		{
			LOG(WARNING) << "Unable to create GL model from render model " << pchRenderModelName.c_str();
		}
		else
			_hmd_ui_meshes.push_back(mesh);
		vr::VRRenderModels()->FreeRenderModel(pModel);
		vr::VRRenderModels()->FreeTexture(pTexture);
	}
	return mesh;
}


void VRInterface::init(
    const std::string& title,
    const Eigen::Vector2i& windowDims,
    GLProgramMaintenance* maintenancePtr,
    GLFWwindow* parentContext)
{
	updateEyeMatrices();
	setupCompanionWindow(title, windowDims, maintenancePtr, parentContext);
	setupStereoRenderTargets();
}

/*
 *This is an init in which we map spheres to the controllers. 
 */
void VRInterface::initDefaultControllerPrograms(std::vector<GLProgram*>* _programs)
{
	/**
	 * @brief We use spheres as default render models visualize controllers
	 *   
	 * @param _programs the list being rendered in renderScene.
	 */
	std::vector<std::string> controllerNames { "controller_left", "controller_right" };
	string projectSrcDir = determinePathToSource();
	for (Controllers eHand = LeftHand; eHand <= RightHand; ((int&)eHand)++)
	{
		handControllerInfo[eHand].sphere = new Sphere(36, 18, 10.0f, true); // radius: 0.1 m
		std::string model_name = "VR Controller sphere render model" + std::string(getTextForControllerEnum(eHand));

		//create render model on init and then use "pose" to change location in shader via updateControllerPositions()
		handControllerInfo[eHand].sphere->createRenderModel(model_name);

		std::string program_name = "VR Controller program (" + std::string(getTextForControllerEnum(eHand)) + ") (sphere as default)";
		handControllerInfo[eHand].sphere_program = new GLDefaultProgram(program_name, handControllerInfo[eHand].sphere->getRenderModel());
		handControllerInfo[eHand].sphere_program->setVertexShaderFilename(projectSrcDir + "Shaders/General/PassWorldPositions.vertex.glsl");
		handControllerInfo[eHand].sphere_program->setFragmentShaderFilename(projectSrcDir + "Shaders/General/ColourWorldPos.fragment.glsl");
		handControllerInfo[eHand].sphere_program->job = controllerNames[eHand];
		handControllerInfo[eHand].sphere_program->setContext(companionWindow->getGLFWwindow());
		handControllerInfo[eHand].sphere_program->display = false;
		_programs->push_back(handControllerInfo[eHand].sphere_program);
	}
}


/*
*This is an init in which we map textured meshes to the controllers.
*/
void VRInterface::initRiftControllerPrograms(std::vector<GLProgram*>* _programs)
{
	/**
	* @brief We initialise with the sphere models and update the controller models later on in handleInput()
	* 
	* 
	* @param _programs the list being rendered in renderScene.
	*/
	std::vector<std::string> controllerNames { "rift_controller_left", "rift_controller_right" };
	string projectSrcDir = determinePathToSource();
	for (Controllers eHand = LeftHand; eHand <= RightHand; ((int&)eHand)++)
	{
		handControllerInfo[eHand].pose = convertSteamVRMatrixToMatrix4(handControllerInfo[eHand].poseData.pose.mDeviceToAbsoluteTracking);
		std::string model_name = "VR Controller sphere render model" + std::string(getTextForControllerEnum(eHand));


		std::string program_name = "VR Controller program (" + std::string(getTextForControllerEnum(eHand)) + ") (rift controller)";
		handControllerInfo[eHand].controller_program = new GLDefaultProgram(program_name, nullptr); //initialize with dummy and update it later
		handControllerInfo[eHand].controller_program->setVertexShaderFilename(projectSrcDir + "Shaders/General/Interleaved.vertex.glsl");
		handControllerInfo[eHand].controller_program->setFragmentShaderFilename(projectSrcDir + "Shaders/General/TextureMesh.fragment.glsl");
		handControllerInfo[eHand].controller_program->job = controllerNames[eHand];
		handControllerInfo[eHand].controller_program->setContext(companionWindow->getGLFWwindow());
		//Essential to account for the few cycles needed to load the rift controllers
		handControllerInfo[eHand].controller_program->display = false;

		_programs->push_back(handControllerInfo[eHand].controller_program);
	}
}


void VRInterface::handleInput(std::vector<GLProgram*>* _programs)
{
	// Process SteamVR events
	vr::VREvent_t event;
	while (m_pHMD->PollNextEvent(&event, sizeof(event)))
	{
		processVREvent(event);
	}

	// Process SteamVR action state
	// UpdateActionState is called each frame to update the state of the actions themselves. The application
	// controls which action sets are active with the provided array of VRActiveActionSet_t structs.
	vr::VRActiveActionSet_t actionSet = { 0 };
	actionSet.ulActionSet = actionsetOmniphotos;
	vr::VRInput()->UpdateActionState(&actionSet, sizeof(actionSet), 1);

	handleControllerActions();

	/*
	* The problem is that the VR context has to run a few iterations before we can read the controllers which is kind of rubbish,
	* but it is what it is.
	* Nevertheless, I would consider it a post-init or pre-run step logically. 
	*/
	loadControllers();
}


void VRInterface::handleControllerActions()
{
	vr::VRInputValueHandle_t ulHapticDevice;

	/**
	* In order to bind controls to the controller buttons, one must add the
	* required button mappings to the two json files and assign them in initOpenVR().
	*/
	if (getDigitalActionRisingEdge(actionTriggerHaptic, &ulHapticDevice))
	{
		if (ulHapticDevice == handControllerInfo[LeftHand].m_source)
		{
			vr::VRInput()->TriggerHapticVibrationAction(handControllerInfo[LeftHand].m_actionHaptic, 0, 1, 4.f, 1.0f, vr::k_ulInvalidInputValueHandle);
		}
		if (ulHapticDevice == handControllerInfo[RightHand].m_source)
		{
			vr::VRInput()->TriggerHapticVibrationAction(handControllerInfo[RightHand].m_actionHaptic, 0, 1, 4.f, 1.0f, vr::k_ulInvalidInputValueHandle);
		}
	}

	// Toggle rendering method between MegaParallax/OmniPhotos (default) and Megastereo.
	if (getDigitalActionRisingEdge(actionToggleMethod, vr::k_ulInvalidInputValueHandle))
	{
		app->settings->switchGLProgram = 1 - app->settings->switchGLProgram;
	}

	// Toggle display mode between LinearBlending (default) and WorldLines.
	if (getDigitalActionRisingEdge(actionToggleDisplayMode, vr::k_ulInvalidInputValueHandle))
	{
		if (app->settings->displayMode == LinearBlending)
		{
			app->settings->displayMode = WorldLines;
		}
		else
		{
			app->settings->displayMode = LinearBlending;
		}
	}

	// Load the next dataset.
	if (getDigitalActionRisingEdge(actionNextDataset, vr::k_ulInvalidInputValueHandle))
	{
		int nextDataset = std::min(app->currentDatasetIdx + 1, app->numberOfDatasets() - 1);
		app->loadDatasetCPUAsync(nextDataset);
	}

	// Load the previous dataset.
	if (getDigitalActionRisingEdge(actionPreviousDataset, vr::k_ulInvalidInputValueHandle))
	{
		app->loadDatasetCPUAsync(std::max(app->currentDatasetIdx - 1, 0));
	}

	// Recentre the headset.
	if (getDigitalActionRisingEdge(actionRecentreHeadset, vr::k_ulInvalidInputValueHandle))
	{
		resetStartPose();
	}

	if (getDigitalActionRisingEdge(actionNextProxy, vr::k_ulInvalidInputValueHandle))
	{
		int nextProxy = app->settings->switchGLRenderModel + 1;
		app->settings->switchGLRenderModel = std::min(nextProxy, int(app->proxies.size() - 1));
	}

	if (getDigitalActionRisingEdge(actionPreviousProxy, vr::k_ulInvalidInputValueHandle))
	{
		app->settings->switchGLRenderModel = std::max(app->settings->switchGLRenderModel - 1, 0);
	}
}


void VRInterface::loadControllers()
{
	for (Controllers eHand = LeftHand; eHand <= RightHand; ((int&)eHand)++)
	{
		//essential -> global origin kind of, says something about environment
		bool errorIsNonZero = vr::VRInput()->GetPoseActionDataForNextFrame(
		                          handControllerInfo[eHand].m_actionPose,
		                          vr::TrackingUniverseSeated,
		                          &handControllerInfo[eHand].poseData,
		                          sizeof(handControllerInfo[eHand].poseData),
		                          vr::k_ulInvalidInputValueHandle)
		                      != vr::VRInputError_None;

		//update pose of controller
		handControllerInfo[eHand].pose = convertSteamVRMatrixToMatrix4(handControllerInfo[eHand].poseData.pose.mDeviceToAbsoluteTracking);

		handControllerInfo[eHand].sphere_program->display = false;
		handControllerInfo[eHand].controller_program->display = false;

		//Check for error
		if (errorIsNonZero || !handControllerInfo[eHand].poseData.bActive || !handControllerInfo[eHand].poseData.pose.bPoseIsValid) //action pose = 0
		{
			if (cycleCounter % 3000 == 0)
			{
				//reduce the spamming a little
				LOG(WARNING) << "Controller " << eHand << " hidden (errorIsNonZero=" << errorIsNonZero << "; bActive=" << handControllerInfo[eHand].poseData.bActive << "; bPoseIsValid=" << handControllerInfo[eHand].poseData.pose.bPoseIsValid << ")";
			}
			handControllerInfo[eHand].showController = false;
		}
		//go for it
		else
		{
			if (!use_default_controller)
			{
				vr::InputOriginInfo_t originInfo;
				if (vr::VRInput()->GetOriginTrackedDeviceInfo(
				        handControllerInfo[eHand].poseData.activeOrigin,
				        &originInfo,
				        sizeof(originInfo))
				        == vr::VRInputError_None
				    && originInfo.trackedDeviceIndex != vr::k_unTrackedDeviceIndexInvalid)
				{
					std::string sRenderModelName = getTrackedDeviceString(originInfo.trackedDeviceIndex, vr::Prop_RenderModelName_String);
					handControllerInfo[eHand].mesh = findOrLoadControllerMesh(hmd_ui_meshes, sRenderModelName);

					//Adding texture
					if (!handControllerInfo[eHand].added_texture)
					{
						handControllerInfo[eHand].controller_program->addTexture(handControllerInfo[eHand].mesh->getRenderModel()->getPrimitive()->tex.get(), 10, "diffuse"); //10 important, used in createRenderModel
						handControllerInfo[eHand].added_texture = true;
					}
					handControllerInfo[eHand].controller_program->setActiveRenderModel(handControllerInfo[eHand].mesh->getRenderModel());
					use_controllers = true;
					handControllerInfo[eHand].controller_program->display = true;
				}
			}
		}

		if (!use_controllers)
			handControllerInfo[eHand].sphere_program->display = true;
	}
}


void VRInterface::updateControllerPositions()
{
	//This works!!! :)
	for (Controllers eHand = LeftHand; eHand <= RightHand; ((int&)eHand)++)
	{
		Eigen::Matrix4f B = handControllerInfo[eHand].pose;
		//DLOG(INFO) << "Controller pose: " << B;
		Eigen::Matrix4f M = B.inverse() * startPose;

		// Camera centre from rigid-body transform.
		Eigen::Matrix3f R = M.block(0, 0, 3, 3);
		Eigen::Vector3f t = M.block(0, 3, 3, 1);
		Eigen::Point3f centre = -R.transpose() * t * scaleFactor;

		//DLOG(INFO) << "controller centre: " << centre;

		//apply scale factor to translation of M

		Eigen::Matrix4f scaled_M = composeTransform44(R, scaleFactor * t);
		//pose used in vertex shader. Initalized with identity in the GLRenderModel's Primitive
		if (!use_controllers)
			handControllerInfo[eHand].sphere->getRenderModel()->setPose(scaled_M.inverse());
		else
		{
			//This might need a few iterations before it's set in handleInput()
			if (handControllerInfo[eHand].mesh)
			{
				if (handControllerInfo[eHand].mesh->getRenderModel())
					handControllerInfo[eHand].mesh->getRenderModel()->setPose(scaled_M.inverse());
			}
		}
	}
}


//that's the one called from the application
void VRInterface::renderStereoTargets(std::vector<GLProgram*>& _programs)
{
	//once to render all the programs.
	updateEyeMatrices();
	updateControllerPositions();

	//WIP
	//updateProxyPlanes();

	ErrorChecking::checkGLError();
	//render left eye
	renderEye(_programs, leftEyeDesc, vr::Eye_Left);
	//render right eye
	renderEye(_programs, rightEyeDesc, vr::Eye_Right);
	glFinish();
	ErrorChecking::checkGLError();
}


//setup Rift controller mesh
Mesh* VRInterface::loadRiftControllerMesh(const std::string& pchRenderModelName, const vr::RenderModel_t& _vrModel, const vr::RenderModel_TextureMap_t& _vrDiffuseTexture)
{
	const VertexInterleaved* ptr = (const VertexInterleaved*)_vrModel.rVertexData;
	MeshData mesh_data = MeshData(_vrModel.unVertexCount, ptr,
	                              _vrModel.unTriangleCount, _vrModel.rIndexData,
	                              _vrDiffuseTexture.unWidth, _vrDiffuseTexture.unHeight, _vrDiffuseTexture.rubTextureMapData);

	Mesh* controller = new Mesh(mesh_data, pchRenderModelName, scaleFactor);
	return controller;
}


void VRInterface::resetStartPose()
{
	LOG(INFO) << "Resetting start pose.";

	//// Full pose reset: viewing direction becomes positive z-axis and so on.
	//// This means that if one is looking down, this becomes straight ahead, which quickly leads to disorientation.
	//startPose = hmdPose.inverse();

	// Partial pose reset: reset the camera centre to the origin, but keep orientation the same.
	// For this, we need to first decompose the HMD pose M = [R | t; 0 | 1].
	Eigen::Matrix3f R = hmdPose.block(0, 0, 3, 3);
	Eigen::Vector3f t = hmdPose.block(0, 3, 3, 1);

	// We now update the translation component of the initial 'startPose' such that
	//   hmdPose * startPose = [R | t; 0 | 1] * [R_s | s; 0 | 1] = [R R_s | Rs+t; 0 | 1] = [R R_s | 0; 0 | 1],
	// i.e. the result is a pure rotation without translation. We thus have
	//   0 = R s + t   =>   s = -R^-1 t = -R' t (as R is a rotation matrix).
	Eigen::Vector3f s = -R.transpose() * t;
	startPose.block(0, 3, 3, 1) = s;
}


void VRInterface::setStartPose(Eigen::Matrix4f _startPose)
{
	// We want to keep the translation of the origin from previous start pose,
	// so we can reuse it and stay centred after loading new datasets.
	Eigen::Vector3f t = startPose.block(0, 3, 3, 1);
	startPose = _startPose;
	startPose.block(0, 3, 3, 1) = t;
}


void VRInterface::setScaleFactor(float factor)
{
	VLOG(1) << "Setting scale factor to " << factor;

	scaleFactor = factor;
	scaleMat = Eigen::Vector4f(1.f / factor, 1.f / factor, 1.f / factor, 1).asDiagonal();
}


bool VRInterface::createFramebuffer(Eigen::Vector2i dim, FramebufferDesc& desc)
{
	glGenFramebuffers(1, &desc.m_nRenderFramebufferId);
	glBindFramebuffer(GL_FRAMEBUFFER, desc.m_nRenderFramebufferId);

	glGenRenderbuffers(1, &desc.m_nDepthBufferId);
	glBindRenderbuffer(GL_RENDERBUFFER, desc.m_nDepthBufferId);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH_COMPONENT, dim.x(), dim.y());
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, desc.m_nDepthBufferId);

	glGenTextures(1, &desc.m_nRenderTextureId);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, desc.m_nRenderTextureId);
	glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGBA8, dim.x(), dim.y(), true);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, desc.m_nRenderTextureId, 0);

	glGenFramebuffers(1, &desc.m_nResolveFramebufferId);
	glBindFramebuffer(GL_FRAMEBUFFER, desc.m_nResolveFramebufferId);

	glGenTextures(1, &desc.m_nResolveTextureId);
	glBindTexture(GL_TEXTURE_2D, desc.m_nResolveTextureId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
	             dim.x(), dim.y(), 0,
	             GL_RGBA, GL_UNSIGNED_BYTE,
	             nullptr);
	glFramebufferTexture2D(GL_FRAMEBUFFER,
	                       GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
	                       desc.m_nResolveTextureId, 0);

	// check FBO status
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE)
		return false;

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	return true;
}

Eigen::Matrix4f VRInterface::getHMDMatrixProjectionEye(vr::Hmd_Eye nEye, float _eyeNearClip, float _eyeFarClip)
{
	if (!m_pHMD)
		return Eigen::Matrix4f::Identity();

	vr::HmdMatrix44_t mat = m_pHMD->GetProjectionMatrix(nEye, _eyeNearClip, _eyeFarClip);

	return convertSteamVRMatrixToMatrix4(mat);
}


Eigen::Matrix4f VRInterface::getHMDMatrixPoseEye(vr::Hmd_Eye nEye)
{
	if (!m_pHMD)
		return Eigen::Matrix4f::Identity();

	vr::HmdMatrix34_t matEyeRight = m_pHMD->GetEyeToHeadTransform(nEye);

	return convertSteamVRMatrixToMatrix4(matEyeRight).inverse();
}


Eigen::Matrix4f VRInterface::getEyeProjectionMatrix(vr::Hmd_Eye nEye)
{
	Eigen::Matrix4f matMVP;

	if (nEye == vr::Eye_Left)
	{
		matMVP = projectionLeft * leftEyePos * hmdPose * startPose * scaleMat;
	}
	else if (nEye == vr::Eye_Right)
	{
		matMVP = projectionRight * rightEyePos * hmdPose * startPose * scaleMat;
	}

	return matMVP; // Note that the matrix values are uninitialised if nEye is neither Left nor Right.
}


void VRInterface::updateHMDMatrixPose()
{
	if (!m_pHMD)
		return;

	vr::VRCompositor()->WaitGetPoses(trackedDevicePose, vr::k_unMaxTrackedDeviceCount, nullptr, 0);

	validPoseCount = 0;
	poseClasses = "";
	for (int nDevice = 0; nDevice < vr::k_unMaxTrackedDeviceCount; ++nDevice)
	{
		if (trackedDevicePose[nDevice].bPoseIsValid)
		{
			validPoseCount++;
			devicePose[nDevice] = convertSteamVRMatrixToMatrix4(trackedDevicePose[nDevice].mDeviceToAbsoluteTracking);

			if (devClassChar[nDevice] == 0)
			{
				switch (m_pHMD->GetTrackedDeviceClass(nDevice))
				{
				case vr::TrackedDeviceClass_Controller:        devClassChar[nDevice] = 'C'; break;
				case vr::TrackedDeviceClass_HMD:               devClassChar[nDevice] = 'H'; break;
				case vr::TrackedDeviceClass_Invalid:           devClassChar[nDevice] = 'I'; break;
				case vr::TrackedDeviceClass_GenericTracker:    devClassChar[nDevice] = 'G'; break;
				case vr::TrackedDeviceClass_TrackingReference: devClassChar[nDevice] = 'T'; break;
				default:                                       devClassChar[nDevice] = '?'; break;
				}
			}
			poseClasses += devClassChar[nDevice];
		}
	}

	if (trackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].bPoseIsValid)
	{
		hmdPose = devicePose[vr::k_unTrackedDeviceIndex_Hmd].inverse();

		// Reset the start pose if that hasn't been done yet.
		if (startPose.isIdentity())
			resetStartPose();
	}
}


Eigen::Matrix4f VRInterface::convertSteamVRMatrixToMatrix4(const vr::HmdMatrix34_t& matPose)
{
	Eigen::Matrix4f ret;
	ret <<
		matPose.m[0][0], matPose.m[0][1], matPose.m[0][2], matPose.m[0][3],
		matPose.m[1][0], matPose.m[1][1], matPose.m[1][2], matPose.m[1][3],
		matPose.m[2][0], matPose.m[2][1], matPose.m[2][2], matPose.m[2][3],
		0, 0, 0, 1;
	return ret;
}


Eigen::Matrix4f VRInterface::convertSteamVRMatrixToMatrix4(const vr::HmdMatrix44_t& matPose)
{
	Eigen::Matrix4f ret;
	ret <<
		matPose.m[0][0], matPose.m[0][1], matPose.m[0][2], matPose.m[0][3],
		matPose.m[1][0], matPose.m[1][1], matPose.m[1][2], matPose.m[1][3],
		matPose.m[2][0], matPose.m[2][1], matPose.m[2][2], matPose.m[2][3],
		matPose.m[3][0], matPose.m[3][1], matPose.m[3][2], matPose.m[3][3];
	return ret;
}


CameraInfo VRInterface::getEyeCameraInfo(vr::Hmd_Eye nEye)
{
	// Full model-view-projection matrix for shaders.
	Eigen::Matrix4f mvp_matrix = getEyeProjectionMatrix(nEye);

	// The model-view matrix (no projection or scale), from which we extract the camera pose.
	Eigen::Matrix4f M;
	if (nEye == vr::Eye_Left)  M = leftEyePos  * hmdPose * startPose;
	if (nEye == vr::Eye_Right) M = rightEyePos * hmdPose * startPose;

	// Camera centre from rigid-body transform.
	Eigen::Matrix3f R = M.block(0, 0, 3, 3);
	Eigen::Vector3f t = M.block(0, 3, 3, 1);
	Eigen::Point3f centre = -R.transpose() * t * scaleFactor; // scale from [m] to [cm]

	// Get forward and up directions.
	// NB. Invert forward direction to convert from MVG (look along +z) to OpenGL convention (look along -z).
	Eigen::Vector3f forward = -M.block(2, 0, 1, 3).transpose();
	Eigen::Vector3f up = M.block(1, 0, 1, 3).transpose();

	return CameraInfo(mvp_matrix, centre, forward);
}


void VRInterface::setupCompanionWindow(
    const std::string& title,
    const Eigen::Vector2i& windowDims,
    GLProgramMaintenance* maintenancePtr,
    GLFWwindow* parentContext)
{
	if (!m_pHMD)
		return;
	companionWindow = new GLWindow(title, windowDims, maintenancePtr, false, parentContext);

	int nWindowPosX = 700;
	int nWindowPosY = 100;

	companionWindow->setPosition(Eigen::Vector2i(nWindowPosX, nWindowPosY));

	std::vector<VertexDataWindow> vVerts;
	// left eye verts
	vVerts.push_back(VertexDataWindow(OpenVR::Vector2(-1, -1), OpenVR::Vector2(0, 1)));
	vVerts.push_back(VertexDataWindow(OpenVR::Vector2(0, -1), OpenVR::Vector2(1, 1)));
	vVerts.push_back(VertexDataWindow(OpenVR::Vector2(-1, 1), OpenVR::Vector2(0, 0)));
	vVerts.push_back(VertexDataWindow(OpenVR::Vector2(0, 1), OpenVR::Vector2(1, 0)));

	// right eye verts
	vVerts.push_back(VertexDataWindow(OpenVR::Vector2(0, -1), OpenVR::Vector2(0, 1)));
	vVerts.push_back(VertexDataWindow(OpenVR::Vector2(1, -1), OpenVR::Vector2(1, 1)));
	vVerts.push_back(VertexDataWindow(OpenVR::Vector2(0, 1), OpenVR::Vector2(0, 0)));
	vVerts.push_back(VertexDataWindow(OpenVR::Vector2(1, 1), OpenVR::Vector2(1, 0)));

	GLushort vIndices[] = { 0, 1, 3, 0, 3, 2, 4, 5, 7, 4, 7, 6 };
	companionWindowIndexSize = _countof(vIndices);

	glGenVertexArrays(1, &companionWindowVAO);
	glBindVertexArray(companionWindowVAO);

	companionWindowVertexBuffer = new GLBuffer();
	glGenBuffers(1, &companionWindowVertexBuffer->gl_ID);
	glBindBuffer(GL_ARRAY_BUFFER, companionWindowVertexBuffer->gl_ID);
	glBufferData(GL_ARRAY_BUFFER, vVerts.size() * sizeof(VertexDataWindow), &vVerts[0], GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(VertexDataWindow), (void*)offsetof(VertexDataWindow, position));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(VertexDataWindow), (void*)offsetof(VertexDataWindow, texCoord));

	companionWindowIndexBuffer = new GLBuffer();

	glGenBuffers(1, &companionWindowIndexBuffer->gl_ID);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, companionWindowIndexBuffer->gl_ID);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, companionWindowIndexSize * sizeof(GLushort), &vIndices[0], GL_STATIC_DRAW);

	//cleanup state changes
	glBindVertexArray(0);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}


void VRInterface::renderCompanionWindow()
{
	glClearColor(0.5f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	glViewport(0, 0, companionWindowDims.x(), companionWindowDims.y());

	glUseProgram(companionWindowProgram->getProgramID());
	glBindVertexArray(companionWindowVAO);
	glActiveTexture(GL_TEXTURE0);
	ErrorChecking::checkGLError();

	// render left eye (first half of index array )
	glBindTexture(GL_TEXTURE_2D, leftEyeDesc.m_nResolveTextureId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glDrawElements(GL_TRIANGLES, companionWindowIndexSize / 2, GL_UNSIGNED_SHORT, 0);

	// render right eye (second half of index array )
	glBindTexture(GL_TEXTURE_2D, rightEyeDesc.m_nResolveTextureId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glDrawElements(GL_TRIANGLES, companionWindowIndexSize / 2, GL_UNSIGNED_SHORT, (const void*)(uintptr_t)(companionWindowIndexSize));

	glBindVertexArray(0);
	glUseProgram(0);

	companionWindow->swapBuffers();
	glfwPollEvents();
}


void VRInterface::renderScene(vr::Hmd_Eye nEye, GLProgram& program, GLRenderModel& sceneModel)
{
	if (sceneModel.show)
	{
		program.setActiveRenderModel(&sceneModel);
		program.draw();
	}

	glUseProgram(0);
}


VRInterface::~VRInterface()
{
	if (m_pHMD)
	{
		vr::VR_Shutdown();
		m_pHMD = nullptr;
	}

	//if( m_pContext )
	auto window = companionWindow->getGLFWwindow();
	if (window)
	{
		glfwMakeContextCurrent(window);

		glDeleteRenderbuffers(1, &leftEyeDesc.m_nDepthBufferId);
		glDeleteTextures(1, &leftEyeDesc.m_nRenderTextureId);
		glDeleteFramebuffers(1, &leftEyeDesc.m_nRenderFramebufferId);
		glDeleteTextures(1, &leftEyeDesc.m_nResolveTextureId);
		glDeleteFramebuffers(1, &leftEyeDesc.m_nResolveFramebufferId);

		glDeleteRenderbuffers(1, &rightEyeDesc.m_nDepthBufferId);
		glDeleteTextures(1, &rightEyeDesc.m_nRenderTextureId);
		glDeleteFramebuffers(1, &rightEyeDesc.m_nRenderFramebufferId);
		glDeleteTextures(1, &rightEyeDesc.m_nResolveTextureId);
		glDeleteFramebuffers(1, &rightEyeDesc.m_nResolveFramebufferId);

		if (companionWindowVAO != 0)
		{
			glDeleteVertexArrays(1, &companionWindowVAO);
		}
	}
}

void VRInterface::updateEyeMatrices()
{
	projectionLeft = getHMDMatrixProjectionEye(vr::Eye_Left, eyeNearClip, eyeFarClip);
	projectionRight = getHMDMatrixProjectionEye(vr::Eye_Right, eyeNearClip, eyeFarClip);

	leftEyePos = getHMDMatrixPoseEye(vr::Eye_Left);
	rightEyePos = getHMDMatrixPoseEye(vr::Eye_Right);
}


// called by application
void VRInterface::initGLPrograms(std::vector<GLProgram*>* _programs)
{
	string projectSrcDir = determinePathToSource();
	string pathToShaders = projectSrcDir + "Shaders/General/";

	//Companion window program
	companionWindowProgram = new GLDefaultProgram(nullptr); //we do not use geometry, but the vao
	companionWindowProgram->setVertexShaderFilename(pathToShaders + "OpenVRCompanionWindow.vertex.glsl");
	companionWindowProgram->setFragmentShaderFilename(pathToShaders + "OpenVRCompanionWindow.fragment.glsl");
	companionWindowProgram->setContext(companionWindow->getGLFWwindow());
	companionWindowProgram->display = true;
	companionWindowProgram->job = "Clones the HMD views on the screen";
	_programs->push_back(companionWindowProgram);

	handControllerInfo[LeftHand].showController = true;
	handControllerInfo[RightHand].showController = true;
	initDefaultControllerPrograms(_programs);
	initRiftControllerPrograms(_programs);
}


bool VRInterface::setupStereoRenderTargets()
{
	if (!m_pHMD)
		return false;

	m_pHMD->GetRecommendedRenderTargetSize(&renderWidth, &renderHeight);
	perEyeBufferDims = Eigen::Vector2i(renderWidth, renderHeight);
	createFramebuffer(perEyeBufferDims, leftEyeDesc);
	createFramebuffer(perEyeBufferDims, rightEyeDesc);
	return true;
}


void VRInterface::renderEye(std::vector<GLProgram*>& _programs, const FramebufferDesc& fb_descr, vr::Hmd_Eye nEye)
{
	glEnable(GL_MULTISAMPLE);
	glBindFramebuffer(GL_FRAMEBUFFER, fb_descr.m_nRenderFramebufferId);
	glClearColor(0.5f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, renderWidth, renderHeight);
	ErrorChecking::checkGLError();

	CameraInfo cam_info = getEyeCameraInfo(nEye);
	for (int i = 0; i < _programs.size(); i++)
	{
		GLProgram* prog = _programs[i];
		if (prog->display || render_all_programs)
		{
			GLRenderModel* model = prog->getActiveRenderModel();
			if (model)
			{
				prog->passUniforms(cam_info);
				renderScene(nEye, *prog, *model);
			}
		}
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDisable(GL_MULTISAMPLE);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, fb_descr.m_nRenderFramebufferId);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fb_descr.m_nResolveFramebufferId);

	glBlitFramebuffer(0, 0, renderWidth, renderHeight, 0, 0, renderWidth, renderHeight,
	                  GL_COLOR_BUFFER_BIT,
	                  GL_LINEAR);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	ErrorChecking::checkGLError();
}


void VRInterface::render(std::vector<GLProgram*>& _programs)
{
	ErrorChecking::checkGLError();
	if (m_pHMD)
	{
		cycleCounter++;
		renderStereoTargets(_programs);
		renderCompanionWindow();

		vr::Texture_t leftEyeTexture = { (void*)(uintptr_t)leftEyeDesc.m_nResolveTextureId, vr::TextureType_OpenGL, vr::ColorSpace_Gamma };
		vr::VRCompositor()->Submit(vr::Eye_Left, &leftEyeTexture);
		vr::Texture_t rightEyeTexture = { (void*)(uintptr_t)rightEyeDesc.m_nResolveTextureId, vr::TextureType_OpenGL, vr::ColorSpace_Gamma };
		vr::VRCompositor()->Submit(vr::Eye_Right, &rightEyeTexture);
	}

	if (m_bVblank && m_bGlFinishHack)
	{
		//$ HACKHACK. From gpuview profiling, it looks like there is a bug where two renders and a present
		// happen right before and after the vsync causing all kinds of jittering issues. This glFinish()
		// appears to clear that up. Temporary fix while I try to get nvidia to investigate this problem.
		// 1/29/2014 mikesart
		glFinish();
	}

	//companionWindow->swapBuffers();

	// Clear
	{
		// We want to make sure the glFinish waits for the entire present to complete, not just the submission
		// of the command. So, we do a clear here right here so the glFinish will wait fully for the swap.
		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	// Flush and wait for swap.
	if (m_bVblank)
	{
		glFlush();
		glFinish();
	}
	pollChanges();
	updateHMDMatrixPose();
}

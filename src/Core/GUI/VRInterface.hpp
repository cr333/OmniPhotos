/**************************************************************************
 * @file   VRInterface.hpp 
 * 
 * @authors Tobias Bertel, Mingze Yuan, Reuben Lindroos, Christian Richardt
 **************************************************************************/

#pragma once

#include "3rdParty/Eigen.hpp"

#include "Core/GL/GLDefaultPrograms.hpp"
#include "Core/Geometry/Mesh.hpp"
#include "Core/Geometry/Sphere.hpp"
#include "Viewer/ViewerApp.hpp"

#include <3rdParty/openvr/files/Matrices.h>

#include <openvr.h>

#include <memory>
#include <string>
#include <vector>


std::string getTrackedDeviceString(vr::TrackedDeviceIndex_t unDevice, vr::TrackedDeviceProperty prop, vr::TrackedPropertyError* peError = nullptr);
bool getDigitalActionRisingEdge(vr::VRActionHandle_t action, vr::VRInputValueHandle_t* pDevicePath = nullptr);
bool getDigitalActionFallingEdge(vr::VRActionHandle_t action, vr::VRInputValueHandle_t* pDevicePath = nullptr);
bool getDigitalActionState(vr::VRActionHandle_t action, vr::VRInputValueHandle_t* pDevicePath = nullptr);


enum Controllers
{
	LeftHand = 0,
	RightHand = 1,
};


static const char* getTextForControllerEnum(int enumVal)
{
	static const char* controller_enum_strings[] = { "Left", "Right" };

	return controller_enum_strings[enumVal];
}

struct ControllerInfo_t
{
	~ControllerInfo_t();

	vr::VRInputValueHandle_t m_source = vr::k_ulInvalidInputValueHandle;
	vr::VRActionHandle_t m_actionPose = vr::k_ulInvalidActionHandle;
	vr::VRActionHandle_t m_actionHaptic = vr::k_ulInvalidActionHandle;
	Eigen::Matrix4f pose = Eigen::Matrix4f::Identity();

	vr::InputPoseActionData_t poseData;

	Sphere* sphere = nullptr;
	GLDefaultProgram* sphere_program = nullptr; // initialised in GLHMD::initControllerPrograms
	Mesh* mesh = nullptr;
	GLDefaultProgram* controller_program = nullptr; // belongs to fancy controller findOrLoadModel...

	//We add texture slots to the programs once after we loaded the mesh for the first time
	bool added_texture = false;
	bool showController = true;
};


struct FramebufferDesc
{
	GLuint m_nDepthBufferId;
	GLuint m_nRenderTextureId;
	GLuint m_nRenderFramebufferId;
	GLuint m_nResolveTextureId;
	GLuint m_nResolveFramebufferId;
};
/**
 * @brief  VRI - Virtual Reality Interface. 
 * 
 * This file acts as the layer between the ViewerApp, OpenGL and OpenVR. 
 * Its functionality includes: 
 *	- Initialising the OpenVR environment. 
 *	- handling the user interface in the VR environment. 
 *	- Implementing OpenGL specifically for OpenVR.   .
 */
class VRInterface
{
public:
	/**
	 * Default constructor for the VRI. Calls initOpenVR
	 * so SteamVR is initialised here.
	 * 
	 * @param app the ViewerApp. Passed here such that handleInput 
	 * can pass commands back to the application.
	 */
	VRInterface(float _eyeNearClip, float _eyeFarClip, Eigen::Matrix4f initRot, ViewerApp* app);

	/**
	 * Deletes handcontroller, companion window, 
	 * controllerPlane and other GL programs on exit.
	 */
	~VRInterface();

	/**
	 * Initialises the proxy geometry and passes the
	 * parameters to @fn setupCompanionWindow.
	 */
	void init(const std::string& title, const Eigen::Vector2i& windowDims, GLProgramMaintenance* maintenancePtr, GLFWwindow* parentContext = nullptr);


	/**
	 * companinon window parameters
	 * These need to be public for the ViewerApp to initialise it correctly. 
	 * TODO: fix this.
	 */
	GLWindow* companionWindow = nullptr;
	Eigen::Vector2i companionWindowDims;
	GLDefaultProgram* companionWindowProgram = nullptr;

	////////////////////
	//This is the renderModel usually
	GLRenderModel* companion_window_model = nullptr;

	//TODO:Is all part of the RenderModel.
	GLuint companionWindowVAO = 0;
	GLBuffer* companionWindowVertexBuffer = nullptr;
	GLBuffer* companionWindowIndexBuffer = nullptr;
	unsigned int companionWindowIndexSize;

	/*
	@brief GLPrograms used by the HMD: companion window, 2 controllers, dataset-plane on left controller (todo)
	*/
	void initGLPrograms(std::vector<GLProgram*>* gl_programs);

	//------------------------------------------------------------------

	//TODO: rename initControllers()
	/**
	 * @brief initialises the controller programs + models
	 * The models will be initialised as spheres. if spheres
	 * are displayed in the HMD, something has gone wrong in 
	 * handleInput().
	 */
	void initDefaultControllerPrograms(std::vector<GLProgram*>* _programs);
	void initRiftControllerPrograms(std::vector<GLProgram*>* _programs);

	/**
	 * @brief handles all input from the controllers.
	 * called before or after every render cycle.
	 * Will also update controller render models when these are 
	 * located at runtime.
	 */
	void handleInput(std::vector<GLProgram*>* _programs);

	//void render(std::vector<std::pair<GLProgram*, GLRenderModel*>>& programModelPairs);
	void render(std::vector<GLProgram*>& _gl_programs);

	// Matrices -----------------------------------------------------------------------------------
	void setScaleFactor(float factor);
	inline float getScaleFactor() { return scaleFactor; }

	/** resets headset start pose to center of camera circle */
	void resetStartPose();

	// Sets a new start pose, e.g. after loading a new dataset.
	void setStartPose(Eigen::Matrix4f _startPose);


private:
	/** the ViewerApp */
	ViewerApp* app = nullptr;

	/** internal tracker, mostly used for debugging */
	int cycleCounter = 0;

	////////////////////////////////////////Rendering/////////////////////////////////////////////////////

	FramebufferDesc leftEyeDesc;
	FramebufferDesc rightEyeDesc;

	//determined by OpenVR. HMD specific.
	uint32_t renderWidth;
	uint32_t renderHeight;
	Eigen::Vector2i perEyeBufferDims;
	//displayed on monitor when HMD is used.
	float eyeNearClip = 0.1f;
	float eyeFarClip = 30.0f;
	bool createFramebuffer(Eigen::Vector2i dim, FramebufferDesc& desc);
	bool setupStereoRenderTargets();
	void renderEye(std::vector<GLProgram*>& _programs, const FramebufferDesc& fb_descr, vr::Hmd_Eye nEye);
	void renderScene(vr::Hmd_Eye nEye, GLProgram& program, GLRenderModel& sceneModel);
	void renderCompanionWindow();
	/**
	* handles iteration through the program. Also passes the uniforms to
	* the controller meshes in order to update their positions.
	*
	* @param _gl_programs passed by GLApplication
	*/
	void renderStereoTargets(std::vector<GLProgram*>& _gl_programs);

	/////////////////////////////////////////HMD UI/////////////////////////////////////////////////////

	// All meshes that relate to HMD UI, i.e. controllers and plane.
	std::vector<Mesh*> hmd_ui_meshes;

	// Set true to use controller geometry from OpenVR.
	bool use_controllers = false;

	/**
	 * Will initialise the programs needed to render mini versions.
	 * of the proxy geometries above one of the controllers.
	 */
	void initProxySelectionPrograms();
	void updateControllerPositions();
	void handleControllerActions();
	void loadControllers();

	Mesh* loadRiftControllerMesh(const std::string& pchRenderModelName, const vr::RenderModel_t& _vrModel, const vr::RenderModel_TextureMap_t& _vrDiffuseTexture);
	Mesh* findOrLoadControllerMesh(std::vector<Mesh*>& _hmd_ui_meshes, std::string pchRenderModelName);

	void setupCompanionWindow(const std::string& title, const Eigen::Vector2i& windowDims, GLProgramMaintenance* maintenancePtr, GLFWwindow* parentContext = nullptr);

	/**
	 * @defgroup hmdControls
	 * 
	 * These are the controls being used by 
	 * the HMD to toggle settings in the CCSVA.
	 * 
	 * @{
	 */

	/** toggle method */
	vr::VRActionHandle_t actionToggleMethod = vr::k_ulInvalidActionHandle;

	/** toggle proxy */
	vr::VRActionHandle_t actionNextProxy = vr::k_ulInvalidActionHandle;
	vr::VRActionHandle_t actionPreviousProxy = vr::k_ulInvalidActionHandle;

	/** toggle display mode (e.g world lines) */
	vr::VRActionHandle_t actionToggleDisplayMode = vr::k_ulInvalidActionHandle;

	/** load next dataset */
	vr::VRActionHandle_t actionNextDataset = vr::k_ulInvalidActionHandle;

	/** load previous dataset */
	vr::VRActionHandle_t actionPreviousDataset = vr::k_ulInvalidActionHandle;

	/** equivalent of pressing 's' */
	vr::VRActionHandle_t actionRecentreHeadset = vr::k_ulInvalidActionHandle;

	/** other */
	vr::VRActionHandle_t actionTriggerHaptic = vr::k_ulInvalidActionHandle;
	vr::VRActionSetHandle_t actionsetOmniphotos = vr::k_ulInvalidActionSetHandle;

	/** @} */ //end of group hmdControls

	/////////////////////////////////////Low-level Rendering////////////////////////////////////////
	// Matrices -----------------------------------------------------------------------------------
	float scaleFactor = 100;
	Eigen::Matrix4f startPose;
	Eigen::Matrix4f scaleMat;
	Eigen::Matrix4f hmdPose;
	Eigen::Matrix4f leftEyePos;
	Eigen::Matrix4f rightEyePos;
	Eigen::Matrix4f projectionLeft;
	Eigen::Matrix4f projectionRight;

	Eigen::Matrix4f convertSteamVRMatrixToMatrix4(const vr::HmdMatrix34_t& matPose);
	Eigen::Matrix4f convertSteamVRMatrixToMatrix4(const vr::HmdMatrix44_t& matPose);

	Eigen::Matrix4f getHMDMatrixProjectionEye(vr::Hmd_Eye nEye, float _eyeNearClip, float _eyeFarClip);
	Eigen::Matrix4f getHMDMatrixPoseEye(vr::Hmd_Eye nEye);

	CameraInfo getEyeCameraInfo(vr::Hmd_Eye nEye);
	Eigen::Matrix4f getEyeProjectionMatrix(vr::Hmd_Eye nEye);

	void updateHMDMatrixPose();

	/*
	 * Updates the camera matrices for left and right eye in the hmd
	 */
	void updateEyeMatrices();

	////////////////////////////////////////// OpenVR //////////////////////////////////////////////////////

	vr::IVRSystem* m_pHMD = nullptr;
	vr::TrackedDevicePose_t trackedDevicePose[vr::k_unMaxTrackedDeviceCount];
	Eigen::Matrix4f devicePose[vr::k_unMaxTrackedDeviceCount];

	bool m_bVblank = true;
	bool m_bGlFinishHack = true;
	void processVREvent(const vr::VREvent_t& event);

	ControllerInfo_t handControllerInfo[2];

	int trackedControllerCount;
	int trackedControllerCount_Last;
	int controllerVertcount = 0;
	int validPoseCount;
	int validPoseCount_Last;
	std::string poseClasses;                          // what classes we saw poses for this frame
	char devClassChar[vr::k_unMaxTrackedDeviceCount]; // for each device, a character representing its class

	bool initOpenVR();
	void pollChanges();

	//debug
	bool render_all_programs = false;
	bool use_default_controller = false;
};

/**************************************************************************
 * @file   GLApplication.hpp
 * @brief  The parent class to ViewerApp, acts as an interface
 *         for certain classes (e.g. VRInterface).
 * 
 * @authors Tobias Bertel, Mingze Yuan, Reuben Lindroos, Christian Richardt
 **************************************************************************/
#pragma once

#include "Core/Application.hpp"
#include "Core/DisplaySettings.hpp"
#include "Core/GL/GLCamera.hpp"
#include "Core/GL/GLProgram.hpp"
#include "Core/GUI/GLWindow.hpp"
#include "Core/GUI/InputHandler.hpp"
#include "Core/GUI/Mouse.hpp"
#include "Core/Loaders/TextureLoader.hpp"

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

class VRInterface;

class GLApplication : public Application
{
public:
	/**
	 * Default constructor
	 */
	GLApplication();

	/**
	 * (virtual) Destructor
	 */
	virtual ~GLApplication();

	/**
	 * Pointer to the VR Interface class, which is always defined but points to an empty class
	 * when not compiling with OpenVR. This it to avoid different class layouts depending on
	 * preprocessor defines, which can lead to weird bugs.
	 */
	VRInterface* hmd = nullptr;

	// Whether or not VR rendering with a head-mounted display is enabled.
	bool enableVR = false;

	/**
	 * This needs to be used in GLApplications. Each program is registered with a
	 * render model initially. GLPrograms can be re-used by just exchanging the used rendermodel.
	 */
	//std::vector<std::pair<GLProgram*, GLRenderModel*>> programModelPairs;
	std::vector<GLProgram*> app_gl_programs;

	void addProgram(GLProgram* _gl_program)
	{
		app_gl_programs.push_back(_gl_program);
	}

	void addPrograms(std::vector<GLProgram*> _gl_programs)
	{
		for (int i = 0; i < _gl_programs.size(); i++)
			app_gl_programs.push_back(_gl_programs[i]);
	}

	//std::vector<GLRenderModel*> models;


	//GUI

	bool* ignoreMouseInput = nullptr;    //pointer assigned by imgui
	bool* ignoreKeyboardInput = nullptr; //pointer assigned by imgui

	bool recordingSessionAvailable = false;

	TextureLoader* textureLoader = nullptr;

	// (0, Megastereo)
	// (1, OmniPhotos)
	std::vector<std::pair<int, GLProgram*>> methods;
	//TODO: look into std::map instead

	// (0, nullptr) -> black background
	// (1, GLCamera plane)
	// (2, cylinder)
	// (3, sphere)
	// (4, deformed sphere A)
	// (5, deformed sphere B)
	std::vector<std::pair<int, GLRenderModel*>> proxies;


	// Keys and mouse
	std::shared_ptr<Mouse> mouse;
	std::shared_ptr<InputHandler> inputHandler;
	std::shared_ptr<InputHandler> getInputHandler();

	GLProgramMaintenance maintenance;
	std::string windowTitle;

	bool guiInit = false;
	bool showImGui = false; // set to true in ViewerGUI::init()
	double lastRunTime = 0.;

	void setGLwindow(GLWindow* _glWindow); // when? -> I think it doesn't matter as soon we are all in the same OpenGL context all the time
	GLWindow* getGLwindow();               // very important

	DisplaySettings* settings = nullptr;

	/**
	* Input is handled here unless it is specific to the application, e.g. compute optical flow for PreprocessingApps.
	* This method will be called last in GLApplication::handleUserInput().
	* Whenever an input event (mouse or key) is consumed, the handling process will return.
	*
	* The (optional) display of overlays to the main window, e.g. created by
	* CameraSetupVisualization, is updated by using this pointer.
	*/
	void sharedUserInput();

	//Window information needed to setup the Framebuffer.
	void setWindowDimensions(int dimX, int dimY);

	//TODO: Do we need these two?
	float getAspect();
	Eigen::Vector2i getWindowSize();

	virtual void setupRecordingSession() = 0;

	/**
	 * @brief initPrograms initialises GLPrograms, i.e. setting a default GLRenderModel, textures and shader files.
	 * Initialises variables used by GLPrograms to render the scene.
	 * -> It sets up the GLProgram with all its inputs, i.e. textures and buffers (proxy geometry in form of a GLRenderModel)
	 * 
	 * @return int 
	 */
	virtual int initPrograms() = 0;
	virtual void initGLCamera() = 0;
	virtual void initGLCameraToCentre() = 0;

	virtual std::string getScreenShotName() = 0;

	/**
	 * Initializes the GLApplication. This is usually over-ridden.
	 *
	 * @returns	An error code.
	 */
	int init();
	virtual void run() = 0; // Function should not be used directly; ubclasses need to override it.

	/**
	 *	Called on every loop. The render loop for the GLShaders is called from here. This function checks whether there 
	 * 	there was a GL Error and pauses the rendering (but does not exit the application) if there is.
	 */
	void render(); // is called at the beginning of children methods
	void postRender();

	virtual void handleUserInput() = 0;
	virtual void updateProgramDisplay() = 0;


	/**
	* Checks whether display properties have changed.
	*
	* @param The flag points to the GLProgram which is currently running.
	*/
	void updateProgramDisplay(int method_id);
	void updateRenderModel(int render_model_id);


	// List of supported rendering methods (GLProgram).
	std::vector<std::string> methods_str;
	// List of supported proxy geometries (GLRenderModel) for rendering.
	std::vector<std::string> proxies_str;


	// Updates shader programs.
	int update(bool forceUpdate = false);

	/**
	 * A function designed to reduce the ammount of repeated code when calling an
	 * application from a mainFile.
	 *  
	 * The argument func needs to have a pointer to the initialised Application object for this
	 * trick to work.
	 *
	 * @param func windowsuzefun initialized with pointer to correct object.
	 */
	void callbackBoilerplate(GLFWwindowsizefun func); //GLFWwindow* window,int width, int height);

	GLCamera* getGLCamera() { return gl_cam; }
	void setGLCamera(GLCamera* _gl_cam) { gl_cam = _gl_cam; }

	void setActiveMethod(GLProgram* new_method) { active_method = new_method; }
	void setActiveProxy(GLRenderModel* new_proxy) { active_proxy = new_proxy; }
	GLRenderModel* getActiveProxy() { return active_proxy; }

private:
	GLWindow* window = nullptr;

	/** @brief	Holds pointer of the active GLCamera of the running app. Shared with visualisations. */
	GLCamera* gl_cam = nullptr;

	/** @brief	Holds pointer of the active GLProgram of the running app. Shared with visualisations. (Method Combobox)*/
	GLProgram* active_method = nullptr;

	/** @brief	Holds pointer of the active proxy (GLRenderModel) of the running app. Shared with visualisations. (Proxies Combobox)*/
	GLRenderModel* active_proxy = nullptr;

	/**     
	* Set to false by ::render() method as it was clogging the output with a lot of errors.
	* May want to override this as it may hide other exceptions being thrown and
	* make it more difficult to debug.
	*/
	bool print_exception_to_terminal = true;
};

#include "GLApplication.hpp"

#include "Utils/Logger.hpp"

#ifdef WITH_OPENVR
	#include "Core/GUI/VRInterface.hpp"
#endif


GLApplication::GLApplication()
{
	windowTitle = "GLApplication";
	settings = new DisplaySettings();
}


int GLApplication::init()
{
	inputHandler = std::make_shared<InputHandler>();
	mouse = std::make_shared<Mouse>(inputHandler);
	return 0;
}


GLApplication::~GLApplication()
{
	if (window)
	{
		delete window;
		window = nullptr;
	}

	if (gl_cam)
	{
		delete gl_cam;
		gl_cam = nullptr;
	}
}


int GLApplication::update(bool forceUpdate)
{
	maintenance.updatePrograms(forceUpdate);
	return 0;
}


/**
 * @fn	void GLApplication::updateProgramDisplay(int method_id)
 *
 * @brief	Checks whether display properties have changed. 
 *          It basically sets all dynamic programs except the "active" one invisible
 * 			
 * @param 	The "method_id" points to the GLProgram which is currently running.
 */
void GLApplication::updateProgramDisplay(int method_id)
{
	if (methods.size() == 0)
	{
		LOG(WARNING) << "No programs to display.";
		return;
	}

	// Setting all programs to not display.
	for (auto& program : methods)
		program.second->display = false;

	if (method_id < 0 || method_id > methods.size())
	{
		LOG(WARNING) << "Program " << method_id << " out of range [0, " << methods.size() << "]. Ignoring.";
		return;
	}

	// Display the program according to viewerSettings-method_id.
	methods[method_id].second->display = true;
}


void GLApplication::updateRenderModel(int render_model_id)
{
	if (proxies.size() == 0)
	{
		LOG(ERROR) << "No proxies to use!";
		return;
	}
	//set new renderModel to "active" GlProgram, determined by GUI Combobox
	active_method->setActiveRenderModel(proxies[render_model_id].second);
}


void GLApplication::postRender()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	getGLwindow()->renderQuad();
	glfwSwapBuffers(getGLwindow()->getGLFWwindow());
	glfwPollEvents();
}


/**
* @fn	void GLApplication::render()
*
* @brief	Called in "run()"-loop .
* 1. Renders all visible GLPrograms to screen (GLCamera::renderPrograms()), including programs from the Visualization
* 2. Triggers video recording using GLCameraControl
*/
void GLApplication::render()
{
#ifdef WITH_OPENVR
	if (settings->enableVR) // use OpenVR for VR rendering
	{
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);
		settings->distanceToProjectionPlane = 1000; // hmd->getPlaneDistance();
		hmd->render(app_gl_programs);
	}
	else // standard window-based rendering
#endif   // WITH_OPENVR
	{
		// CameraSetup-specific
		settings->distanceToProjectionPlane = gl_cam->getPlaneDistance();
		glfwMakeContextCurrent(getGLwindow()->getGLFWwindow());
		glBindFramebuffer(GL_FRAMEBUFFER, getGLwindow()->framebuffer->id);

		// background
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);

		if (gl_cam->record)
			gl_cam->cameraControl->step();

		try
		{
			gl_cam->renderPrograms(app_gl_programs);
			print_exception_to_terminal = true;
		}
		catch (std::exception& e)
		{
			if (print_exception_to_terminal)
			{
				LOG(WARNING) << "Exception: " << e.what();
				LOG(INFO) << "Press 'r' to reload shaders and try again or close window to exit.";
				print_exception_to_terminal = false;
			}
		}

		if (gl_cam->record)
		{
			// Save a screenshot if a filename is defined.
			const auto screenshot_name = gl_cam->cameraControl->screenshot_name;
			if (!screenshot_name.empty())
				getGLwindow()->takeScreenshot(screenshot_name);
		}
	}
}


std::shared_ptr<InputHandler> GLApplication::getInputHandler()
{
	return inputHandler;
}


void GLApplication::sharedUserInput()
{
	auto inputHandler = getInputHandler();
	if (inputHandler == nullptr)
	{
		LOG(WARNING) << "No InputHandler set.";
		return;
	}

	// Quit the application.
	if (inputHandler->escape_pressed)
	{
		shouldShutdown = true;
		inputHandler->escape_pressed = false;
		return;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////
	// Show visualisations.
	// Show camera circle.
	if (inputHandler->c_key_pressed)
	{
		settings->showFittedCircle = !settings->showFittedCircle;
		inputHandler->c_key_pressed = false;
		return;
	}

	// Show radial lines.
	if (inputHandler->d_key_pressed)
	{
		settings->showRadialLines = !settings->showRadialLines;
		inputHandler->d_key_pressed = false;
		return;
	}

	// Show world positions.
	if (inputHandler->w_key_pressed)
	{
		if (!settings->load3DPoints)
			return;
		settings->showWorldPoints = !settings->showWorldPoints;
		inputHandler->w_key_pressed = false;
		return;
	}

	// Show coordinate axes at the origin of the world.
	if (inputHandler->a_key_pressed)
	{
		settings->showCoordinateAxes = !settings->showCoordinateAxes;
		inputHandler->a_key_pressed = false;
		return;
	}

	// Show camera setup/geometry.
	if (inputHandler->m_key_pressed)
	{
		settings->showCameraGeometry = !settings->showCameraGeometry;
		inputHandler->m_key_pressed = false;
		return;
	}
	///////////////////////////////////////////////////////////////////////////////////////////////


	///////////////////////////////////////////////////////////////////////////////////////////////
	// GLCamera
	// Set GLCamera (intrinsics + extrinsics) to intial state defined by GLApplication::initGLCamera();
	if (inputHandler->plus_pressed)
	{
		initGLCamera();
		inputHandler->plus_pressed = false;
		return;
	}

	// Set GLCamera to centroid of the setup and align with the orientation of the fitted shape
	// (circle normal, eigenvector with smallest eigenwert in general for linear or planar trajectories)
	if (inputHandler->num1_pressed || inputHandler->backspace_pressed)
	{
		initGLCameraToCentre();
		mouse->freeTranslate = false;
		inputHandler->num1_pressed = false;
		inputHandler->backspace_pressed = false;
		return;
	}

	// Resets the recording session according to GLApplication::setupRecordingSession().
	if (inputHandler->u_key_pressed)
	{
		gl_cam->record = false;
		setupRecordingSession();
		inputHandler->u_key_pressed = false;
	}

	// GLCamera, move along path defined in your GLApplication's setupRecordingSession().
	if (inputHandler->f9_pressed)
	{
		if (!recordingSessionAvailable)
			return;
		gl_cam->record = !gl_cam->record;
		inputHandler->f9_pressed = false;
		return;
	}

	// Move to the left.
	if (inputHandler->leftArrow_pressed)
	{
		gl_cam->moveRight(-0.5f);
	}

	// Move to the right.
	if (inputHandler->rightArrow_pressed)
	{
		gl_cam->moveRight(0.5f);
	}

	// Move forward.
	if (inputHandler->upArrow_pressed)
	{
		// "forward" might mean moving out of the camera's plane.
		if (mouse->freeTranslate)
			gl_cam->moveForward(0.5f);
		else
			gl_cam->moveForwardInPlane(0.5f);
	}

	// Move backward.
	if (inputHandler->downArrow_pressed)
	{
		// "backward" might mean moving out of the camera's plane.
		if (mouse->freeTranslate)
			gl_cam->moveForward(-0.5f);
		else
			gl_cam->moveForwardInPlane(-0.5f);
	}
	///////////////////////////////////////////////////////////////////////////////////////////////


	///////////////////////////////////////////////////////////////////////////////////////////////
	// GLPrograms:

	// Toggle through GLPrograms associated to the GLApplication
	if (inputHandler->num0_pressed || inputHandler->tab_pressed)
	{
		int numPrograms = (int)methods.size();

		if (inputHandler->ctrl_pressed) // previous
			settings->switchGLProgram = (settings->switchGLProgram + numPrograms - 1) % numPrograms;
		else // next
			settings->switchGLProgram = (settings->switchGLProgram + numPrograms + 1) % numPrograms;

		//if (settings->switchGLProgram == 0)
		//	LOG(INFO) << "Switched DisplayMode: None";
		//else
		LOG(INFO) << "Switched method: " << methods[settings->switchGLProgram].second->job;

		inputHandler->num0_pressed = false;
		inputHandler->tab_pressed = false;
		return;
	}

	// Toggle Display mode (previous)
	if (inputHandler->b_key_pressed && inputHandler->ctrl_pressed)
	{
		DisplayMode& mode = settings->displayMode;
		mode = (DisplayMode)((mode + 12 - 1) % 12);
		inputHandler->b_key_pressed = false;
		return;
	}

	// Toggle Display mode (next)
	if (inputHandler->b_key_pressed)
	{
		DisplayMode& mode = settings->displayMode;
		mode = (DisplayMode)((mode + 1) % 12);
		inputHandler->b_key_pressed = false;
		return;
	}

	// Toggle to use only the central ray of the GLCamera to determine image pairs (this is basically video interpolation).
	// OR use realistic rays per pixel (which introduces non-linear perspective for closeby objects).
	if (inputHandler->z_key_pressed)
	{
		settings->raysPerPixel = (settings->raysPerPixel + 1) % 2;
		inputHandler->z_key_pressed = false;
		return;
	}

	// toggle flow on/off
	if (inputHandler->o_key_pressed)
	{
		if (settings->opticalFlowLoaded)
			settings->useOpticalFlow = !settings->useOpticalFlow;

		inputHandler->o_key_pressed = false;
		return;
	}
	///////////////////////////////////////////////////////////////////////////////////////////////


	///////////////////////////////////////////////////////////////////////////////////////////////
	//Ask for stuff (pixel colour, hide GUI, print info)
	//Read pixel colour from GLwindow (accesses framebuffer internally)
	if (inputHandler->alt_pressed)
	{
		double xpos, ypos;
		glfwGetCursorPos(getGLwindow()->getGLFWwindow(), &xpos, &ypos);
		getGLwindow()->readColor(xpos, ypos);
		return;
	}

	// Print timing
	if (inputHandler->t_key_pressed)
	{
		LOG(INFO) << "This run took: " << 1000 * inputHandler->deltaTime << " ms";
		inputHandler->t_key_pressed = false;
		return;
	}

	// Print camera info
	if (inputHandler->n_key_pressed)
	{
		gl_cam->print(true);
		inputHandler->n_key_pressed = false;
		return;
	}

	// Take screenshot
	if (inputHandler->f11_pressed)
	{
		window->takeScreenshot(getScreenShotName());
		inputHandler->f11_pressed = false;
		return;
	}


	///////////////////////////////////////////////////////////////////////////////////////////////
	// GLApplication
	// Reload, recompile and relink GLPrograms to reflect changes in shader files on runtime.
	if (inputHandler->r_key_pressed)
	{
		update(true);
		inputHandler->r_key_pressed = false;
		return;
	}

#ifdef WITH_OPENVR
	// VR-specific keys.
	if (settings->enableVR)
	{
		// Reset the HMD pose to the current pose.
		if (inputHandler->s_key_pressed)
		{
			hmd->resetStartPose();
			inputHandler->s_key_pressed = false;
			return;
		}
	}
#endif // WITH_OPENVR

	mouse->handleUserInput(gl_cam, *window);
}


void GLApplication::setGLwindow(GLWindow* _glWindow)
{
	window = _glWindow;
	setWindowDimensions(window->getWidth(), window->getHeight());
	glfwFocusWindow(window->getGLFWwindow());
}


GLWindow* GLApplication::getGLwindow()
{
	return window;
}


void GLApplication::callbackBoilerplate(GLFWwindowsizefun func)
{
	auto window = getGLwindow()->getGLFWwindow();
	//if (getInputHandler())
	//	getInputHandler()->registerCallbacks(window);
	//glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_FALSE);
	glfwSetWindowSizeCallback(window, func);
}


float GLApplication::getAspect()
{
	return window->getAspect();
}


Eigen::Vector2i GLApplication::getWindowSize()
{
	return window->getWindowSize();
}


void GLApplication::setWindowDimensions(int dimX, int dimY)
{
	window->setWidth(dimX);
	window->setHeight(dimY);
}

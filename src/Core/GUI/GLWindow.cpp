#include "GLWindow.hpp"

#include "3rdParty/fs_std.hpp"
#include "3rdParty/json/json.h"

#include "Core/GL/GLDefaultPrograms.hpp"

#include "Utils/ErrorChecking.hpp"
#include "Utils/Exceptions.hpp"
#include "Utils/IOTools.hpp"
#include "Utils/Logger.hpp"
#include "Utils/Utils.hpp"
#include "Utils/cvutils.hpp"


using namespace std;


static void glfw_error_callback(int error, const char* description)
{
	LOG(ERROR) << "GLFW Error #" << error << ": " << description;
}


GLWindow::GLWindow(
    const std::string& _name,
    const Eigen::Vector2i& _windowDims,
    GLProgramMaintenance* _maintenance,
    bool useQuadRendering,
    GLFWwindow* parentContext,
    bool _visible)
{
	VLOG(2) << "GLWindow constructor";

	maintenance = _maintenance;
	windowVisible = _visible;
	wInfo = make_shared<WindowInfo>(_windowDims.x(), _windowDims.y());
	wInfo->print();

	projectSrcDir = determinePathToSource();

	int fbRes = setupFramebuffer(_name, useQuadRendering, parentContext);
	if (fbRes == 0)
		VLOG(1) << "Framebuffer creation successful.";
	else
		LOG(WARNING) << "Framebuffer creation failed.";

	readUserFile();
	wInfo->print();
	glfwSetWindowSize(glfwWindow, wInfo->width, wInfo->height);
	glfwSetWindowPos(glfwWindow, wInfo->posX, wInfo->posY);
	ErrorChecking::checkGLError();
}


float GLWindow::getAspect() const
{
	return float(wInfo->width) / float(wInfo->height);
}


void GLWindow::setPosition(const Eigen::Vector2i& pos)
{
	wInfo->posX = pos(0);
	wInfo->posY = pos(1);
}


void GLWindow::setWindowSize(const Eigen::Vector2i& size)
{
	wInfo->width = size(0);
	wInfo->height = size(1);
}


const Eigen::Vector2i GLWindow::getWindowSize()
{
	return Eigen::Vector2i(wInfo->width, wInfo->height);
}


// TODO: resolve ambiguity with setPosition() function
void GLWindow::setWindowPosition(int x, int y)
{
	VLOG(2) << "GLWindow::setWindowPosition(" << x << ", " << y << ")";
	glfwSetWindowPos(getGLFWwindow(), x, y);
}


void GLWindow::setupQuadRendering()
{
	if (quadVAO == 0)
		glGenVertexArrays(1, &quadVAO);
	glBindVertexArray(quadVAO);

	if (quadVAO > 0)
		glDeleteBuffers(1, &quadVAO);

	const GLfloat g_quad_vertex_buffer_data[] = {
		-1.0f, -1.0f, 0.0f,
		1.0f, -1.0f, 0.0f,
		-1.0f, 1.0f, 0.0f,
		-1.0f, 1.0f, 0.0f,
		1.0f, -1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,
	};

	glEnableVertexAttribArray(0);
	glGenBuffers(1, &quadVAO);
	glBindBuffer(GL_ARRAY_BUFFER, quadVAO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_quad_vertex_buffer_data), g_quad_vertex_buffer_data, GL_STATIC_DRAW);

	// 1st attribute buffer : vertices
	glVertexAttribPointer(
	    0,        // attribute 0. No particular reason for 0, but must match the layout in the shader.
	    3,        // size
	    GL_FLOAT, // type
	    GL_FALSE, // normalized?
	    0,        // stride
	    (void*)0  // array buffer offset
	);

	quadProgram = make_shared<GLQuadProgram>(framebuffer, quadVertexbuffer);
	quadProgram->setVertexShaderFilename(projectSrcDir + "Shaders/General/Quad.vertex.glsl");
	quadProgram->setFragmentShaderFilename(projectSrcDir + "Shaders/General/Quad.fragment.glsl");
	quadProgram->setVertexArray(quadVAO);
}


void GLWindow::renderQuad()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, wInfo->width, wInfo->height);
	quadProgram->draw();
}


int GLWindow::setupFramebuffer(const std::string& title, bool useQuadRendering, GLFWwindow* parentContext)
{
	glfwSetErrorCallback(glfw_error_callback);

	if (glfwWindow)
	{
		glfwMakeContextCurrent(glfwWindow);
		glfwSetWindowSize(glfwWindow, wInfo->width, wInfo->height);
	}
	else
	{
		//setup OpenGL
		if (!glfwInit())
			RUNTIME_EXCEPTION("glfwInit() failed.");

		// Create an OpenGL 4.1 core profile context.
		// This needs to be explicitly done on macOS.
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
		if (!windowVisible)
		{
			LOG(INFO) << "Setting window to hidden.";
			glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
		}

		// Create GLFW window.
		glfwWindow = glfwCreateWindow(wInfo->width, wInfo->height, title.c_str(), nullptr, parentContext);
		if (!glfwWindow)
		{
			LOG(FATAL) << "Failed to create GLFW window with OpenGL 4.1 core profile context.";
			glfwTerminate();
			return -1;
		}

		glfwMakeContextCurrent(glfwWindow);
		if (gl3wInit() != 0)
		{
			LOG(FATAL) << "Failed to initialize GL3W";
			glfwTerminate();
			return -1;
		}

		// Test version of OpenGL context.
		LOG(INFO) << "OpenGL context version: " << glGetString(GL_VERSION);
	}

	// Main framebuffer
	if (!framebuffer)
	{
		framebuffer = make_shared<GLFramebuffer>(
		    wInfo->width, wInfo->height, "GL_RGB", "GL_FLOAT", "GL_RGB16F");
	}
	else
	{
		framebuffer->init(wInfo->width, wInfo->height);
	}

	if (useQuadRendering && !quadProgram)
	{
		setupQuadRendering();
		quadProgram->setContext(glfwWindow);
		//maintenance->programModelPairs.push_back(std::pair<GLProgram*, GLRenderModel*>(quadProgram.get(), nullptr));
		//maintenance->updateProgramsFromPairs(true);
		//quadProgram->setActiveRenderModel(nullptr);
		maintenance->addProgram(quadProgram.get());
		maintenance->updatePrograms(true);
	}
	ErrorChecking::checkGLError();

	return 0;
}


// TODO: unused?
void GLWindow::bindFramebuffer()
{
	glfwMakeContextCurrent(glfwWindow);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer->id);
	setViewport();
}


void GLWindow::resizeFramebuffer(int width, int height)
{
	VLOG(2) << "GLWindow::resizeFramebuffer(" << width << ", " << height << ")";

	glfwMakeContextCurrent(glfwWindow);
	ErrorChecking::checkGLError();

	glfwSetWindowSize(glfwWindow, width, height);
	ErrorChecking::checkGLError();

	setViewport();
	setupFramebuffer("bla", false);
}


void GLWindow::setViewport()
{
	// Query the framebuffer size to set the viewport.
	// On High-DPI devices, a 512x512 window might have a framebuffer with 1024x1024 pixels.
	// Unfortunately, this still doesn't fix things on Mac :( -- CR
	int w, h;
	glfwGetFramebufferSize(glfwWindow, &w, &h);

	// glViewport(0, 0, wInfo->width, wInfo->height);
	glViewport(0, 0, w, h);

	setWindowSize(Eigen::Vector2i(w, h));
	ErrorChecking::checkGLError();
}


void GLWindow::readUserFile()
{
	VLOG(2) << "GLWindow::readUserFile()";

	std::string filename = projectSrcDir + "../userSettings.json";
	if (fs::exists(filename))
	{
		VLOG(2) << "UserSettings.json exists at " << filename;

		std::string fileContent = readFile(filename);

		Json::Value root;
		Json::Reader reader;
		bool parsedSuccess = reader.parse(fileContent, root, false);

		if (!parsedSuccess)
		{
			LOG(WARNING) << "Reading from JSON file '" << filename << "' failed.";
			LOG(WARNING) << "Error message: " << reader.getFormattedErrorMessages();
			return;
		}

		wInfo->posX = root["Window"]["LastPosition"]["X"].asInt();
		wInfo->posY = root["Window"]["LastPosition"]["Y"].asInt();
	}
	else
	{
		VLOG(2) << "UserSettings.json not found at " << filename;
		// Using default window position.
	}
}


// That is very dangerous if there is more than one GLWindow!
void GLWindow::writeUserFile()
{
	//write lastPosition
	glfwGetWindowPos(getGLFWwindow(), &wInfo->posX, &wInfo->posY);

	// CR: I removed this as coordinates can easily be >1500 on large monitors,
	//     and also negative on multi-monitor setups.
	//wInfo->posX = max(100, min(wInfo->posX, 1500));
	//wInfo->posY = max(100, min(wInfo->posY, 1000));

	Json::Value root;
	root["Window"]["LastPosition"]["X"] = wInfo->posX;
	root["Window"]["LastPosition"]["Y"] = wInfo->posY;

	std::string filename = projectSrcDir + "../userSettings.json";
	if (fs::exists(filename))
		std::remove(filename.c_str());

	std::ofstream file;
	file.open(filename, std::fstream::out);
	file << root;
	file.close();
}


void GLWindow::takeScreenshot(const std::string& name)
{
	// Bind the framebuffer's render texture.
	glBindTexture(GL_TEXTURE_2D, framebuffer->renderedTextureID);

	// Allocate OpenCV image.
	cv::Mat3b screenshot(wInfo->height, wInfo->width);

	// Read texture as BGR image to match OpenCV's assumptions.
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_BGR, GL_UNSIGNED_BYTE, screenshot.data);
	ErrorChecking::checkGLError();

	// Vertical flip to convert from OpenGL's bottom-left image origin to OpenCV's top-left origin.
	screenshot = cv::flip(screenshot, 0);

	//std::chrono::time_point <std::chrono::system_clock> seconds;
	//seconds = std::chrono::system_clock::now();
	//std::time_t time = std::chrono::system_clock::to_time_t(seconds);
	//std::string filename = name;
	//std::stringstream ss;
	//ss << "../Screenshots/" << filename << "-" << std::to_string(time) << ".jpg";
	//filename = ss.str();

	// Make sure the screenshot directory exists.
	fs::path screenshotDir = fs::path(projectSrcDir) / "../Screenshots/";
	if (!fs::exists(screenshotDir))
		fs::create_directories(screenshotDir);

	// Write screenshot to disk.
	string screenshotFilename = (screenshotDir / name).generic_string();
	LOG(INFO) << "Saving screenshot to '" << screenshotFilename << "'";
	cv::imwrite(screenshotFilename, screenshot);
}


void GLWindow::readColor(double xpos, double ypos)
{
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glBindTexture(GL_TEXTURE_2D, framebuffer->renderedTextureID);
	int ixpos = (int)xpos;
	int iypos = wInfo->height - (int)ypos;
	ixpos = std::min<int>(wInfo->width - 1, std::max<int>(0, ixpos));
	iypos = std::min<int>(wInfo->height - 1, std::max<int>(0, iypos));

	//int channels = getChannelsFromInternalFormat(framebuffer->internalFormat);
	int channels = 3;
	GLfloat* texImage = new GLfloat[wInfo->width * wInfo->height * channels];

	if (channels == 3)
	{
		glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_FLOAT, texImage);
		ErrorChecking::checkGLError();

		char msg[128];
		sprintf(msg, "Unclamped pixel at (%4d, %4d) is: (%f, %f, %f) \n",
		        ixpos, iypos,
		        texImage[(iypos * wInfo->width + ixpos) * channels + 0],
		        texImage[(iypos * wInfo->width + ixpos) * channels + 1],
		        texImage[(iypos * wInfo->width + ixpos) * channels + 2]);
		LOG(INFO) << msg;
	}

	if (channels == 4)
	{
		glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, texImage);
		ErrorChecking::checkGLError();

		char msg[128];
		sprintf(msg, "Unclamped pixel at (%4d, %4d) is: (%f, %f, %f, %f) \n",
		        ixpos, iypos,
		        texImage[(iypos * wInfo->width + ixpos) * channels + 0],
		        texImage[(iypos * wInfo->width + ixpos) * channels + 1],
		        texImage[(iypos * wInfo->width + ixpos) * channels + 2],
		        texImage[(iypos * wInfo->width + ixpos) * channels + 3]);
		LOG(INFO) << msg;
	}

	delete[] texImage;
}


void GLWindow::swapBuffers()
{
	if (glfwWindow)
		glfwSwapBuffers(glfwWindow);
}

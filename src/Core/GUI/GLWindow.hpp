#pragma once

#include "Core/GL/GLProgramMaintenance.hpp"
#include "Core/GL/GLFramebuffer.hpp"
#include "Core/GUI/WindowInfo.hpp"
#include "Core/LinearAlgebra.hpp"

#include <GL/gl3w.h>

#include <memory>
#include <string>


class GLFramebuffer;
class GLQuadProgram;

struct GLFWwindow;


class GLWindow
{
public:
	GLWindow(const std::string& _name,
	         const Eigen::Vector2i& _windowDims,
	         GLProgramMaintenance* _maintenance,
	         bool useQuadRendering = true,
	         GLFWwindow* parentContext = nullptr,
	         bool _visible = true);

	float getAspect() const;

	inline const int getHeight() const { return wInfo->height; }
	inline void setHeight(int _height) { wInfo->height = _height; }

	inline const int getWidth() const { return wInfo->width; }
	inline void setWidth(int _width) { wInfo->width = _width; }

	void setPosition(const Eigen::Vector2i& pos);
	void setWindowSize(const Eigen::Vector2i& size);
	const Eigen::Vector2i getWindowSize();


	void setWindowPosition(int x, int y);

	// Binds the framebuffer (with its GL context) and update its viewport.
	void bindFramebuffer();


	void resizeFramebuffer(int width, int height);

	void readUserFile();
	void writeUserFile();

	inline void setGLFWwindow(GLFWwindow* _glfwWindow) { glfwWindow = _glfwWindow; }
	GLFWwindow* getGLFWwindow() const { return glfwWindow; }

	void takeScreenshot(const std::string& name);
	void renderQuad();
	void readColor(double xpos, double ypos);
	void swapBuffers();

	// Size and position of window.
	std::shared_ptr<WindowInfo> wInfo;

	// Framebuffer associated with this window.
	std::shared_ptr<GLFramebuffer> framebuffer;


private:
	// Sets up the framebuffer.
	int setupFramebuffer(const std::string& title, bool useQuadRendering = true, GLFWwindow* parentContext = nullptr);

	void setupQuadRendering();
	void setViewport();

	GLFWwindow* glfwWindow = nullptr;
	GLProgramMaintenance* maintenance = nullptr;

	std::string projectSrcDir;

	bool windowVisible = true; // for preprocessing only flag

	// GL program for the quad corresponding to this window.
	std::shared_ptr<GLQuadProgram> quadProgram;

	GLuint quadVAO = 0;
	GLuint quadVertexbuffer = 0;
};

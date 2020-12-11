#pragma once

#include "Core/GL/GLApplication.hpp"
#include "Core/GL/GLCamera.hpp"
#include "Utils/Logger.hpp"

#include <GLFW/glfw3.h>


void windowSize_callback(GLFWwindow* window, int width, int height)
{
	if (width <= 0 || height <= 0)
	{
		LOG(WARNING) << "Warning: invalid call to windowSize_callback(..., " << width << ", " << height << ") ignored.";
		return;
	}

	// Only GLApplications have windows
	GLApplication* glApp = dynamic_cast<GLApplication*>(app);
	if (glApp != nullptr)
	{
		glfwMakeContextCurrent(window);
		glApp->getGLwindow()->resizeFramebuffer(width, height);

		// Update aspect ratio of window.
		if (glApp->getGLCamera())
			glApp->getGLCamera()->setAspect((float)width / (float)height);
	}
}

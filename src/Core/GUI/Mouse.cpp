#include "Mouse.hpp"

#include "Core/GL/GLCamera.hpp"
#include "Core/GUI/GLWindow.hpp"

#include "Utils/Logger.hpp"

#include <algorithm> // for std::min/max


float keepInRange(float x, float min, float max)
{
	return std::max<float>(min, std::min<float>(x, max));
}


Mouse::Mouse(std::shared_ptr<InputHandler> handler)
{
	inputHandler = handler;
}


void Mouse::handleUserInput(GLCamera* appCam, GLWindow& glWindow)
{
	// Decrease mouse speed.
	if (inputHandler->delete_key_pressed)
	{
		float factor = 5.0f;
		if (inputHandler->ctrl_pressed)
			factor *= 10.0f;

		speed -= factor * (float)inputHandler->deltaTime;
		speed = std::max<float>(0.01f, speed);
		LOG(INFO) << "Mouse speed = " << speed;
		return;
	}

	// Increase mouse speed.
	if (inputHandler->insert_key_pressed)
	{
		float factor = 5.0f;
		if (inputHandler->ctrl_pressed)
			factor *= 10.0f;

		speed += factor * (float)inputHandler->deltaTime;
		speed = std::min<float>(10000.0f, speed);
		LOG(INFO) << "Mouse speed = " << speed;
		return;
	}

	// Camera dollying (forward/backward motion).
	if (inputHandler->mouse_scrolled)
	{
		appCam->moveForward((inputHandler->mouseScrollOffset(1) * zoomSpeed * speed));
		appCam->update();
		inputHandler->mouse_scrolled = false;
		return;
	}

	// Camera translation.
	if (inputHandler->leftMouseButton_pressed)
	{
		double xpos, ypos;
		glfwGetCursorPos(glWindow.getGLFWwindow(), &xpos, &ypos);

		// Measures how far the click is away from the centre of the window
		float x = glWindow.getWidth() / 2.0f - (float)xpos;
		float y = glWindow.getHeight() / 2.0f - (float)ypos;

		//cap values
		//float radius = 200.0f;
		//float changeX = settings->translationSpeed * settings->speed * (float)inputHandler->deltaTime * keepInRange(x, -radius, radius);
		//float changeY = settings->translationSpeed * settings->speed * (float)inputHandler->deltaTime * keepInRange(y, -radius, radius);
		float changeX = translationSpeed * speed * x / glWindow.getWidth();
		float changeY = translationSpeed * speed * y / glWindow.getHeight();

		appCam->moveRight(-changeX);
		if (freeTranslate)
			appCam->moveUp(changeY);

		if ((abs(changeX) > (glWindow.getWidth() / 4.0f)) || (abs(changeY) > (glWindow.getHeight() / 4.0f)))
			glfwSetCursorPos(glWindow.getGLFWwindow(), glWindow.getWidth() / 2.0f, glWindow.getHeight() / 2.0f);

		appCam->update();
		return;
	}

	// Camera rotation (yaw/pitch).
	if (inputHandler->rightMouseButton_pressed)
	{
		double xpos, ypos;
		glfwGetCursorPos(glWindow.getGLFWwindow(), &xpos, &ypos);

		// Measures how far the click is away from the centre of the window
		float x = glWindow.getWidth() / 2.0f - (float)xpos;
		float y = glWindow.getHeight() / 2.0f - (float)ypos;

		//float changeX = settings->translationSpeed * settings->speed * (float)inputHandler->deltaTime * x / 10.f;
		//float changeY = settings->translationSpeed * settings->speed * (float)inputHandler->deltaTime * y / 10.f;
		float changeX = rotationSpeed * speed * x / glWindow.getWidth();
		float changeY = rotationSpeed * speed * y / glWindow.getHeight();

		appCam->yaw(changeX);
		appCam->pitch(changeY);

		if ((abs(changeX) > (glWindow.getWidth() / 4.0f)) || (abs(changeY) > (glWindow.getHeight() / 4.0f)))
			glfwSetCursorPos(glWindow.getGLFWwindow(), glWindow.getWidth() / 2.0f, glWindow.getHeight() / 2.0f);

		appCam->update();
		return;
	}

	//// Roll the camera counter-clockwise.
	//if (inputHandler->num7_pressed)
	//{
	//	//appCam->roll(-settings->rollSpeed * settings->speed * (float)inputHandler->deltaTime);
	//	appCam->roll(-settings->rollSpeed * settings->speed);
	//	appCam->update();
	//	//inputHandler->num7_pressed = false; // not used to keep rotating when pressed
	//	return;
	//}

	//// Roll the camera clockwise.
	//if (inputHandler->num9_pressed)
	//{
	//	//appCam->roll(settings->rollSpeed * settings->speed * (float)inputHandler->deltaTime);
	//	appCam->roll(settings->rollSpeed * settings->speed);
	//	appCam->update();
	//	//inputHandler->num9_pressed = false; // not used to keep rotating when pressed
	//	return;
	//}

	if (inputHandler->space_pressed)
	{
		freeTranslate = !freeTranslate;
		LOG(INFO) << "Mouse free translation mode: " << (freeTranslate ? "enabled" : "disabled");
		inputHandler->space_pressed = false;
		return;
	}
}

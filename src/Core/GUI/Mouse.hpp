#pragma once

#include "3rdParty/Eigen.hpp"
#include "InputHandler.hpp"

#include <memory>

class GLCamera;
class GLWindow;


class Mouse
{
public:
	Mouse(std::shared_ptr<InputHandler> handler);

	bool freeTranslate = false;
	bool sweep = false;

	float speed = 1;
	float translationSpeed = 5;
	float rotationSpeed = 5;
	float zoomSpeed = 10;

	void handleUserInput(GLCamera* appCam, GLWindow& glWindow);

private:
	Eigen::Vector2i position;
	std::shared_ptr<InputHandler> inputHandler;
};

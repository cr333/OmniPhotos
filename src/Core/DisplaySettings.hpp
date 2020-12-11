#pragma once

#include "3rdParty/Eigen.hpp"


enum DisplayMode
{
	Left = 0,
	Right = 1,
	LinearBlending = 2,
	WorldLines = 3,
	CameraPair = 4,
	TexLeft = 5,
	TexRight = 6,
	FlowLeft = 7,
	FlowRight = 8,
	CompFlowLeft = 9,
	CompFlowRight = 10,
	WorldPositions = 11,
};


struct DisplaySettings
{
	DisplaySettings() = default;
	virtual ~DisplaySettings() = default;

	// Size of the window.
	Eigen::Vector2i windowDims = Eigen::Vector2i(1280, 720);

	// Whether or not VR rendering with a head-mounted display is enabled.
	bool enableVR = false;

	int switchGLProgram = 1;

	// RenderModel = 0 -> render black screen
	int switchGLRenderModel = 1;

	DisplayMode displayMode = LinearBlending;

	int raysPerPixel = 1;
	int fadeNearBoundary = 1;

	bool showWorldPoints = false;
	bool showCoordinateAxes = false;
	bool showFittedCircle = false;
	bool showCameraGeometry = false;
	bool showRadialLines = false;

	bool load3DPoints = false;
	int usePointCloud = 0;

	bool opticalFlowLoaded = false;
	int useOpticalFlow = 0;
    
	float lookAtDirection = 0.0f; // [deg]
	float lookAtDistance = 1000.0f; // [cm]

	// use for interactively control the pointSize in ViewerGLprogram.
	// "p_key" (+ CTRL) range between
	float pointSize = 5.0f;
	Eigen::Vector2f pointSizeRange = { 1.0f, 20.f };

	float physicalScale = 1.0f;  // rescale factor, e.g. when fitting to physical dimensions
	int circle_resolution = 512; // vertices on each circle
	float distanceToProjectionPlane = 0.0f;

	// TODO: This doesn't belong here, but the GUI uses it.
	int numberOfCameras = 0;
};

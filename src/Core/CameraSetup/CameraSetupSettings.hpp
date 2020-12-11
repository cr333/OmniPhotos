#pragma once

#include "Core/DisplaySettings.hpp"
#include "Core/Loaders/MultiViewDataLoader.hpp"
#include "Core/OpticalFlow/FlowParameters.hpp"

class CameraSetupDataset;


struct VideoParameters
{
	int firstFrame = -1;
	int lastFrame = -1;
	int frameInterval = 1; //every frame, 2nd, 3rd, ...
	Eigen::Vector2i getClip()
	{
		return Eigen::Vector2i(firstFrame, lastFrame);
	}
};


class CameraSetupSettings : public DisplaySettings
{
public:
	CameraSetupSettings() = default;
	CameraSetupSettings(CameraSetupSettings& _settings);
	virtual ~CameraSetupSettings() = default;

	float circleRadius = -1.0f;
	float cylinderRadius = -1.0f;

	int readSettingsFromFile(const std::string& pathToFile, CameraSetupDataset* _dataset);

	int changeBasis = 0;
	int shapeFit = 0;
	int shapeSampling = 0;
	int computeOpticalFlow = 0;
	FlowMethod opticalFlowMethod;

	// Makes a difference in Preprocess and Viewer.
	int useEquirectCamera = 1;

	std::string configFile;
	std::string preprocessingSetupFilename;

	// Paths to multi-view geometry files.
	std::string cameraGeometryFile;

	bool loadSparsePointCloud = true;
	//I think it's formats "0001" bla
	std::string imageFileNames = "NULL";

	//imagename digits 0001, 0000001 etc.
	int digits;

	SfmFormat sfmFormat;
	float intrinsicScale = 1.0f; //different resolutions between COLMAP recon and images on runtime?

	VideoParameters videoParams;
	BroxFlowParameters broxFlowParams;
	int downsampleFlow;

	// Geometry
	float max3DPointError = -1.0f;
	int maxLoad3Dpoints = -1; //debug purposes, don't want to load all everytime.
};

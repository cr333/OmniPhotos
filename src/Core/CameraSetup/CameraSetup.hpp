#pragma once

#include "3rdParty/Eigen.hpp"

#include "Core/Camera.hpp"
#include "Core/CameraSetup/CameraSetupSettings.hpp"

#include <vector>


/**
 * @brief			Model input viewpoints (calibration and input image per viewpoint)
 * 
 * Used to model the viewpoints (or input images) of a dataset using some camera model, i.e. pinhole or equirectangular.
 * CameraSetup could be specialized according to camera path, but we use solely circular paths.
 * 
 * @param cameras	Stores all viewpoints
 * @param settings Provides options that can be changed for preprocessing or during viewing
 */
class CameraSetup
{
public:
	CameraSetup();
	CameraSetup(const std::vector<Camera*>& _cameras);
	CameraSetup(CameraSetupSettings& _settings);
	CameraSetup(const std::vector<Camera*>& _cameras, CameraSetupSettings& _settings);

	//NOTE: shouldn't be needed, but I don't anybody to change the settings reference lightly.
	CameraSetupSettings* getSettings();
	void setSettings(CameraSetupSettings* _settings);

	std::vector<Camera*>* getCameras();
	void setCameras(std::vector<Camera*> _cameras);

	float getCameraPathLength();
	int getNumberOfCameras() const;
	std::vector<Eigen::Vector3f> getOpticalCentres();
	Eigen::Point3f getCentroid() const;
	Eigen::Vector3f getAverageForward() const;
	Eigen::Vector3f getAverageUp() const;
	Eigen::Vector3f getAverageLeft() const;

private:
	std::vector<Camera*> cameras;

	//TODO: There should be a struct : "LoadingSettings"
	CameraSetupSettings* settings = nullptr;
};

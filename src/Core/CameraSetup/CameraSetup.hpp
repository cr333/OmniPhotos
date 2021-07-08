#pragma once

#include "3rdParty/Eigen.hpp"

#include "Core/Camera.hpp"
#include "Core/CameraSetup/CameraSetupSettings.hpp"

#include <vector>


/**
 * @brief   Models input viewpoints (calibration and input image per viewpoint).
 * 
 * Used to model the viewpoints (or input images) of a dataset using some camera model, i.e. pinhole or equirectangular.
 * CameraSetup could be specialised according to camera path, but we only use circular paths.
 */
class CameraSetup
{
public:
	CameraSetup();
	CameraSetup(const std::vector<Camera*>& _cameras);
	CameraSetup(CameraSetupSettings& _settings);
	CameraSetup(const std::vector<Camera*>& _cameras, CameraSetupSettings& _settings);

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
	/** Stores all viewpoints. */
	std::vector<Camera*> cameras;

	/** Provides options that can be changed for preprocessing or during viewing. */
	CameraSetupSettings* settings = nullptr;
};

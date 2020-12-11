#include "CameraSetup.hpp"


CameraSetup::CameraSetup()
{
	settings = new CameraSetupSettings();
}


CameraSetup::CameraSetup(const std::vector<Camera*>& _cameras) :
    CameraSetup()
{
	// Deep copy of the pointers
	for (int i = 0; i < _cameras.size(); i++)
		cameras.push_back(_cameras[i]);
}


CameraSetup::CameraSetup(CameraSetupSettings& _settings)
{
	settings = new CameraSetupSettings(_settings);
}


CameraSetup::CameraSetup(const std::vector<Camera*>& _cameras, CameraSetupSettings& _settings) :
    CameraSetup(_cameras)
{
	settings = new CameraSetupSettings(_settings);
}


//CameraSetup::~CameraSetup()
//{
//	//leak?
//	if (settings)
//		delete settings;
//}


std::vector<Camera*>* CameraSetup::getCameras()
{
	return &cameras;
}


void CameraSetup::setCameras(std::vector<Camera*> _cameras)
{
	cameras = _cameras;
}


float CameraSetup::getCameraPathLength()
{
	float pathLength = 0.0f;
	Camera* cam = cameras[0];
	Camera* nextCam = NULL;
	for (int i = 1; i < getNumberOfCameras(); i++)
	{
		nextCam = cameras[i];
		float currentBaseLine = (nextCam->getCentre() - cam->getCentre()).norm();
		pathLength += currentBaseLine;
		cam = nextCam;
	}
	return pathLength;
}


// Note: used for visualisation.
std::vector<Eigen::Vector3f> CameraSetup::getOpticalCentres()
{
	std::vector<Eigen::Vector3f> centres;
	int n_cameras = getNumberOfCameras();
	for (int i = 0; i < n_cameras; i++)
	{
		Eigen::Vector3f C = getCameras()->at(i)->getCentre();
		centres.push_back(C);
	}
	return centres;
}


int CameraSetup::getNumberOfCameras() const
{
	return (int)cameras.size();
}


Eigen::Point3f CameraSetup::getCentroid() const
{
	Eigen::Vector3f centroid = Eigen::Vector3f(0.0f, 0.0f, 0.0f);
	for (int i = 0; i < cameras.size(); i++)
		centroid += cameras.at(i)->getCentre();
	centroid /= (float)cameras.size();
	return centroid;
}


Eigen::Vector3f CameraSetup::getAverageForward() const
{
	Eigen::Vector3f forward(0.0f, 0.0f, 0.0f);
	for (int i = 0; i < cameras.size(); i++)
		forward += cameras.at(i)->getZDir();
	return forward.normalized();
}


Eigen::Vector3f CameraSetup::getAverageUp() const
{
	Eigen::Vector3f up(0.0f, 0.0f, 0.0f);
	for (int i = 0; i < cameras.size(); i++)
		up += cameras.at(i)->getYDir();
	return up.normalized();
}


Eigen::Vector3f CameraSetup::getAverageLeft() const
{
	Eigen::Vector3f left(0.0f, 0.0f, 0.0f);
	for (int i = 0; i < cameras.size(); i++)
		left += cameras.at(i)->getXDir();
	return left.normalized();
}


CameraSetupSettings* CameraSetup::getSettings()
{
	return settings;
}


void CameraSetup::setSettings(CameraSetupSettings* _settings)
{
	settings = _settings;
}

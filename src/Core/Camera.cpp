#include "Camera.hpp"

#include "Utils/DepthIO.hpp"
#include "Utils/Exceptions.hpp"
#include "Utils/Logger.hpp"
#include "Utils/Utils.hpp"


Camera::Camera()
{
	P.setZero();
	C.setZero();
	K.setZero();
	R.setZero();

	frame = -1;
}


Camera::Camera(const Camera& cam)
{
	P = cam.P;
	C = cam.C;
	K = cam.K;
	R = cam.R;
	imageName = cam.imageName;
	frame = cam.frame;
}


Camera::Camera(Eigen::Matrix3f _rotation, Eigen::Vector3f _centre) :
    Camera()
{
	K.setIdentity();
	R = _rotation;
	C = _centre;
	updateProjectionMatrix();
}


Camera::Camera(Eigen::Matrix3f _K, Eigen::Matrix3f _R, Eigen::Point3f _C) :
    Camera()
{
	K = _K;
	R = _R;
	C = _C;
	updateProjectionMatrix();
}


float Camera::getPhi() const
{
	// Direction from origin to the camera.
	Eigen::Vector3f p = C.normalized();

	// Make the phi according to world frame
	return 180.0f + cartesianToAngle(Eigen::Vector2f(p.x(), p.z()));
}


void Camera::updateParameters(Eigen::Matrix3f _K, Eigen::Matrix3f _R, Eigen::Vector3f _C)
{
	K = _K;
	setExtrinsics(_R, _C);
}


void Camera::updateProjectionMatrix()
{
	Eigen::Matrix34f E = getExtrinsics();
	P = K * E;
}


/// Returns true if the image was loaded successfully.
bool Camera::loadImageWithOpenCV()
{
	if (img.empty())
	{
		VLOG(1) << "Loading image '" << imageName << "'";
		img = cv::imread(imageName, cv::IMREAD_COLOR);

		if (img.empty())
		{
			LOG(WARNING) << "Loading image '" << imageName << "' failed.";
			return false;
		}
	}

	return true;
}


Eigen::Matrix4f Camera::getProjection44()
{
	Eigen::Matrix4f _P;
	_P.setIdentity();
	_P.block(0, 0, 3, 4) = P;
	return _P;
}


Eigen::Matrix34f Camera::getExtrinsics()
{
	Eigen::Matrix34f E;
	//start at (0,0) top left and copy 3x3 matrix
	E.block(0, 0, 3, 3) = R;
	//translation
	E.col(3) = getTranslation();
	return E;
}


void Camera::setIntrinsics(Eigen::Matrix3f _intrinsics)
{
	K = _intrinsics;
	updateProjectionMatrix();
}

#pragma once

#include "Core/Geometry/Plane.hpp"
#include "Core/LinearAlgebra.hpp"

#include <opencv2/core.hpp>

#include <vector>


/*
	This is a finite pinhole camera model according Hartley & Zisserman, Multi-view Geometry. 
	The camera coordinate frame R = [ uxv | u | v ], where v = viewing direction, u = up and cross(u,v) = left
	The projection x' = K(RX + t) where X a point in world space and t is the translation from world to camera origin.
	The camera centre c = -R'*t and thus t = -R*c
*/
class Camera
{
public:
	Camera();
	Camera(const Camera& cam);
	Camera(Eigen::Matrix3f _rotation, Eigen::Vector3f _centre);
	Camera(Eigen::Matrix3f _K, Eigen::Matrix3f _R, Eigen::Point3f _C);

	virtual ~Camera() = default;

	//needed when loading stabilized images -> bad, since I read the written images from disk instead of RAM..
	bool needsReload = false;

	int frame; //associates with input stream
	std::string imageName;

	// Computes the azimuth angle relative to the origin. Used for sorting cameras.
	float getPhi() const;

	void updateParameters(Eigen::Matrix3f _K, Eigen::Matrix3f _R, Eigen::Vector3f _C);

	// Gets camera centre C.
	inline const Eigen::Vector3f getCentre() const { return C; }

	// Sets camera centre C.
	inline void setCentre(const Eigen::Vector3f& c)
	{
		C = c;
		updateProjectionMatrix();
	}

	// Gets rotation R.
	inline const Eigen::Matrix3f getRotation() const { return R; }

	// Sets rotation R.
	inline void setRotation(const Eigen::Matrix3f& r)
	{
		R = r;
		updateProjectionMatrix();
	}

	// Gets translation vector t.
	inline const Eigen::Vector3f getTranslation() const { return -R * C; }

	// Gets the extrinsics [R | t] as a 3-by-4 matrix.
	Eigen::Matrix34f getExtrinsics();

	// Sets both rotation R and camera centre C at the same time.
	inline void setExtrinsics(const Eigen::Matrix3f& r, const Eigen::Vector3f& c)
	{
		R = r;
		C = c;
		updateProjectionMatrix();
	}

	inline const Eigen::Vector3f getXDir() const { return R.row(0); } // previously "getLeftDir"
	inline const Eigen::Vector3f getYDir() const { return R.row(1); } // previously "getUpDir"
	inline const Eigen::Vector3f getZDir() const { return R.row(2); } // previously "getViewDir"

	bool loadImageWithOpenCV();

	inline cv::Mat getImage() const { return img; }
	inline void setImage(cv::Mat _img) { img = _img; }

	inline const Eigen::Matrix34f getProjection() const { return P; }
	Eigen::Matrix4f getProjection44();

	Eigen::Matrix3f getIntrinsics() const { return K; }
	void setIntrinsics(Eigen::Matrix3f _intrinsics);

private:
	// Projection matrix P = K [R|t]
	Eigen::Matrix34f P;

	// Camera centre (translation is t = -R * C)
	Eigen::Point3f C;

	// Intrinsic matrix
	Eigen::Matrix3f K;

	// Rotation matrix
	Eigen::Matrix3f R;

	void updateProjectionMatrix();

	cv::Mat img;
};

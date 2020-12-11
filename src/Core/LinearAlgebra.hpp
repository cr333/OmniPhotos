#pragma once

#include "3rdParty/Eigen.hpp"

#ifndef M_PI
	#define M_PI 3.1415926535897932384626433832
#endif

#ifndef M_PI_2
	#define M_PI_2 1.5707963267948966192313216916
#endif


struct ConstantMatrices
{
public:
	Eigen::Matrix3f invX;
	Eigen::Matrix3f invY;
	Eigen::Matrix3f invZ;
	Eigen::Matrix3f invXY;
	Eigen::Matrix3f invXZ;
	Eigen::Matrix3f invYZ;
	Eigen::Matrix3f invXYZ;

	Eigen::Matrix3f flipXY;
	Eigen::Matrix3f flipYZ;
	Eigen::Matrix3f flipXZ;

	ConstantMatrices()
	{
		// clang-format off
		invX   << -1, 0, 0,   0,  1, 0,   0, 0,  1;
		invY   <<  1, 0, 0,   0, -1, 0,   0, 0,  1;
		invZ   <<  1, 0, 0,   0,  1, 0,   0, 0, -1;
		invXY  << -1, 0, 0,   0, -1, 0,   0, 0,  1;
		invXZ  << -1, 0, 0,   0,  1, 0,   0, 0, -1;
		invYZ  <<  1, 0, 0,   0, -1, 0,   0, 0, -1;
		invXYZ << -1, 0, 0,   0, -1, 0,   0, 0, -1;

		flipXY <<  0, 1, 0,   1,  0, 0,   0, 0,  1;
		flipYZ <<  1, 0, 0,   0,  0, 1,   0, 1,  0;
		flipXZ <<  0, 0, 1,   0,  1, 0,   1, 0,  0;
		// clang-format on
	}
};


// Creates a 3-by-3 rotation matrix from an axis and an angle (in degrees).
Eigen::Matrix3f createRotationTransform33(Eigen::Vector3f axis, float degree);

// Creates a 4-by-4 rotation matrix [R 0; 0 1] from an axis and an angle (in degrees).
Eigen::Matrix4f createRotationTransform44(Eigen::Vector3f axis, float degree);

// Creates a 4-by-4 transformation [I t; 0 1] from a translation vector t.
Eigen::Matrix4f createTranslationTransform44(Eigen::Point3f translation);

// Composes a rotation and translation into a 4-by-4 transformation [R t; 0 1].
Eigen::Matrix4f composeTransform44(Eigen::Matrix3f rotation, Eigen::Vector3f translation);

// Stacks three column vectors into a 3-by-3 matrix.
Eigen::Matrix3f concatenateColumnVectors(Eigen::Vector3f x, Eigen::Vector3f y, Eigen::Vector3f z);

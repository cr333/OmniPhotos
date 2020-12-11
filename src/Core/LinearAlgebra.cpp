#include "LinearAlgebra.hpp"

using namespace Eigen;


Matrix3f createRotationTransform33(Vector3f axis, float degree)
{
	return Matrix3f(AngleAxis<float>(degree * (float)M_PI / 180.0f, axis));
}


Matrix4f createRotationTransform44(Vector3f axis, float degree)
{
	Matrix4f transform = Matrix4f::Identity();
	transform.block<3, 3>(0, 0) = createRotationTransform33(axis, degree);
	return transform;
}


Matrix4f createTranslationTransform44(Point3f translation)
{
	Matrix4f transform = Matrix4f::Identity();
	transform.block<3, 1>(0, 3) = translation;
	return transform;
}


Matrix4f composeTransform44(Matrix3f rotation, Vector3f translation)
{
	Matrix4f transform = Matrix4f::Identity();
	transform.block<3, 3>(0, 0) = rotation;
	transform.block<3, 1>(0, 3) = translation;
	return transform;
}


Matrix3f concatenateColumnVectors(Vector3f x, Vector3f y, Vector3f z)
{
	Matrix3f stacked;
	stacked << x, y, z;
	return stacked;
}

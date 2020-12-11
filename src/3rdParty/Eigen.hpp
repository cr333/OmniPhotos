#pragma once

#include <Eigen/Dense>
#include <Eigen/Geometry>


namespace Eigen
{
	// Provide alternative types for points vs vectors.
	// Think in homogeneous coordinates:
	//   - last component of points is 1.
	//   - last component of vectors is 0.

	typedef Vector2f Point2f;
	typedef Vector2d Point2d;
	typedef Vector2i Point2i;

	typedef Vector3f Point3f;
	typedef Vector3d Point3d;
	typedef Vector3i Point3i;

	typedef Vector4f Point4f;
	typedef Vector4d Point4d;
	typedef Vector4i Point4i;


	// Non-square matrices.

	typedef Eigen::Matrix<float, 3, 4> Matrix34f;
	typedef Eigen::Matrix<float, 4, 3> Matrix43f;

} // namespace Eigen

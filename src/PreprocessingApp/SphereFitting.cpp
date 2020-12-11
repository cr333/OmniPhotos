#include "SphereFitting.hpp"

#include "3rdParty/fs_std.hpp"
#include "Core/LinearAlgebra.hpp"
#include "Utils/DepthIO.hpp"
#include "Utils/Logger.hpp"
#include "Utils/cvutils.hpp"

#include <ceres/ceres.h>
#include <opencv2/core/eigen.hpp>

#include <cstdio>
#include <fstream>


using namespace ceres;
using namespace cv;
using namespace Eigen;

#define USE_NORMALISED_RESIDUALS 1


//---- Cost terms for sphere fitting -----------------------------------------//


struct SphereFittingDataTerm
{
	typedef AutoDiffCostFunction<SphereFittingDataTerm, 1, 1> SphereFittingDataTermFunction;

	double x_;

	SphereFittingDataTerm(double x) :
	    x_(x) {}

	template <typename T>
	bool operator()(const T* const x, T* residual) const
	{
#if USE_NORMALISED_RESIDUALS
		// normalised residual
		residual[0] = (x_ - *x) / (x_ + *x);
#else
		// standard residual
		residual[0] = x_ - *x;
#endif

		return true;
	}

	static SphereFittingDataTermFunction* Create(double x)
	{
		return new SphereFittingDataTermFunction(new SphereFittingDataTerm(x));
	}
};


struct SphereFittingBarycentricDataTerm
{
	typedef AutoDiffCostFunction<SphereFittingBarycentricDataTerm, 1, 1, 1, 1> SphereFittingBarycentricDataTermFunction;

	double point_radius_;
	double u_, v_, w_; // barycentric coordinates

	SphereFittingBarycentricDataTerm(double point_radius, Eigen::Vector3f bary) :
	    point_radius_(point_radius), u_(bary.x()), v_(bary.y()), w_(bary.z())
	{
	}

	template <typename T>
	bool operator()(
	    const T* const a,
	    const T* const b,
	    const T* const c,
	    T* residual) const
	{
		T est_point_radius = u_ * (*a) + v_ * (*b) + w_ * (*c);

#if USE_NORMALISED_RESIDUALS
		// normalised residual
		residual[0] = (point_radius_ - est_point_radius) / (point_radius_ + est_point_radius);
#else
		// standard residual
		residual[0] = point_radius_ - est_point_radius;
#endif

		return true;
	}

	static SphereFittingBarycentricDataTermFunction* Create(float radius, Eigen::Vector3f bary)
	{
		return new SphereFittingBarycentricDataTermFunction(new SphereFittingBarycentricDataTerm(radius, bary));
	}
};


struct SphereFittingHalfwayDataTerm
{
	typedef AutoDiffCostFunction<SphereFittingHalfwayDataTerm, 1, 1, 1> SphereFittingHalfwayDataTermFunction;

	double point_radius_;

	SphereFittingHalfwayDataTerm(double point_radius) :
	    point_radius_(point_radius)
	{
	}

	template <typename T>
	bool operator()(
	    const T* const a,
	    const T* const b,
	    T* residual) const
	{
		T est_point_radius = 0.5 * (*a + *b);

#if USE_NORMALISED_RESIDUALS
		// normalised residual
		residual[0] = (point_radius_ - est_point_radius) / (point_radius_ + est_point_radius);
#else
		// standard residual
		residual[0] = point_radius_ - est_point_radius;
#endif

		return true;
	}

	static SphereFittingHalfwayDataTermFunction* Create(float radius)
	{
		return new SphereFittingHalfwayDataTermFunction(new SphereFittingHalfwayDataTerm(radius));
	}
};


struct SphereFittingBinarySmoothnessCost
{
	typedef AutoDiffCostFunction<SphereFittingBinarySmoothnessCost, 1, 1, 1> SphereFittingBinarySmoothnessCostFunction;

	template <typename T>
	bool operator()(
	    const T* const p,
	    const T* const q,
	    T* residual) const
	{
#if USE_NORMALISED_RESIDUALS
		// normalised residual
		residual[0] = (*p - *q) / (*p + *q);
#else
		// standard residual
		residual[0] = (*p - *q);
#endif

		return true;
	}

	static SphereFittingBinarySmoothnessCostFunction* Create()
	{
		return new SphereFittingBinarySmoothnessCostFunction(new SphereFittingBinarySmoothnessCost());
	}
};


struct SphereFittingCrossSmoothnessCost
{
	typedef AutoDiffCostFunction<SphereFittingCrossSmoothnessCost, 1, 1, 1, 1, 1, 1> SphereFittingCrossSmoothnessCostFunction;

	template <typename T>
	bool operator()(
	    const T* const p,
	    const T* const t,
	    const T* const r,
	    const T* const b,
	    const T* const l,
	    T* residual) const
	{
		T avg_point_radius = 0.25 * (*l + *r + *t + *b);

#if USE_NORMALISED_RESIDUALS
		// normalised residual
		residual[0] = (*p - avg_point_radius) / (*p + avg_point_radius);
#else
		// standard residual
		residual[0] = (*p - avg_point_radius);
#endif

		return true;
	}

	static SphereFittingCrossSmoothnessCostFunction* Create()
	{
		return new SphereFittingCrossSmoothnessCostFunction(new SphereFittingCrossSmoothnessCost());
	}
};


struct SphereFitting1DLaplacianSmoothnessCost
{
	typedef AutoDiffCostFunction<SphereFitting1DLaplacianSmoothnessCost, 1, 1, 1, 1> SphereFitting1DLaplacianSmoothnessCostFunction;

	template <typename T>
	bool operator()(
	    const T* const p,
	    const T* const q,
	    const T* const r,
	    T* residual) const
	{
#if USE_NORMALISED_RESIDUALS
		// normalised residual
		residual[0] = (*p - 2. * (*q) + *r) / (*p + 2. * (*q) + *r);
#else
		// standard residual -- untested
		residual[0] = (*p - 2. * (*q) + *r);
#endif

		return true;
	}

	static SphereFitting1DLaplacianSmoothnessCostFunction* Create()
	{
		return new SphereFitting1DLaplacianSmoothnessCostFunction(new SphereFitting1DLaplacianSmoothnessCost());
	}
};


//---- Sphere fitting class --------------------------------------------------//


SphereFitting::SphereFitting(int polar_steps, int azimuth_steps) :
    polar_steps(polar_steps), azimuth_steps(azimuth_steps)
{
}


void SphereFitting::solveProblem()
{
	if (points.size() == 0)
	{
		LOG(WARNING) << "No points given. Exiting early.";
		return;
	}

	// Set up optimisation problem for Ceres
	Problem problem;
	LOG(INFO) << "Solving SphereFitting for " << points.size() << " points at " << azimuth_steps << "x" << polar_steps << " resolution";
	LOG(INFO) << "Using weights: data=" << data_weight << ", smoothness=" << smoothness_weight << ", prior=" << prior_weight;

	// Initialise with a sphere that has the average distance of points as radius.
	// TODO: Work out a better initial solution.
	Vec2d accumulator;
	for (auto& point : points)
		accumulator += Vec2d(point.norm(), 1.0);
	double average_radius = accumulator[0] / accumulator[1];
	LOG(INFO) << "Average radius of points: " << average_radius;

	est_depth_map = MatrixXd::Ones(polar_steps, azimuth_steps);
	est_depth_map *= average_radius;

	//// Add the residuals: data term on the sphere vertices only for now
	//for (int i = 0; i < depth_map.rows(); i++)
	//{
	//	for (int j = 1; j < depth_map.cols() - 1; j++)
	//	{
	//		CostFunction* cost = new AutoDiffCostFunction<SphereFittingDataTerm, 1, 1>(
	//			new SphereFittingDataTerm(depth_map(i, j)));
	//		problem.AddResidualBlock(cost, loss, &est_depth_map(i, j));
	//	}
	//}

	if (data_weight == 0)
	{
		LOG(INFO) << "Ignoring data term as (data_weight == 0)";
	}
	else
	{
		LossFunction* dataterm_loss = nullptr;
		switch (robust_data_loss)
		{
			case RobustLoss::None:
				// nothing to do
				LOG(INFO) << "Using L2 data loss";
				break;

			case RobustLoss::Huber:
				dataterm_loss = new HuberLoss(robust_data_loss_scale);
				LOG(INFO) << "Using Huber data loss (scale: " << robust_data_loss_scale << ")";
				break;

			case RobustLoss::SoftLOne:
				dataterm_loss = new SoftLOneLoss(robust_data_loss_scale);
				LOG(INFO) << "Using SoftLOne data loss (scale: " << robust_data_loss_scale << ")";
				break;

			case RobustLoss::Cauchy:
				dataterm_loss = new CauchyLoss(robust_data_loss_scale);
				LOG(INFO) << "Using Cauchy data loss (scale: " << robust_data_loss_scale << ")";
				break;
		}

		// Add the residuals, normalised by number of points.
		dataterm_loss = new ScaledLoss(dataterm_loss, data_weight / points.size(), TAKE_OWNERSHIP);
		for (auto& point : points)
		{
			// Convert 3D point to spherical coordinates
			Eigen::Vector3f spherical = cartesian2spherical(point);
			float radius  = spherical.x();
			float azimuth = spherical.y();
			float polar   = spherical.z();

			//LOG(INFO) << "Cartesian point: " << point.transpose();
			//LOG(INFO) << "-> Spherical point: " << spherical.transpose();

			// Find the quad this point falls into
			int azimuth_index = (int)floor(azimuth / (2 * M_PI) * azimuth_steps - 0.5);
			azimuth_index = (azimuth_index + azimuth_steps) % azimuth_steps; // positive remainder
			//LOG(INFO) << "azimuth_index = " << azimuth_index;

			int polar_index = int(polar / M_PI * (polar_steps - 1)); // NB: assuming caps!
			//LOG(INFO) << "polar_index = " << polar_index;

			// Indices of sphere mesh vertices to the left / right / top / bottom
			int index_l = azimuth_index;
			int index_r = (index_l + 1) % azimuth_steps;
			int index_t = polar_index;
			int index_b = min(index_t + 1, polar_steps - 1);
			//LOG(INFO) << "indices: L=" << index_l << ", R=" << index_r << ", T=" << index_t << ", B=" << index_b;

			// Point falls on the south pole = > special case
			if (index_t == index_b)
			{
				LOG(INFO) << "Hit the South pole!";
				//# The 4 corners of the quad are only two different points, as top = bottom.
				//# So we pick both points with weight 0.5 and duplicate the last one,
				//# but with a weight of 0.
				//tri_face = [index_b, index_l, index_b, index_r, index_b, index_r]
				//points_in_faces.append((i, tri_face, (0.5, 0.5, 0), points_radius[i]))
				//
				// CR: This gives a Ceres error -- cannot use parameter more than once in a residual
				//problem.AddResidualBlock(
				//	SphereFittingBarycentricDataTerm::Create(radius, Eigen::Vector3f(0.5f, 0.5f, 0)),
				//	loss,
				//	&est_depth_map(index_b, index_l),
				//	&est_depth_map(index_b, index_r),
				//	&est_depth_map(index_b, index_r));
				problem.AddResidualBlock(
				    SphereFittingHalfwayDataTerm::Create(radius),
				    dataterm_loss,
				    &est_depth_map(index_b, index_l),
				    &est_depth_map(index_b, index_r));
				continue;
			}

			Eigen::Vector3f sph_tl = depthmap2spherical(est_depth_map, index_l, index_t);
			Eigen::Vector3f sph_bl = depthmap2spherical(est_depth_map, index_l, index_b);
			Eigen::Vector3f sph_tr = depthmap2spherical(est_depth_map, index_r, index_t);
			Eigen::Vector3f sph_br = depthmap2spherical(est_depth_map, index_r, index_b);

			// We assume that b has a higher azimuth angle than a.
			// If that is not the case, we have wrapped around and need to compensate.
			// We do that by adding pi to the azimuth of all points + wrapping.
			if (index_l > index_r)
			{
				//LOG(INFO) << "Wrap around!";
				spherical.y() = fmod(fmod(spherical.y() + M_PI, 2 * M_PI), 2 * M_PI);
				sph_tl.y() = fmod(fmod(sph_tl.y() + M_PI, 2 * M_PI), 2 * M_PI);
				sph_bl.y() = fmod(fmod(sph_bl.y() + M_PI, 2 * M_PI), 2 * M_PI);
				sph_tr.y() = fmod(fmod(sph_tr.y() + M_PI, 2 * M_PI), 2 * M_PI);
				sph_br.y() = fmod(fmod(sph_br.y() + M_PI, 2 * M_PI), 2 * M_PI);
			}

			// ignoring the radius of the barycentric coordinates
			Eigen::Vector3f bary1 = computeBarycentricCoords(spherical.bottomRows(2), sph_tl.bottomRows(2), sph_tr.bottomRows(2), sph_bl.bottomRows(2)); // top-left tri (cw)
			//LOG(INFO) << "bary1 = " << bary1.transpose();

			if (bary1.x() >= -1e-6f && bary1.x() <= 1 &&
				bary1.y() >= -1e-6f && bary1.y() <= 1 &&
				bary1.z() >= -1e-6f && bary1.z() <= 1)
			{
				//LOG(INFO) << "good1";
				problem.AddResidualBlock(
				    SphereFittingBarycentricDataTerm::Create(radius, bary1),
				    dataterm_loss,
				    &est_depth_map(index_t, index_l),
				    &est_depth_map(index_t, index_r),
				    &est_depth_map(index_b, index_l));

				continue;
			}

			Eigen::Vector3f bary2 = computeBarycentricCoords(spherical.bottomRows(2), sph_bl.bottomRows(2), sph_br.bottomRows(2), sph_tr.bottomRows(2)); // bottom-right tri (ccw)
			//LOG(INFO) << "bary2 = " << bary2.transpose();

			if (bary2.x() >= -1e-6f && bary2.x() <= 1 &&
				bary2.y() >= -1e-6f && bary2.y() <= 1 &&
				bary2.z() >= -1e-6f && bary2.z() <= 1)
			{
				//LOG(INFO) << "good2";
				problem.AddResidualBlock(
				    SphereFittingBarycentricDataTerm::Create(radius, bary2),
				    dataterm_loss,
				    &est_depth_map(index_b, index_l),
				    &est_depth_map(index_b, index_r),
				    &est_depth_map(index_t, index_r));

				continue;
			}

			LOG(WARNING) << "Missed triangle ... skipping point";
		} // looping over all points

	} // data_weight != 0

	// Friendly information.
	if (smoothness_weight == 0) LOG(INFO) << "Ignoring smoothness term as (smoothness_weight == 0)";
	if (prior_weight == 0) LOG(INFO) << "Ignoring prior term as (prior_weight == 0)";

	// The weight of the smoothness loss is the parameter smoothness_weight normalised by the number of mesh vertices.
	LossFunction* smoothness_loss = new ScaledLoss(nullptr, smoothness_weight / (est_depth_map.rows() * est_depth_map.cols()), TAKE_OWNERSHIP);
	LossFunction* prior_loss = new ScaledLoss(nullptr, prior_weight / (est_depth_map.rows() * est_depth_map.cols()), TAKE_OWNERSHIP);
	for (int i = 1; i < est_depth_map.rows() - 1; i++) // skip top and bottom rows (poles)
	{
		for (int j = 0; j < est_depth_map.cols(); j++)
		{
			// Smoothness
			if (smoothness_weight != 0)
				problem.AddResidualBlock(
				    SphereFittingCrossSmoothnessCost::Create(),
				    smoothness_loss,
				    &est_depth_map(i, j),                                                    // central vertex
				    &est_depth_map(i - 1, j),                                                // top vertex
				    &est_depth_map(i, (j + 1) % est_depth_map.cols()),                       // right vertex (wrap around if necessary)
				    &est_depth_map(i + 1, j),                                                // bottom vertex
				    &est_depth_map(i, (j - 1 + est_depth_map.cols()) % est_depth_map.cols()) // left vertex (wrap around if necessary)
				);

			// Prior
			if (prior_weight != 0)
				problem.AddResidualBlock(
				    SphereFittingDataTerm::Create(average_radius),
				    prior_loss,
				    &est_depth_map(i, j) // central pixel
				);
		}
	}

	if (smoothness_weight != 0)
	{
		// Smoothness terms at the poles.
		for (int j = 0; j < azimuth_steps; j++) // loop over all pole pixels
		{
			// Constrain all adjacent vertices at the poles to have similar depth.
			problem.AddResidualBlock(
			    SphereFittingBinarySmoothnessCost::Create(),
			    smoothness_loss,
			    &est_depth_map(0, j),                      // pole vertex
			    &est_depth_map(0, (j + 1) % azimuth_steps) // right neighbour (wrap around if necessary)
			);
			problem.AddResidualBlock(
			    SphereFittingBinarySmoothnessCost::Create(),
			    smoothness_loss,
			    &est_depth_map(polar_steps - 1, j),                      // pole vertex
			    &est_depth_map(polar_steps - 1, (j + 1) % azimuth_steps) // right neighbour (wrap around if necessary)
			);

			// Smoothness terms across the poles, computed on 3 vertices centred on a pole vertex.
			problem.AddResidualBlock(
			    SphereFitting1DLaplacianSmoothnessCost::Create(),
			    smoothness_loss,
			    &est_depth_map(1, j),                                      // bottom neighbour
			    &est_depth_map(0, j),                                      // pole vertex
			    &est_depth_map(1, (j + azimuth_steps / 2) % azimuth_steps) // top neighbour (across the pole)
			);
			problem.AddResidualBlock(
			    SphereFitting1DLaplacianSmoothnessCost::Create(),
			    smoothness_loss,
			    &est_depth_map(polar_steps - 2, j),                                      // top pixel
			    &est_depth_map(polar_steps - 1, j),                                      // pole vertex
			    &est_depth_map(polar_steps - 2, (j + azimuth_steps / 2) % azimuth_steps) // bottom neighbour (across the pole)
			);
		}
	}

	// Solve the problem
	Solver::Options options;
	Solver::Summary summary;
	options.num_threads = 8;
	options.max_num_iterations = 100;
	options.max_linear_solver_iterations = 10;
	options.linear_solver_type = ceres::SPARSE_NORMAL_CHOLESKY;
	//options.linear_solver_type = ceres::CGNR; // faster for bigger problems
	options.minimizer_progress_to_stdout = true;
	ceres::Solve(options, &problem, &summary);
	LOG(INFO) << "Full Ceres solver report:\n"
	          << summary.FullReport() << "\n";
}


/** Converts from spherical coordinates (radius, azimuth, polar) to Cartesian coordinates (x, y, z).
*    - azimuth: 0 is -z, pi / 2 is -x, pi is +z, 3pi / 4 is +x
*    - polar : 0 is -y, pi / 2 is equatorial plane, pi is +y
*/
Eigen::Vector3f SphereFitting::spherical2cartesian(Eigen::Vector3f point)
{
	float radius  = point.x();
	float azimuth = point.y();
	float polar   = point.z();

	float x = -radius * sinf(polar) * sinf(azimuth);
	float y = -radius * cosf(polar);
	float z = -radius * sinf(polar) * cosf(azimuth);

	return Eigen::Vector3f(x, y, z);
}


/** Converts from Cartesian coordinates (x, y, z) to spherical coordinates (radius, azimuth, polar).
*    - azimuth: 0 is -z, pi / 2 is -x, pi is +z, 3pi / 4 is +x
*    - polar : 0 is -y, pi / 2 is equatorial plane, pi is +y
*/
Eigen::Vector3f SphereFitting::cartesian2spherical(Eigen::Vector3f point)
{
	float radius = point.norm();

	// Special case: zero radius
	if (radius < 1e-10)
		return Eigen::Vector3f(0, 0, 0);

	float polar = acosf(-point.y() / radius);
	float azimuth = atan2f(-point.x(), -point.z());
	azimuth = fmod(fmod(azimuth, 2 * M_PI) + 2 * M_PI, 2 * M_PI); // find positive remainder, i.e. in range [0, 2pi]

	return Eigen::Vector3f(radius, azimuth, polar);
}


Eigen::Vector3f SphereFitting::depthmap2spherical(const MatrixXd& depth_map, int azimuth_index, int polar_index)
{
	// Compute spherical coordinates.
	double azimuth = (azimuth_index + 0.5f) / depth_map.cols() * 2 * M_PI;
	double polar = double(polar_index) / (depth_map.rows() - 1.f) * M_PI; // stretch top/bottom to caps

	// Convert from spherical coordinates to Cartesian coordinates (x, y, z).
	//   - azimuth: 0 is -z, pi / 2 is -x, pi is +z, 3pi / 4 is +x
	//   - polar: 0 is -y, pi / 2 is equatorial plane, pi is +y
	double radius = depth_map(polar_index, azimuth_index);

	return Eigen::Vector3f(radius, azimuth, polar);
}


Eigen::Vector3f SphereFitting::computeBarycentricCoords(Eigen::Vector2f p, Eigen::Vector2f a, Eigen::Vector2f b, Eigen::Vector2f c)
{
	//## source: https://gamedev.stackexchange.com/a/23745
	Eigen::Vector2f v0 = b - a;
	Eigen::Vector2f v1 = c - a;
	Eigen::Vector2f v2 = p - a;
	//d00 = np.dot(v0, v0)
	//d01 = np.dot(v0, v1)
	//d11 = np.dot(v1, v1)
	//d20 = np.dot(v2, v0)
	//d21 = np.dot(v2, v1)
	float d00 = v0.dot(v0);
	float d01 = v0.dot(v1);
	float d11 = v1.dot(v1);
	float d20 = v2.dot(v0);
	float d21 = v2.dot(v1);
	float denom = d00 * d11 - d01 * d01;
	float v = (d11 * d20 - d01 * d21) / denom;
	float w = (d00 * d21 - d01 * d20) / denom;
	float u = 1 - v - w;
	return Eigen::Vector3f(u, v, w);
}


void SphereFitting::writeSphereMesh(MatrixXd depth_map, std::string filename)
{
	// Open the mesh file.
	std::ofstream objFile;
	objFile.open(filename, std::fstream::out);

	// TODO: error handling ... overwrite existing file or not?

	// Write vertices for depth map: "v x y z"
	for (int i = 0; i < depth_map.rows(); i++)
	{
		for (int j = 0; j < depth_map.cols(); j++)
		{
			// Compute spherical coordinates.
			Eigen::Vector3f spherical = depthmap2spherical(depth_map, j, i);

			// Convert from spherical coordinates to Cartesian coordinates (x, y, z).
			Eigen::Vector3f vertex = spherical2cartesian(spherical);

			// Write vertex coordinates.
			objFile << "v " << vertex.x() << " " << vertex.y() << " " << vertex.z() << '\n';
		}
	}

	// Write triangle faces: "f i1 i2 i3"
	for (int i = 0; i < depth_map.rows() - 1; i++) // skip last row, as each row outputs triangles connecting to next row
	{
		for (int j = 0; j < depth_map.cols(); j++)
		{
			// Triangle vertices of quad: (t_op, b_ottom) x (l_eft, r_ight)
			unsigned int index_tl = depth_map.cols() * i + j + 1;
			unsigned int index_tr = depth_map.cols() * i + (j + 1) % depth_map.cols() + 1;
			unsigned int index_bl = depth_map.cols() * (i + 1) + j + 1;
			unsigned int index_br = depth_map.cols() * (i + 1) + (j + 1) % depth_map.cols() + 1;

			objFile << "f " << index_tl << " " << index_tr << " " << index_bl << '\n'; // top-left triangle
			objFile << "f " << index_tr << " " << index_br << " " << index_bl << '\n'; // bottom-right triangle
		}
	}

	objFile.close();
}


void SphereFitting::appendPointsToMesh(const std::vector<Eigen::Vector3f>& points, std::string filename)
{
	// Open the mesh file to append stuff.
	std::ofstream objFile;
	objFile.open(filename, std::fstream::out | std::fstream::app);

	// TODO: error handling ... overwrite existing file or not?

	// Write vertices for depth map: "v x y z"
	for (auto&& point : points)
	{
		objFile << "v " << point.x() << " " << point.y() << " " << point.z() << '\n';
	}

	objFile.close();
}


/**
 * Exports the {@code depth_map} as an OBJ mesh and colour-mapped PNG depth map.
 * 
 * If {@code min_depth} and {@code max_depth} are not set or both zero, then the depth map
 * visualisation uses the log depth mapped to the unit range. This is good for visualising
 * any range, but makes different depth maps uncomparable as their ranges may differ.
 * 
 * To fix the depth scaling, set {@code min_depth} and {@code max_depth} to the desired values.
 * The exported depth maps will still be logarithmic.
 * 
 * @param depth_map Input depth map in equirectangular format.
 * @param filename_prefix Filename prefix (including path) for OBJ and PNG outputs.
 * @param min_depth Minimum depth in scaled depth map.
 * @param max_depth Maximum depth in scaled depth map.
 */
void SphereFitting::exportSphereMesh(const Eigen::MatrixXd& depth_map, std::string filename_prefix, float min_depth, float max_depth)
{
	// Save clean sphere-fit mesh.
	writeSphereMesh(depth_map, filename_prefix + ".obj");

	// Save depth map in Sintel DPT format.
	Mat1f depth_map_cv;
	eigen2cv(Eigen::MatrixXf(depth_map.cast<float>()), depth_map_cv);
	//writeSintelDptFile(filename_prefix + ".dpt", depth_map_cv);

	// Rotate depth map 180 degrees, so it's the right way up.
	// (Also making a copy, to not overwrite the depth map passed into this function.)
	depth_map_cv = flip(depth_map_cv, -1);

	if (min_depth == 0 && max_depth == 0)
	{
		// Save normalised log depth map.
		writeNormalisedLogDepthMap(filename_prefix + "-log-norm-cm.png", depth_map_cv);
	}
	else
	{
		// Save scaled log depth map.
		writeLogDepthMap(filename_prefix + "-log-scaled-cm.png", depth_map_cv, min_depth, max_depth);
	}
}


void SphereFitting::exportSphereMeshAndPoints(const Eigen::MatrixXd& depth_map, const std::vector<Eigen::Vector3f>& points, std::string filename_prefix, float min_depth, float max_depth)
{
	// Save clean sphere-fit mesh + depth map.
	exportSphereMesh(depth_map, filename_prefix, min_depth, max_depth);

	//// Save sphere-fit mesh with points.
	//writeSphereMesh(depth_map, filename_prefix + "+points.obj");
	//appendPointsToMesh(points, filename_prefix + "+points.obj");

	// Save sphere-fit input points for debug.
	appendPointsToMesh(points, filename_prefix + "-points.obj");
}

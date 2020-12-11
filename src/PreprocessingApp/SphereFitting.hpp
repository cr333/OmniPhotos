#pragma once

#include "3rdParty/Eigen.hpp"

#include <string>
#include <vector>


class SphereFitting
{
public:
	SphereFitting() = default;
	SphereFitting(int polar_steps, int azimuth_steps);

	/**
	 * @brief Solves for the optimal sphere mesh given the points and parameters set as members.
	 */
	void solveProblem();


	static Eigen::Vector3f cartesian2spherical(Eigen::Vector3f point);
	static Eigen::Vector3f spherical2cartesian(Eigen::Vector3f point);

	static Eigen::Vector3f depthmap2spherical(const Eigen::MatrixXd& depth_map, int azimuth_index, int polar_index);

	static Eigen::Vector3f computeBarycentricCoords(Eigen::Vector2f p, Eigen::Vector2f a, Eigen::Vector2f b, Eigen::Vector2f c);

	static void writeSphereMesh(Eigen::MatrixXd depth_map, std::string filename);
	static void appendPointsToMesh(const std::vector<Eigen::Vector3f>& points, std::string filename);
	static void exportSphereMesh(const Eigen::MatrixXd& depth_map, std::string filename_prefix, float min_depth = 0.f, float max_depth = 0.f);
	static void exportSphereMeshAndPoints(const Eigen::MatrixXd& depth_map, const std::vector<Eigen::Vector3f>& points, std::string filename_prefix, float min_depth = 0.f, float max_depth = 0.f);


	// Number of mesh subdivisions along the polar angle, i.e. between the poles.
	int polar_steps = 18;

	// Number of mesh subdivisions along the equator.
	int azimuth_steps = 36;

	// Weight of the data term.
	double data_weight = 1;

	/** Types of robust losses supported by Ceres. */
	enum class RobustLoss
	{
		None = 0,
		Huber = 1,
		SoftLOne = 2,
		Cauchy = 3,
	};

	// Robust loss to use for the data term.
	RobustLoss robust_data_loss = RobustLoss::None;

	// Scale factor for the robust loss.
	double robust_data_loss_scale = 0.01;

	// Weight of the smoothness term.
	double smoothness_weight = 1;

	// Weight of the prior term.
	double prior_weight = 0;

	// Points that the sphere mesh is fitted to.
	std::vector<Eigen::Vector3f> points;

	// The estimated spherical depth map (per vertex depth, i.e. sphere radius).
	Eigen::MatrixXd est_depth_map;
};

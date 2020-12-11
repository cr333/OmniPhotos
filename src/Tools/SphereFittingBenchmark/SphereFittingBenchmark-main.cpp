#include "3rdParty/fs_std.hpp"

#include "PreprocessingApp/SphereFitting.hpp"

#include "Utils/DepthIO.hpp"
#include "Utils/Logger.hpp"
#include "Utils/Utils.hpp"
#include "Utils/cvutils.hpp"

#include <opencv2/core/eigen.hpp>
#include <opencv2/opencv.hpp>

#include <cstdio>
#include <fstream>


using namespace cv;
using namespace Eigen;
using namespace std;


std::vector<Eigen::Vector3f> generateCubePoints(int num_points, float width)
{
	float radius = width / 2;
	std::vector<Eigen::Vector3f> points;
	for (int i = 0; i < num_points / 6; i++)
	{
		Eigen::Vector3f point = radius * Eigen::Vector3f::Random();

		// points on a cube
		points.push_back(Eigen::Vector3f(-radius, point.y(), point.z()));
		points.push_back(Eigen::Vector3f(radius, point.y(), point.z()));
		points.push_back(Eigen::Vector3f(point.x(), -radius, point.z()));
		points.push_back(Eigen::Vector3f(point.x(), radius, point.z()));
		points.push_back(Eigen::Vector3f(point.x(), point.y(), -radius));
		points.push_back(Eigen::Vector3f(point.x(), point.y(), radius));
	}

	return points;
}


std::vector<Eigen::Vector3f> generateDepthMapPoints(const Eigen::MatrixXd& depth_map)
{
	std::vector<Eigen::Vector3f> points;

	for (int i = 0; i < depth_map.rows(); i++)
	{
		for (int j = 0; j < depth_map.cols(); j++)
		{
			points.push_back(SphereFitting::spherical2cartesian(SphereFitting::depthmap2spherical(depth_map, j, i)));
		}
	}

	return points;
}


std::vector<Eigen::Vector3f> generateRandomPointsInCube(int num_points, float width)
{
	std::vector<Eigen::Vector3f> points;
	for (int i = 0; i < num_points; i++)
	{
		// Eigen's Random function returns values in [-1, 1], which we scale to [-range/2, range/2].
		Eigen::Vector3f point = 0.5f * width * Eigen::Vector3f::Random();
		points.push_back(point);
	}

	return points;
}


std::vector<Eigen::Vector3f> generateRandomPointsInSphere(int num_points, float radius)
{
	std::vector<Eigen::Vector3f> points;
	for (int i = 0; i < num_points; i++)
	{
		// Rejection sampling: if a random point is outside the sphere, try again.
		for (int attempt = 0; attempt < 100; attempt++)
		{
			Eigen::Vector3f point = radius * Eigen::Vector3f::Random();

			if (point.norm() > radius)
				continue;

			points.push_back(point);
			break;
		}
	}

	return points;
}


std::vector<Eigen::Vector3f> generateSpherePoints(int num_points, float radius)
{
	std::vector<Eigen::Vector3f> points;
	for (int i = 0; i < num_points; i++)
	{
		Eigen::Vector3f point = Eigen::Vector3f::Random();

		// point on a sphere
		point *= radius / point.norm();
		points.push_back(point);
	}

	return points;
}


void addUniformNoiseToPoints(std::vector<Eigen::Vector3f>& points, float range)
{
	for (auto& point : points)
		point += range * Eigen::Vector3f::Random();
}


void addRadialUniformNoiseToPoints(std::vector<Eigen::Vector3f>& points, float range)
{
	for (auto& point : points)
	{
		float old_radius = point.norm();
		float new_radius = old_radius + range * (2 * float(std::rand()) / float(RAND_MAX) - 1);
		point = point * (new_radius / old_radius);
	}
}


double computeReconstructionErrorCube(const Eigen::MatrixXd& depth_map, double width)
{
	double sse = 0;
	const auto& vertices = generateDepthMapPoints(depth_map);
	for (auto& vertex : vertices)
	{
		// Project each vertex onto the nearest cube face, which reduces to finding the maximum absolute coordinate.
		float p = (vertex.maxCoeff() > -vertex.minCoeff() ? vertex.maxCoeff() : vertex.minCoeff());
		Eigen::Vector3f gt_point(0, 0, 0);
		if (vertex.x() == p) gt_point = vertex * (float(width / 2) / abs(vertex.x())); // faces +/- x
		if (vertex.y() == p) gt_point = vertex * (float(width / 2) / abs(vertex.y())); // faces +/- y
		if (vertex.z() == p) gt_point = vertex * (float(width / 2) / abs(vertex.z())); // faces +/- z
		sse += (vertex - gt_point).dot(vertex - gt_point);
	}

	return sqrt(sse / vertices.size());
}


double computeReconstructionErrorGT_RMSE(const Eigen::MatrixXd& depth_map, const Eigen::MatrixXd& ground_truth)
{
	const auto diff = depth_map - ground_truth;
	return sqrt(diff.array().square().mean());
}


double computeReconstructionErrorGT_MAE(const Eigen::MatrixXd& depth_map, const Eigen::MatrixXd& ground_truth)
{
	const auto diff = depth_map - ground_truth;
	return sqrt(diff.array().abs().mean());
}


double computeReconstructionErrorSphere(const Eigen::MatrixXd& depth_map, double radius)
{
	double sse = 0;
	const auto& vertices = generateDepthMapPoints(depth_map);
	for (auto& vertex : vertices)
	{
		sse += pow(vertex.norm() - radius, 2);
	}

	return sqrt(sse / vertices.size());
}


void downsampleDepthMaps()
{
	std::vector<std::string> scenes { "apartment_0", "hotel_0", "office_0", "office_4", "room_0", "room_1" };
	const fs::path basepath = fs::current_path();

	for (auto scene : scenes)
	{
		// Load depth map and image.
		Mat1f depth_map = readSintelDptFile((basepath / scene / "0000_depth.dpt").generic_string());
		Mat3b image = imread((basepath / scene / "0000_rgb.jpg").generic_string());

		// Downsample depth map and image to 320x160.
		Mat1f depth_map_low = resize(depth_map, 320, 160, cv::INTER_AREA);
		Mat3b image_low = resize(image, 320, 160, cv::INTER_AREA);

		// Save depth map and image.
		writeSintelDptFile((basepath / (scene + "-depth.dpt")).generic_string(), depth_map_low);
		writeLogDepthMap((basepath / (scene + "-depth-vis.png")).generic_string(), depth_map_low, 0.5f, 5.f);
		imwrite((basepath / (scene + "-rgb.png")).generic_string(), image_low);
	}
}


int main(int argc, char* argv[])
{
	Logger logger(argv[0]);

	// Files are written in this directory. Default: current working directory.
	string basepath = "";

	//SphereFitting fit(32, 64);
	//SphereFitting fit(50, 100);

	//// Generate a depth map with some (optional) additive noise
	//MatrixXd depth_map = MatrixXd::Ones(32, 64);

	//// Visualise depth map in Image Watch
	//Mat1d depth_map_cv;
	//eigen2cv(depth_map, depth_map_cv);

	//// Add some noise
	//depth_map += 0.2 * MatrixXd::Random(depth_map.rows(), depth_map.cols());
	//eigen2cv(depth_map, depth_map_cv);

	//// Spikes along axes for debugging
	//depth_map(depth_map.rows() / 2, (3 * depth_map.cols()) / 4) = 3; // x-axis
	////depth_map.row(depth_map.rows() - 1).setConstant(3); // z-axis
	////depth_map(depth_map.rows() / 2, depth_map.cols() / 2) = 3; // z-axis
	//cv::eigen2cv(depth_map, depth_map_cv);


	// Synthetic ground-truth evaluation on ground-truth depth maps from Replica.
	vector<string> scenes { "apartment_0", "hotel_0", "office_0", "office_4", "room_0", "room_1" };

	for (auto scene : scenes)
	{
		// Load and downsample GT depth maps as starting point.
		std::string filename = determinePathToSource() + "Tools/SphereFittingBenchmark/Data/" + scene + "-depth.dpt";
		Mat1f depth_map = readSintelDptFile(filename) * 100.f;                      // [m] to [cm]
		flip(depth_map, depth_map, -1);                                             // Rotate depth map 180 degrees to make consistent with global coordinate system.
		depth_map = cv::clamp(depth_map, 0, 1000);                                  // clamp infinite points to 10 m
		cv::Mat1f depth_map_low = cv::resize(depth_map, 160, 80, cv::INTER_AREA);
		//depth_map = cv::resize(depth_map, 320, 160, cv::INTER_AREA); // 51,200 points ~= COLMAP point density
		depth_map = cv::resize(depth_map, 80, 40, cv::INTER_AREA); // 3,200 points ~= OpenVSLAM point density

		// Create per-scene output directory.
		string output_path = (fs::path(basepath) / scene).generic_string();
		fs::create_directory(output_path);

		// Save ground-truth depth maps at fitting resolution (to compute errors later).
		Eigen::MatrixXd depth_map_eigen, depth_map_low_eigen;
		cv::cv2eigen(depth_map, depth_map_eigen);
		cv::cv2eigen(depth_map_low, depth_map_low_eigen);
		auto depth_map_points = generateDepthMapPoints(depth_map_eigen);
		SphereFitting::exportSphereMeshAndPoints(depth_map_eigen, depth_map_points, output_path + "/groundtruth", 50, 500);
		SphereFitting::exportSphereMeshAndPoints(depth_map_low_eigen, depth_map_points, output_path + "/groundtruth-low", 50, 500);

		// Open the CSV log file to write our statistics to.
		std::ofstream csvFile;
		csvFile.open(output_path + "/_run.csv", std::fstream::out);
		csvFile << "Run,MeshResolution,Mode,Noise,Outliers,RobustDataLoss,RobustLossScale,DataWeight,SmoothnessWeight,PriorWeight,Count,RMSE,RMSE2,RMSE_STDEV,MAE,MAE2,MAE_STDEV\n";

		// Settings for various comparisons.
		int repeats = 10;
		std::vector<bool> depth_vs_disp { false, true };
		std::vector<int> mesh_resolutions { 80 /*16, 24, 32, 40, 50, 60, 72, 80, 100, 120, 150, 180*/ };
		std::vector<double> noise_levels { 2 /*0, 1, 2, 5, 10, 20*/ };
		std::vector<double> outlier_levels { 800 /*0, 1, 2, 5, 10, 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000*/ };
		std::vector<SphereFitting::RobustLoss> robust_losses {
			SphereFitting::RobustLoss::None,
			SphereFitting::RobustLoss::Huber,
			//SphereFitting::RobustLoss::SoftLOne,
			SphereFitting::RobustLoss::Cauchy
		};
		std::vector<double> robust_scales { 0.1 };
		std::vector<double> smoothness_values { 100 };
		std::vector<double> prior_values { 0.001 /*0, 1e-8, 1e-7, 1e-6, 1e-5, 1e-4, 0.001, 0.01, 0.1, 1, 10, 100*/ };
		// COLMAP setting: 40000 points + 10000 outliers
		// OpenVSLAM setting: 2000 points + 500 outliers

		//// Values and ranges used for Figure 3 in the OmniPhotos paper.
		//std::vector<double> outlier_levels { 0, 160, 320, 480, 640, 800, 960, 1120, 1280, 1440, 1600 }; // 0-50% additional outliers, in steps of 5%
		//std::vector<double> smoothness_values { 1, 2, 5, 10, 20, 50, 100, 200, 500, 1000 };
		//std::vector<double> robust_scales { 0.0001, 0.0002, 0.0005, 0.001, 0.002, 0.005, 0.01, 0.02, 0.05, 0.1, 0.2, 0.5, 1, /*2, 5, 10*/ };

		int run = 0;
		for (int mesh_resolution : mesh_resolutions)
		{
			for (bool compute_in_disparity_space : depth_vs_disp)
			{
				for (double noise_level : noise_levels)
				{
					for (double outlier_level : outlier_levels)
					{
						for (auto robust_loss : robust_losses)
						{
							for (double robust_scale : robust_scales)
							{
								for (double smoothness_value : smoothness_values)
								{
									for (double prior_value : prior_values)
									{
										double total_count = 0;
										double total_rmse = 0;
										double total_rmse_squared = 0;
										double total_mae = 0;
										double total_mae_squared = 0;

										// clang-format off
									#pragma omp parallel for reduction(+ : total_count, total_rmse, total_rmse_squared, total_mae, total_mae_squared)
										// clang-format on
										for (int repeat = 0; repeat < repeats; repeat++)
										{
#ifdef _OPENMP
											// Set a different random seed for every OpenMP thread, otherwise they all use the same random numbers.
											srand(int(time(NULL)) ^ omp_get_thread_num());
#endif // _OPENMP

											SphereFitting fit(mesh_resolution, 2 * mesh_resolution);

											// Sample some random ground-truth points.
											//fit.points = generateCubePoints(80000, 600); // 600 cm cube length = 3 m radius
											//fit.points = generateDepthMapPoints(depth_map);
											//fit.points = generateSpherePoints(2000, 300); // 3 m sphere radius
											fit.points = depth_map_points; // GT depth map points

											// Disturb the clean points.
											addUniformNoiseToPoints(fit.points, noise_level);
											//addRadialUniformNoiseToPoints(fit.points, 0.1f);

											// Add some random outlier points on top.
											auto outliers = generateRandomPointsInCube(outlier_level, 1000); // 10 m cubed
											//auto outliers = generateRandomPointsInSphere(outlier_level, 1000); // 10 m radius
											fit.points.insert(fit.points.end(), outliers.begin(), outliers.end());

											// Convert points to disparity space.
											if (compute_in_disparity_space)
											{
												for (auto& point : fit.points)
													point = point / pow(point.norm(), 2); // updates point
											}

											// Apply settings
											fit.data_weight = 1;
											fit.robust_data_loss = robust_loss;
											fit.robust_data_loss_scale = robust_scale;
											fit.smoothness_weight = smoothness_value;
											fit.prior_weight = prior_value;

											fit.solveProblem();

											// Convert points and depth map back to depth from disparity.
											if (compute_in_disparity_space)
											{
												for (auto& point : fit.points)
													point = point / pow(point.norm(), 2); // updates point

												fit.est_depth_map = fit.est_depth_map.cwiseInverse();
											}

											// Encode all settings in filename.
											stringstream suffix;
											suffix << "-res" << mesh_resolution;
											suffix << (compute_in_disparity_space ? "-disp" : "-depth");
											suffix << "-noise" << noise_level;
											suffix << "-outliers" << outlier_level;
											suffix << "-loss" << int(robust_loss);
											suffix << "-scale" << robust_scale;
											suffix << "-sm" << smoothness_value;
											suffix << "-pr" << prior_value;
											suffix << "-" << repeat;

											//fit.exportSphereMesh(depth_map, basepath + "depth_map");
											//fit.exportSphereMeshAndPoints(fit.est_depth_map, fit.points, basepath + "est_depth_map" + suffix.str(), 250, 500); // for syntheticd cube
											fit.exportSphereMeshAndPoints(fit.est_depth_map, fit.points, output_path + "/est_depth_map" + suffix.str(), 50, 500); // for Replica depth map

											// Export as result mesh + depth map.
											//double error = computeReconstructionErrorCube(fit.est_depth_map, 600);
											double rmse = computeReconstructionErrorGT_RMSE(fit.est_depth_map, depth_map_low_eigen);
											double mae = computeReconstructionErrorGT_MAE(fit.est_depth_map, depth_map_low_eigen);
											//double error = computeReconstructionErrorSphere(fit.est_depth_map, 300);

											// not actually a warning, but more visible
											LOG(WARNING) << "Reconstruction error: RMSE = " << rmse << "; MAE = " << mae;

											// ignore big outliers
											bool broken = (fit.est_depth_map.array() > 10000).any();
											if (broken || rmse > 1000)
											{
												LOG(WARNING) << "Skipped due to broken solution";
											}
											else
											{
												// for computing the mean and standard deviation on the fly
												total_count++;
												total_rmse += rmse;
												total_rmse_squared += rmse * rmse;
												total_mae += mae;
												total_mae_squared += mae * mae;
											}
										}
										csvFile << run << ","
										        << mesh_resolution << ","
										        << (compute_in_disparity_space ? "disparity" : "depth") << ","
										        << noise_level << ","
										        << outlier_level << ","
										        << int(robust_loss) << ","
										        << robust_scale << ","
										        << 1 << "," // fit.data_weight
										        << smoothness_value << ","
										        << prior_value << ","
										        << total_count << ","
										        << total_rmse / total_count << ","
										        << total_rmse_squared / total_count << ","
										        << sqrt(total_rmse_squared / total_count - pow(total_rmse / total_count, 2)) << ","
										        << total_mae / total_count << ","
										        << total_mae_squared / total_count << ","
										        << sqrt(total_mae_squared / total_count - pow(total_mae / total_count, 2)) << "\n";
										run++;
									}
								}
							}
						}
					}
				}
			}
		}

		csvFile.close();
	}
}

#pragma once

#include "Core/GL/GLRenderable.hpp"
#include "Core/Geometry/Point3D.hpp"
#include "Core/Geometry/Shape.hpp"

/**
 * @brief Class to represent a 3D point cloud.
 */
class PointCloud : public Shape, public GLRenderable
{
public:
	PointCloud() = default;
	PointCloud(std::vector<Point3D*>* _points, bool _use_colour_buffer = false);
	PointCloud(std::vector<Point3D*> _vertices);
	PointCloud(std::string file);
	virtual ~PointCloud() = default;

	Eigen::Point3f getCentroid();

	void createRenderModel(const std::string& _name) override; //GLRenderable

	std::vector<Point3D*>* getPoints();
	void setPoints(const std::vector<Point3D*>& _points);

	void merge(PointCloud& otherCloud);

	void writeToObj(std::string filename);

private:
	void computeCentreAndBox();
};

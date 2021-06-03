#pragma once

#include "Core/GL/GLRenderable.hpp"
#include "Core/Geometry/Circle.hpp"
#include "Core/Geometry/Shape.hpp"

#include <memory>

/**
 * Class to represent cylinders.
 */
class Cylinder : public Shape, public GLRenderable
{
public:
	Cylinder();
	Cylinder(Eigen::Point3f _centroid, float _radius, float _height);
	Cylinder(Eigen::Point3f _centroid, Eigen::Vector3f _up, Eigen::Vector3f _forward, float _radius, float _height);
	Cylinder(Eigen::Point3f _centroid, Eigen::Matrix3f _orientation, float _radius, float _height);

	//copy constructor
	Cylinder(const Cylinder& otherCylinder);

	virtual ~Cylinder() = default;

	void createRenderModel(const std::string& _name) override; // GLRenderable

	float getRadius();
	void setRadius(float _radius);
	void changeRadius(float _change);

	float getHeight();
	void setHeight(float _height);

	Eigen::Vector3f getUp();
	void setUp(Eigen::Vector3f _up);

	Eigen::Vector3f getForward();
	void setForward(Eigen::Vector3f _forward);

	// overriding Shape::setCentre to update the circles
	void setCentre(Eigen::Point3f _centre);

	void init(int resolution); // creates circles

	void createVerticesForTriangleStrip(int pointsOnCircles);

	void setCircleResolution(int _resolution);

	float radius = 1; // radius [cm]

private:
	int circle_resolution = 512;
	Eigen::Vector3f up;
	Eigen::Vector3f forward;
	Eigen::Matrix3f basis;

	float height = 1;                     // height should be infinite
	std::shared_ptr<Circle> heavenCircle; // infinitely up
	std::shared_ptr<Circle> hellCircle;   // infinitely down
};

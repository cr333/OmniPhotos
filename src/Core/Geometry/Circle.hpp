#pragma once

#include "3rdParty/Eigen.hpp"

#include "Core/Geometry/Shape.hpp"

#include <vector>


/*
@brief A circle
@param normal of the plane in which the circle lies;
@param radius of the circle
@param x left vector of normal tangent space
@param z forward vector of normal tangent space
@method createVertices() used to create line segments for rendering.
@method sampleCircle is used to sample vertices along the circle
*/
class Circle : public Shape
{
public:
	Circle() {}
	Circle(Eigen::Point3f _centre, float _radius);
	Circle(Eigen::Point3f _centre, Eigen::Vector3f _normal, Eigen::Vector3f _forward, float _radius);
	Circle(Eigen::Point3f _centre, Eigen::Matrix3f _orientation, float _radius);

	// copy constructor
	Circle(const Circle& otherCircle);
	virtual ~Circle();

	Eigen::Vector3f getNormal();
	void setNormal(Eigen::Vector3f _normal);

	Eigen::Vector3f getForward();
	void setForward(Eigen::Vector3f _forward);

	float getRadius();
	void setRadius(float _radius);

	Eigen::Matrix3f getBase();

	void computeTangentPlane();
	void computeTangentPlane(Eigen::Vector3f _normal, Eigen::Vector3f _forward);

	//step in radians
	Eigen::Point3f generateVertex(float step);

	void createVertices(std::vector<float>* _vertices, int numberOfPoints);

	void sampleCircle(std::vector<Eigen::Point3f>* points, int numberOfPoints);

private:
	Eigen::Vector3f normal;
	float radius = 0;

	// tangent plane of the normal
	Eigen::Vector3f x; // left
	Eigen::Vector3f z; // forward
};

#pragma once

#include "3rdParty/Eigen.hpp"

#include "Core/Geometry/Shape.hpp"

#include <vector>


/** Class to represent circles. */
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

	// step in radians
	Eigen::Point3f generateVertex(float step);

	/**
	 * @brief Creates vertices for rendering.
	 * 
	 * @param _vertices      Output vector to which vertices will be written.
	 * @param numberOfPoints Number of vertices on the circle.
	 */
	void createVertices(std::vector<float>* _vertices, int numberOfPoints);

	/**
	 * @brief Sample vertices along the circle.
	 * 
	 * @param points         Output vector to which vertices will be written.
	 * @param numberOfPoints Number of vertices on the circle.
	 */
	void sampleCircle(std::vector<Eigen::Point3f>* points, int numberOfPoints);

private:
	/** Normal of the plane in which the circle lies. */
	Eigen::Vector3f normal;

	/** Radius of the circle [cm]. */
	float radius = 0;

	/** Left vector of normal tangent space. */
	Eigen::Vector3f x;

	/** Forward vector of normal tangent space. */
	Eigen::Vector3f z;
};

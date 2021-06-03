#pragma once

#include "3rdParty/Eigen.hpp"

#include "Core/Geometry/Shape.hpp"

#include <vector>


/** 
 * @brief A circle
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
	/**
	 * @brief createVertices() used to create line segments for rendering..
	 * 
	 * @param _vertices
	 * @param numberOfPoints
	 */
	void createVertices(std::vector<float>* _vertices, int numberOfPoints);
	
	/**
	 * @brief used to sample vertices along the circle.
	 * 
	 * @param points
	 * @param numberOfPoints
	 */
	void sampleCircle(std::vector<Eigen::Point3f>* points, int numberOfPoints);

private:
	/** normal of the plane in which the circle lies; */
	Eigen::Vector3f normal;

	/** radius of the circle */
	float radius = 0;

	// tangent plane of the normal
	/** left vector of normal tangent space */
	Eigen::Vector3f x;
	/** forward vector of normal tangent space */
	Eigen::Vector3f z;
};

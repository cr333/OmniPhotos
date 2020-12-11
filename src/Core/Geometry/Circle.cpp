#include "Circle.hpp"

#include "Core/LinearAlgebra.hpp"


Circle::Circle(Eigen::Point3f _centre, float _radius)
{
	centre = _centre;
	radius = _radius;
	computeTangentPlane();
}


Circle::Circle(Eigen::Point3f _centre, Eigen::Vector3f _normal, Eigen::Vector3f _forward, float _radius)
{
	centre = _centre;
	radius = _radius;
	computeTangentPlane(_normal, _forward);
}


Circle::Circle(Eigen::Point3f _centre, Eigen::Matrix3f _orientation, float _radius)
{
	//for (int i = 0; i < 3; i++)
	//{
	//	x(i) = _orientation(i, 0);
	//	normal(i) = _orientation(i, 1);
	//	z(i) = _orientation(i, 2);
	//}

	x = _orientation.block<3, 1>(0, 0);
	normal = _orientation.block<3, 1>(0, 1);
	z = _orientation.block<3, 1>(0, 2);

	centre = _centre;
	radius = _radius;
}


//copy constructor
Circle::Circle(const Circle& otherCircle)
{
	centre = otherCircle.centre;
	normal = otherCircle.normal;
	radius = otherCircle.radius;
	computeTangentPlane();
}


Circle::~Circle()
{
}


void Circle::setNormal(Eigen::Vector3f _normal)
{
	normal = _normal;
}


Eigen::Vector3f Circle::getForward()
{
	return z;
}


void Circle::setForward(Eigen::Vector3f _forward)
{
	z = _forward;
}


float Circle::getRadius()
{
	return radius;
}


void Circle::setRadius(float _radius)
{
	radius = _radius;
}


Eigen::Vector3f Circle::getNormal()
{
	return normal;
}


Eigen::Matrix3f Circle::getBase()
{
	Eigen::Matrix3f base;
	base.col(0) = x;
	base.col(1) = normal;
	base.col(2) = z;
	return base;
}


//interpret as MVG
void Circle::computeTangentPlane()
{
	x = Eigen::Vector3f(1, 0, 0);
	normal = Eigen::Vector3f(0, 1, 0);
	z = Eigen::Vector3f(0, 0, 1);
}


//The circles is rotated in the MVG space.
//This is important for non straight captures, e.g. the mountain dataset.
void Circle::computeTangentPlane(Eigen::Vector3f _normal, Eigen::Vector3f _forward)
{
	normal = _normal;
	z = _forward;
	x = normal.cross(z);
}


void Circle::sampleCircle(std::vector<Eigen::Point3f>* points, int numberOfPoints)
{
	float stepSize = 2.0f * (float)M_PI / float(numberOfPoints);
	Eigen::Point3f pos;
	for (int i = 0; i < numberOfPoints; i++)
	{
		pos = generateVertex(i * stepSize);
		points->push_back(pos);
	}
}


//step in radians
Eigen::Point3f Circle::generateVertex(float step)
{
	return centre + radius * cosf(step) * x + radius * sinf(step) * z;
}


void Circle::createVertices(std::vector<float>* _vertices, int numberOfPoints)
{
	float stepSize = 2.0f * (float)M_PI / float(numberOfPoints);
	Eigen::Point3f pos;
	for (int i = 0; i < numberOfPoints; i++)
	{
		//always consider line segments (1,2) -> (2,3) -> (3,4) -> ...
		pos = generateVertex(i * stepSize);
		for (int k = 0; k < 3; k++)
			_vertices->push_back(pos(k));
		pos = generateVertex((i + 1) * stepSize);
		for (int k = 0; k < 3; k++)
			_vertices->push_back(pos(k));
	}
}

#include "Cylinder.hpp"

#include "Core/GL/GLRenderModel.hpp"
#include "Core/Geometry/Primitive.hpp"
#include "Core/LinearAlgebra.hpp"

using namespace std;


Cylinder::Cylinder()
{
}


Cylinder::Cylinder(Eigen::Point3f _centroid, float _radius, float _height)
{
	up = Eigen::Vector3f(0, 1, 0);
	centre = _centroid;
	radius = _radius;
	height = _height;
	hellCircle = make_shared<Circle>(centre - up * height / 2.0, radius);
	heavenCircle = make_shared<Circle>(centre + up * height / 2.0, radius);
}


Cylinder::Cylinder(Eigen::Point3f _centroid, Eigen::Vector3f _up, Eigen::Vector3f _forward, float _radius, float _height)
{
	up = _up;
	centre = _centroid;
	forward = _forward;
	radius = _radius;
	height = _height;
}


Cylinder::Cylinder(Eigen::Point3f _centroid, Eigen::Matrix3f _orientation, float _radius, float _height)
{
	up = _orientation.row(1);
	forward = _orientation.row(2);

	centre = _centroid;
	radius = _radius;
	height = _height;
}


Cylinder::Cylinder(const Cylinder& otherCylinder)
{
	centre = otherCylinder.centre;
	radius = otherCylinder.radius;
	height = otherCylinder.height;
	up = otherCylinder.up;
	heavenCircle = otherCylinder.heavenCircle;
	hellCircle = otherCylinder.hellCircle;
	circle_resolution = otherCylinder.circle_resolution;
}


void Cylinder::init(int resolution)
{
	setCircleResolution(resolution);
	Eigen::Point3f heavenCentre = centre + (up * height / 2);
	Eigen::Point3f hellCentre = centre - (up * height / 2);
	heavenCircle = std::make_shared<Circle>(heavenCentre, up, forward, radius);
	hellCircle = std::make_shared<Circle>(hellCentre, up, forward, radius);
}


void Cylinder::setCircleResolution(int _resolution)
{
	circle_resolution = _resolution;
}


void Cylinder::createRenderModel(const std::string& _name)
{
	createVerticesForTriangleStrip(circle_resolution);
	int size = (int)numberOfVertices();
	GLRenderModel* model = getRenderModel();
	if (!model)
		model = new GLRenderModel(_name, make_shared<Primitive>(size, PrimitiveType::TriangleStrip, white));
	else
		model->updateVertCnt(size);

	model->glGenVAO();
	glBindVertexArray(model->getVAO());
	glEnableVertexAttribArray(0);

	GLMemoryLayout memLayout = GLMemoryLayout(size, 3, "", "GL_FLOAT", "", 0, nullptr);
	GLBufferLayout bufLayout = GLBufferLayout(memLayout, 0); //location 0

	model->glGenVBO(bufLayout);
	glBindBuffer(GL_ARRAY_BUFFER, model->getVertBufID());
	model->setVertexAttrib(0);
	model->setVertexBufferData(0, vertices.data());
	glBindVertexArray(0);
	glDisableVertexAttribArray(0);

	setRenderModel(model);
}


void Cylinder::setRadius(float _radius)
{
	radius = _radius;

	// TODO: what if the circles are NULL (e.g. when using default constructor?)
	heavenCircle->setRadius(_radius);
	hellCircle->setRadius(_radius);
}


void Cylinder::setCentre(Eigen::Point3f _centre)
{
	centre = _centre;
	// TODO: update cylinder?
	//->no, we just "update" the cylinder when we update a render model.
	//If we do math, we don't need GL buffers and thus no updates
}


void Cylinder::setHeight(float _height)
{
	height = _height;
}


void Cylinder::setUp(Eigen::Vector3f _up)
{
	up = _up;
}


void Cylinder::setForward(Eigen::Vector3f _forward)
{
	forward = _forward;
}


void Cylinder::createVerticesForTriangleStrip(int pointsOnCircles)
{
	if ((!heavenCircle) || (!hellCircle))
		init(pointsOnCircles);

	vertices.clear();
	float step = 2.0f * (float)M_PI / float(pointsOnCircles);
	for (int i = 0; i < pointsOnCircles + 1; i++)
	{
		//heaven
		Eigen::Vector3f v = heavenCircle->generateVertex(step * i);
		for (int j = 0; j < 3; j++)
			vertices.push_back(v(j));
		//hell
		v = hellCircle->generateVertex(step * i);
		for (int j = 0; j < 3; j++)
			vertices.push_back(v(j));
	}
	this->fillVertex3DVectorFromVertices();
}


float Cylinder::getRadius()
{
	return radius;
}


void Cylinder::changeRadius(float _change)
{
	setRadius(radius + _change);
}


float Cylinder::getHeight()
{
	return height;
}


Eigen::Vector3f Cylinder::getUp()
{
	return up;
}


Eigen::Vector3f Cylinder::getForward()
{
	return forward;
}

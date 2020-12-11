#include "Shape.hpp"


Shape::Shape()
{
	centre = Eigen::Point3f(0.0f, 0.0f, 0.0f);
}


Eigen::Point3f Shape::getCentre() const
{
	return centre;
}


void Shape::setCentre(Eigen::Point3f _centre)
{
	centre = _centre;
}

#include "Point3D.hpp"


Point3D::Point3D()
{
	id = 0;
	pos = Eigen::Vector3f(0.0f, 0.0f, 0.0f);
	colour = Eigen::Vector3f(0.0f, 0.0f, 0.0f);
}


Point3D::Point3D(Eigen::Vector3f _pos, Eigen::Vector3f _normal, Eigen::Vector2f _tex) :
    pos(_pos), normal(_normal), tex(_tex)
{
	colour = Eigen::Vector3f(1.0f, 0.0f, 0.0f);
}


Point3D::Point3D(int _id, const Eigen::Vector3f& _pos, const Eigen::Vector3f& _colour, float _error)
{
	id = _id;
	pos = _pos;
	colour = _colour;
	error = _error;
}


Point3D::Point3D(float x, float y, float z, float r, float g, float b) :
    pos(Eigen::Vector3f(x, y, z)), colour(Eigen::Vector3f(r, g, b))
{
}


Point3D::Point3D(Eigen::Vector3f p, Eigen::Vector3f c) :
    pos(p), colour(c)
{
}

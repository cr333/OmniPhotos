#pragma once

#include "3rdParty/Eigen.hpp"


/*
@brief Some sort of abstract geometrical shape.
*/
class Shape
{
public:
	virtual ~Shape() = default;

	Eigen::Point3f getCentre() const;
	void setCentre(Eigen::Point3f _centre);

	std::string name;

protected:
	Shape();
	Eigen::Point3f centre;
};

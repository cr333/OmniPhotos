#pragma once

#include "Core/GL/GLRenderable.hpp"
#include "Core/Geometry/Shape.hpp"

/**
 * @brief Class to represent a sphere.
 */
class Sphere : public Shape, public GLRenderable
{
public:
	Sphere(int _phiRes, int _thetaRes, float _radius, bool flipNormals = false);
	virtual ~Sphere() = default;

	inline int getPhiRes() const { return phiRes; }
	inline int getThetaRes() const { return thetaRes; }

	inline float getRadius() const { return radius; }
	inline void setRadius(float _radius) { radius = _radius; }
	inline void changeRadius(float _change) { setRadius(radius + _change); }

	void generateVertices();

	void createRenderModel(const std::string& _name) override; // GLRenderable

private:
	int phiRes;
	int thetaRes;
	float radius;
};

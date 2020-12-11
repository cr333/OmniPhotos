#pragma once

#include "Core/GL/GLRenderable.hpp"
#include "Core/Geometry/Point3D.hpp"
#include "Core/Geometry/Shape.hpp"

class GLRenderModel;


class Plane : public Shape, public GLRenderable
{
public:
	Plane(Eigen::Vector2i _dimensions);
	Plane(Eigen::Point3f _centre, Eigen::Vector3f _normal, Eigen::Vector2i _dimensions);
	//Plane(std::vector<Point3D*>& verticesPoint3D, Eigen::Vector2i dim, Eigen::Vector3f c, Eigen::Vector3f n);
	//Plane(std::vector<float>& vertices, Eigen::Vector2i dim, Eigen::Vector3f _centre, Eigen::Vector3f _normal);
	virtual ~Plane() = default;

	void moveAlongNormal(float distance);

	// Generate vertices for buffer (two triangles).
	float* generateVertices();

	std::vector<float> generateVertices(Eigen::Vector3f up, Eigen::Vector3f right);

	void createRenderModel(const std::string& _name) override; // GLRenderable
	//void updateRenderModel(Eigen::Vector3f up, Eigen::Vector3f right);


	/**
	 * Generates a plane with submeter range. Implemented this way since
	 * the dimensions in the class are stored as ints, and therfore
	 * submeter range is not allowed.
	 *
	 * Note: If you assume your ints are [mm] or [cm] or [dm] etc, you are looking at submeter range. 
	 * It's an assumption that the "1" stands for 1m. 
	 * I assumed [cm] to be the default unit when I started coding the Framework since 
	 * I thought that's naturally an interesting unit to measure viewpoint differences, 
	 * especially if they are close together, as for instance in video input.
	 * 
	 * @param _dimensions float of the dimensions of the vertices.
	 * @return vertices
	 */
	std::vector<float> generateSubMeterVertices(Eigen::Vector2f _dimensions);

	Eigen::Vector3f getNormal();
	void setNormal(Eigen::Vector3f _normal);

	Eigen::Vector2i getDimensions();
	void setDimensions(Eigen::Vector2i _dims);


private:
	void generateIndices();

	Eigen::Vector3f normal;

	//consider plane as a whole centred around "centre"
	//width of 100 means, 50 to the left and 50 to the right relative to "centre"
	Eigen::Vector2i dimensions; // TODO FIXME: why should this an int, not a float?
};

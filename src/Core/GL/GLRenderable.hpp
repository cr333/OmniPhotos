#pragma once

#include "Core/GL/GLRenderModel.hpp"
#include "Core/Geometry/Point3D.hpp"
#include "Core/Geometry/Primitive.hpp"

#include <vector>



/**
 * @brief A child of the Shape class is renderable via OpenGL if it inherits from GLRenderable.
 * @param render_model Encapsulates all proxy information needed to render it using a GLProgram.
 */
class GLRenderable
{
public:
	GLRenderable() = default;
	GLRenderable(const std::vector<Point3D*>& _verticesPoint3D);
	GLRenderable(const std::vector<float>& _vertices);
	GLRenderable(const std::vector<float>& _vertices, const std::vector<float>& _colours);
	GLRenderable(const std::vector<float>& _vertices, const std::vector<unsigned int>& _indices);
	virtual ~GLRenderable() = default;

	/**
	 * @brief Needs to be implemented by every child, e.g.Plane, Sphere, Mesh...
	 * 
	 * Every class that inherits needs to define how it is rendered here.
	 */
	virtual void createRenderModel(const std::string& _name) = 0;

	GLRenderModel* getRenderModel() { return render_model; }

	void fillVertex3DVectorFromVertices();
	void fillStructuresFromVertex3DVector();

	int numberOfVertices();

	/////////////////////////////////////////////
	// TODO: This should be all part of the RenderModel's Primitive!
	// A mesh consists basically just out of a contiguous range of triangles

	/**
	 * Vertices stored in a linear vector, every 3 floats -> one vertex, easy geometry submission.
	 */
	std::vector<float> vertices;

	//A vertex has a stride of 3 floats (x,y,z).
	uint32_t stride = 3;

	std::vector<unsigned int> vertex_indices;

	/**
	 * Stores vertices with additional information..
	 */
	std::vector<Point3D*> points3D;

	// same size as vertices or 0
	std::vector<float> colours;
	bool use_colour_buffer = false;

	// e.g. pos + normal + tex per vertex
	bool use_interleaved_vertex_buffer = false;
	bool use_indices = false;
	bool use_texture = false;

protected:
	void setRenderModel(GLRenderModel* _render_model) { render_model = _render_model; }

private:
	GLRenderModel* render_model = nullptr;
};

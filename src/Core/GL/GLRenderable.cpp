#include "GLRenderable.hpp"

#include "Core/GL/GLRenderModel.hpp"
#include "Core/Geometry/Primitive.hpp"

#include "Utils/ErrorChecking.hpp"


GLRenderable::GLRenderable(const std::vector<Point3D*>& _verticesPoint3D) :
    GLRenderable()
{
	for (int i = 0; i < _verticesPoint3D.size(); i++)
	{
		Point3D* p = _verticesPoint3D[i];
		points3D.push_back(p);
	}
	fillStructuresFromVertex3DVector();
}


GLRenderable::GLRenderable(const std::vector<float>& _vertices) :
    GLRenderable()
{
	for (int i = 0; i < _vertices.size() / stride; i++)
	{
		float x = _vertices[i * stride + 0];
		float y = _vertices[i * stride + 1];
		float z = _vertices[i * stride + 2];
		Point3D* p = new Point3D(x, y, z);
		points3D.push_back(p);
	}
	fillStructuresFromVertex3DVector();
}


GLRenderable::GLRenderable(const std::vector<float>& _vertices, const std::vector<float>& _colours) :
    GLRenderable()
{
	assert(_vertices.size() == _colours.size());
	for (int i = 0; i < _vertices.size() / 3; i++)
	{
		float x = _vertices[i * 3 + 0];
		float y = _vertices[i * 3 + 1];
		float z = _vertices[i * 3 + 2];

		float r = _colours[i * 3 + 0];
		float g = _colours[i * 3 + 1];
		float b = _colours[i * 3 + 2];

		Point3D* p = new Point3D(x, y, z, r, g, b);
		points3D.push_back(p);
	}
	fillStructuresFromVertex3DVector();
}


GLRenderable::GLRenderable(const std::vector<float>& _vertices, const std::vector<unsigned int>& _indices) :
    GLRenderable(_vertices)
{
	for (int i = 0; i < _indices.size(); i++)
	{
		vertex_indices.push_back(_indices[i]);
	}
}


void GLRenderable::fillVertex3DVectorFromVertices()
{
	points3D.clear();
	for (int v = 0; v < vertices.size(); v += 3)
	{
		float x = vertices[v + 0];
		float y = vertices[v + 1];
		float z = vertices[v + 2];
		points3D.push_back(new Point3D(x, y, z));
	}
}


void GLRenderable::fillStructuresFromVertex3DVector()
{
	vertices.clear();
	colours.clear();
	for (int i = 0; i < points3D.size(); i++)
	{
		const Point3D* p = points3D[i];
		vertices.push_back(p->pos.x());
		vertices.push_back(p->pos.y());
		vertices.push_back(p->pos.z());
		if (use_interleaved_vertex_buffer)
		{
			vertices.push_back(p->normal.x());
			vertices.push_back(p->normal.y());
			vertices.push_back(p->normal.z());
			vertices.push_back(p->tex.x());
			vertices.push_back(p->tex.y());
		}
		colours.push_back(p->colour.x());
		colours.push_back(p->colour.y());
		colours.push_back(p->colour.z());
	}
}


// TODO: should be a Primitive method
int GLRenderable::numberOfVertices()
{
	assert((vertices.size() / stride) == points3D.size());
	return (int)points3D.size();
}

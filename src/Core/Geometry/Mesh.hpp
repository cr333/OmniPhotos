#pragma once

#include "Core/GL/GLRenderable.hpp"
#include "Core/Geometry/Point3D.hpp"
#include "Core/Geometry/Shape.hpp"

#include <vector>


class GLRenderModel;


struct VertexInterleaved
{
	Eigen::Vector3f pos; // position in meters in device space
	Eigen::Vector3f normal;
	Eigen::Vector2f tex;
	VertexInterleaved(const Eigen::Vector3f& _pos, const Eigen::Vector3f& _normal, const Eigen::Vector2f& _tex) :
	    pos(_pos), normal(_normal), tex(_tex) {}
};


struct MeshData
{
	//Geometry
	//# of vertices
	const uint32_t size;
	//data pointer
	const VertexInterleaved* vertex_data;
	//adjacency
	const uint16_t* index_data;
	// number of triangles
	uint32_t n_triangles;

	//Diffuse texture
	//Texture -> Primitive
	uint16_t tex_width;
	uint16_t tex_height;
	const uint8_t* tex_data;

	uint32_t stride;

	MeshData(uint32_t _size, const VertexInterleaved* _vertex_data, uint32_t _n_triangles, const uint16_t* _index_data, uint16_t _tex_width, uint16_t _tex_height, const uint8_t* _tex_data) :
	    size(_size), vertex_data(_vertex_data), n_triangles(_n_triangles), index_data(_index_data), tex_width(_tex_width), tex_height(_tex_height), tex_data(_tex_data)
	{
		stride = sizeof(VertexInterleaved);
	}
};


/** Class to represent a triangle mesh. */
class Mesh : public Shape, public GLRenderable
{
public:
	Mesh() = default;
	Mesh(const std::vector<float>& _vertices);
	Mesh(const std::vector<float>& _vertices,
	     const std::vector<unsigned int>& _indices);

	// We use this constructor to use the controller meshes coming from OpenVR.
	// We apply a scale factor of 100 to the mesh vertices to convert from [m] to [cm]
	Mesh(MeshData& mesh_dataconst, const std::string& pchRenderModelName, float scale_factor);
	virtual ~Mesh();

	void createRenderModel(const std::string& _name) override; // GLRenderable

	static Mesh* load(const std::string& filename);

private:
	uint16_t tex_width;
	uint16_t tex_height;
	const uint8_t* tex_data;
};

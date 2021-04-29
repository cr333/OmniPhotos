#pragma once

#include "Core/GL/GLBuffer.hpp"
#include "Core/GL/GLTexture.hpp"
#include "Core/Geometry/Point3D.hpp"

#include <GL/gl3w.h>

#include <memory>


// TODO: could put the coordinate axes here.

enum class PrimitiveType
{
	Point = 0,
	Line = 1,
	Triangle = 2,
	TriangleStrip = 3,
};


// Globally defined colours.
extern const float white[3];
extern const float static_colors[3][3];


//TODO: Check whether most of the public things actually need to be public.
/**
 * @brief Rendering primitives: points, lines, triangles.
 */
class Primitive
{
public:
	Primitive(int _vertexCount, PrimitiveType _type,
	          bool _use_interleaved_vertex_buffer,
	          bool _use_indices,
	          bool _use_texture);
	Primitive(int _vertexCount, PrimitiveType _type, const float* _colourRGB);
	Primitive(int _vertexCount, PrimitiveType _type);
	//TODO: Add interfaces with one or two GLBuffers as input -> vertices + color
	~Primitive();

	// Uniform colour //should be Eigen::Vec3f
	const float* colourRGB = nullptr;

	// All I need to draw the program
	int pointSize = 1;

	//TODO: Don't like that this is in GLRenderable as well -.-
	bool use_indices = false;
	bool use_colour_buffer = false;
	bool use_interleaved_vertex_buffer = false;
	bool use_texture = false;

	//Used to model the current position and orientation of the Primitive
	//We introduce this to avoid deleting and re-initing geometry "onRender()"
	Eigen::Matrix4f pose;

	// Vertex Array Object: Memorize buffer allocations
	GLuint vao = 0;

	// GLBuffer: holds actual vertices (and texture coordinates) of the model which is to be rendered
	std::unique_ptr<GLBuffer> vertexBuffer;

	// Number of elements to draw buffer (vertexBuffer) -> seems redundant
	GLsizei vertexCount = 0;


	// TODO: used for coloured points (point clouds) or coordinate axes.
	//Assert same size as vertex Buffer if useColourBuffer = true
	std::unique_ptr<GLBuffer> colourBuffer;

	// Index buffer: defines geometry (triangles) via connectivities among points (stored in vertexBuffer)
	std::unique_ptr<GLBuffer> indexBuffer;
	GLsizei indexCount = 0;

	PrimitiveType type;

	// Rendering using indexBuffers is slightly more complicated
	GLuint getRenderingMode();

	// Optional texture, e.g. for OpenVR controllers.
	std::unique_ptr<GLTexture> tex;
};

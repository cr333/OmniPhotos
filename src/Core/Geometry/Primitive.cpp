#include "Primitive.hpp"


const float white[3] = { 1.0, 1.0, 1.0 };

const float static_colors[3][3] = { { 1.0, 0.0, 0.0 },   // red
	                                { 0.0, 1.0, 0.0 },   // green
	                                { 0.0, 0.0, 1.0 } }; // blue


Primitive::Primitive(int _vertexCount, PrimitiveType _type)
{
	vertexCount = _vertexCount;
	type = _type;
	colourRGB = nullptr;
	pose.setIdentity();
}


//TODO: Maybe a better way of setting default colours would be to attach something to GLPrograms -> settings/configuration of a method.
//Or it's a global visualization setting.
Primitive::Primitive(int _vertexCount, PrimitiveType _type, const float* _colourRGB) :
    Primitive(_vertexCount, _type)
{
	colourRGB = _colourRGB;
}


Primitive::Primitive(int _vertexCount, PrimitiveType _type,
                     bool _use_interleaved_vertex_buffer,
                     bool _use_indices,
                     bool _use_texture) :
    Primitive(_vertexCount, _type, nullptr)
{
	use_interleaved_vertex_buffer = _use_interleaved_vertex_buffer;
	use_indices = _use_indices;
	use_texture = _use_texture;
}


Primitive::~Primitive()
{
	if (vao)
	{
		glDeleteVertexArrays(1, &vao);
		vao = 0;
	}
}


GLuint Primitive::getRenderingMode()
{
	switch (type)
	{
		case PrimitiveType::Point:
			return GL_POINTS;

		case PrimitiveType::Line:
			return GL_LINES;

		case PrimitiveType::Triangle:
			return GL_TRIANGLES;

		case PrimitiveType::TriangleStrip:
			return GL_TRIANGLE_STRIP;

			// No default to trigger compiler warning for missed entries.
	}

	return 0;
}

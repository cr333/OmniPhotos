#pragma once

#include "3rdParty/Eigen.hpp"

#include <GL/gl3w.h>

#include <string>


/*
@brief Define memory layout of VBOs.
@param elements number of vertices in the underlying rendering Primitive
@param size number of entries per element 
*/
struct GLMemoryLayout
{
	GLMemoryLayout() = default;
	GLMemoryLayout(int _elements, int _size, const std::string& _format, const std::string& _type, const std::string& _internalFormat);

	// stride and offsets needed for working with interleaved buffers
	GLMemoryLayout(int _elements, int _size, const std::string& _format, const std::string& _type, const std::string& _internalFormat, GLsizei _stride, GLvoid* _offset);

	int elements = -1; //number of elements
	int size = -1;     // e.g.: 3 -> rgb, 1 -> float

	// TODO: I don't think that this belongs here.
	std::string format = "";         // GL_RGB
	std::string type = "";           // GL_FLOAT
	std::string internalFormat = ""; // GL_RGB16F

	GLsizei stride;
	GLvoid* ptrOffset = nullptr;
};


struct GLBufferLayout
{
	GLBufferLayout() = default;
	GLBufferLayout(GLMemoryLayout _mem, int _location);

	GLMemoryLayout mem;
	int location = -1; // connects buffers in the shader. Important for "glVertexAttribPointer"
};


struct GLTextureLayout
{
	GLTextureLayout() = default;
	GLTextureLayout(GLMemoryLayout _mem, Eigen::Vector2i _resolution, std::string _targetUniform, int _activeTexture);

	GLMemoryLayout mem;

	Eigen::Vector2i resolution = Eigen::Vector2i(-1, -1); // width, height
	std::string targetUniform = "";
	int activeTexture = -1;
};


/*
@brief Used as base class for OpenGL objects
@param GLuint gl_ID Identifier from the generated OpenGL context.
*/
class GLObject
{
public:
	GLObject() = default;
	virtual ~GLObject() = default;

	GLuint gl_ID = 0;
	std::string name = "";
	std::string job = "";

	virtual void init() = 0;
};

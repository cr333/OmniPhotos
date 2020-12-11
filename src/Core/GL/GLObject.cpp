#include "GLObject.hpp"


GLMemoryLayout::GLMemoryLayout(int _elements, int _size, const std::string& _format, const std::string& _type, const std::string& _internalFormat)
{
	elements = _elements;
	size = _size;
	stride = size;
	format = _format;
	type = _type;
	internalFormat = _internalFormat;
}


//Define memory formats for GLBuffers and GLTextures. TODO: These should be separate things. 
GLMemoryLayout::GLMemoryLayout(int _elements, int _size, const std::string& _format, const std::string& _type, const std::string& _internalFormat, GLsizei _stride, GLvoid* _offset) :
    elements(_elements), size(_size), format(_format), type(_type), internalFormat(_internalFormat)
{
	stride = _stride;
	ptrOffset = _offset;
}

//This keep a mapping between memory layouts and shader locations. 
//TODO: I believe that this would look very differently if we would go to OpenGL 4.3+.. 
//The interleaved vertex buffer consisting of (pos, normal, tex) used in the VR controller causes issues at the moment
//since the structure is not tested for that yet.
GLBufferLayout::GLBufferLayout(GLMemoryLayout _mem, int _location)
{
	mem = _mem;
	location = _location;
}


GLTextureLayout::GLTextureLayout(
    GLMemoryLayout _mem, //should be explicitly "texture" memory
	Eigen::Vector2i _resolution,
	std::string _targetUniform,
	int _activeTexture)
{
	mem = _mem;
	resolution = _resolution;
	targetUniform = _targetUniform;
	activeTexture = _activeTexture;
}

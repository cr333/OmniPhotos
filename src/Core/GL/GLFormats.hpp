#pragma once

#include <GL/gl3w.h>

#include <string>


// These are not defined by gl3w.
#define GL_COMPRESSED_RGB_S3TC_DXT1_NV 0x83F0
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT 0x83F1
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83F3


// Returns the OpenGL image format corresponding to the given string 'in'.
GLuint getGLFormat(const std::string& in);

// Returns the OpenGL data type corresponding to the given string 'in'.
GLenum getGLType(const std::string& in);

// Returns the OpenGL internal format corresponding to the given string 'in'.
GLuint getGLInternalFormat(const std::string& in);

// Returns the number of colour channels for the given 'internalFormat' (string).
int getChannelsFromInternalFormat(const std::string& internalFormat);

// Returns the number of colour channels for the given 'internalFormat' (GLint).
int getChannelsFromInternalFormat(GLint internalFormat);

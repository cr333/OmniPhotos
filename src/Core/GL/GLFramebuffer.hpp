#pragma once

#include <GL/gl3w.h>

#include <string>


class GLFramebuffer
{
public:
	GLFramebuffer(
	    int width,
	    int height,
	    const std::string& _imageFormat,
	    const std::string& _imageType,
	    const std::string& _internalFormat);

	~GLFramebuffer();

	GLuint imageFormat;
	GLuint imageType;
	GLuint internalFormat;

	GLuint id = 0;
	GLuint renderedTextureID = 0;
	GLuint depthTextureID = 0;

	int init(int width, int height);
};

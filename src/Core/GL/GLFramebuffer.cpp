#include "GLFramebuffer.hpp"

#include "Core/GL/GLFormats.hpp"

#include "Utils/ErrorChecking.hpp"
#include "Utils/Logger.hpp"


GLFramebuffer::GLFramebuffer(
    int width,
    int height,
    const std::string& _imageFormat,
    const std::string& _imageType,
    const std::string& _internalFormat)
{
	imageFormat = getGLFormat(_imageFormat);
	imageType = getGLType(_imageType);
	internalFormat = getGLInternalFormat(_internalFormat);

	init(width, height);
}


GLFramebuffer::~GLFramebuffer()
{
	if (renderedTextureID != 0)
		glDeleteTextures(1, &renderedTextureID);
}


int GLFramebuffer::init(int width, int height)
{
	LOG(INFO) << "Setting up framebuffers";

	// Colour buffer/render texture (COLOR0).
	if (renderedTextureID == 0)
		glGenTextures(1, &renderedTextureID);
	glBindTexture(GL_TEXTURE_2D, renderedTextureID);
	glTexImage2D(GL_TEXTURE_2D, 0,
	             internalFormat,
	             width, height, 0,
	             imageFormat,
	             imageType,
	             nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);
	ErrorChecking::checkGLError();

	// Depth buffer (DEPTH0).
	glGenTextures(1, &depthTextureID);
	glBindTexture(GL_TEXTURE_2D, depthTextureID);
	glTexImage2D(GL_TEXTURE_2D, 0,
	             GL_DEPTH_COMPONENT16,
	             width, height, 0,
	             GL_DEPTH_COMPONENT,
	             GL_FLOAT,
	             nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);
	ErrorChecking::checkGLError();

	// Framebuffer with colour and depth.
	if (id == 0)
		glGenFramebuffers(1, &id);
	glBindFramebuffer(GL_FRAMEBUFFER, id);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderedTextureID, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTextureID, 0);
	ErrorChecking::checkGLError();

	GLenum DrawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, DrawBuffers);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		return -1;
	ErrorChecking::checkGLError();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	ErrorChecking::checkGLError();

	return 0;
}

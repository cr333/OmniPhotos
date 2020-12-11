#include "GLTexture.hpp"

#include "Utils/Logger.hpp"


GLTexture::GLTexture(const GLTextureLayout& _layout, const std::string& _textureType)
{
	layout = _layout;
	textureType = _textureType;
}


GLTexture::~GLTexture()
{
	glDeleteTextures(1, &gl_ID);
}


void GLTexture::setActiveTexture(int i)
{
	if (i < 0 || i > 31)
	{
		LOG(WARNING) << "Texture unit " << i << " is not supported.";
		return;
	}

	glActiveTexture(GL_TEXTURE0 + i);
}


void GLTexture::bindTexture()
{
	if (textureType == "GL_TEXTURE_2D_ARRAY")
		glBindTexture(GL_TEXTURE_2D_ARRAY, gl_ID);

	if (textureType == "GL_TEXTURE_2D")
		glBindTexture(GL_TEXTURE_2D, gl_ID);

	if (textureType == "GL_TEXTURE_1D")
		glBindTexture(GL_TEXTURE_1D, gl_ID);
}


void GLTexture::preDraw(GLuint programID)
{
	setActiveTexture(layout.activeTexture);
	bindTexture();
	GLint location = glGetUniformLocation(programID, layout.targetUniform.c_str());

	if (location < 0 && uniformWarnings.count(layout.targetUniform) == 0)
	{
		//DLOG(WARNING) << "Uniform '" << uniformName << "' not found in '" << name << "'";
		uniformWarnings.insert(layout.targetUniform);
	}

	if (location > -1)
		glUniform1i(location, layout.activeTexture);
	////useful for debug
	//else
	//	DLOG(WARNING) <<"uniform location: " << layout.targetUniform << " does not exist!";
}

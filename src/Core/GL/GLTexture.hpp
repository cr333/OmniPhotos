#pragma once

#include "GLObject.hpp"

#include <unordered_set>


class GLTexture : public GLObject
{
public:
	GLTexture() = default;
	GLTexture(const GLTextureLayout& _layout, const std::string& _textureType);
	virtual ~GLTexture();

	GLTextureLayout layout;
	std::string textureType = "GL_TEXTURE_2D";

	void setActiveTexture(int i);
	void bindTexture();

	void init() override {};
	void preDraw(GLuint programID);

private:
	std::unordered_set<std::string> uniformWarnings;
};

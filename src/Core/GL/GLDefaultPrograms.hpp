#pragma once

#include "Core/GL/GLFramebuffer.hpp"
#include "Core/GL/GLProgram.hpp"
#include "Utils/Logger.hpp"


// Quad rendering
class GLQuadProgram : public GLProgram
{
public:
	GLQuadProgram(std::shared_ptr<GLFramebuffer> _fb, GLuint _vbo);

	void setVertexArray(GLuint _vao)
	{
		vao = _vao;
	}

	void passUniforms(CameraInfo& camInfo) override;

	void preDraw() override;
	void draw() override;
	void postDraw() override;

private:
	std::shared_ptr<GLFramebuffer> framebuffer;
	GLuint vbo;
	GLuint vao;
};


class GLDefaultProgram : public GLProgram
{
public:
	GLDefaultProgram(GLRenderModel* _render_model = nullptr);
	GLDefaultProgram(const std::string& _name, GLRenderModel* _render_model = nullptr);

	void passUniforms(CameraInfo& camInfo) override;
	void draw() override;

	//TODO: this is a special case for the companion window in VR
	//void setVertexArray(GLuint _vao)
	//{
	//	vao = _vao;
	//	DLOG(WARNING) << "Using GLDefaultProgram::setVertexArray() is deprecated and should be done after the next VR iteration.";
	//}
};


class GLPointsProgram : public GLProgram
{
public:
	GLPointsProgram(GLRenderModel* _render_model);

	void passUniforms(CameraInfo& camInfo) override;
	void draw() override;
};


class GLDrawUniformLinesProgram : public GLProgram
{
public:
	GLDrawUniformLinesProgram(GLRenderModel* _render_model);

	void passUniforms(CameraInfo& camInfo) override;
	void draw() override;

	Eigen::Vector3f color;
};

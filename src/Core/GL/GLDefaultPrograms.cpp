#include "GLDefaultPrograms.hpp"

#include "Core/GL/GLRenderModel.hpp"
#include "Core/Geometry/Primitive.hpp"

#include "Utils/EigenOpenGL.hpp"


//---- Quad -----------------------------------------------------------------//
GLQuadProgram::GLQuadProgram(std::shared_ptr<GLFramebuffer> _fb, GLuint _vbo) :
    GLProgram("GLQuadProgram")
{
	framebuffer = _fb;
	vbo = _vbo;
}


// CameraInfo not used, but this is the only exception
void GLQuadProgram::passUniforms(CameraInfo& camInfo)
{
	glUseProgram(programID);

	// LOCATE texture to attachment 0
	glUniform1i(glGetUniformLocation(programID, "renderedTexture"), 0);
}


void GLQuadProgram::preDraw()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(programID);
	glBindVertexArray(vao);

	// BIND our texture in Texture Unit 0
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, framebuffer->renderedTextureID);
}


void GLQuadProgram::draw()
{
	preDraw();
	glDrawArrays(GL_TRIANGLES, 0, 6); // 2*3 indices starting at 0 -> 2 triangles
	postDraw();
}


void GLQuadProgram::postDraw()
{
	glBindVertexArray(0);
}


//---- Default --------------------------------------------------------------//
GLDefaultProgram::GLDefaultProgram(GLRenderModel* _render_model) :
    GLProgram("GLDefaultProgram", _render_model) {}


GLDefaultProgram::GLDefaultProgram(const std::string& _name, GLRenderModel* _render_model) :
    GLProgram(_name, _render_model)
{
}


void GLDefaultProgram::passUniforms(CameraInfo& camInfo)
{
	GLProgram::passUniformsBase(camInfo.matrix);
}


void GLDefaultProgram::draw()
{
	preDraw();

	auto prim = getActiveRenderModel()->getPrimitive();

	// Used by deformed sphere, but not the sphere nor the cylinder.
	// These two render GL_TRIANGLE_STRIPs without indices

	//TODO: Check for making member of Primitive
	if (prim->use_indices)
	{
		ErrorChecking::checkGLError();
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, prim->indexBuffer->gl_ID);
		ErrorChecking::checkGLError();
		glDrawElements(GL_TRIANGLES, prim->indexCount, GL_UNSIGNED_INT, 0);
		ErrorChecking::checkGLError();
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}
	else
	{
		ErrorChecking::checkGLError();
		glDrawArrays(prim->getRenderingMode(), 0, prim->vertexCount);
		ErrorChecking::checkGLError();
	}

	postDraw();
}


//---- Points ---------------------------------------------------------------//
GLPointsProgram::GLPointsProgram(GLRenderModel* _render_model) :
    GLProgram("GLDrawPointsProgram", _render_model)
{
}


void GLPointsProgram::passUniforms(CameraInfo& camInfo)
{
	GLProgram::passUniformsBase(camInfo.matrix);

	// primitive vertex count should be connect to the renderModel long-term -> later.
	glUniform1i(glGetUniformLocation(programID, "numberOfPoints"), getActiveRenderModel()->getPrimitive()->vertexCount);
}


void GLPointsProgram::draw()
{
	preDraw();
	glPointSize(GLfloat(getActiveRenderModel()->getPrimitive()->pointSize));
	glDrawArrays(GL_POINTS, 0, getActiveRenderModel()->getPrimitive()->vertexCount);
	postDraw();
}


//---- Uniform Lines ---------------------------------------------------------//
GLDrawUniformLinesProgram::GLDrawUniformLinesProgram(GLRenderModel* _render_model) :
    GLProgram("GLDrawUniformLines", _render_model)
{
	setActiveRenderModel(_render_model);

	// Try to copy the colour from primitive, whatever that means.
	if (_render_model->getPrimitive() && _render_model->getPrimitive()->colourRGB)
	{
		color = Eigen::Vector3f(_render_model->getPrimitive()->colourRGB[0],
		                        _render_model->getPrimitive()->colourRGB[1],
		                        _render_model->getPrimitive()->colourRGB[2]);
	}
	else
	{
		// default colour: cyan
		color = Eigen::Vector3f(0, 1, 1);
	}
}


void GLDrawUniformLinesProgram::passUniforms(CameraInfo& camInfo)
{
	GLProgram::passUniformsBase(camInfo.matrix);
	setUniform("inColor", color);
}


void GLDrawUniformLinesProgram::draw()
{
	preDraw();
	glDrawArrays(GL_LINES, 0, getActiveRenderModel()->getPrimitive()->vertexCount);
	postDraw();
}

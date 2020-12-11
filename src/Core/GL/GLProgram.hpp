#pragma once

#include "3rdParty/Eigen.hpp"

#include "Core/GL/GLRenderModel.hpp"
#include "Core/GL/GLSLParser.hpp"
#include "Core/GL/GLTexture.hpp"

#include "Utils/EigenOpenGL.hpp"
#include "Utils/ErrorChecking.hpp" // Not needed here, but usually everywhere that includes this file.
#include "Utils/Logger.hpp"

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>


struct CameraInfo
{
	CameraInfo(const Eigen::Matrix4f& _matrix, Eigen::Point3f _centre, Eigen::Vector3f _forward)
	{
		matrix = _matrix;
		centre = _centre;
		forward = _forward;
	}

	Eigen::Matrix4f matrix;
	Eigen::Point3f centre;
	Eigen::Vector3f forward;
};


/*
	The most important construct for rendering OpenGL stuff.

	Rendering does nothing but telling the GLCamera to render all its attached programs.
	This makes intuitively sense. You want to render content with respect to the GLCamera.
	
	If you want to render the same setup with a different camera (different view, parameters,
	setup static), you just replace the camera, NOT the GLProgram.

	However, if you want to use a different method, the GLProgram will use a different fragment
	shader or uses different settings via changing uniforms.
	
	To compare different methods at runtime, you have to create separate GLPrograms for them.
	Every GLProgram in an OpenGL context potentially set by Application and Visualization is
	drawn according to a Boolean flag "display" -- we just render the visible programs.
	
	This leads to the following situation:
	1. Everyone who wants to contribute to the final rendering registers in the GLCamera. 
	   -> Why is that? All GLPrograms should be passed to the GLCamera, not be registered.
	   It is how I expected, programs are passed and rendered with the camera's MVP matrix.
	2. The "contributors" store respective buffer objects (vertices and textures)
	   -> which will be split into GLProgram and GLRenderModel. The GLProgram will hold all
	   input texture related things for IBR sure. Note that if we want to render textured
	   geometry, e.g. Casual3D stuff, we do NOT need textures on the GLProgram side,
	   everything would be in the RenderModel/Mesh/Primitive.
	3. The buffers are passed to the GLProgram when initializing in the methods "initPrograms()".
	4. Uniforms are passed to the GLProgram in the form of a Settings* pointer, which comes
	   from the "contributors".
	5. All GLProgram drawing (except the GLQuadProgram) does not draw in the default framebuffer,
	   but into a framebuffer associated to the GLWindow.
	6. Every GL::Application::render() ends with setting the default framebuffer active and
	   rendering a quad (renderedTexture GL_COLOR_ATTACHMENT0) on it.

	I think that this was a huge step towards better living.
*/
class GLProgram
{
public:
	GLProgram(std::string _name);
	GLProgram(std::string _name, GLRenderModel* _render_model);
	virtual ~GLProgram();


	// Return the program ID.
	inline GLuint getProgramID() const { return programID; }


	// Sets the GL context via a GLFWwindow.
	inline void setContext(GLFWwindow* window) { context = window; }


	// Sets the active GLRenderModel.
	void setActiveRenderModel(GLRenderModel* _model) { active_render_model = _model; }

	// Gets the active GLRenderModel.
	GLRenderModel* getActiveRenderModel() const { return active_render_model; }


	// Sets path to the fragment shader.
	void setFragmentShaderFilename(std::string filename) { fragment_shader_filename = filename; }

	// Sets path to the vertex shader.
	void setVertexShaderFilename(std::string filename) { vertex_shader_filename = filename; }


	// Adds a texture input to the program.
	void addTexture(GLTexture* texture, int activeTexture, std::string targetUniform);


	// Passes the basic uniforms (MVP, pose) to the program.
	void passUniformsBase(Eigen::Matrix4f eyeMatrix);

	// Passes uniforms to the program (overridden by subclasses).
	virtual void passUniforms(CameraInfo& camInfo) = 0;


	// Prepares the drawing (binding VAO + textures).
	virtual void preDraw();

	// The rendering camera just calls draw() after passUniforms()!
	virtual void draw() = 0;

	// Finishes after the drawing.
	virtual void postDraw();


	// Returns the filenames to the shaders.
	std::vector<std::string> getShaderFiles();


	// Recompiles all shaders. State is defined in "passUniforms()" and "initPrograms()".
	// Geometry/Vertex buffer stuff is in the Primitive of the RenderModel.
	// Input textures, e.g. different textures from different datasets, come from "initPrograms()".
	// -> changing datasets means updating buffers, not creating new GLPrograms, but it is convenient.
	// Note that the Primitives (within the RenderModels) aren't touched when "initialising programs".
	void compileAndLink(std::shared_ptr<GLSLParser> parser);


	// Description of what the program is doing.
	std::string job;

	// Name of the program.
	std::string name = "GLProgram";

	// Sets whether stuff is drawn or not.
	bool display = true;


protected:
	// Returns the location of a uniform by name. Emits a warning if not found (only once, in debug).
	GLint getUniformLocation(const std::string& uniformName);

	// Sets an integer-valued uniform.
	void setUniform(const std::string& uniformName, int value);

	// Sets a float-valued uniform.
	void setUniform(const std::string& uniformName, float value);

	// Sets the value of a uniform to an Eigen vector or matrix.
	template <typename Derived>
	inline void setUniform(const std::string& uniformName, const Eigen::MatrixBase<Derived>& value)
	{
		GLint location = getUniformLocation(uniformName);

		// Ignore if uniform not found.
		if (location < 0) return;

		glUniform(location, value);
		ErrorChecking::checkGLError();
	}

	// Program ID generated by OpenGL.
	GLuint programID = 0;

	std::unordered_set<std::string> uniform_warnings;


private:
	// Path to the fragment shader.
	std::string fragment_shader_filename = "NULL";

	// Path to the vertex shader.
	std::string vertex_shader_filename = "NULL";

	// Since everything happens within the same OpenGL context, it's not stricly necessary
	// to store the context per program. Nevertheless, it should simplify the process
	// of adding multiple windows (or OpenGL contexts).
	GLFWwindow* context = nullptr;

	// We store everything geometry-related in the GLRenderModel.
	// A Mesh (->Shape) is the geometric entity. In connection with a Primitive, we render.
	// -> The Shape implements "GLRenderable" which contains the "GLRenderModel".
	// The cool thing about the Primitive is that it holds all the "geometry"-related
	// OpenGL state for rendering.
	GLRenderModel* active_render_model = nullptr;

	// Mapped textures. These are set in the "initPrograms()".
	std::vector<GLTexture*> inputTextures;
};

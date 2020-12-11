#include "GLProgram.hpp"

#include <iostream>

using namespace std;


GLProgram::GLProgram(std::string _name) :
    name(_name)
{
}


GLProgram::GLProgram(std::string _name, GLRenderModel* _render_model) :
    active_render_model(_render_model),
    name(_name)
{
}


GLProgram::~GLProgram()
{
	if (programID)
	{
		glDeleteProgram(programID);
		programID = 0;
	}
}


void GLProgram::addTexture(GLTexture* texture, int activeTexture, std::string targetUniform)
{
	texture->layout.activeTexture = activeTexture; // GL context
	texture->layout.targetUniform = targetUniform; // connection to uniform in shader
	inputTextures.push_back(texture);
}


void GLProgram::passUniformsBase(Eigen::Matrix4f eyeMatrix)
{
	glUseProgram(programID);

	// MVP = P * V * M * vertex. M = identity -> GLCamera needs re-factoring.
	setUniform("MVP", eyeMatrix);

	// This is basically "M" in the equation above
	setUniform("pose", getActiveRenderModel()->getPose());
}


void GLProgram::preDraw()
{
	if (active_render_model)
	{
		active_render_model->preDraw(programID);
	}
	else
	{
		LOG(ERROR) << "Render model not set!";
	}

	if (programID == 0)
	{
		LOG(WARNING) << "GLProgram '" << name << "' has programID == 0!";
		LOG(WARNING) << "Something went wrong in the program compilation. Press any key to proceed.";
		int i;
		std::cin >> i;
	}
	else
	{
		// Bind textures to texture uniforms/units.
		for (auto texture : inputTextures)
			texture->preDraw(programID);
	}
}


void GLProgram::postDraw()
{
	if (active_render_model)
		active_render_model->postDraw();

	// Reset active texture unit.
	glActiveTexture(GL_TEXTURE0);
}


vector<string> GLProgram::getShaderFiles()
{
	vector<string> shaderFiles;
	shaderFiles.push_back(vertex_shader_filename);
	shaderFiles.push_back(fragment_shader_filename);
	return shaderFiles;
}


void GLProgram::compileAndLink(shared_ptr<GLSLParser> parser)
{
	try
	{
		VLOG(1) << "------------------------------------";
		VLOG(1) << "Compile and link GLProgram: " << name;

		ErrorChecking::checkGLError();
		glDeleteProgram(programID);
		glfwMakeContextCurrent(context);
		ErrorChecking::checkGLError();

		if (programID != 0)
			LOG(INFO) << "GLProgram '" << name << "' updated";
		else
			LOG(INFO) << "GLProgram '" << name << "' initialised";

		programID = parser->loadShaders(vertex_shader_filename, fragment_shader_filename);

		if (programID == 0)
		{
			LOG(ERROR) << "--------------------------------------------------";
			LOG(ERROR) << "GLProgram linking error in '" << name << "'!";
			LOG(ERROR) << "--------------------------------------------------";
			LOG(ERROR) << "GLProgram couldn't be built properly. Press any key to proceed.";
			int i;
			std::cin >> i;
		}

		// Note for shiny future:
		// It would be IDEAL to scan for all uniforms in the parsing process and
		// to map each uniform "entry" to a database: name -> (location, type).
	}
	catch (std::exception& e)
	{
		LOG(ERROR) << "Exception: " << e.what();
	}
}


//---- Protected functions ------------------------------------------------------------------------


GLint GLProgram::getUniformLocation(const std::string& uniformName)
{
	GLint location = glGetUniformLocation(programID, uniformName.c_str());

	// Warn about uniforms that are not found (only once, in debug mode only).
	if (location < 0 && uniform_warnings.count(uniformName) == 0)
	{
		DLOG(WARNING) << "Uniform '" << uniformName << "' not found in '" << name << "'";
		uniform_warnings.insert(uniformName);
	}

	return location;
}


void GLProgram::setUniform(const std::string& uniformName, int value)
{
	const GLint location = getUniformLocation(uniformName);

	// Ignore if uniform not found.
	if (location < 0) return;

	glUniform1i(location, value);
	ErrorChecking::checkGLError();
}


void GLProgram::setUniform(const std::string& uniformName, float value)
{
	const GLint location = getUniformLocation(uniformName);

	// Ignore if uniform not found.
	if (location < 0) return;

	glUniform1f(location, value);
	ErrorChecking::checkGLError();
}

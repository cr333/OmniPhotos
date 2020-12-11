#pragma once

#include "Core/GL/GLProgram.hpp"
#include "Core/GL/GLSLParser.hpp"

#include <memory>
#include <vector>


/*
@brief  Compiles, links and stores all OpenGL programs that are used in the running GLApplication.
		GLPrograms are added in GLApplication::initPrograms()
		Shader editing and re-loading at runtime is provided by a shortcut: 'r' -> reload shaders.
*/
class GLProgramMaintenance
{
public:
	GLProgramMaintenance();
	~GLProgramMaintenance() = default;

	void clear();

	void addProgram(GLProgram* _gl_program);
	void addPrograms(std::vector<GLProgram*> _gl_programs);
	void updatePrograms(bool forceUpdate);

private:
	std::shared_ptr<GLSLParser> glslParser;
	std::vector<GLProgram*> glPrograms;
};

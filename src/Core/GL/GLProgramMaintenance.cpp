#include "GLProgramMaintenance.hpp"

#include "3rdParty/fs_std.hpp"

#include "Core/GL/GLSLFile.hpp"

#include "Utils/ErrorChecking.hpp"
#include "Utils/Exceptions.hpp"

using namespace std;


GLProgramMaintenance::GLProgramMaintenance()
{
	glslParser = make_shared<GLSLParser>();
}


void GLProgramMaintenance::clear()
{
	glPrograms.clear();
}


void GLProgramMaintenance::addProgram(GLProgram* _gl_program)
{
	glPrograms.push_back(_gl_program);
}


void GLProgramMaintenance::addPrograms(std::vector<GLProgram*> _gl_programs)
{
	for (GLProgram* program : _gl_programs)
		glPrograms.push_back(program);
}


void GLProgramMaintenance::updatePrograms(bool forceUpdate)
{
	for (GLProgram* program : glPrograms)
	{
		DLOG(INFO) << "Update program: " << program->name;
		DLOG(INFO) << "Job: " << program->job;
		bool filesOK = true;
		try
		{
			// add GLSLfiles to database
			for (string& shaderFile : program->getShaderFiles())
			{
				std::vector<std::string> includedKeys;
				if (shaderFile.compare("NULL") != 0)
				{
					// Missing file.
					if (!fs::exists(shaderFile))
					{
						LOG(WARNING) << "Error in Maintenance:\n"
						             << "\tFile \"" << shaderFile << "\" could not be opened.";
						RUNTIME_EXCEPTION("Shader file does not exist. Check that the shaders are written out correctly in initPrograms()");
					}

					GLSLFile* glslFileTmp = new GLSLFile(shaderFile);
					filesOK &= glslFileTmp->exists;
					if (filesOK)
					{
						glslFileTmp->parse(glslParser, &includedKeys, forceUpdate);
						glslParser->addGLSLfile(glslFileTmp);
					}
					else
					{
						LOG(ERROR) << "GLSL file '" << shaderFile << "' broken.";
						break;
					}
				}
			}

			if (filesOK)
			{
				program->compileAndLink(glslParser);
				DLOG(INFO) << "Program ID: " << program->getProgramID() << "\n";
			}
		}
		catch (std::exception& e)
		{
			LOG(ERROR) << "Exception: " << e.what();
		}
	}
}

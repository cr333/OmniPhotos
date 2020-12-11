#include "GLSLParser.hpp"

#include "Core/GL/GLSLFile.hpp"

#include "Utils/ErrorChecking.hpp"
#include "Utils/Exceptions.hpp"
#include "Utils/Logger.hpp"
#include "Utils/Utils.hpp"

using namespace std;


//called when a glsl file was fully parsed
void GLSLParser::addGLSLfile(GLSLFile* glslFile)
{
	if (glslFile->exists)
	{
		string key = stripExtensionFromFilename(glslFile->getFilename());
		glslFile->key = key;

		if (keyExists(key))
		{
			DLOG(INFO) << "File '" << glslFile->getFilename() << "' is already in database.";
		}
		else
		{
			string text_file = glslFile->getSource();
			fileDB.insert(std::make_pair(key, text_file));
		}

		glslFile->isLoaded = true;
	}
	else
	{
		// TODO: This seems dodgy.
		delete glslFile;
	}
}


bool GLSLParser::keyExists(const std::string& key) const
{
	return (fileDB.count(key) > 0);
}


bool GLSLParser::retrieveDatabaseEntry(const std::string& key, std::string& value) const
{
	if (keyExists(key))
	{
		value = fileDB.find(key)->second;
		return true;
	}

	value = "NULL";
	return false;
}


void GLSLParser::removeDatabaseEntry(const std::string& filename)
{
	const string key = stripFilenameToDBkey(filename);
	fileDB.erase(key);
}


GLuint GLSLParser::loadShaders(const string& vertex_file_path,
                               const string& fragment_file_path)
{
	// Create vertex and fragment shaders.
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);
	ErrorChecking::checkGLError();

	// Compile shaders ...
	bool allGood = true;
	allGood &= compileShaderSource(VertexShaderID, vertex_file_path);
	allGood &= compileShaderSource(FragmentShaderID, fragment_file_path);

	if (!allGood)
		RUNTIME_EXCEPTION("Shaders did not compile correctly.");

	// Link all shaders into one program.
	DLOG(INFO) << "Linking program";

	GLuint programID = glCreateProgram();
	glAttachShader(programID, VertexShaderID);
	glAttachShader(programID, FragmentShaderID);

	ErrorChecking::checkGLError();
	glLinkProgram(programID);
	ErrorChecking::checkGLError();

	// Check the linked program.
	GLint result = GL_FALSE;
	GLint infoLogLength;
	glGetProgramiv(programID, GL_LINK_STATUS, &result);
	glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &infoLogLength);
	ErrorChecking::checkGLError();

	if (infoLogLength > 0)
	{
		std::vector<char> errorMessage(infoLogLength + 1);
		glGetProgramInfoLog(programID, infoLogLength, nullptr, &errorMessage[0]);
		ErrorChecking::checkGLError();

		LOG(WARNING) << "Program failed to link:\n"
		           << &errorMessage[0];
		return 0;
	}

	VLOG(1) << "Program " << programID << " linked successfully. Result: " << result;
	ErrorChecking::checkGLError();

	// Clean up compiled shaders
	glDetachShader(programID, VertexShaderID);
	glDetachShader(programID, FragmentShaderID);
	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);
	ErrorChecking::checkGLError();

	return programID;
}


bool GLSLParser::compileShaderSource(GLuint shaderID, const std::string& shader_file_path)
{
	VLOG(1) << "compileShaderSource(" << shaderID << ", " << shader_file_path << ")";

	// Retrieve shader source from database.
	std::string shader_source;
	std::string key = stripFilenameToDBkey(shader_file_path);
	if (!retrieveDatabaseEntry(key, shader_source))
	{
		LOG(WARNING) << "Shader '" << shader_file_path << "' not found in database.";
		return false;
	}

	// Compile shader from source.
	char const* sourcePointer = shader_source.c_str();
	glShaderSource(shaderID, 1, &sourcePointer, nullptr);
	glCompileShader(shaderID);

	// Check for compile errors.
	GLint result = GL_FALSE;
	GLint infoLogLength;
	glGetShaderiv(shaderID, GL_COMPILE_STATUS, &result);
	glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &infoLogLength);
	ErrorChecking::checkGLError();

	if (infoLogLength > 0)
	{
		std::vector<char> errorMessage(infoLogLength + 1);
		glGetShaderInfoLog(shaderID, infoLogLength, nullptr, &errorMessage[0]);
		ErrorChecking::checkGLError();

		LOG(WARNING) << "Shader '" << shader_file_path << "' failed to compile:\n"
		           << &errorMessage[0];
		return false;
	}

	VLOG(1) << "Shader compiled successfully. Result: " << result;
	return true;
}


string GLSLParser::stripFilenameToDBkey(const string& filename)
{
	size_t pos = filename.find_last_of("/");
	string str = filename.substr(pos + 1);
	str = stripExtensionFromFilename(str);
	return str;
}

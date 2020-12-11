#include "ErrorChecking.hpp"
#include "Exceptions.hpp"
#include "Logger.hpp"

#include <vector>


void ErrorChecking::checkGLShaderError(GLuint shaderID)
{
	int InfoLogLength;
	glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0)
	{
		std::vector<char> shaderErrorMessage(InfoLogLength + 1);
		glGetShaderInfoLog(shaderID, InfoLogLength, nullptr, &shaderErrorMessage[0]);
		LOG(WARNING) << &shaderErrorMessage[0];
	}
}

// there is something wrong with that!
int ErrorChecking::checkGLProgramError(GLuint programID)
{
	int ret = 0;
	//printf("CheckGLProgramError causes some error. Don't use it! \n");
	int InfoLogLength;
	glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0)
	{
		std::vector<char> shaderErrorMessage(InfoLogLength + 1);
		glGetProgramInfoLog(programID, InfoLogLength, nullptr, &shaderErrorMessage[0]);
		LOG(WARNING) << &shaderErrorMessage[0];
		ret = -1;
	}
	return ret;
}


/// Throws an exception if an OpenGL error occurred.
void ErrorChecking::checkGLError()
{
	GLint errorCode = glGetError();

	// No error
	if (errorCode == GL_NO_ERROR)
		return;

	// Known errors
	if (errorCode == GL_INVALID_ENUM)
		RUNTIME_EXCEPTION("GL_INVALID_ENUM: An unacceptable value is specified for an enumerated argument.");

	if (errorCode == GL_INVALID_VALUE)
		RUNTIME_EXCEPTION("GL_INVALID_VALUE: A numeric argument is out of range.");

	if (errorCode == GL_INVALID_OPERATION)
		RUNTIME_EXCEPTION("GL_INVALID_OPERATION: The specified operation is not allowed in the current state.");

	if (errorCode == GL_INVALID_FRAMEBUFFER_OPERATION)
		RUNTIME_EXCEPTION("GL_INVALID_FRAMEBUFFER_OPERATION: The framebuffer object is not complete.");

	if (errorCode == GL_OUT_OF_MEMORY)
		RUNTIME_EXCEPTION("GL_OUT_OF_MEMORY: There is not enough memory left to execute the command.");

	if (errorCode == GL_STACK_UNDERFLOW)
		RUNTIME_EXCEPTION("GL_STACK_UNDERFLOW: An attempt has been made to perform an operation that would cause an internal stack to underflow.");

	if (errorCode == GL_STACK_OVERFLOW)
		RUNTIME_EXCEPTION("GL_STACK_OVERFLOW: An attempt has been made to perform an operation that would cause an internal stack to overflow.");

	// Unknown error
	RUNTIME_EXCEPTION("Unknown OpenGL error.");
}

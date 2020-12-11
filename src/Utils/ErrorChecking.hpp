#pragma once

#include <GL/gl3w.h>


class ErrorChecking
{
public:
	static void checkGLShaderError(GLuint shaderID);
	static int checkGLProgramError(GLuint programID);
	static void checkGLError();
};

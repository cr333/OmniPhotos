#include "GLBuffer.hpp"

#include "Core/GL/GLFormats.hpp"

#include "Utils/ErrorChecking.hpp"
#include "Utils/Logger.hpp"


GLBuffer::GLBuffer(bool _arrayBuffer) :
    arrayBuffer(_arrayBuffer)
{
}


GLBuffer::GLBuffer(std::vector<GLBufferLayout>& _layouts, bool _arrayBuffer) :
    GLBuffer(_arrayBuffer)
{
	layouts = _layouts;
}


GLBuffer::GLBuffer(const GLBufferLayout& _layout, bool _arrayBuffer) :
    GLBuffer(_arrayBuffer)
{
	layouts.push_back(_layout);
}


GLBuffer::~GLBuffer()
{
	if (gl_ID)
	{
		glDeleteBuffers(1, &gl_ID);
		gl_ID = 0;
	}
}


void GLBuffer::init()
{
	if (!gl_ID)
		glGenBuffers(1, &gl_ID);

	if (arrayBuffer)
		glBindBuffer(GL_ARRAY_BUFFER, gl_ID);
	else
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gl_ID);
}


// TB: I don't think that this is needed since all this information resides in the vertex array object
void GLBuffer::preDraw()
{
	glBindBuffer(GL_ARRAY_BUFFER, gl_ID);
	ErrorChecking::checkGLError();

	for (int i = 0; i < layouts.size(); i++)
	{
		GLBufferLayout layout = layouts[i];
		glEnableVertexAttribArray(layout.location);
		glVertexAttribPointer(
		    layout.location,            // attribute 0. No particular reason for 0, but must match the layout in the shader.
		    layout.mem.size,            // size
		    getGLType(layout.mem.type), // type
		    GL_FALSE,                   // normalized?
		    layout.mem.stride,          // stride
		    layout.mem.ptrOffset        // array buffer offset
		);
		ErrorChecking::checkGLError();
	}
}


void GLBuffer::postDraw()
{
	for (int i = 0; i < layouts.size(); i++)
	{
		glDisableVertexAttribArray(layouts[i].location);
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

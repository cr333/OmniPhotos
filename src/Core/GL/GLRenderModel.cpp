#include "GLRenderModel.hpp"

#include "Core/GL/GLFormats.hpp"
#include "Core/Geometry/Primitive.hpp"

#include "Utils/Logger.hpp"


GLRenderModel::GLRenderModel()
{
	name = "GLRenderModel";
}


GLRenderModel::GLRenderModel(const std::string& _name)
{
	name = _name;
}


GLRenderModel::GLRenderModel(const std::string& _name, std::shared_ptr<Primitive> _primitive) :
    GLRenderModel(_name)
{
	setPrimitive(_primitive);
}


void GLRenderModel::setPrimitive(std::shared_ptr<Primitive> _primitive)
{
	// Does anything needs to be updated  when a Primitive is set?
	// TB: Not yet, but first of all I don't want programmers to touch it.
	// Everything should go via the GLRenderModel.
	// I don't want a programmer use the Primitive directly.
	// Whenever this method is called (apart from constructors),
	// it indicates a non-optimal interfacing -> improve GLRenderModel interface.
	primitive = _primitive;
}


const Eigen::Matrix4f GLRenderModel::getPose()
{
	return primitive->pose;
}


void GLRenderModel::setPose(const Eigen::Matrix4f& _pose)
{
	primitive->pose = _pose;
}


void GLRenderModel::setVertexBufferData(int attrib, const void* data)
{
	GLBufferLayout layout = getVertexBufferLayout(attrib);
	glBufferData(GL_ARRAY_BUFFER,
	             layout.mem.elements * layout.mem.size * sizeof(float),
	             data,
	             GL_STATIC_DRAW);
}


void GLRenderModel::setColourBufferData(int attrib, const void* data)
{
	GLBufferLayout layout = getColourBufferLayout(attrib);
	glBufferData(GL_ARRAY_BUFFER,
	             layout.mem.elements * layout.mem.size * sizeof(float),
	             data,
	             GL_STATIC_DRAW);
}


void GLRenderModel::setIndicesData(const std::vector<unsigned int>& indices)
{
	int size = (int)indices.size();
	primitive->indexCount = size;
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, primitive->indexBuffer->gl_ID);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,
	             size * sizeof(unsigned int),
	             (void*)indices.data(),
	             GL_STATIC_DRAW);
	primitive->use_indices = true;
}


void GLRenderModel::setVertexAttrib(int attrib)
{
	GLBufferLayout layout = getVertexBufferLayout(attrib);
	glVertexAttribPointer(
	    layout.location,            // must match the layout in the shader.
	    layout.mem.size,            // size
	    getGLType(layout.mem.type), // type
	    GL_FALSE,                   // normalized?
	    layout.mem.stride,          // stride
	    layout.mem.ptrOffset        // array buffer offset
	);
}


void GLRenderModel::setColourAttrib(int attrib)
{
	GLBufferLayout layout = getColourBufferLayout(0);
	glVertexAttribPointer(
	    layout.location,            // attribute 0. No particular reason for 0, but must match the layout in the shader.
	    layout.mem.size,            // size
	    getGLType(layout.mem.type), // type
	    GL_FALSE,                   // normalized?
	    layout.mem.stride,          // stride
	    layout.mem.ptrOffset        // array buffer offset
	);
}


GLuint GLRenderModel::getVAO()
{
	return primitive->vao;
}


GLint GLRenderModel::getVertBufID()
{
	return primitive->vertexBuffer->gl_ID;
}


GLint GLRenderModel::getColourBufID()
{
	return primitive->colourBuffer->gl_ID;
}


GLint GLRenderModel::getIndBufID()
{
	return primitive->indexBuffer->gl_ID;
}


GLsizei GLRenderModel::getVertCnt()
{
	return primitive->vertexCount;
}


void GLRenderModel::glGenVAO()
{
	if (primitive->vao == 0)
		glGenVertexArrays(1, &primitive->vao);
}


void GLRenderModel::glGenVBO()
{
	if (!primitive->vertexBuffer)
		primitive->vertexBuffer = std::unique_ptr<GLBuffer>(new GLBuffer());

	if (primitive->vertexBuffer->gl_ID == 0)
		glGenBuffers(1, &primitive->vertexBuffer->gl_ID);
}


void GLRenderModel::glGenVBO(GLBufferLayout& bufLayout)
{
	if (!primitive->vertexBuffer)
		primitive->vertexBuffer = std::unique_ptr<GLBuffer>(new GLBuffer(bufLayout, true));

	glGenVBO();
}


void GLRenderModel::glGenVBO(std::vector<GLBufferLayout>& _buffer_layouts)
{
	if (!primitive->vertexBuffer)
		primitive->vertexBuffer = std::unique_ptr<GLBuffer>(new GLBuffer(_buffer_layouts, true));
	assert(_buffer_layouts.size() == primitive->vertexBuffer->layouts.size());
	glGenVBO();
}


void GLRenderModel::glGenCBO()
{
	if (!primitive->colourBuffer)
		primitive->colourBuffer = std::unique_ptr<GLBuffer>(new GLBuffer());

	if (primitive->colourBuffer->gl_ID == 0)
		glGenBuffers(1, &primitive->colourBuffer->gl_ID);
}


void GLRenderModel::glGenCBO(GLBufferLayout& bufLayout)
{
	if (!primitive->colourBuffer)
		primitive->colourBuffer = std::unique_ptr<GLBuffer>(new GLBuffer(bufLayout, true));

	glGenCBO();
}


void GLRenderModel::glGenIBO(GLBufferLayout& bufLayout)
{
	if (!primitive->indexBuffer)
		primitive->indexBuffer = std::unique_ptr<GLBuffer>(new GLBuffer(bufLayout));

	if (primitive->indexBuffer->gl_ID == 0)
		glGenBuffers(1, &primitive->indexBuffer->gl_ID);

	glGenIBO();
}


void GLRenderModel::glGenIBO()
{
	if (!primitive->indexBuffer)
		primitive->indexBuffer = std::unique_ptr<GLBuffer>(new GLBuffer(false));

	if (primitive->indexBuffer->gl_ID == 0)
		glGenBuffers(1, &primitive->indexBuffer->gl_ID);
}


GLBufferLayout& GLRenderModel::getColourBufferLayout(int attrib)
{
	return primitive->colourBuffer->layouts[attrib];
}


GLBufferLayout& GLRenderModel::getVertexBufferLayout(int attrib)
{
	// TB: I think we always use attrib = 0
	return primitive->vertexBuffer->layouts[attrib];
}


void GLRenderModel::updateVertCnt(int cnt)
{
	primitive->vertexCount = cnt;
	for (int i = 0; i < primitive->vertexBuffer->layouts.size(); i++)
		primitive->vertexBuffer->layouts[i].mem.elements = cnt;
}


void GLRenderModel::preDraw(GLuint programID)
{
	if (getVAO() == 0)
	{
		LOG(WARNING) << "VertexArray not set!";
	}
	else
	{
		glBindVertexArray(getVAO());
		primitive->vertexBuffer->preDraw();
		if (primitive->use_colour_buffer)
			primitive->colourBuffer->preDraw();
	}

	// Bind primitive texture, e.g. for OpenVR controllers.
	if (primitive->tex)
		primitive->tex->preDraw(programID);
}


void GLRenderModel::postDraw()
{
	primitive->vertexBuffer->postDraw();
	if (primitive->use_colour_buffer)
		primitive->colourBuffer->postDraw();
}

#include "Lines.hpp"

#include "Core/GL/GLRenderModel.hpp"
#include "Core/Geometry/Primitive.hpp"

using namespace std;


// Uniform lines
Lines::Lines(const std::vector<float>& _vertices) :
    GLRenderable(_vertices)
{
}


// Coloured lines
Lines::Lines(const std::vector<float>& _vertices, const std::vector<float>& _colours) :
    GLRenderable(_vertices, _colours)
{
	use_colour_buffer = true;
}


void Lines::createRenderModel(const std::string& _name)
{
	int n_vertices = numberOfVertices(); // 3 float per vertex
	int n_lines = n_vertices / 2;        // come as pairs
	GLRenderModel* model = getRenderModel();
	if (!model)
		model = new GLRenderModel(_name, make_shared<Primitive>(2 * n_lines, PrimitiveType::Line));
	else
		model->updateVertCnt(2 * n_lines);

	//vertices
	GLMemoryLayout memLayout = GLMemoryLayout(n_lines * 2, 3, "", "GL_FLOAT", "", 0, NULL);
	GLBufferLayout bufLayout = GLBufferLayout(memLayout, 0);
	model->glGenVBO(bufLayout);
	model->glGenVAO();
	glBindVertexArray(model->getVAO());

	glBindBuffer(GL_ARRAY_BUFFER, model->getVertBufID());
	glEnableVertexAttribArray(0);
	model->setVertexAttrib(0);
	model->setVertexBufferData(0, vertices.data());

	//colour
	bufLayout = GLBufferLayout(memLayout, 1);
	glEnableVertexAttribArray(1);
	model->glGenCBO(bufLayout);
	glBindBuffer(GL_ARRAY_BUFFER, model->getPrimitive()->colourBuffer->gl_ID);
	model->setColourAttrib(1);
	model->setColourBufferData(0, colours.data());

	glBindVertexArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(0);

	setRenderModel(model);
}

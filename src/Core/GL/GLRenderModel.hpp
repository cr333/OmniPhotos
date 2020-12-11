#pragma once

#include "Core/GL/GLBuffer.hpp"
#include "Core/Geometry/Primitive.hpp"

#include <memory>


/*
@brief Encapsulates all geometry objects, i.e. points, lines and triangles,
       used for drawing GLPrograms.
@param primitive   Interface to create and manage GLBuffers modelling proxies.
@description At runtime, our rendering consists of displaying multiple rendered
             GLPrograms with different GLRenderModels. Rendering our OmniPhotos
             datasets comes down to rendering the visualisation of the
             CameraSetupDataset itself and the chosen method, corresponding to
             some underlying GLProgram, that provides the 5-DoF real-world VR
             experience.
*/
class GLRenderModel
{
public:
	GLRenderModel();
	GLRenderModel(const std::string& _name);
	GLRenderModel(const std::string& _name, std::shared_ptr<Primitive> _primitive);
	virtual ~GLRenderModel() = default;


	std::shared_ptr<Primitive> getPrimitive() const { return primitive; }
	void setPrimitive(std::shared_ptr<Primitive> _primitive);

	const Eigen::Matrix4f getPose();
	void setPose(const Eigen::Matrix4f& _pose);

	// Vertex array object
	void glGenVAO();

	// Vertex buffer object
	void glGenVBO();
	void glGenVBO(GLBufferLayout& bufLayout);
	void glGenVBO(std::vector<GLBufferLayout>& _buffer_layouts);

	// Index buffer object
	void glGenIBO();
	void glGenIBO(GLBufferLayout& bufLayout);

	// Colour buffer object
	void glGenCBO();
	void glGenCBO(GLBufferLayout& bufLayout);

	GLBufferLayout& getVertexBufferLayout(int attrib);
	GLBufferLayout& getColourBufferLayout(int attrib);
	void setVertexBufferData(int attrib, const void* data);
	void setColourBufferData(int attrib, const void* data);

	void setIndicesData(const std::vector<unsigned int>& indices);
	void updateVertCnt(int cnt);

	GLuint getVAO();
	GLint getVertBufID();
	GLsizei getVertCnt();
	GLint getColourBufID();
	GLint getIndBufID();

	void setVertexAttrib(int attrib);
	void setColourAttrib(int attrib);

	const std::string& getName() const { return name; }
	void setName(const std::string& _name) { name = _name; }

	void preDraw(GLuint programID);
	void postDraw();

	bool show = true;


private:
	// Essential element to draw geometry by executing a GLProgram.
	std::shared_ptr<Primitive> primitive;

	std::string name;
};

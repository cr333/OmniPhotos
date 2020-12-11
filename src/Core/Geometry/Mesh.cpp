#include "Mesh.hpp"

#include "Core/GL/GLRenderModel.hpp"
#include "Core/Geometry/Primitive.hpp"

#include "Utils/ErrorChecking.hpp"
#include "Utils/IOTools.hpp"
#include "Utils/Logger.hpp"
#include "Utils/Timer.hpp"
#include "Utils/Utils.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
//#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

using namespace Eigen;
using namespace std;


Mesh::Mesh(const vector<float>& _vertices) :
    GLRenderable(_vertices) {}


Mesh::Mesh(const vector<float>& _vertices, const vector<unsigned int>& _indices) :
    GLRenderable(_vertices, _indices)
{
	use_indices = true;
}


Mesh::Mesh(MeshData& mesh_data, const string& pchRenderModelName, float scale_factor)
{
	name = pchRenderModelName;
	stride = sizeof(VertexInterleaved) / 4;
	//vertices
	points3D.clear();
	for (uint32_t v = 0; v < mesh_data.size; v++)
	{
		VertexInterleaved vertex = mesh_data.vertex_data[v];
		Eigen::Vector3f pos = scale_factor * vertex.pos;
		Eigen::Vector3f normal = vertex.normal;
		Eigen::Vector2f tex = vertex.tex;

		Point3D* p = new Point3D(pos, normal, tex);
		points3D.push_back(p);
	}
	use_interleaved_vertex_buffer = true;
	fillStructuresFromVertex3DVector();

	//indices
	for (uint16_t i = 0; i < mesh_data.n_triangles * 3; i++)
	{
		uint16_t index = mesh_data.index_data[i];
		vertex_indices.push_back(index);
	}
	use_indices = true;

	//textures
	tex_data = mesh_data.tex_data;
	tex_width = mesh_data.tex_width;
	tex_height = mesh_data.tex_height;
	use_texture = true;
}


Mesh::~Mesh()
{
	for (int i = 0; i < points3D.size(); i++)
		delete points3D[i];
}


void Mesh::createRenderModel(const string& _name)
{
	int size = numberOfVertices();

	// standard way using vertices and (optionally) indices
	if (!use_interleaved_vertex_buffer)
	{
		GLRenderModel* model = getRenderModel();
		if (!model)
			model = new GLRenderModel(_name, make_shared<Primitive>(size, PrimitiveType::Triangle, nullptr));
		else
			model->updateVertCnt(size);

		model->glGenVAO();
		glBindVertexArray(model->getVAO());
		glEnableVertexAttribArray(0);

		GLMemoryLayout memLayout = GLMemoryLayout(size, 3, "", "GL_FLOAT", "", 0, nullptr);
		GLBufferLayout bufLayout = GLBufferLayout(memLayout, 0); //location 0
		model->glGenVBO(bufLayout);
		glBindBuffer(GL_ARRAY_BUFFER, model->getVertBufID());
		ErrorChecking::checkGLError();
		model->setVertexAttrib(0);
		ErrorChecking::checkGLError();
		model->setVertexBufferData(0, vertices.data());
		ErrorChecking::checkGLError();
		glBindVertexArray(0);
		glDisableVertexAttribArray(0);

		//Index buffer
		if (use_indices)
		{
			model->glGenIBO();
			model->setIndicesData(vertex_indices);
		}
		//in GLRenderable
		setRenderModel(model);
		ErrorChecking::checkGLError();
	}
	// textured meshes
	else
	{
		if (size > 0)
		{
			auto controller_primitive = make_shared<Primitive>(size, PrimitiveType::Triangle, true, true, true);
			GLRenderModel* model = getRenderModel();
			if (!model)
				model = new GLRenderModel(_name, controller_primitive);
			else
				model->updateVertCnt(size);

			ErrorChecking::checkGLError();
			model->glGenVAO();
			glBindVertexArray(model->getVAO());
			ErrorChecking::checkGLError();
			vector<GLBufferLayout> buffer_layouts;
			glEnableVertexAttribArray(0);
			glEnableVertexAttribArray(1);
			glEnableVertexAttribArray(2);
			//the offset structure needs to be transferred into our stuff
			GLMemoryLayout memLayout = GLMemoryLayout(size, 3, "", "GL_FLOAT", "", sizeof(VertexInterleaved), (void*)offsetof(VertexInterleaved, pos));
			GLBufferLayout bufLayout = GLBufferLayout(memLayout, 0); //location 1 -> TODO: What about vertex attrib?!
			buffer_layouts.push_back(bufLayout);
			memLayout = GLMemoryLayout(size, 3, "", "GL_FLOAT", "", sizeof(VertexInterleaved), (void*)offsetof(VertexInterleaved, normal));
			bufLayout = GLBufferLayout(memLayout, 1);
			buffer_layouts.push_back(bufLayout);
			memLayout = GLMemoryLayout(size, 2, "", "GL_FLOAT", "", sizeof(VertexInterleaved), (void*)offsetof(VertexInterleaved, tex));
			bufLayout = GLBufferLayout(memLayout, 2);
			buffer_layouts.push_back(bufLayout);
			ErrorChecking::checkGLError();
			// Populate a vertex buffer -> this is not what we want. We intialize a Vertex buffer without memory layout!
			model->glGenVBO(buffer_layouts);
			glBindBuffer(GL_ARRAY_BUFFER, model->getVertBufID());
			ErrorChecking::checkGLError();
			// Identify the components in the vertex buffer
			model->setVertexAttrib(0);
			model->setVertexAttrib(1);
			model->setVertexAttrib(2);

			glBufferData(GL_ARRAY_BUFFER, size * stride * sizeof(float), vertices.data(), GL_STATIC_DRAW);
			ErrorChecking::checkGLError();

			//Index buffer
			if (use_indices)
			{
				model->glGenIBO();
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model->getIndBufID());
				model->setIndicesData(vertex_indices);
			}
			//in GLRenderable
			ErrorChecking::checkGLError();

			//all thrown into GLRenderModel::vao
			glBindVertexArray(0);
			glDisableVertexAttribArray(0);
			glDisableVertexAttribArray(1);
			glDisableVertexAttribArray(2);

			ErrorChecking::checkGLError();
			//TODO: Should distinguish between "GLBufferMemory" and "GLTextureMemory", drop the "Layout"
			GLMemoryLayout textureMemLayout = GLMemoryLayout(1, 4, "GL_RGBA", "GL_UNSIGNED_BYTE", "GL_RGBA8");                        // these are settings from config.yaml																										   // "0" connects to input uniform in fragment shader. Maintenance should provive free texture slot.
			GLTextureLayout textureLayout = GLTextureLayout(textureMemLayout, Eigen::Vector2i(tex_width, tex_height), "diffuse", 10); //up to Texture unit 9 in use by other methods
			model->getPrimitive()->tex = make_unique<GLTexture>(textureLayout, "GL_TEXTURE_2D");

			// create and populate the texture
			glGenTextures(1, &model->getPrimitive()->tex->gl_ID);
			ErrorChecking::checkGLError();


			//Would prefer to have this in the textures of render model.
			//(Don't exist yet ^^) We basically need to add a texture to the GLProgram
			//It might make sense to define textures as a std::map in which we use the uniform name as key and the active texture unit as value
			glActiveTexture(GL_TEXTURE10);

			glBindTexture(GL_TEXTURE_2D, model->getPrimitive()->tex->gl_ID);
			ErrorChecking::checkGLError();
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex_width, tex_height,
			             0, GL_RGBA, GL_UNSIGNED_BYTE, tex_data);
			// If this renders black ask McJohn what's wrong.
			glGenerateMipmap(GL_TEXTURE_2D);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

			glBindTexture(GL_TEXTURE_2D, 0);
			ErrorChecking::checkGLError();
			setRenderModel(model);
			ErrorChecking::checkGLError();
		}
	}
}


Mesh* Mesh::load(const string& filename)
{
	string base_dir, file;
	splitFilename(filename, &base_dir, &file);
	if (base_dir.empty())
	{
		base_dir = ".";
	}
#ifdef _WIN32
	base_dir += "\\";
#else
	base_dir += "/";
#endif

	Timer obj_loader_timer;
	obj_loader_timer.startTiming();

	//dummy to satisfy the tinyobj interface
	vector<tinyobj::material_t> materials;

	//actually used
	tinyobj::attrib_t attrib;
	vector<tinyobj::shape_t> shapes;
	string warn;
	string err;
	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials,
	                            &warn, &err,
	                            filename.c_str(), base_dir.c_str());

	if (!warn.empty())
		LOG(WARNING) << warn;

	if (!err.empty())
		LOG(ERROR) << err;

	if (!ret)
	{
		LOG(WARNING) << "Failed to load '" << filename << "'.";
		return false;
	}

	if (shapes.size() > 1)
		LOG(ERROR) << "We do not support groups, just a single triangle mesh.";

	DLOG(INFO) << "Loading object file took " << obj_loader_timer.getElapsedSeconds() << "s";
	VLOG(1) << "# of vertices  = " << (int)(attrib.vertices.size()) / 3;

	//Create a Mesh for each shape
	//collect floats
	vector<float> vertices;
	for (int i = 0; i < (int)(attrib.vertices.size()); i++)
	{
		vertices.push_back(attrib.vertices[i]);
	}

	VLOG(1) << "# of shapes    = " << (int)shapes.size();

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////
	vector<unsigned int> indices;
	//Just collect indices

	//for all indices of shape[s]

	size_t meshIndicesSize = shapes[0].mesh.indices.size();
	//for each face
	for (size_t f = 0; f < meshIndicesSize / 3; f++)
	{
		tinyobj::index_t idx0 = shapes[0].mesh.indices[3 * f + 0];
		tinyobj::index_t idx1 = shapes[0].mesh.indices[3 * f + 1];
		tinyobj::index_t idx2 = shapes[0].mesh.indices[3 * f + 2];

		indices.push_back(idx0.vertex_index);
		indices.push_back(idx1.vertex_index);
		indices.push_back(idx2.vertex_index);
	}

	Mesh* m = new Mesh(vertices, indices);
	m->name = file;
	return m;
}

#include "Plane.hpp"

#include "3rdParty/fs_std.hpp"

#include "Core/GL/GLRenderModel.hpp"
#include "Core/GL/GLRenderable.hpp"
#include "Core/Geometry/Primitive.hpp"

#include "Utils/ErrorChecking.hpp"
#include "Utils/Exceptions.hpp"
#include "Utils/Logger.hpp"
#include "Utils/Utils.hpp"

using namespace std;


Plane::Plane(Eigen::Vector2i _dimensions)
{
	dimensions = _dimensions;
}


Plane::Plane(Eigen::Point3f _centre, Eigen::Vector3f _normal, Eigen::Vector2i _dimensions) :
    Plane(_dimensions)
{
	centre = _centre;
	normal = _normal;
}


//Plane::Plane(std::vector<Point3D*>& verticesPoints3D, Eigen::Vector2i dim,
//             Eigen::Vector3f c, Eigen::Vector3f n) :
//    GLRenderable(verticesPoints3D)
//{
//	dimensions = dim;
//	centre = c;
//	normal = n;
//	generateIndices();
//}
//
//
//Plane::Plane(std::vector<float>& vertices, Eigen::Vector2i dim, Eigen::Vector3f _centre, Eigen::Vector3f _normal) :
//    GLRenderable(vertices)
//{
//	centre = _centre;
//	normal = _normal;
//	dimensions = dim;
//	generateIndices();
//}


void Plane::moveAlongNormal(float distance)
{
	setCentre(getCentre() + distance * getNormal());
}


float* Plane::generateVertices()
{
	Eigen::Vector3f up = Eigen::Vector3f(0., 1., 0.);
	Eigen::Vector3f left;
	Eigen::Matrix3f rotation = createRotationTransform33(up, 90.0f);
	left = rotation * normal;

	Eigen::Vector3f right = left;

	//printf("up vector = (%f, %f, %f), right vector = (%f, %f, %f). \n",
	//	up(0), up(1), up(2), right(0), right(1), right(2));
	//6 vertices
	int dimX_2 = dimensions.x() / 2;
	int dimY_2 = dimensions.y() / 2;
	//printf("Image dimensions: (%d,%d).\n", dimensions(0), dimensions(1));

	if (dimensions(0) == 0 || dimensions(1) == 0)
		RUNTIME_EXCEPTION("Plane dimensions are zero.");

	float* vertices = new float[6 * 3];

	Eigen::Vector3f planeCorners[4];
	//upper left
	planeCorners[0] = centre - dimX_2 * right + dimY_2 * up;
	//upper right
	planeCorners[1] = centre + dimX_2 * right + dimY_2 * up;
	//lower left
	planeCorners[2] = centre - dimX_2 * right - dimY_2 * up;
	//lower right
	planeCorners[3] = centre + dimX_2 * right - dimY_2 * up;

	for (int i = 0; i < 3; i++)
	{
		//first triangle
		vertices[0 + i] = planeCorners[2][i];
		vertices[0 + 3 + i] = planeCorners[1][i];
		vertices[0 + 6 + i] = planeCorners[0][i];

		//second triangle
		vertices[0 + 9 + i] = planeCorners[2][i];
		vertices[0 + 12 + i] = planeCorners[3][i];
		vertices[0 + 15 + i] = planeCorners[1][i];
	}
	return vertices;
}

//returns 6 vertices (6*3=18 floats) corresponding to two triangles winded up CCW

//Assumes orthogonal tangent space (2D). We look perpendicularly on that space -> -normal direction

//TODO: the plane vertices needs to be adjusted according to their normal
// 1. generate vertices around origin
// 2. rotate locally according to normal
// 3. move into proper position
std::vector<float> Plane::generateVertices(Eigen::Vector3f up, Eigen::Vector3f right)
{
	//6 vertices
	int dimX_2 = dimensions.x() / 2;
	int dimY_2 = dimensions.y() / 2;

	if (dimensions(0) == 0 || dimensions(1) == 0)
		RUNTIME_EXCEPTION("Plane dimensions are zero.");

	//float* vertices = new float[6 * 3];
	std::vector<float> _vertices;
	for (int i = 0; i < 6 * 3; i++)
		_vertices.push_back(0.0f);

	Eigen::Vector3f planeCorners[4];
	//upper left
	planeCorners[0] = centre - dimX_2 * right + dimY_2 * up;
	//upper right
	planeCorners[1] = centre + dimX_2 * right + dimY_2 * up;
	//lower left
	planeCorners[2] = centre - dimX_2 * right - dimY_2 * up;
	//lower right
	planeCorners[3] = centre + dimX_2 * right - dimY_2 * up;

	for (int i = 0; i < 3; i++)
	{
		//first triangle
		_vertices[0 + i] = planeCorners[2][i];
		_vertices[0 + 3 + i] = planeCorners[1][i];
		_vertices[0 + 6 + i] = planeCorners[0][i];

		//second triangle
		_vertices[0 + 9 + i] = planeCorners[2][i];
		_vertices[0 + 12 + i] = planeCorners[3][i];
		_vertices[0 + 15 + i] = planeCorners[1][i];
	}
	return _vertices;
}


void Plane::createRenderModel(const std::string& _name)
{
	GLRenderModel* _render_model = getRenderModel();
	if (!_render_model)
		_render_model = new GLRenderModel(_name, make_shared<Primitive>(6, PrimitiveType::Triangle, nullptr));
	else
		_render_model->updateVertCnt(numberOfVertices());

	_render_model->glGenVAO();
	glBindVertexArray(_render_model->getVAO());
	glEnableVertexAttribArray(0);

	GLMemoryLayout memLayout = GLMemoryLayout(6, 3, "", "GL_FLOAT", "", 0, nullptr);
	GLBufferLayout bufLayout = GLBufferLayout(memLayout, 0); //location 0
	_render_model->glGenVBO(bufLayout);
	glBindBuffer(GL_ARRAY_BUFFER, _render_model->getVertBufID());
	ErrorChecking::checkGLError();
	_render_model->setVertexAttrib(0);
	ErrorChecking::checkGLError();
	_render_model->setVertexBufferData(0, vertices.data());
	ErrorChecking::checkGLError();
	glBindVertexArray(0);
	glDisableVertexAttribArray(0);

	setRenderModel(_render_model);
}


////special since it updates whenever the GLCamera moves.
////TODO: This could be replaced with an update to a "model transformation matrix".
//void Plane::updateRenderModel(Eigen::Vector3f up, Eigen::Vector3f right)
//{
//	GLRenderModel* planeRenderModel = getRenderModel();
//	ErrorChecking::checkGLError();
//	planeRenderModel->glGenVAO();
//	glBindVertexArray(planeRenderModel->getVAO());
//	ErrorChecking::checkGLError();
//
//	GLMemoryLayout memLayout = GLMemoryLayout(6, 3, "", "GL_FLOAT", "", 0, nullptr);
//	GLBufferLayout bufLayout = GLBufferLayout(memLayout, 0); //location 0
//	planeRenderModel->glGenVBO(bufLayout);
//
//	glBindBuffer(GL_ARRAY_BUFFER, planeRenderModel->getVertBufID());
//	ErrorChecking::checkGLError();
//	glEnableVertexAttribArray(0);
//	planeRenderModel->setVertexAttrib(0);
//	ErrorChecking::checkGLError();
//
//	std::vector<float> planeVertices = generateVertices(up, right);
//	planeRenderModel->setVertexBufferData(0, planeVertices.data());
//	glBindVertexArray(0);
//	glDisableVertexAttribArray(0);
//}


std::vector<float> Plane::generateSubMeterVertices(Eigen::Vector2f _dimensions)
{
	Eigen::Vector3f up = Eigen::Vector3f(0., 1., 0.);
	Eigen::Vector3f left;
	Eigen::Matrix3f rotation = createRotationTransform33(up, 90.0f);
	left = rotation * normal;

	Eigen::Vector3f right = left;

	//printf("up vector = (%f, %f, %f), right vector = (%f, %f, %f). \n",
	//	up(0), up(1), up(2), right(0), right(1), right(2));
	//6 vertices
	float dimX_2 = _dimensions.x() / 2;
	float dimY_2 = _dimensions.y() / 2;
	//printf("Image dimensions: (%d,%d).\n", dimensions(0), dimensions(1));

	if (_dimensions(0) == 0.0f || _dimensions(1) == 0.0f)
		RUNTIME_EXCEPTION("Plane dimensions are zero.");

	std::vector<float> _vertices;
	for (int i = 0; i < 6 * 3; i++)
		_vertices.push_back(0.0f);
	//float* vertices = new float[6 * 3];

	Eigen::Vector3f planeCorners[4];
	//upper left
	planeCorners[0] = centre - dimX_2 * right + dimY_2 * up;
	//upper right
	planeCorners[1] = centre + dimX_2 * right + dimY_2 * up;
	//lower left
	planeCorners[2] = centre - dimX_2 * right - dimY_2 * up;
	//lower right
	planeCorners[3] = centre + dimX_2 * right - dimY_2 * up;

	for (int i = 0; i < 3; i++)
	{
		//first triangle
		_vertices[0 + i] = planeCorners[2][i];
		_vertices[0 + 3 + i] = planeCorners[1][i];
		_vertices[0 + 6 + i] = planeCorners[0][i];
		//second triangle
		_vertices[0 + 9 + i] = planeCorners[2][i];
		_vertices[0 + 12 + i] = planeCorners[3][i];
		_vertices[0 + 15 + i] = planeCorners[1][i];
	}
	return _vertices;
}


Eigen::Vector3f Plane::getNormal()
{
	return normal;
}


void Plane::setNormal(Eigen::Vector3f _normal)
{
	normal = _normal;
}


Eigen::Vector2i Plane::getDimensions()
{
	return dimensions;
}


void Plane::setDimensions(Eigen::Vector2i _dims)
{
	dimensions = _dims;
}


void Plane::generateIndices()
{
	for (int y = 0; y < dimensions.y() - 1; y++)
	{
		for (int x = 0; x < dimensions.x() - 1; x++)
		{
			// Triangle indices.
			// Vertex origin is top left not facing the normal, like looking through it.
			// TODO: This needs to be tested!!! -> color coding

			// 1st triangle
			unsigned int idx0 = (y)*dimensions.x() + (x);
			unsigned int idx1 = (y + 1) * dimensions.x() + (x);
			unsigned int idx2 = (y)*dimensions.x() + (x + 1);
			vertex_indices.push_back(idx0);
			vertex_indices.push_back(idx1);
			vertex_indices.push_back(idx2);

			// 2nd triangle
			idx0 = (y + 1) * dimensions.x() + (x);
			idx1 = (y + 1) * dimensions.x() + (x + 1);
			idx2 = (y)*dimensions.x() + (x + 1);
			vertex_indices.push_back(idx0);
			vertex_indices.push_back(idx1);
			vertex_indices.push_back(idx2);
		}
	}

	use_indices = true;
}

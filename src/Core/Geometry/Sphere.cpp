#include "Sphere.hpp"

#include "Core/GL/GLRenderModel.hpp"
#include "Core/Geometry/Primitive.hpp"

#include "Utils/ErrorChecking.hpp"
#include "Utils/Logger.hpp"
#include "Utils/Utils.hpp"


using namespace Eigen;
using namespace std;


Sphere::Sphere(int _phiRes, int _thetaRes, float _radius, bool flipNormals) :
    phiRes(_phiRes), thetaRes(_thetaRes), radius(_radius)
{
	centre = { 0, 0, 0 };
}


void Sphere::createRenderModel(const std::string& _name)
{
	generateVertices();
	int size = numberOfVertices();

	GLRenderModel* model = getRenderModel();
	if (!model)
		model = new GLRenderModel(_name, make_shared<Primitive>(size, PrimitiveType::TriangleStrip, nullptr));
	else
		model->updateVertCnt(size);

	model->glGenVAO();
	glBindVertexArray(model->getVAO());
	glEnableVertexAttribArray(0);
	GLMemoryLayout memLayout = GLMemoryLayout(size, 3, "", "GL_FLOAT", "", 0, nullptr);
	GLBufferLayout bufLayout = GLBufferLayout(memLayout, 0); //location 0
	model->glGenVBO(bufLayout);
	glBindBuffer(GL_ARRAY_BUFFER, model->getVertBufID());
	model->setVertexAttrib(0);
	model->setVertexBufferData(0, vertices.data());
	glBindVertexArray(0);
	glDisableVertexAttribArray(0);

	setRenderModel(model);
}


void Sphere::generateVertices()
{
	vertices.clear();
	const float phiStep = 360.0f / phiRes;
	const float thetaStep = 180.0f / thetaRes;

	// Create a triangle strip for each polar angle isocontour.
	for (int thetaIndex = 0; thetaIndex < thetaRes; thetaIndex++)
	{
		const float thetaTop_rad = degToRad(thetaIndex * thetaStep);
		const float thetaBot_rad = degToRad((thetaIndex + 1) * thetaStep);

		for (int phiIndex = 0; phiIndex <= phiRes; phiIndex++)
		{
			const float phi_rad = degToRad(phiIndex * phiStep);

			// y up, x and z ground plane. Exact directions don't matter as sphere is completely symmetric.
			Eigen::Vector3f vertexTop(sinf(thetaTop_rad) * sinf(phi_rad), cosf(thetaTop_rad), sinf(thetaTop_rad) * cosf(phi_rad));
			Eigen::Vector3f vertexBot(sinf(thetaBot_rad) * sinf(phi_rad), cosf(thetaBot_rad), sinf(thetaBot_rad) * cosf(phi_rad));

			vertexTop = radius * vertexTop + centre;
			vertexBot = radius * vertexBot + centre;

			for (int j = 0; j < 3; j++)
				vertices.push_back(vertexTop(j));

			for (int j = 0; j < 3; j++)
				vertices.push_back(vertexBot(j));
		}
	}
	fillVertex3DVectorFromVertices();
	return;
}

#include "PointCloud.hpp"

#include "3rdParty/fs_std.hpp"

#include "Core/GL/GLRenderModel.hpp"
#include "Core/GL/GLRenderable.hpp"
#include "Core/Geometry/Primitive.hpp"

#include "Utils/ErrorChecking.hpp"
#include "Utils/Exceptions.hpp"
#include "Utils/Logger.hpp"
#include "Utils/STLutils.hpp"
#include "Utils/Utils.hpp"

#include <fstream>

using namespace Eigen;
using namespace std;


PointCloud::PointCloud(std::vector<Point3D*> _vertices)
{
	points3D = _vertices;
	fillStructuresFromVertex3DVector();
}


PointCloud::PointCloud(std::vector<Point3D*>* _points, bool _use_colour_buffer)
{
	for (int i = 0; i < _points->size(); i++)
	{
		Point3D* p = _points->at(i);
		points3D.push_back(p);
		vertices.push_back(p->pos.x());
		vertices.push_back(p->pos.y());
		vertices.push_back(p->pos.z());
		colours.push_back(p->colour.x());
		colours.push_back(p->colour.y());
		colours.push_back(p->colour.z());
	}
	computeCentreAndBox();
	use_colour_buffer = _use_colour_buffer;
}


void PointCloud::createRenderModel(const std::string& _name)
{
	int n_vertices = numberOfVertices();

	//Note: We want to update render models instead of deleting and re-creating them.
	GLRenderModel* model = getRenderModel();
	if (!model)
		model = new GLRenderModel(_name, make_shared<Primitive>(n_vertices, PrimitiveType::Point));
	else
		model->updateVertCnt(n_vertices);

	model->glGenVAO();
	glBindVertexArray(model->getVAO());

	GLMemoryLayout memory_layout = GLMemoryLayout(n_vertices, 3, "", "GL_FLOAT", "", 0, nullptr);
	GLBufferLayout buffer_layout = GLBufferLayout(memory_layout, 0);
	model->glGenVBO(buffer_layout);
	glBindBuffer(GL_ARRAY_BUFFER, model->getVertBufID());
	glEnableVertexAttribArray(0);
	model->setVertexAttrib(0);
	model->setVertexBufferData(0, this->vertices.data());

	//colour
	if (use_colour_buffer)
	{
		buffer_layout = GLBufferLayout(memory_layout, 1);
		glEnableVertexAttribArray(1);
		model->glGenCBO(buffer_layout);
		glBindBuffer(GL_ARRAY_BUFFER, model->getColourBufID());
		model->setColourAttrib(1);
		model->setColourBufferData(0, colours.data());
	}
	glBindVertexArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(0);

	setRenderModel(model);
}


PointCloud::PointCloud(std::string file)
{
	string fileExtension = extractFileExtension(file);

	int mode = -1;
	if (contains(fileExtension, "obj"))
	{
		mode = 0;
		LOG(INFO) << "Loading pointCloud from obj file.";
	}
	if (contains(fileExtension, "csv"))
	{
		mode = 1;
		LOG(INFO) << "Loading pointCloud from csv file.";
	}

	if (mode == -1)
		RUNTIME_EXCEPTION("file format not supported!");

	std::vector<Point3D*> _verticesPoints3D;
	string cloudFilename = file;
	// Check if the file can be read.
	std::ifstream ifsPointCloud(cloudFilename);
	if (!ifsPointCloud.is_open())
		RUNTIME_EXCEPTION(string("PointCloud::PointCloud: Could not open file '") + cloudFilename + "'.");

	// Read and parse the file line by line.
	string line;
	int count = 0;
	std::vector<string> tokens;

	Point3D* point;
	while (getline(ifsPointCloud, line))
	{
		// Skip comments.
		if (line[0] == '#')
			continue;

		// Parse camera line.
		if (mode == 1)
		{
			tokens = split(line, ',');
			point = new Point3D(stoi(tokens[0]),
			                    Eigen::Vector3f(stof(tokens[1]), stof(tokens[2]), stof(tokens[3])),
			                    Eigen::Vector3f(stof(tokens[4]), stof(tokens[5]), stof(tokens[6])),
			                    stof(tokens[7]));
		}
		else
		{
			if (mode == 0)
			{
				tokens = split(line, ' ');
				//tokens[0] = 0
				point = new Point3D(Eigen::Vector3f(stof(tokens[1]), stof(tokens[2]), stof(tokens[3])),
				                    Eigen::Vector3f(stof(tokens[4]), stof(tokens[5]), stof(tokens[6])));
			}
		}

		_verticesPoints3D.push_back(point);
		count++;
	}
	ifsPointCloud.close();

	for (int i = 0; i < _verticesPoints3D.size(); i++)
	{
		Point3D* p = _verticesPoints3D[i];
		points3D.push_back(p);
		vertices.push_back(p->pos.x());
		vertices.push_back(p->pos.y());
		vertices.push_back(p->pos.z());
		colours.push_back(p->colour.x());
		colours.push_back(p->colour.y());
		colours.push_back(p->colour.z());
	}
}


Eigen::Point3f PointCloud::getCentroid()
{
	Eigen::Point3f centroid = Eigen::Point3f(0.0f, 0.0f, 0.0f);
	for (int i = 0; i < numberOfVertices(); i++)
	{
		centroid += points3D.at(i)->pos;
	}
	centroid /= (float)numberOfVertices();
	return centroid;
}


void PointCloud::setPoints(const std::vector<Point3D*>& _points)
{
	//points = _points;
	points3D = _points;
}


std::vector<Point3D*>* PointCloud::getPoints()
{
	return &points3D;
}


void PointCloud::computeCentreAndBox()
{
	Eigen::Vector3f centroid;
	centroid.setZero();

	float minX, minY, minZ, maxX, maxY, maxZ;
	minX = minY = minZ = 1000;
	maxX = maxY = maxZ = 0;

	for (int i = 0; i < points3D.size(); i++)
	{
		Eigen::Vector3f p = points3D.at(i)->pos;
		centroid = centroid + p;
		// clang-format off
		if (p.x() < minX) { minX = p.x(); }
		if (p.x() > maxX) { maxX = p.x(); }
		if (p.y() < minY) { minY = p.y(); }
		if (p.y() > maxY) { maxY = p.y(); }
		if (p.z() < minZ) { minZ = p.z(); }
		if (p.z() > maxZ) { maxZ = p.z(); }
		// clang-format on
	}
	centre = centroid / (float)points3D.size();
}


void PointCloud::merge(PointCloud& otherCloud)
{
	std::vector<Point3D*> otherVertices = otherCloud.points3D;
	for (int i = 0; i < otherVertices.size(); i++)
	{
		points3D.push_back(otherVertices[i]);
	}
}


void PointCloud::writeToObj(std::string filename)
{
	if (fs::exists(filename))
		std::remove(filename.c_str());

	const int N = numberOfVertices();
	std::ofstream cloudFile(filename.c_str(), std::fstream::out);

	//obj file format: http://paulbourke.net/dataformats/obj/
	// v x y z , blank as delimiter
	for (int i = 0; i < N; i++)
	{
		const Point3D* p = points3D.at(i);
		cloudFile << "v " << p->pos.x() << " " << p->pos.y() << " " << p->pos.z();
		if (p->colour.x() > -1.0f)
			cloudFile << " " << p->colour.x() << " " << p->colour.y() << " " << p->colour.z();
		cloudFile << "\n";
	}

	cloudFile.close();
}

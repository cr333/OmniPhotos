#include "CameraSetupVisualization.hpp"

#include "Core/CameraSetup/CameraSetupDataset.hpp"
#include "Core/GL/GLDefaultPrograms.hpp"
#include "Core/Geometry/Lines.hpp"

#include "Utils/Utils.hpp"


using namespace Eigen;
using namespace std;


CameraSetupVisualization::CameraSetupVisualization(CameraSetupDataset* _dataset, DisplaySettings* _settings)
{
	dataset = _dataset;
	visSettings = _settings;
	initCoordinateAxes();
}


CameraSetupVisualization::~CameraSetupVisualization()
{
	delete coordinate_axes_prog;
	delete camera_geometry_points_prog;
	delete camera_geometry_lines_prog;
	delete camera_orientations_model;
	delete camera_positions_model;
	delete coordinate_axes_model;
	delete point_cloud_model;
	delete circle_prog;
	delete circle_model;
	delete cylinder_model;
	delete sphere_model;
	delete radial_directions_model;
	delete radial_directions_prog;
}


void CameraSetupVisualization::setDataset(CameraSetupDataset* _dataset)
{
	dataset = _dataset;
	updateModels();
}


void CameraSetupVisualization::setSettings(DisplaySettings* _settings)
{
	this->visSettings = _settings;
}


void CameraSetupVisualization::initCoordinateAxes()
{
	int size = 6; //6 vertices -> 18 floats
	std::vector<float> vertices;
	vertices.reserve(size * 3);
	std::vector<float> colours;
	colours.reserve(size * 3);
	for (int n = 0; n < size * 3; n++)
	{
		vertices.push_back(0.0f);
		colours.push_back(1.0f);
	}

	//vertices
	const GLfloat coordinateAxesPoints[] = {
		0.0, 0.0, 0.0,   100.0, 0.0, 0.0, // x
		0.0, 0.0, 0.0,   0.0, 100.0, 0.0, // y
		0.0, 0.0, 0.0,   0.0, 0.0, 100.0  // z
	};
	std::memcpy(vertices.data(), coordinateAxesPoints, sizeof(float) * size * 3);

	//colours
	for (int i = 0; i < 3; i++)
	{
		int j = i * 6;
		// left, up, view
		for (int k = 0; k < 3; k++)
		{
			colours[j + k] = static_colors[i][k];
			colours[j + k + 3] = static_colors[i][k];
		}
	}

	//Create GLRenderable Shape
	std::shared_ptr<Lines> coordinate_axes = std::make_shared<Lines>(vertices, colours);
	coordinate_axes->createRenderModel("Coordinate axes");
	coordinate_axes_model = coordinate_axes->getRenderModel();
}


//Technically updates all render models using the shapes stored in the dataset which is referenced by the visualization.
void CameraSetupVisualization::updateModels()
{
	updateCircleModel(visSettings->circle_resolution);
	updateCameraPositionsModel();
	updateCameraOrientationsModel();
	updateCylinderModel(visSettings->circle_resolution);
	updateSphereModel();
	updateRadialLinesModel();
	updateWorldPointCloudModel();
}


void CameraSetupVisualization::updateWorldPointCloudModel()
{
	auto pc = dataset->getWorldPointCloud();
	if (pc)
	{
		//TODO: One could do this directly in the constructor whenever the point cloud is intialized?
		pc->use_colour_buffer = true;

		//I try to circumvent that we consider "all" Shapes as GLRenderable.
		//We might just want to do maths etc with these entities without actually rendering them.
		pc->createRenderModel("Scene point cloud");

		//TODO: could be set in createRenderModel(), could be a setting of the point cloud.
		pc->getRenderModel()->getPrimitive()->pointSize = 5;

		point_cloud_model = pc->getRenderModel();
		pointCloudLoaded = true;
		VLOG(1) << "Loading point cloud done.";
	}
}


void CameraSetupVisualization::updateCameraPositionsModel()
{
	//get optical centres from the camera setup
	//Note: could do "dataset->getOpticalCentres()" instead
	std::vector<Eigen::Vector3f> camera_centres = dataset->getCameraSetup()->getOpticalCentres();

	//TODO: Needed to creat point cloud. Could be a default return of the CameraSetup method. Currently including "Shape" in Utils to do the conversion. -> :(
	//Could change the interface of the point cloud as well -> discussion
	std::vector<Point3D*> camera_centres_vertices;
	for (const Eigen::Vector3f& v : camera_centres)
		camera_centres_vertices.push_back(new Point3D(v.x(), v.y(), v.z()));

	//the goal is to update the cameraPositions rendermodel
	//PointCloud* pc = new PointCloud(camera_centres_vertices);
	std::shared_ptr<PointCloud> pc = std::make_shared<PointCloud>(camera_centres_vertices);

	//TODO: One could do this directly in the constructor.
	//I try to circumvent that we consider "all" Shapes as GLRenderable. We might just want to do maths etc with these entities without actually rendering them.
	pc->createRenderModel("Optical centres");
	camera_positions_model = pc->getRenderModel();
	//delete pc;
}


void CameraSetupVisualization::updateCameraOrientationsModel()
{
	// We need the optical centres and the rotation matrices of each camera to fill this method with life.
	int numCameras = dataset->getCameraSetup()->getNumberOfCameras();
	int size = 3 * 6 * numCameras;
	std::vector<float> vertices;
	std::vector<float> colours;
	vertices.reserve(size);
	colours.reserve(size);

	// Fill it with placeholder data.
	for (int n = 0; n < size; n++)
	{
		vertices.push_back(0.0f);
		colours.push_back(1.0f);
	}

	// Creating vertices and per-vertex colours
	int i = 0;
	for (int j = 0; j < numCameras; j++)
	{
		const Camera* cam = dataset->getCameraSetup()->getCameras()->at(j);
		const Vector3f camPos = cam->getCentre(); // in world space

		// The camera centre is the starting point of the x-axis line.
		for (int k = 0; k < 3; k++)
		{
			vertices[i + k] = camPos(k);
			colours[i + k] = static_colors[0][k];
		}
		// x-axis end (left? right?)
		Vector3f axis = camPos + 10 * cam->getXDir();
		for (int k = 0; k < 3; k++)
		{
			vertices[i + k + 3] = axis(k);
			colours[i + k + 3] = static_colors[0][k];
		}
		i += 6;

		// The camera centre is the starting point of the y-axis line.
		for (int k = 0; k < 3; k++)
		{
			vertices[i + k] = camPos(k);
			colours[i + k] = static_colors[1][k];
		}
		// y-axis (up?)
		axis = camPos + 10 * cam->getYDir();
		for (int k = 0; k < 3; k++)
		{
			vertices[i + k + 3] = axis(k);
			colours[i + k + 3] = static_colors[1][k];
		}
		i += 6;

		// The camera centre is the starting point of the z-axis line.
		for (int k = 0; k < 3; k++)
		{
			vertices[i + k] = camPos(k);
			colours[i + k] = static_colors[2][k];
		}
		// z-axis (view?)
		axis = camPos + 10 * cam->getZDir();
		for (int k = 0; k < 3; k++)
		{
			vertices[i + k + 3] = axis(k);
			colours[i + k + 3] = static_colors[2][k];
		}
		i += 6;
	}

	// Create GLRenderable shape.
	Lines* camera_orientations = new Lines(vertices, colours);

	// Create and register GLRenderModel to Visualization.
	camera_orientations->createRenderModel("Camera orientations");
	camera_orientations_model = camera_orientations->getRenderModel();
}


void CameraSetupVisualization::updateCircleModel(int circleResolution)
{
	std::vector<float> vertices;
	//TODO:rename? vertices are for lines
	dataset->circle->createVertices(&vertices, circleResolution);
	std::shared_ptr<Lines> circle = std::make_shared<Lines>(vertices);
	circle->createRenderModel("Camera circle");
	circle_model = circle->getRenderModel();
}


void CameraSetupVisualization::updateSphereModel()
{
	dataset->sphere->createRenderModel("Sphere model");
	sphere_model = dataset->sphere->getRenderModel();
}


void CameraSetupVisualization::updateCylinderModel(int circle_resolution)
{
	dataset->cylinder->createRenderModel("Cylinder model");
	cylinder_model = dataset->cylinder->getRenderModel();
}


void CameraSetupVisualization::updateRadialLinesModel()
{
	//setup radial directions. From the origin to the optical centres
	int numberRadialLines = dataset->getCameraSetup()->getNumberOfCameras();
	int size = 3 * 2 * numberRadialLines;

	std::vector<float> vertices;
	vertices.reserve(size);
	for (int n = 0; n < size; n++)
		vertices.push_back(0.0f);

	Eigen::Point3f pos = dataset->getCameraSetup()->getCentroid();
	Eigen::Vector3f normal = dataset->circle->getNormal();
	std::vector<Camera*>* _cams = dataset->getCameraSetup()->getCameras();
	for (int i = 0; i < numberRadialLines; i++)
	{
		pos = _cams->at(i)->getCentre();
		pos = pos - normal.dot(pos.normalized()) * normal;
		pos = 2.0 * pos;
		vertices[3 * 2 * i + 0] = 0.0;
		vertices[3 * 2 * i + 1] = 0.0;
		vertices[3 * 2 * i + 2] = 0.0;

		vertices[3 * 2 * i + 3] = pos(0);
		vertices[3 * 2 * i + 4] = pos(1);
		vertices[3 * 2 * i + 5] = pos(2);
	}

	//make shared poiner -> automatically cleans up
	Lines* radial_directions = new Lines(vertices);
	radial_directions->createRenderModel("Radial directions");
	radial_directions_model = radial_directions->getRenderModel();
	delete radial_directions;
}


void CameraSetupVisualization::initPrograms(GLWindow* gl_window, std::vector<GLProgram*>* _programs)
{
	const string projectSrcDir = determinePathToSource();
	coordinate_axes_prog = new GLDefaultProgram(coordinate_axes_model);
	coordinate_axes_prog->setVertexShaderFilename(projectSrcDir + "Shaders/General/PassColour.vertex.glsl");
	coordinate_axes_prog->setFragmentShaderFilename(projectSrcDir + "Shaders/General/PassedColour.fragment.glsl");
	coordinate_axes_prog->job = "World coordinate axes";
	_programs->push_back(coordinate_axes_prog);

	if (visSettings->load3DPoints)
	{
		world_points_prog = new GLDefaultProgram(point_cloud_model);
		world_points_prog->setVertexShaderFilename(projectSrcDir + "Shaders/General/PassColour.vertex.glsl"); //uniformly coloured point clouds
		world_points_prog->setFragmentShaderFilename(projectSrcDir + "Shaders/General/PassedColour.fragment.glsl");
		world_points_prog->job = "Visualize reconstructed world positions";
		_programs->push_back(world_points_prog);
	}

	//TODO: set pointSize via model to save accessing Primitive?
	camera_positions_model->getPrimitive()->pointSize = 10;
	camera_geometry_points_prog = new GLPointsProgram(camera_positions_model);
	camera_geometry_points_prog->setVertexShaderFilename(projectSrcDir + "Shaders/General/Default.vertex.glsl");
	camera_geometry_points_prog->setFragmentShaderFilename(projectSrcDir + "Shaders/General/ColourByPrimitiveID.fragment.glsl");
	camera_geometry_points_prog->job = "Optical centres of the cameras";
	_programs->push_back(camera_geometry_points_prog);

	camera_geometry_lines_prog = new GLDefaultProgram(camera_orientations_model);
	camera_geometry_lines_prog->setVertexShaderFilename(projectSrcDir + "Shaders/General/PassColour.vertex.glsl");
	camera_geometry_lines_prog->setFragmentShaderFilename(projectSrcDir + "Shaders/General/PassedColour.fragment.glsl");
	camera_geometry_lines_prog->job = "Orientations of the cameras";
	_programs->push_back(camera_geometry_lines_prog);

	circle_prog = new GLDrawUniformLinesProgram(circle_model);
	circle_prog->setVertexShaderFilename(projectSrcDir + "Shaders/General/Default.vertex.glsl");
	circle_prog->setFragmentShaderFilename(projectSrcDir + "Shaders/General/UniformColour.fragment.glsl");
	circle_prog->job = "Camera circle";
	circle_prog->color = Eigen::Vector3f(1, 0, 1); // magenta
	circle_prog->display = false;
	_programs->push_back(circle_prog);

	radial_directions_prog = new GLDrawUniformLinesProgram(radial_directions_model);
	radial_directions_prog->setVertexShaderFilename(projectSrcDir + "Shaders/General/Default.vertex.glsl");
	radial_directions_prog->setFragmentShaderFilename(projectSrcDir + "Shaders/General/UniformColour.fragment.glsl");
	radial_directions_prog->job = "Radial directions through the camera centres";
	radial_directions_prog->display = false;
	_programs->push_back(radial_directions_prog);
}


void CameraSetupVisualization::updateProgramDisplay()
{
	camera_geometry_lines_prog->display = visSettings->showCameraGeometry;
	camera_geometry_points_prog->display = visSettings->showCameraGeometry;
	coordinate_axes_prog->display = visSettings->showCoordinateAxes;

	if (visSettings->load3DPoints)
		world_points_prog->display = visSettings->showWorldPoints;
}

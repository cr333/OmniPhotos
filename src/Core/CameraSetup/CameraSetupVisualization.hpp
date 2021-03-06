#pragma once

#include "Core/DisplaySettings.hpp"
#include "Core/Geometry/PointCloud.hpp"

#include <memory>
#include <vector>

class CameraSetup;
class CameraSetupDataset;
class CameraSetupSettings;
class GLVisualizationRenderModel;
class GLProgram;
class GLPointsProgram;
class GLDefaultProgram;
class GLDrawUniformLinesProgram;
class GLWindow;

struct Point3D;
class Lines;


/**
 * Visualizes a CameraSetup in the Viewer.
 */
class CameraSetupVisualization
{
public:
	CameraSetupVisualization(CameraSetupDataset* _dataset, DisplaySettings* _settings);
	virtual ~CameraSetupVisualization();

	void initPrograms(GLWindow* gl_window, std::vector<GLProgram*>* _programs);
	
	void setDataset(CameraSetupDataset* _dataset);
	void setSettings(DisplaySettings* _settings);
	
	void updateProgramDisplay();

	void updateModels();
	void updateCylinderModel(int circle_resolution);
	void updateSphereModel();


	GLDrawUniformLinesProgram* circle_prog = nullptr;
	GLDrawUniformLinesProgram* radial_directions_prog = nullptr;

	// Render model for the camera circle.
	GLRenderModel* circle_model = nullptr;

	// Default proxies
	GLRenderModel* cylinder_model = nullptr;
	GLRenderModel* sphere_model = nullptr;

	GLRenderModel* radial_directions_model = nullptr;

private:
	// The Visualization is always only "reading" dataset info
	CameraSetupDataset* dataset;
	const CameraSetupSettings* settings;
	DisplaySettings* visSettings = nullptr;

	// static for each visualization
	void initCoordinateAxes();

	// updates when we switch datasets
	void updateCircleModel(int circleResolution);
	bool pointCloudLoaded = false;
	void updateWorldPointCloudModel();
	void updateCameraPositionsModel();
	void updateCameraOrientationsModel();
	void updateRadialLinesModel();

	GLDefaultProgram* coordinate_axes_prog = nullptr;
	GLDefaultProgram* camera_geometry_lines_prog = nullptr;

	// coloured points
	// TODO: could be called "scene_points_prog"?
	GLDefaultProgram* world_points_prog = nullptr;
	// passUniform is adjusted to colour vertices according to their id.
	GLPointsProgram* camera_geometry_points_prog = nullptr;

	GLRenderModel* camera_positions_model = nullptr;
	GLRenderModel* camera_orientations_model = nullptr;
	GLRenderModel* coordinate_axes_model = nullptr;
	GLRenderModel* point_cloud_model = nullptr;
};

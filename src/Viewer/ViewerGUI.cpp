#include "ViewerGUI.hpp"

#include "3rdParty/fs_std.hpp"

#include "Utils/Logger.hpp"
#include "Utils/Utils.hpp"

#include "Viewer/ImGuiUtils.hpp"
#include "Viewer/ViewerApp.hpp"


ViewerGUI::ViewerGUI(ViewerApp* _app) :
    app(_app)
{
	// Hide the datasets window by default if only one dataset was found/loaded.
	showDatasetWindow = app->numberOfDatasets() > 1;
}


ViewerGUI::~ViewerGUI()
{
	// Release the dataset thumbnail textures.
	for (int idx = 0; idx < datasetThumbList.size(); idx++)
	{
		const GLuint text_id = static_cast<GLint>(reinterpret_cast<intptr_t>(datasetThumbList[idx]));
		glDeleteTextures(1, &text_id);
	}

	// Destroy the imgui context.
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}


void ViewerGUI::init()
{
	window = app->getGLwindow()->getGLFWwindow();

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();

	app->ignoreMouseInput = &io.WantCaptureMouse;
	app->ignoreKeyboardInput = &io.WantCaptureKeyboard;
	app->showImGui = true;

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 130");
}


void ViewerGUI::draw()
{
	// Start the Dear ImGui frame
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	// Shows debug window
	showSettingsWindow();

	//// Demo window
	//ImGui::ShowDemoWindow();

	// Render imgui
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}


void ViewerGUI::showSettingsWindow()
{
	ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(300, 400), ImGuiCond_FirstUseEver);
	ImGui::Begin("OmniPhotos Viewer");

	//-------------------------------------------------------------------------
	if (ImGui::CollapsingHeader("Rendering Settings", ImGuiTreeNodeFlags_DefaultOpen))
	{
		showRenderingSettings();
	}

	//-------------------------------------------------------------------------
	if (ImGui::CollapsingHeader("Visualisation Tools"))
	{
		showVisualisationTools();
	}

	//-------------------------------------------------------------------------
	if (ImGui::CollapsingHeader("Camera Path Animation"))
	{
		showCameraPathAnimation();
	}

	//-------------------------------------------------------------------------
	if (ImGui::CollapsingHeader("Information", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Checkbox("Show datasets window", &showDatasetWindow);

		ImGui::BulletText("Dataset: %s", app->getDataset()->name.c_str());
		ImGui::BulletText("Cameras: %d", app->settings->numberOfCameras);

		const auto cacheDir = fs::path(app->getDataset()->pathToCacheFolder).filename().string();
		ImGui::BulletText("Cache directory: %s", cacheDir.c_str());

		// Framerate estimate by Dear ImGUI
		ImGui::BulletText("Rendering speed: %.2f ms", 1000.f / ImGui::GetIO().Framerate);
	}

	ImGui::End();

	// Show the dataset window
	if (showDatasetWindow)
		showDatasets();
}


void ViewerGUI::showCameraPathAnimation()
{
	static bool refresh = true;
	static std::vector<std::string> path_files;
	static std::vector<std::string> path_files_display_names;
	static int selected_file = 0;

	// Sanity check.
	if (!app)
		return;
	GLCamera* gl_cam = app->getGLCamera();
	if (!gl_cam)
		return;
	if (!gl_cam->cameraControl)
		return;

	// Refresh file list.
	if (refresh)
	{
		path_files.clear();
		path_files_display_names.clear();

		// Add the first items for the camera swing motions.
		path_files.push_back("");
		path_files_display_names.push_back("Circle swing");
		path_files.push_back("");
		path_files_display_names.push_back("Translation swing");

		// Find all CSV files in /src/Paths/.
		std::error_code ec;
		auto src_path = fs::path(determinePathToSource()) / "Paths";
		if (fs::exists(src_path, ec))
		{
			for (const auto& entry : fs::directory_iterator(src_path))
			{
				// Skip if not a file.
				if (entry.is_regular_file() == false) continue;

				// Skip non-CSVs.
				if (entry.path().extension() != ".csv") continue;

				// Full path.
				path_files.push_back(entry.path().generic_string());

				// Just filename.
				path_files_display_names.push_back("src/" + entry.path().filename().generic_string());
			}
		}

		// Find all CSV files in the config directory.
		for (const auto& entry : fs::directory_iterator(app->getDataset()->pathToConfigFolder))
		{
			// Skip if not a file.
			if (entry.is_regular_file() == false) continue;

			// Skip non-CSVs.
			if (entry.path().extension() != ".csv") continue;

			path_files.push_back(entry.path().generic_string());                          // full path
			path_files_display_names.push_back(entry.path().filename().generic_string()); // just filename
		}

		refresh = false;
	}

	// List camera paths in a dropdown
	if (ImGui::Combo("Camera path", &selected_file, path_files_display_names))
	{
		// Stop any ongoing animation.
		gl_cam->record = false;
		std::vector<std::shared_ptr<Command>> commands;

		if (selected_file == 0) // circle swing
		{
			auto swing = std::make_shared<CircleSwingCommand>();
			swing->dataset_name = app->getDataset()->name;
			swing->radius = app->getDataset()->circle->getRadius() / 2;
			swing->basis = app->getDataset()->circle->getBase();
			float look_dist = app->settings->lookAtDistance;
			float dir_rad = degToRad(app->settings->lookAtDirection);
			swing->lookAt = look_dist * Eigen::Vector3f(sinf(dir_rad), 0.f, -cosf(dir_rad));
			commands.push_back(swing);
		}
		else if (selected_file == 1) // translate swing
		{
			auto swing = std::make_shared<TranslateSwingCommand>();
			swing->dataset_name = app->getDataset()->name;
			swing->radius = app->getDataset()->circle->getRadius() / 2;
			swing->basis = app->getDataset()->circle->getBase();
			float look_dist = app->settings->lookAtDistance;
			float dir_rad = degToRad(app->settings->lookAtDirection);
			swing->angle = degToRad(app->settings->lookAtDirection + 90);
			swing->lookAt = look_dist * Eigen::Vector3f(sinf(dir_rad), 0.f, -cosf(dir_rad));
			commands.push_back(swing);
		}
		else // camera path CSV
		{
			// Replace camera control commands with the selected file.
			auto path_csv = fs::path(app->getDataset()->pathToConfigFolder) / path_files[selected_file];
			LOG(INFO) << "Loading camera path from '" << path_csv.generic_string() << "'";
			commands.push_back(std::make_shared<PathCommand>(path_csv.generic_string(), app->getDataset()->name));
		}

		gl_cam->cameraControl->init(commands);
	}

	if (ImGui::Button("Refresh file list"))
		refresh = true;

	ImGui::SameLine();
	if (selected_file <= 1)
		ImGui::Checkbox("Animate", &(gl_cam->record));
	else
		ImGui::Checkbox("Animate + Export", &(gl_cam->record));
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("toggle with 'F9'");

	// GUI controls for the circle swing animation.
	auto swing = std::dynamic_pointer_cast<CircleSwingCommand>(gl_cam->cameraControl->recording_session[0]);
	if (swing != nullptr)
	{
		ImGui::SameLine();
		ImGui::Checkbox("Record", &swing->record_frames);

		ImGui::Separator();
		ImGui::DragFloat("Circle radius", &swing->radius, 1, 0, 100, "%.1f cm");

		float swing_speed = radToDeg(*(swing->speed));
		if (ImGui::DragFloat("Angular speed", &swing_speed, 5, 5, 720, "%.1f deg/s"))
			*(swing->speed) = degToRad(swing_speed);

		ImGui::InputFloat3("Look-at point", swing->lookAt.data(), 0);

		// Alternative controls to manipulate the look-at point with polar coordinates.
		float dir_rad = atan2f(swing->lookAt.x(), -swing->lookAt.z());
		float look_dist = swing->lookAt.norm();
		bool update_lookat = false;
		update_lookat |= ImGui::SliderAngle("Look-at dir.", &dir_rad, -180, 180);
		update_lookat |= ImGui::DragFloat("Look-at rad.", &look_dist, 10, 0, 2000, "%.0f cm");
		if (update_lookat)
			swing->lookAt = look_dist * Eigen::Vector3f(sinf(dir_rad), 0.f, -cosf(dir_rad));
	}

	// GUI controls for the translate swing animation.
	auto translate = std::dynamic_pointer_cast<TranslateSwingCommand>(gl_cam->cameraControl->recording_session[0]);
	if (translate != nullptr)
	{
		ImGui::SameLine();
		ImGui::Checkbox("Record", &translate->record_frames);

		ImGui::Separator();
		ImGui::DragFloat("Transl. radius", &translate->radius, 1, 0, 100, "%.1f cm");
		ImGui::SliderAngle("Transl. angle", &translate->angle, -180, 180);

		float swing_speed = radToDeg(*(translate->speed));
		if (ImGui::DragFloat("Angular speed", &swing_speed, 5, 5, 720, "%.1f deg/s"))
			*(translate->speed) = degToRad(swing_speed);

		ImGui::InputFloat3("Look-at point", translate->lookAt.data(), 0);

		// Alternative controls to manipulate the look-at point with polar coordinates.
		float dir_rad = atan2f(translate->lookAt.x(), -translate->lookAt.z());
		float look_dist = translate->lookAt.norm();
		bool update_lookat = false;
		update_lookat |= ImGui::SliderAngle("Look-at dir.", &dir_rad, -180, 180);
		update_lookat |= ImGui::DragFloat("Look-at rad.", &look_dist, 10, 0, 2000, "%.0f cm");
		if (update_lookat)
			translate->lookAt = look_dist * Eigen::Vector3f(sinf(dir_rad), 0.f, -cosf(dir_rad));
	}
}


void ViewerGUI::showRenderingSettings()
{
	ImGui::Combo("Method", &app->settings->switchGLProgram, app->methods_str);
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("'Tab/Num 0' for next; +Ctrl for previous");

	ImGui::Combo("Proxy", &app->settings->switchGLRenderModel, app->proxies_str);

	const char* displayMode[] = {
		"Left camera view",
		"Right camera view",
		"Linear blending",
		"World lines",
		"Debug: Camera pair",
		"Debug: TexLeft",
		"Debug: TexRight",
		"Debug: FlowLeft",
		"Debug: FlowRight",
		"Debug: CompFlowLeft",
		"Debug: CompFlowRight",
		"Debug: World positions",
	};
	ImGui::Combo("Display mode", (int*)&app->settings->displayMode, displayMode, IM_ARRAYSIZE(displayMode));
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("'b' for next, +Ctrl for previous");

	// Change distance of the GLCamera Plane or change radius of Cylinder or Sphere.
	// We do not allow changing the deformed spheres.
	std::string proxy_str = app->proxies_str[app->settings->switchGLRenderModel];
	//TODO: We "physically" create the geometric shapes with its vertices from scratch and updateGLBuffers -> This is stupid! :)
	//What we want is to translate these operations into changes of Primitive's model matrix (not implemented yet).
	//TODO: partially replicated in ViewerApp::handleUserInput()
	if (proxy_str.compare("Plane") == 0)
	{
		float plane_distance = app->getGLCamera()->getPlaneDistance() / 100.f; // cm to m
		if (ImGui::DragFloat("Plane dist.", &plane_distance, 0.1f, 1, 50, "%.1f m"))
		{
			// This might crash in the HMD, but then again we don't show the GUI in the HMD.
			// We probably don't want to change these things in VR anyway.
			app->getGLCamera()->setPlaneDistance(100.f * plane_distance); // m to cm
		}
	}

	//TODO: We lose real-time interactivity for Cylinder and Sphere since it consists of many more vertices than a Plane.
	//Cylinder is okayish, Sphere is slowly updating. -> Solution is to use model matrices for Primitives -> TODO!!!
	if (proxy_str.compare("Cylinder") == 0)
	{
		float cylinder_radius = app->getDataset()->cylinder->radius / 100.f; // cm to m
		if (ImGui::DragFloat("Cylinder rad.", &cylinder_radius, 0.1f, 1, 50, "%.1f m"))
		{
			app->getDataset()->cylinder->setRadius(100.f * cylinder_radius); // m to cm
			app->getVisualization()->updateCylinderModel(app->settings->circle_resolution);
		}
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("increase with 'f'/decrease with 'v'");
	}
	if (proxy_str.compare("Sphere") == 0)
	{
		float sphere_radius = app->getDataset()->sphere->getRadius() / 100.f; // cm to m
		if (ImGui::DragFloat("Sphere radius", &sphere_radius, 0.1f, 1, 50, "%.1f m"))
		{
			app->getDataset()->sphere->setRadius(100.f * sphere_radius); // m to cm
			app->getVisualization()->updateSphereModel();
		}
	}

	// Window size (width x height).
	int window_size[2] = { app->getGLwindow()->getWidth(), app->getGLwindow()->getHeight() };
	ImGui::InputInt2("Window size", window_size);
	if (ImGui::IsItemDeactivatedAfterEdit())
		app->getGLwindow()->resizeFramebuffer(window_size[0], window_size[1]);
	
	// Vertical field of view (fovy) of the camera.
	float fovy_deg = radToDeg(app->getGLCamera()->getFovy());
	if (ImGui::DragFloat("Camera fovy", &fovy_deg, 0.2f, 5, 175, "%.1f deg"))
		app->getGLCamera()->setFovy(degToRad(fovy_deg));

	//Rendering options.
	//per-pixel view-synthesis off -> Use exactly two viewpoints for novel view independent of pixel location
	// -> Parallax360. Think of video frame interpolation.
	ImGui::Checkbox("Per-pixel view synthesis", (bool*)&app->settings->raysPerPixel);
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("toggle with 'z'");
	ImGui::SameLine();
	helpMarker("Enabled = MegaParallax/OmniPhotos\nDisabled = View synthesis is based only on the optical axis, like Parallax360.");

	// Toggle optical flow on/off.
	ImGui::Checkbox("Flow-based blending", (bool*)&app->settings->useOpticalFlow);
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("toggle with 'o'");

	ImGui::Checkbox("Fade out near boundary", (bool*)&app->settings->fadeNearBoundary);
}


void ViewerGUI::loadDatasetThumbnails()
{
	// Upload the thumbnail images as textures to GPU memory.
	if (app->numberOfDatasets() != datasetThumbList.size())
	{
		for (int idx = 0; idx < app->numberOfDatasets(); idx++)
		{
			cv::Mat& image = app->getDatasetFromList(idx).thumbImage;
			app->thumbnails.push_back(image);
		}
	}
	loadedDatasets = true;
}


void ViewerGUI::showDatasets()
{
	// Upload the thumbnail images as textures to GPU memory.
	if (app->numberOfDatasets() != datasetThumbList.size())
	{
		if (!loadedDatasets)
			loadDatasetThumbnails();

		for (int idx = 0; idx < app->numberOfDatasets(); idx++)
		{
			cv::Mat& image = app->thumbnails[idx];
			//TODO: cast this into a "GLTexture" object using memory layouts etc.
			GLuint texture;
			glGenTextures(1, &texture);
			glBindTexture(GL_TEXTURE_2D, texture);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image.cols, image.rows, 0, GL_BGR, GL_UNSIGNED_BYTE, image.data);

			datasetThumbList.push_back(reinterpret_cast<void*>(static_cast<intptr_t>(texture)));
		}
	}

	IM_ASSERT(ImGui::GetCurrentContext() != NULL && "Missing Dear ImGui context. Refer to examples app!");

	// Create a new window to show dataset thumbnails
	ImGui::Begin("Dataset List", &showDatasetWindow);
	//ImGui::TextWrapped("Pressing the button to select the viewing dataset.");

	for (int i = 0; i < app->numberOfDatasets(); i++)
	{
		ImGui::BeginGroup();

		// Show dataset name
		ImGui::TextWrapped(app->getDatasetFromList(i).name.c_str());

		// Change the current/loading dataset visualisation.
		static unsigned int counter = 0;
		if (app->currentDatasetIdx == i)
		{
			std::string info = "(current)";
			if (datasetLoading)
			{
				info = "- Loading ";
				int dot_number = int(counter / 30) % 10;
				for (int i = 0; i < dot_number; i++)
					info += ".";

				counter++;
			}

			ImGui::SameLine();
			ImGui::TextWrapped(info.c_str());
		}

		// Add image button.
		ImGui::PushID(i);
		if (ImGui::ImageButton(datasetThumbList[i], ImVec2(240, 120)))
		{
			LOG(INFO) << "Selected dataset '" << app->getDatasetFromList(i).name << "' to be loaded";
			counter = 0;
			app->loadDatasetCPUAsync(i);
		}
		ImGui::PopID();
		ImGui::EndGroup();
	}
	ImGui::End();
}


void ViewerGUI::showVisualisationTools()
{
	// reset view button
	if (ImGui::Button("Reset View"))
		app->initGLCameraToCentre();
	ImGui::SameLine();
	helpMarker("Press this button, 'backspace' or 'numpad 1' to reset the view to the centre of the camera circle.");

	ImGui::Checkbox("Show camera geometry", (bool*)&app->settings->showCameraGeometry);
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("toggle with 'm'");

	ImGui::Checkbox("Show fitted circle", (bool*)&app->settings->showFittedCircle);
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("toggle with 'c'");

	ImGui::Checkbox("Show world points", &app->settings->showWorldPoints);
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("toggle with 'w'");

	ImGui::Checkbox("Show radial lines", &app->settings->showRadialLines);
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("toggle 'd'");

	ImGui::Checkbox("Vertical translation", &app->mouse->freeTranslate);
	if (ImGui::IsItemHovered())
		ImGui::SetTooltip("toggle with spacebar");
}


void ViewerGUI::helpMarker(const char* desc)
{
	ImGui::TextDisabled("(?)");
	if (ImGui::IsItemHovered())
	{
		ImGui::BeginTooltip();
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
		ImGui::TextUnformatted(desc);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
}

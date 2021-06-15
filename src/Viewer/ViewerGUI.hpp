#pragma once

#include "3rdParty/imgui/examples/imgui_impl_glfw.h"
#include "3rdParty/imgui/examples/imgui_impl_opengl3.h"
#include "3rdParty/imgui/imgui.h"

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#include <vector>

class ViewerApp; // forward declaration to break circular dependency


/**
 * @brief  Generates a Graphical User Interface for the Viewer application.
 */
class ViewerGUI
{
public:
	/**
	 * Constructs a new ViewerGUI object.
	 * 
	 * @param _app should be the initialised ViewerApp.
	 */
	ViewerGUI(ViewerApp* _app);

	/**
	 * destructor.
	 */
	~ViewerGUI();

	/**
	 * Initialises imgui parameters and context.
	 * Also sets the pointers in GLApplication to imgui's pointers. 
	 * This way, none of the other apps need to know about imgui.
	 */
	void init();

	/**
	 * Draws the imgui.
	 * ImGui::Newframe needs to be called before every render.
	 */
	void draw();

	/**
	 * The semaphore of dataset loading. Set to true when loading a dataset from disk to CPU memory.
	 */
	bool datasetLoading = false;

private:
	GLFWwindow* window = nullptr; // initialised in ViewerGUI::init()
	ViewerApp* app = nullptr;     // initialised in the ViewerGUI constructor

	/**
	 * Shows the settings window with all the widgets.
	 */
	void showSettingsWindow();

	void showCameraPathAnimation();
	void showRenderingSettings();
	void showVisualisationTools();

	/**
	 * Creates a little question mark marker that displays a tooltip when hovered.
	 * 
	 * @param desc Tooltip text that will be displayed.
	 */
	void helpMarker(const char* desc);

	/**
	 * Creates a window showing the available datasets, including their name, thumbnail etc.
	 * Users can preview, select and load each dataset.
	 */
	void showDatasets();
	bool loadedDatasets = false;
	void loadDatasetThumbnails();
	bool showDatasetWindow = true;
	std::vector<ImTextureID> datasetThumbList; // Textures for dataset thumbnails
};

#include "GitVersion.hpp"

#include "3rdParty/cxxopts.hpp"
#include "3rdParty/fs_std.hpp"

#include "Utils/Exceptions.hpp"
#include "Utils/IOTools.hpp"
#include "Utils/Logger.hpp"

#include "Viewer/ViewerApp.hpp"

// The app needs to be global.
ViewerApp* app;

// This needs to be included once (*app) is defined.
#include "Core/GL/GLFW_Callbacks.hpp"

using namespace std;


int main(int argc, char* argv[])
{
	try
	{
		// Set command-line options.
		// clang-format off
		cxxopts::Options options("Viewer", "Viewer for OmniPhotos.");
		options.add_options()
			("f, config-file", "Path to a YAML config file or a directory containing multiple datasets.", cxxopts::value<string>())
			("h, help", "Print help.")
			("x, vr", "Render in VR using OpenVR.", cxxopts::value<bool>()->default_value("false"))
			("v, verbose", "Verbose output.", cxxopts::value<bool>()->default_value("false"))
			("t, texture-format", "Specify the texture format [GL_RGB|DXT1|DXT5].", cxxopts::value<string>()->default_value("GL_RGB"));
		// clang-format on

		options.parse_positional({ "f" });
		options.positional_help("path to config file or dataset directory");

		// Parse command-line options.
		auto vm = options.parse(argc, argv);
		if (vm.count("h"))
		{
			cout << options.help() << endl;
			return 0;
		}

		if (vm.count("f") == 0)
		{
			cout << "Error: No path to a config file given." << endl;
			return -__LINE__;
		}

		// Make path to config file/dataset directory absolute (using forward slashes).
		// Also throws an exception if the file/directory does not exist.
		string datasetPath = vm["f"].as<string>();
		datasetPath = fs::canonical(datasetPath).generic_string();

		string logFilename;
		if (fs::is_directory(datasetPath))
		{
			// Put the log file in the datasets directory.
			logFilename = (fs::path(datasetPath) / fs::path(argv[0]).stem()).generic_string();
		}
		else
		{
			// Put the log file in the same directory as the config file and name it similarly,
			// e.g. "config-preprocessing.yaml_Preprocessing-20200505-164200.log".
			logFilename = datasetPath + "_" + fs::path(argv[0]).stem().string();
		}

		// Enable VR rendering if --vr flag is set.
		bool enableVR = vm["vr"].as<bool>();
#ifndef WITH_OPENVR
		if (enableVR)
			throw std::runtime_error("Support for OpenVR is not enabled. Option --vr is not available.");
#endif

		// Get the image texture format.
		string textureFormat = vm["t"].as<string>();
		if (textureFormat != "GL_RGB" && textureFormat != "DXT1" && textureFormat != "DXT5")
		{
			cerr << "Error: Invalid texture format '" << textureFormat << "'.\n"
			     << "Choose one of the following formats: GL_RGB (default) | DXT1 | DXT5."
			     << endl;
			return -__LINE__;
		}

		// Set log level to verbose (optional).
		bool verbose = vm["verbose"].as<bool>();
		if (verbose) FLAGS_v = 10;
		Logger logger(logFilename);

		// Log startup information.
		stringstream ss;
		ss << "  Binary       : " << getBinaryPath().generic_string() << "\n";
		ss << "  Current path : " << fs::current_path().generic_string() << "\n";
		ss << "  Version      : " << g_GIT_VERSION << "\n";
		for (int i = 1; i <= argc; i++)
			ss << "  Argument " << i << "/" << argc << " : '" << argv[i] << "'\n";
		LOG(INFO) << "Startup information:\n"
		          << ss.str() << "\n";

		app = new ViewerApp(datasetPath, enableVR);
		app->imageTextureFormat = textureFormat;

		glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);
		glfwSwapInterval(0);
		glfwWindowHint(GLFW_DECORATED, 0);

		int returnCode = app->init();
		if (returnCode != 0)
		{
			LOG(ERROR) << "app::init() returned code " << returnCode << ". Exiting.";
			return returnCode;
		}
		app->callbackBoilerplate(windowSize_callback);

		// Loop until the user closes the window.
		while (!app->shouldShutdown)
		{
			app->run();
		}

		// Save window position for next time.
		app->getGLwindow()->writeUserFile();

		// Clean up.
		delete app;
		LOG(INFO) << "Finished main()";
	}
	catch (const exception& e)
	{
		LOG(ERROR) << "Exception raised: " << endl
		           << e.what();
		return -__LINE__;
	}

	return 0;
}

#include "GitVersion.hpp"

#include "3rdParty/cxxopts.hpp"
#include "3rdParty/fs_std.hpp"

#include "PreprocessingApp/PreprocessingApp.hpp"

#include "Utils/Exceptions.hpp"
#include "Utils/IOTools.hpp"
#include "Utils/Logger.hpp"


// The app needs to be global.
PreprocessingApp* app;

using namespace std;


int main(int argc, char* argv[])
{
	// Set command-line options.
	cxxopts::Options options("Preprocessing", "Data preprocessing program for OmniPhotos.");
	options.add_options()
		("f, config-file", "Path to a YAML config file.", cxxopts::value<std::string>())
		("h, help", "Print help.")
		("v, verbose", "Verbose output.", cxxopts::value<bool>()->default_value("false"));

	options.parse_positional({ "f" });
	options.positional_help("Path to a YAML config file");

	// Parse command-line options.
	auto vm = options.parse(argc, argv);
	if (vm.count("h"))
	{
		std::cout << options.help() << std::endl;
		return 0;
	}

	if (vm.count("f") == 0)
	{
		std::cout << "Error: No path to a config file given." << std::endl;
		return -__LINE__;
	}

	// Make path to config file absolute (using forward slashes).
	// Also throws an exception if the file does not exist.
	string configFilename = vm["f"].as<string>();
	configFilename = fs::canonical(configFilename).generic_string();

	// Put the log file in the same directory as the config file and name it similarly,
	// e.g. "config-preprocessing.yaml_Preprocessing-20200505-164200.log".
	string logFilename = configFilename + "_" + fs::path(argv[0]).stem().string();

	// Set log level to verbose (optional).
	bool verbose = vm["verbose"].as<bool>();
	if (verbose) FLAGS_v = 10;
	Logger logger(logFilename);

	// Log startup information.
	std::stringstream ss;
	ss << "  Binary       : " << getBinaryPath().generic_string() << "\n";
	ss << "  Current path : " << fs::current_path().generic_string() << "\n";
	ss << "  Version      : " << g_GIT_VERSION << "\n";
	for (int i = 1; i <= argc; i++)
		ss << "  Argument " << i << "/" << argc << " : '" << argv[i] << "'\n";
	LOG(INFO) << "Startup information:\n" << ss.str() << "\n";

	app = new PreprocessingApp(configFilename);

	try
	{
		int returnCode = app->init(); // there is no 'run'; 'init' does everything

		// Negative codes are not a warning
		if (returnCode > 0)
			LOG(WARNING) << "app::init() returned code " << returnCode;
	}
	catch (const exception& e)
	{
		LOG(ERROR) << "Exception raised: " << endl << e.what();
		return -__LINE__;
	}

	// Clean up.
	delete app;
	LOG(INFO) << "Finished main()";
	return 0;
}

#include "CameraSetupSettings.hpp"
#include "CameraSetupDataset.hpp"

#include "3rdParty/fs_std.hpp"

#include "Utils/Exceptions.hpp"
#include "Utils/Logger.hpp"
#include "Utils/Utils.hpp"

#include <opencv2/opencv.hpp>

using namespace std;


CameraSetupSettings::CameraSetupSettings(CameraSetupSettings& _settings)
{
	*this = _settings;
}


int CameraSetupSettings::readSettingsFromFile(const string& pathToFile, CameraSetupDataset* _dataset)
{
	if (!fs::exists(pathToFile))
		RUNTIME_EXCEPTION(string("Settings file '") + pathToFile + "' does not exist.");

	configFile = pathToFile;
	fs::path configDir = fs::path(configFile).parent_path();
	_dataset->workingDirectory = configDir.parent_path().generic_string();

	cv::FileStorage fs(pathToFile, cv::FileStorage::READ);

	_dataset->pathToInputFolder = _dataset->workingDirectory + "/Input";
	fs["General"]["CacheFolder"] >> _dataset->pathToCacheFolder;
	_dataset->pathToCacheFolder = _dataset->workingDirectory + "/Cache" + _dataset->pathToCacheFolder;
	_dataset->pathToConfigFolder = _dataset->workingDirectory + "/Config";

	// Read settings from Camera section.
	fs["Camera"]["Equirectangular"] >> useEquirectCamera;

	// Read settings from Preprocessing section.
	fs["Preprocessing"]["ImageFilenames"] >> imageFileNames;
	fs["Preprocessing"]["NumberOfCameras"] >> numberOfCameras;
	fs["Preprocessing"]["IntrinsicScale"] >> intrinsicScale;
	fs["Preprocessing"]["ShapeSampling"] >> shapeSampling;
	fs["Preprocessing"]["ChangeBasis"] >> changeBasis;
	fs["Preprocessing"]["ShapeFit"] >> shapeFit;
	fs["Preprocessing"]["DownsampleFlow"] >> downsampleFlow;
	fs["Preprocessing"]["ComputeOpticalFlow"] >> computeOpticalFlow;
	if (computeOpticalFlow > 0)
	{
		fs["Preprocessing"]["OpticalFlowMethod"] >> opticalFlowMethod;

		// TODO: This overwrites the default values if the settings are missing in the config file.
		fs["Preprocessing"]["FlowOCVBrox2004Parameters"]["Alpha"] >> broxFlowParams.alpha;
		fs["Preprocessing"]["FlowOCVBrox2004Parameters"]["Gamma"] >> broxFlowParams.gamma;
		fs["Preprocessing"]["FlowOCVBrox2004Parameters"]["ScaleFactor"] >> broxFlowParams.scaleFactor;
		fs["Preprocessing"]["FlowOCVBrox2004Parameters"]["InnerIterations"] >> broxFlowParams.innerIterations;
		fs["Preprocessing"]["FlowOCVBrox2004Parameters"]["OuterIterations"] >> broxFlowParams.outerIterations;
		fs["Preprocessing"]["FlowOCVBrox2004Parameters"]["SolverIterations"] >> broxFlowParams.solverIterations;
	}

	// Read settings from Geometry section.
	fs["Geometry"]["SFM"] >> cameraGeometryFile;
	cameraGeometryFile = _dataset->pathToConfigFolder + "/" + cameraGeometryFile;

	int points, loadSparse;
	fs["Geometry"]["Load3DPoints"] >> points;
	load3DPoints = (points > 0);
	fs["Geometry"]["LoadSparsePointCloud"] >> loadSparse;
	loadSparsePointCloud = (loadSparse > 0);

	fs["Geometry"]["MaxNumber3DPoints"] >> maxLoad3Dpoints;
	fs["Geometry"]["Max3DPointError"] >> max3DPointError;
	fs["Geometry"]["CircleRadius"] >> circleRadius;
	fs["Geometry"]["CylinderRadius"] >> cylinderRadius;

	// Check the file ending of the file and determine the input format.
	std::string fileExtension = extractFileExtension(cameraGeometryFile);
	if (fileExtension.compare(".nvm") == 0) sfmFormat = SfmFormat::VisualSFM;
	if (fileExtension.compare(".out") == 0) sfmFormat = SfmFormat::Bundler;
	if (fileExtension.compare(".txt") == 0) sfmFormat = SfmFormat::Colmap;
	if (fileExtension.compare(".openvslam") == 0) sfmFormat = SfmFormat::OpenVSLAM;

	// Read settings from Video section.
	fs["Video"]["FirstFrame"] >> videoParams.firstFrame;
	fs["Video"]["LastFrame"] >> videoParams.lastFrame;
	fs["Video"]["FrameInterval"] >> videoParams.frameInterval;

	fs.release();

	string imageNameDigits = stripExtensionFromFilename(imageFileNames);
	size_t pos = imageNameDigits.length();
	imageNameDigits = imageNameDigits.substr(pos - 3, 3);
	digits = stoi(imageNameDigits);

	return 0;
}

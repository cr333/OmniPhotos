#include "UnitTestHeader.hpp"

#include "Core/CameraSetup/CameraSetupDataset.hpp"
#include "Core/CameraSetup/CameraSetupSettings.hpp"
#include "Core/Loaders/OpenVSLAMLoader.hpp"

#include <gtest/gtest.h>


class OpenVSLAMLoaderTest : public testing::Test
{
public:
	OpenVSLAMLoader slamLoader;

	// runtime variable
	CameraSetupDataset dataset;
	std::string slamResultListPath;
	std::string pathToInputFolder; // input image folder path
	float intrinsicScale;
	bool load3DPoints;
	bool loadSparsePointCloud;
	int imageNameDigits;
	std::string imageFileNameRegu;

	std::string testDataRoot; // test data root path

	virtual void SetUp()
	{
		testDataRoot = unitTestDataDirectory;

		// initial test run time environment
		imageFileNameRegu = "original-%04d.png";
		loadSparsePointCloud = true;
		load3DPoints = true;
		intrinsicScale = 1;
		pathToInputFolder = testDataRoot + "circular/GenoaLargeRadius/Input/";
		slamResultListPath = testDataRoot + "circular/GenoaLargeRadius/Config/modelFiles.openvslam";
		imageNameDigits = 4;

		// setup dataset
		CameraSetupSettings* cameraSetupSettings = new CameraSetupSettings();
		cameraSetupSettings->videoParams.frameInterval = 1;
		cameraSetupSettings->max3DPointError = 0.1f;
		cameraSetupSettings->maxLoad3Dpoints = 99999;
		dataset.getCameraSetup()->setSettings(cameraSetupSettings);
	}

	virtual void TearDown()
	{
	}
};

TEST_F(OpenVSLAMLoaderTest, loadData)
{
	// test
	slamLoader.setImageNameDigits(imageNameDigits);
	slamLoader.loadData(&dataset,
	                    slamResultListPath,
	                    pathToInputFolder,
	                    intrinsicScale,
	                    load3DPoints,
	                    loadSparsePointCloud);

	// check result
	// 1. output point cloud to OBJ file
	const char* testSuitName = ::testing::UnitTest::GetInstance()->current_test_info()->test_suite_name();
	const char* testName = ::testing::UnitTest::GetInstance()->current_test_info()->name();
	dataset.getWorldPointCloud()->writeToObj(testDataRoot + testSuitName + "_" + testName + ".obj");

	// 2. check the camera pose
	EXPECT_GT(dataset.getCameraSetup()->getCameras()->size(), 0);
}

#include "UnitTestHeader.hpp"

#include "3rdParty/fs_std.hpp"

#include "Utils/FlowIO.hpp"
#include "Utils/IOTools.hpp"
#include "Utils/Logger.hpp"
#include "Utils/Utils.hpp"
#include "Utils/cvutils.hpp"

#include <gtest/gtest.h>
#include <opencv2/opencv.hpp>

#include <string>

using namespace std;


// The tests are separated according to their physical components (source files) in Utils
// The Test Suites are sorted according to logical components.


/////////////
// IOTools
/////////////

// Header-only IO file for general filesystem management

TEST(IOToolsTest, getProjectSrcDirTest)
{
	std::string file_path = __FILE__;
	std::replace(file_path.begin(), file_path.end(), '\\', '/');

	// Remove the source file
	file_path = file_path.substr(0, file_path.rfind("/"));

	// Remove the test directory
	file_path = file_path.substr(0, file_path.rfind("/"));

	// Add the source directory
	ASSERT_STREQ(file_path.c_str(), getProjectSourcePath().generic_string().c_str());
}


///////////
// Utils
///////////

// FileOps
TEST(FileOpsTest, extractFileExtensionTest)
{
	std::string file_path = "foo/bar.txt";
	ASSERT_STREQ(extractFileExtension(file_path).c_str(), ".txt");
}

// StringOps
TEST(StringOpsTest, stripIncludeFromLineTest)
{
	string oldLine = "#include C:/Setup/src/DefaultInterface.glsl";
	string line = stripIncludeFromLine(oldLine);
	ASSERT_STREQ(line.c_str(), "C:/Setup/src/DefaultInterface.glsl");
}

TEST(StringOpsTest, stripIncludeFromLineNullCaseTest)
{
	string oldLine = "C:/Setup/src/DefaultInterface.glsl";
	string line = stripIncludeFromLine(oldLine);
	ASSERT_STREQ(line.c_str(), "C:/Setup/src/DefaultInterface.glsl");
}

TEST(StringOpsTest, stripExtensionFromFilenameTest)
{
	string oldLine = "C:/Setup/src/DefaultInterface.glsl";
	string line = stripExtensionFromFilename(oldLine);
	ASSERT_STREQ(line.c_str(), "C:/Setup/src/DefaultInterface");
}

TEST(StringOpsTest, startsWithTest1)
{
	ASSERT_TRUE(startsWith("abc123doremi", ""));
}

TEST(StringOpsTest, startsWithTest2)
{
	ASSERT_TRUE(startsWith("abc123doremi", "abc"));
}

TEST(StringOpsTest, startsWithTest3)
{
	ASSERT_FALSE(startsWith("abc123doremi", "123"));
}

TEST(StringOpsTest, startsWithTest4)
{
	ASSERT_FALSE(startsWith("abc123doremi", "abc123doremi-extra"));
}

TEST(StringOpsTest, endsWithTest1)
{
	ASSERT_TRUE(endsWith("abc123doremi", "doremi"));
}

TEST(StringOpsTest, endsWithTest2)
{
	ASSERT_TRUE(endsWith("abc123doremi", ""));
}

TEST(StringOpsTest, endsWithTest3)
{
	ASSERT_FALSE(endsWith("abc123doremi", "123"));
}

TEST(StringOpsTest, endsWithTest4)
{
	ASSERT_FALSE(endsWith("abc123doremi", "extra-abc123doremi"));
}

TEST(FolderOpsTest, pathStartsWithTest)
{
	ASSERT_TRUE(pathStartsWith("/home/unix/style/path/", "\\home\\"));
}


///////////
// Logger
///////////

int log_tester()
{
	Logger log("log_tester");
	return 0;
}

TEST(LoggerTest, loggerConstructorTest)
{
	ASSERT_EQ(log_tester(), 0);
}


//////////
// FlowIO
//////////

TEST(FlowIOTest, readFloFileTest)
{
	string floFile = unitTestDataDirectory + "general_sphere/undistorted-1702-FlowToNext.flo";
	ASSERT_FALSE(readFloFile(floFile).empty());
}

TEST(FlowIOTest, readFloFileWithCharTest)
{
	string floFile = unitTestDataDirectory + "general_sphere/undistorted-1702-FlowToNext.flo";
	const char* floFileChar = floFile.c_str();
	ASSERT_FALSE(readFloFile(floFileChar).empty());
}

TEST(FlowIOTest, readWriteFlossFile)
{
	// Read in an existing flow file.
	string floFile = unitTestDataDirectory + "general_sphere/undistorted-1702-FlowToNext.flo";
	auto flow = readFloFile(floFile);

	// Write as a .floss file (this is lossy).
	string flossFile = unitTestDataDirectory + "general_sphere/testfile.floss";
	writeFlossFile(flossFile, flow);

	// Read the flow again and compare to the original flow.
	auto flow_ss = readFlossFile(flossFile);
	ASSERT_TRUE(cv::rmse(flow, flow_ss) < 0.1);

	// Write and read .floss files again using generic flow functions.
	string flossFile2 = unitTestDataDirectory + "general_sphere/testfile2.floss";
	writeFlowFile(flossFile2, flow_ss);
	auto flow_ss2 = readFlowFile(flossFile2);

	// This should have been lossless.
	double reread_error = cv::rmse(flow_ss, flow_ss2);
	ASSERT_TRUE(reread_error == 0.);
}

TEST(FlowIOTest, readBarronFileTest)
{
	string barronFile = unitTestDataDirectory + "general_sphere/L0L1_t.F";
	ASSERT_FALSE(readBarronFile(barronFile.c_str()).empty());
}

TEST(FlowIOTest, writeBarronFileTest)
{
	string barronFile = unitTestDataDirectory + "general_sphere/L0L1_t.F";
	cv::Mat2f matrix = readBarronFile(barronFile.c_str());
	string test_barronFile = unitTestDataDirectory + "general_sphere/testfile.F";
	writeBarronFile(test_barronFile.c_str(), matrix);
	cv::Mat2f diff = matrix - readBarronFile(test_barronFile.c_str());

	ASSERT_TRUE(cv::countNonZero(cv::reduce(diff, 2, 0)) == 0);
}

TEST(FlowIOTest, readFloGenFunction_1)
{
	string barronFile = unitTestDataDirectory + "general_sphere/L0L1_t.F";
	ASSERT_FALSE(readFlowFile(barronFile).empty());
}

TEST(FlowIOTest, readFloGenFunction_2)
{
	string floFile = unitTestDataDirectory + "general_sphere/undistorted-1702-FlowToNext.flo";
	ASSERT_FALSE(readFlowFile(floFile).empty());
}

TEST(FailureCaseTest, fail)
{
	ASSERT_FALSE(true);
}
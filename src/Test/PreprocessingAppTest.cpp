#include "UnitTestHeader.hpp"

#include "3rdParty/fs_std.hpp"
#include "PreprocessingApp/PreprocessingApp.hpp"

#include <exception>
#include <gtest/gtest.h>


class ColmapPreprocessingAppTest : public testing::Test
{
public:
	PreprocessingApp* preprocessingApp = nullptr;

	std::string testConfig = unitTestDataDirectory + "circular/rooftop/Config/jenkins-config-test.yaml";


	virtual void SetUp()
	{
		fs::remove_all(unitTestDataDirectory + "circular/rooftop/Cache/test/");
		preprocessingApp = new PreprocessingApp(testConfig);
	}

	virtual void TearDown()
	{
		if (preprocessingApp)
			delete preprocessingApp;
	}
};


class OpenvslamPreprocessingAppTest : public testing::Test
{
public:
	PreprocessingApp* preprocessingApp = nullptr;
	std::string testConfig = unitTestDataDirectory + "circular/BeijingBeihai5/Config/jenkins-config-test.yaml";

	virtual void SetUp()
	{
		fs::remove_all(unitTestDataDirectory + "circular/BeijingBeihai5/Cache/test/");
		preprocessingApp = new PreprocessingApp(testConfig);
	}

	virtual void TearDown()
	{
		if (preprocessingApp)
			delete preprocessingApp;
	}
};


TEST_F(ColmapPreprocessingAppTest, runTest)
{
	int returncode = preprocessingApp->init();
	ASSERT_EQ(returncode, -1);
}


TEST_F(OpenvslamPreprocessingAppTest, runTest)
{
	int returncode = preprocessingApp->init();
	ASSERT_EQ(returncode, -1);
}

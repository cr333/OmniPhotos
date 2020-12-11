#include "UnitTestHeader.hpp"

#include "3rdParty/cxxopts.hpp"
#include "3rdParty/fs_std.hpp"

#include "Utils/Logger.hpp"

#include <gtest/gtest.h>


std::string unitTestDataDirectory;


int main(int argc, char** argv)
{
	::testing::InitGoogleTest(&argc, argv);

	cxxopts::Options options("OmniPhotosUnittest", "Unit testing module for OmniPhotos.");
	options.add_options()
		("h, help", "print help.")
		("purge_cache_file", "(deprecated) remove anything in cache called 'test'")
		("f, data_root_dir", "path to testDatasets. ", cxxopts::value<std::string>()->default_value("Xdrive"));

	auto vm = options.parse(argc, argv);
	if (vm.count("h"))
	{
		std::cout << options.help();
		return 0;
	}

	if (vm.count("f"))
	{
		unitTestDataDirectory = vm["f"].as<std::string>();
	}
	else
	{
		unitTestDataDirectory = "X:/CompSci/ResearchProjects/CRichardt/TBertel/MegaParallax/testDatasets/";
	}

	if (vm.count("purge_cache_file"))
	{
		// Do nothing. TODO: remove this once all branch test suites have been updated.
	}

	replace(unitTestDataDirectory.begin(), unitTestDataDirectory.end(), '\\', '/');

	return RUN_ALL_TESTS();
}

#pragma once

#include "FileNodeParser.hpp"

#include <opencv2/opencv.hpp>

#include <string>


class ConfigReader
{
public:
	ConfigReader() = default;
	~ConfigReader();

	bool open(std::string filename);

	cv::FileNode readNode(std::string section);
	cv::FileNode readNode(std::string section, std::string name);
	// TODO: one readNode with variadic arguments

	std::string readSetting(std::string section, std::string name, const char* defaultValue = "")
	{
		return readSetting(section, name, std::string(defaultValue));
	}

	template <typename Type>
	Type readSetting(std::string section, std::string name, Type defaultValue)
	{
		return FileNodeParser::read(readNode(section, name), defaultValue);
	}

	std::string configFilename;


private:
	cv::FileStorage m_fileStorage;
};

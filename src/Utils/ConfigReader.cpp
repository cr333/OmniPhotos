#include "ConfigReader.hpp"
#include "IOTools.hpp"


ConfigReader::~ConfigReader()
{
	m_fileStorage.release();
}


bool ConfigReader::open(std::string filename)
{
	configFilename = filename;

	// First check if the config file is found.
	if (configFilename.empty() || fileExists(configFilename) == false)
	{
		LOG(WARNING) << "Error: Config file not found at " << configFilename;
		return false;
	}

	try
	{
		// Open the configuration file.
		m_fileStorage.open(configFilename, cv::FileStorage::READ);
	}
	catch (cv::Exception& e)
	{
		LOG(ERROR) << "Error while reading config file " << configFilename 
		<< "\n" << e.what();
	}

	return m_fileStorage.isOpened();
}


/// <summary>Reads a certain node determined by 'section'. Empty if not found.</summary>
cv::FileNode ConfigReader::readNode(std::string section)
{
	if (m_fileStorage.isOpened() == false)
		return cv::FileNode();

	return m_fileStorage[section.c_str()];
}


/// <summary>Reads a certain node determined by 'section' and 'name'. Empty if not found.</summary>
cv::FileNode ConfigReader::readNode(std::string section, std::string name)
{
	if (m_fileStorage.isOpened() == false)
		return cv::FileNode();

	cv::FileNode sectionNode = m_fileStorage[section.c_str()];
	if (sectionNode.empty())
		return sectionNode;

	cv::FileNode valueNode = sectionNode[name.c_str()];
	if (valueNode.empty())
		return valueNode;

	return valueNode;
}

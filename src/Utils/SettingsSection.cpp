#include "SettingsSection.hpp"


// Reads all entries.
void SettingsSection::readSettings(const cv::FileNode& sectionNode)
{
	for (auto& entry : _entries)
	{
		entry->readFromSection(sectionNode);
	}
}


// Extends vector entries to the specified length.
void SettingsSection::padVectorsAtEnd(int desiredLength)
{
	for (auto& entry : _entries)
	{
		auto vectorEntry = std::dynamic_pointer_cast<VectorSettingEntry>(entry);
		if (vectorEntry) vectorEntry->padVectorAtEnd(desiredLength);
	}
}


// Writes the current values of entries to file.
void SettingsSection::writeSettings(cv::FileStorage& fileStorage)
{
	fileStorage << name << "{";
	for (auto& entry : _entries)
	{
		entry->write(fileStorage);
	}
	fileStorage << "}";
}

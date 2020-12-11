#pragma once

#include "SettingEntry.hpp"

#include <memory>


// Describes a section in a settings file.
class SettingsSection
{
public:
	SettingsSection(std::string name) :
	    name(name) {}


	// Adds an entry to this section.
	template <typename Type>
	void addEntry(std::string name, Type& valueReference)
	{
		_entries.push_back(std::make_shared<TypedSettingEntry<Type>>(name, valueReference, valueReference));
	}

	// Adds an entry to this section using the given default value.
	template <typename Type>
	void addEntry(std::string name, Type& valueReference, Type defaultValue)
	{
		_entries.push_back(std::make_shared<TypedSettingEntry<Type>>(name, valueReference, defaultValue));
	}

	// Convenience overload for strings with char* default value.
	void addEntry(std::string name, std::string& valueReference, const char* defaultValue)
	{
		_entries.push_back(std::make_shared<TypedSettingEntry<std::string>>(name, valueReference, defaultValue));
	}

	// Adds a vector-valued entry to this section.
	template <typename Type>
	void addVectorEntry(std::string name, std::vector<Type>& valueReference)
	{
		_entries.push_back(std::make_shared<TypedVectorSettingEntry<Type>>(name, valueReference));
	}

	// Adds a vector-valued entry to this section using the given default value.
	template <typename Type>
	void addVectorEntry(std::string name, std::vector<Type>& valueReference, Type defaultValue)
	{
		_entries.push_back(std::make_shared<TypedVectorSettingEntry<Type>>(name, valueReference, defaultValue));
	}

	// Convenience overload for strings with char* default value.
	void addVectorEntry(std::string name, std::vector<std::string>& valueReference, const char* defaultValue)
	{
		_entries.push_back(std::make_shared<TypedVectorSettingEntry<std::string>>(name, valueReference, defaultValue));
	}

	// Reads all entries.
	virtual void readSettings(const cv::FileNode& sectionNode);

	// Extends vector entries to the specified length.
	virtual void padVectorsAtEnd(int desiredLength);

	// Writes the current values of entries to file.
	virtual void writeSettings(cv::FileStorage& fileStorage);


	// Name of this section.
	std::string name;


protected:
	// All setting entries defined in this section.
	std::vector<std::shared_ptr<SettingEntry>> _entries;
};

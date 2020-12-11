#pragma once

#include "FileNodeParser.hpp"

#include <string>
#include <vector>


// Abstract base class for all setting entries.
// Exists so that all entries can be put into one list and read/written regardless of their type.
class SettingEntry
{
public:
	SettingEntry(std::string name) :
	    name(name) {}

	// Reads the setting from the given section file node.
	virtual void readFromSection(const cv::FileNode& sectionNode) = 0;

	// Writes the setting to file.
	virtual void write(cv::FileStorage& fileStorage) = 0;

	std::string name;
};


// Abstract base class for all vector-valued setting entries.
// Exists so that all vector-valued settings can be padded to the right length regardless of their type.
class VectorSettingEntry : public SettingEntry
{
public:
	VectorSettingEntry(std::string name) :
	    SettingEntry(name) {}

	// Repeats the last element until the vector has at least the desired length.
	virtual void padVectorAtEnd(int desiredLength) = 0;
};


// Derived class for typed setting entries.
template <typename Type>
class TypedSettingEntry : public SettingEntry
{
public:
	TypedSettingEntry(std::string name, Type& value, Type defaultValue) :
	    SettingEntry(name), value(value)
	{
		value = defaultValue;
	}

	// Reads the setting from the given section file node.
	virtual void readFromSection(const cv::FileNode& sectionNode)
	{
		value = FileNodeParser::read(sectionNode[name], value);
	}

	// Writes the setting to file.
	virtual void write(cv::FileStorage& fileStorage)
	{
		fileStorage << name << value;
	}

	Type& value;
};


// Derived class for vector-valued typed setting entries.
template <typename Type>
class TypedVectorSettingEntry : public VectorSettingEntry
{
public:
	TypedVectorSettingEntry(std::string name, std::vector<Type>& value) :
	    VectorSettingEntry(name), value(value)
	{
		// Using the current value of 'vector' as the default value.
	}

	TypedVectorSettingEntry(std::string name, std::vector<Type>& value, Type defaultValue) :
	    VectorSettingEntry(name), value(value)
	{
		// Initialise vector with a single copy of the default value.
		value.clear();
		value.push_back(defaultValue);
	}

	// Reads the setting from the given section file node.
	virtual void readFromSection(const cv::FileNode& sectionNode)
	{
		// First see if the setting exists.
		cv::FileNode node = sectionNode[name];
		if (node.empty()) return;

		// If it's a sequence, read it as such.
		if (node.type() == cv::FileNode::SEQ)
		{
			value = FileNodeParser::read(node, value);
		}
		else
		{
			// Otherwise assume it's a single value.
			auto new_value = FileNodeParser::read(node, value[0]);
			value.clear();
			value.push_back(new_value);
		}
	}

	// Repeats the last element until the vector has at least the desired length.
	virtual void padVectorAtEnd(int desiredLength)
	{
		while (value.size() < desiredLength)
			value.push_back(value.back());
	}

	// Writes the setting to file.
	virtual void write(cv::FileStorage& fileStorage)
	{
		// Drop repeated values from the end for simplicity.
		std::vector<Type> copiedVector(value);
		while (copiedVector.size() > 1)
		{
			size_t length = copiedVector.size();
			if (copiedVector[length - 2] == copiedVector[length - 1])
				copiedVector.pop_back();
			else
				break;
		}

		fileStorage << name << copiedVector;
	}

	std::vector<Type>& value;
};

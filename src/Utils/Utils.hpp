#pragma once

#include "Core/LinearAlgebra.hpp"

#include <opencv2/core.hpp>

#include <string>


//---- Formerly namespace Convert -----------------------------------------------------------------

float degToRad(float x);
float radToDeg(float x);

float cartesianToAngle(const Eigen::Vector2f& p);
Eigen::Point2f angleToCartesian(const float angle_in_degrees);


//---- Formerly namespace GLDepencencyOps ---------------------------------------------------------

std::string determinePathToSource();


//---- Formerly namespace FileOps -----------------------------------------------------------------

std::string extractFileExtension(const std::string& file);

time_t getTimeStamp(const std::string& file);

std::string addLineNumbersToString(const std::string& fileAsString);

void writeToFile(const std::string& filename, const std::string& contents);

std::string readFile(const std::string& filename);


//---- Formerly namespace FolderOps ---------------------------------------------------------------

bool removeFolder(std::string path);

// Returns the path to 'filename' relative to 'folder'.
std::string createFilenameRelativeToFolder(std::string filename, std::string folder);

// Returns true if 'path' starts with 'candidate' path.
bool pathStartsWith(std::string path, std::string candidate);


//---- Formerly namespace StringOps ---------------------------------------------------------------

void splitFilename(const std::string& filename, std::string* path, std::string* file);

std::string stripExtensionFromFilename(const std::string& file);

int extractLastNDigitsFromFilenameAsInt(const std::string& filename, int n);

std::string stripIncludeFromLine(const std::string& line);

std::string removeAfterSequence(const std::string& line, const std::string& sequence);

bool contains(const std::string& line, const std::string& sequence);

bool startsWith(const std::string& line, const std::string& substr);

bool endsWith(const std::string& line, const std::string& substr);

std::string stripSymbolFromString(const std::string& line, char symbol);

std::string replaceDelimiter(std::string in);

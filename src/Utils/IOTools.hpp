#pragma once

#include "Utils/Logger.hpp"

#include "3rdParty/fs_std.hpp"

#include <algorithm>
#include <fstream>
#include <string>
#if defined(_WIN64) || defined(_WIN32)
	#ifndef NOMINMAX
		#define NOMINMAX
	#endif
	#include <windows.h>
#elif defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__)) // UNIX
	#include <sys/stat.h>
	#include <errno.h>
#endif

#if defined(__APPLE__)
	#include <mach-o/dyld.h>
#endif

inline bool fileExists(const std::string& filename);
inline char getPathSeparator();


// Copies the file contents of <source_file> to <target_file>.
// Returns true if successful. Otherwise prints an error and returns false.
inline bool copyFile(const std::string source_filename, const std::string target_filename, bool overwrite_existing = false)
{
	std::ifstream src(source_filename.c_str(), std::ios::binary);
	if (!src.is_open())
	{
		LOG(WARNING) << "Error in copyFile: source file \"" << source_filename 
		<< "\" could not be opened.";
		return false;
	}

	if (!overwrite_existing && fileExists(target_filename))
	{
		LOG(WARNING) << "Error in copyFile: target file \"" << target_filename << "\" exists.";
		return false;
	}

	std::ofstream dst(target_filename.c_str(), std::ios::binary);
	if (!dst.is_open())
	{
		LOG(WARNING) << "Error in copyFile: target file \"" << target_filename << "\" could not be opened.";
		return false;
	}

	dst << src.rdbuf();
	if (src.tellg() != dst.tellp())
	{
		LOG(WARNING) << "Error in copyFile: not all data written (source: " 
		<< (int)src.tellg() << ", target: " << (int)dst.tellp() << ").";
		return false;
	}

	return true;
}


// Checks if the specified file exists (i.e. it is readable).
inline bool fileExists(const std::string& filename)
{
	std::ifstream infile(filename.c_str());
	return infile.good();
}


// Finds a file by its <path> with an optional path <prefix> that is prepended if <path> does not exist.
inline std::string findFileWithPrefix(const std::string path, const std::string prefix = std::string())
{
	// First try the path as specified, without prefix.
	if (fileExists(path))
		return path;

	// Next, try to add prefix (if not empty).
	if (prefix != "")
	{
		std::string filename = prefix + path;
		if (fileExists(filename))
			return filename;

		// Finally, try adding a path separator between prefix and path.
		filename = prefix + getPathSeparator() + path;
		if (fileExists(filename))
			return filename;
	}

	// If all else fails, return the original path to produce the least confusing result.
	return path;
}


// Returns the directory name of a path (without trailing path separators).
// Returns an empty string if no path separator is found.
inline std::string getDirName(const std::string& path)
{
	std::string::size_type sep_pos =
#if defined(_WIN32)
	    path.find_last_of("\\/");
#else
	    path.find_last_of("/");
#endif

	if (sep_pos == std::string::npos)
	{
		// No path separator found.
		return "";
	}

	return path.substr(0, sep_pos);
}


// Returns the extension of a file name, including the leading dot.
// Returns an empty string if extension is not found.
inline std::string getFileExtension(const std::string& filename)
{
	std::string::size_type dot_pos = filename.rfind('.');
	if (dot_pos == std::string::npos)
	{
		// No extension found.
		return "";
	}

	return filename.substr(dot_pos);
}


// Returns the file name (trailing path component) of a path.
// Returns the input path if no path separator is found.
inline std::string getFileName(const std::string& path)
{
	std::string::size_type sep_pos =
#if defined(_WIN32)
	    path.find_last_of("\\/");
#else
	    path.find_last_of("/");
#endif

	if (sep_pos == std::string::npos)
	{
		// No path separator found.
		return path;
	}

	return path.substr(sep_pos + 1);
}


// Returns the preferred path separator.
inline char getPathSeparator()
{
#if defined(_WIN64) || defined(_WIN32)
	return '\\';
#elif defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__)) // UNIX
	return '/';
#else
	#error Not implemented.
#endif
}


// Returns whether the specified <path> is an absolute path.
inline bool isPathAbsolute(std::string path)
{
	if (path.empty() || path[0] == 0) return false;
#if defined(_WIN64) || defined(_WIN32)
	if (path.length() >= 1 && path[0] == '\\') return true; // Network or UNC path.
	if (path.length() >= 2 && path[1] == ':') return true; // Any drive path (e.g. 'X:...').
#elif defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__)) // UNIX
	if (path.length() >= 1 && path[0] == '/') return true;
#endif
	return false;
}


// Returns whether the character is a legal path separator.
inline bool isPathSeparator(char c)
{
#if defined(_WIN64) || defined(_WIN32)
	if (c == '\\' || c == '/') return true;
#elif defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__)) // UNIX
	if (c == '/') return true;
#else
	#error Not implemented.
#endif
	return false;
}


// Joins two paths, ignoring the first part if the second is absolute.
inline std::string joinPaths(std::string path, std::string path2)
{
	if (isPathAbsolute(path2))
		return path2;

	if (path.empty() || isPathSeparator(path.back()))
		return path + path2;

	return path + getPathSeparator() + path2;
}


// Creates a directory at the specified <path> (not recursively).
// Returns true if the directory has been created successfully or if it already exists.
inline bool makeDirectory(std::string path)
{
#if defined(_WIN64) || defined(_WIN32)
	return (CreateDirectory(path.c_str(), NULL) || ERROR_ALREADY_EXISTS == GetLastError());
#elif defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__)) // UNIX
	return (mkdir(path.c_str(), 0777) == 0 || errno == EEXIST);
#endif
	return false;
}


/**
* @brief Returns the path to the current binary.
*
* @return fs::path
*/
inline fs::path getBinaryPath()
{
#if defined(_WIN64) || defined(_WIN32)
	WCHAR wc_path[MAX_PATH];
	GetModuleFileNameW(NULL, wc_path, MAX_PATH);
	std::wstring ws(wc_path);
	return fs::path(ws);
#elif defined(__APPLE__)
	unsigned int bufferSize = 512;
	std::vector<char> buffer(bufferSize + 1);
	if (_NSGetExecutablePath(&buffer[0], &bufferSize))
	{
		buffer.resize(bufferSize);
		_NSGetExecutablePath(&buffer[0], &bufferSize);
	}
	std::string s = &buffer[0];
	return s;
#else
	#error Cannot find the executable on this platform
#endif
}


/**
 * @brief Returns the path to the project source directory, as compiled.
 * 
 * This function assumes the IOTools.hpp file is located in the subdirectory
 * /src/Utils/. It then moves up one level in that path and returns that directory (i.e. /src).
 *
 * @return fs::path
 */
inline fs::path getProjectSourcePath()
{
	fs::path file_path(__FILE__);

	// Remove the filename 'IOTools.hpp'.
	file_path = file_path.parent_path();

	// Remove the 'Utils' directory.
	file_path = file_path.parent_path();

	return file_path;
}

#include "Utils.hpp"

#include "IOTools.hpp"
#include "Logger.hpp"
#include "STLutils.hpp"

#include "3rdParty/fs_std.hpp"

#if defined(_WIN64) || defined(_WIN32)
	#ifndef NOMINMAX
		#define NOMINMAX
	#endif
	#include <windows.h>
#elif defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__)) // UNIX
	#include <sys/stat.h>
	#include <errno.h>
#endif

using namespace std;


float degToRad(float x)
{
	return static_cast<float>(x * M_PI / 180.);
}


float radToDeg(float x)
{
	return static_cast<float>(x * 180. / M_PI);
}


float cartesianToAngle(const Eigen::Vector2f& p)
{
	const float x = p(0);
	const float z = p(1);
	const float phi = atan2(z, x);
	return radToDeg(phi);
}


Eigen::Point2f angleToCartesian(const float angle_in_degrees)
{
	float angle_in_radians = degToRad(angle_in_degrees);
	return Eigen::Point2f(cosf(angle_in_radians), sinf(angle_in_radians));
}


string determinePathToSource()
{
	static string source_path;

	if (source_path.empty())
	{
		// First try to find the /src/ directory based on where the source code is.
		// This should work fine on development machines.
		fs::path path = getProjectSourcePath();
		error_code ec; // use an error code to avoid any exceptions below
		if (fs::exists(path, ec))
		{
			source_path = path.generic_string() + "/";
		}
		else
		{
			// Otherwise, look for the /src/ directory where the binary is located.
			// This is the standalone mode.
			fs::path srcPath = getBinaryPath().parent_path() / "src";
			source_path = srcPath.generic_string() + "/";
		}
	}

	return source_path;
}


string extractFileExtension(const string& file)
{
	return fs::path(file).extension().generic_string();
}


time_t getTimeStamp(const string& file)
{
	struct stat statStruct;
	stat(file.c_str(), &statStruct);
	return statStruct.st_mtime;
}


string addLineNumbersToString(const string& fileAsString)
{
	vector<string> strs = split(fileAsString, '\n'); // CR: untested
	stringstream sstr("");

	for (size_t i = 0; i < strs.size(); i++)
		sstr << i + 1 << ":\t\t" << strs[i] << "\n";

	return sstr.str();
}


void writeToFile(const string& filename, const string& contents)
{
	remove(filename.c_str());
	ofstream out(filename);
	out << contents;
	out.close();
}


string readFile(const string& filename)
{
	auto ss = ostringstream {};
	ifstream file(filename);
	ss << file.rdbuf();
	return ss.str();
}


bool removeFolder(string path)
{
	uintmax_t n = fs::remove_all(path); //(path / "abcdef");
	LOG(INFO) << "Deleted " << n << " files or directories";
	return true;
}


string createFilenameRelativeToFolder(string filename, string folder)
{
	return fs::relative(fs::path(filename), fs::path(folder)).generic_string();
}


bool pathStartsWith(string path, string prefix)
{
	// Replaces separators in the string with OS-specific separators.
	return startsWith(fs::path(path).string(), fs::path(prefix).string());
}


//This basically splits a path into [0..N-2] + [N-1], where indices are folders
void splitFilename(const string& filename, string* path, string* file)
{
	if (filename == "NULL")
		return;

	// Search for last "/" and check where there file extension is.
	size_t found = filename.find_last_of("/\\");
	*path = filename.substr(0, found) + "/";
	*file = filename.substr(found + 1);
}


string stripExtensionFromFilename(const string& file)
{
	string extension = extractFileExtension(file);
	size_t found = file.find(extension);
	return file.substr(0, found);
}


int extractLastNDigitsFromFilenameAsInt(const string& filename, int n)
{
	string extension = extractFileExtension(filename);
	size_t pos = filename.find(extension);
	string frameNumber = filename.substr(pos - n, n);
	return stoi(frameNumber);
}


string stripIncludeFromLine(const string& line)
{
	if (!contains(line, "#include "))
	{
		return line;
	}

	//0123456789
	//#include "

	return line.substr(9, line.length());
}


string removeAfterSequence(const string& line, const string& sequence)
{
	size_t foundComment = line.find(sequence);
	string ret = line.substr(0, foundComment);
	return ret;
}


bool contains(const string& line, const string& sequence)
{
	return ((line.find(sequence) == string::npos) ? false : true);
}


bool startsWith(const string& line, const string& substr)
{
	if (substr.size() >= line.size())
		return false;

	return line.rfind(substr, 0) == 0;
}


bool endsWith(const string& line, const string& substr)
{
	if (substr.size() >= line.size())
		return false;

	return line.compare(line.size() - substr.size(), substr.size(), substr) == 0;
}


string stripSymbolFromString(const string& line, char symbol)
{
	string newString = line;
	newString.erase(remove(newString.begin(), newString.end(), symbol), newString.end());
	return newString;
}


string replaceDelimiter(string in)
{
	string out = in;
	replace(out.begin(), out.end(), '\\', '/');
	return out;
}

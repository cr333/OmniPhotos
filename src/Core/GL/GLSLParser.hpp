#pragma once

#include <GL/gl3w.h>

#include <map>
#include <string>

class GLSLFile;


class GLSLParser
{
public:
	GLSLParser() = default;
	~GLSLParser() = default;

	void addGLSLfile(GLSLFile* glslFile);

	GLuint loadShaders(
	    const std::string& vertex_file_path,
	    const std::string& fragment_file_path);

	bool keyExists(const std::string& key) const;
	bool retrieveDatabaseEntry(const std::string& key, std::string& value) const;
	void removeDatabaseEntry(const std::string& key);

	static std::string stripFilenameToDBkey(const std::string& filename);


private:
	bool compileShaderSource(GLuint shaderID, const std::string& shader_file_path);

	// The database maps from filenames (without extension) to the actual source string.
	// Folder hierarchy might differ when files are nested. Names must stay unique.
	std::map<const std::string, const std::string> fileDB;
};

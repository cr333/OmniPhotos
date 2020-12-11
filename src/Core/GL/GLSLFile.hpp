#pragma once

#include <memory>
#include <string>
#include <vector>

class GLSLParser;


class GLSLFile
{
public:
	GLSLFile(const std::string& _filename);
	~GLSLFile() = default;

	// Parses files and register them in the Parser DB.
	int parse(std::shared_ptr<GLSLParser> parser, std::vector<std::string>* includedKeys, bool forceReload = false);

	const std::string getFilename() const;
	inline const std::string& getSource() const { return source; }

	bool exists = false;
	bool isLoaded = false;

	std::string key;

private:
	std::string filename;
	std::string source;
};

#include "GLSLFile.hpp"

#include "3rdParty/fs_std.hpp"

#include "Core/GL/GLSLParser.hpp"

#include "Utils/Exceptions.hpp"
#include "Utils/Logger.hpp"
#include "Utils/Utils.hpp"

#include <algorithm>
#include <fstream>

using namespace std;


GLSLFile::GLSLFile(const string& _filename) :
    filename(_filename)
{
	exists = fs::exists(filename);

	// Just a debug error for now as parse() below will throw an exception later.
	if (!exists)
		DLOG(ERROR) << "File '" << filename << "' could not be found.";
}


int GLSLFile::parse(shared_ptr<GLSLParser> parser, vector<string>* includedKeys, bool forceReload)
{
	if (forceReload)
		parser->removeDatabaseEntry(filename);

	bool leaf = true;
	if (!isLoaded || forceReload)
	{
		ifstream ifs(filename);

		if (!ifs.is_open())
		{
			std::string exception = "Could not open file: " + filename;
			RUNTIME_EXCEPTION(exception);
		}

		// Parse the shader line by line for #includes.
		string line;
		stringstream outStream;
		while (getline(ifs, line))
		{
			// Keep non-include lines as is (don't parse them).
			if (!startsWith(line, "#include "))
			{
				outStream << line << "\n";
				continue;
			}

			// Strip "#include " from the line.
			string fileName = line.substr(9, line.length());

			// Remove trailing comments.
			fileName = removeAfterSequence(fileName, "//");

			// Remove double quotes.
			fileName = stripSymbolFromString(fileName, '"');

			// Add source path to filename.
			fileName = determinePathToSource() + fileName;
			DLOG(INFO) << "Found shader include file: " << fileName;

			GLSLFile* newInclude = new GLSLFile(fileName);
			string key = parser->stripFilenameToDBkey(fileName);
			string shaderFileString;
			if (!forceReload)
			{
				// Include already exists in DB.
				if (parser->retrieveDatabaseEntry(key, shaderFileString))
				{
					ifs.close();
					return 0;
				}
			}

			// Okay, we've found an include, so this file is definitely not a leaf in the include tree.
			leaf = false;

			// Check if we have already included this #include file.
			if (std::find(includedKeys->begin(), includedKeys->end(), key) == includedKeys->end())
			{
				// Parse #include recursively.
				newInclude->parse(parser, includedKeys, forceReload);
				includedKeys->push_back(key);

				// Literally include the #include if it hasn't been included already.
				outStream << newInclude->getSource();
			}
		} // looping over lines in 'ifs'

		source = outStream.str();
		ifs.close();

		// A GLSL file with all other GLSL files "included".
		if (leaf)
		{
			parser->addGLSLfile(this);
		}
	}

	return 0;
}


const std::string GLSLFile::getFilename() const
{
	return fs::path(filename).filename().string();
}

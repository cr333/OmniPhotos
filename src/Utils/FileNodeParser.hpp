#pragma once

#include "Utils/Logger.hpp"

#include <opencv2/opencv.hpp>

#include <string>


// Reads a cv::FileNode of a given type and with a given default value if it is not set.
class FileNodeParser
{
public:
	// Reads a std::string value.
	static inline std::string read(const cv::FileNode& node, std::string defaultValue = "")
	{
		return _internalRead<std::string, cv::FileNode::STRING>(node, defaultValue);
	}

	// Reads a std::string value with a c-string default value for convenience.
	static std::string read(const cv::FileNode& node, const char* defaultValue = "")
	{
		return _internalRead<std::string, cv::FileNode::STRING>(node, std::string(defaultValue));
	}

	// Reads a Boolean value.
	static bool read(const cv::FileNode& node, bool defaultValue)
	{
		return 0 != _internalRead<int, cv::FileNode::INT>(node, (int)defaultValue);
	}

	// Reads an integer value.
	static int read(const cv::FileNode& node, int defaultValue)
	{
		return _internalRead<int, cv::FileNode::INT>(node, defaultValue);
	}

	// Reads an unsigned integer value.
	static unsigned int read(const cv::FileNode& node, unsigned int defaultValue)
	{
		return (unsigned int)_internalRead<int, cv::FileNode::INT>(node, defaultValue);
	}

	// Reads a float value.
	static float read(const cv::FileNode& node, float defaultValue)
	{
		return _internalRead<float, cv::FileNode::FLOAT>(node, defaultValue);
	}

	// Reads a double value.
	static double read(const cv::FileNode& node, double defaultValue)
	{
		return _internalRead<double, cv::FileNode::FLOAT>(node, defaultValue);
	}

	// Reads an OpenCV vector of varying type and length (defined by default value).
	template <typename Type, int Length>
	static cv::Vec<Type, Length> read(const cv::FileNode& node, cv::Vec<Type, Length> defaultValue)
	{
		return _internalRead<cv::Vec<Type, Length>, cv::FileNode::SEQ>(node, defaultValue);
	}

	// Reads an OpenCV matrix value of varying type (defined by default value).
	template <typename Type>
	static cv::Mat_<Type> read(const cv::FileNode& node, cv::Mat_<Type> defaultValue)
	{
		// First read in the matrix as a general cv::Mat.
		cv::Mat mat = _internalRead<cv::Mat_<Type>, cv::FileNode::MAP>(node, defaultValue, "opencv-matrix");

		// If the matrix type is correct, return the matrix as is.
		if (mat.type() == cv::DataType<Type>::type) return mat;

		// Else print a warning and convert to the correct type before returning.
		LOG(WARNING) << "Warning: '" << node.name() << "' is not of the expected type "
		             << cv::DataType<Type>::type << ". Converting data types explicitly.";
		cv::Mat_<Type> result;
		mat.convertTo(result, cv::DataType<Type>::type);
		return result;
	}


// OpenCV 3.0 breaks reading std::vector<std::string> because it is using cv::String instead of std::string :(
// TODO: OpenCV 4 uses cv::String == std::string, so this needs testing.
#if CV_VERSION_MAJOR >= 3

	// Reads a std::vector<std::string>.
	static std::vector<std::string> read(const cv::FileNode& node, std::vector<std::string> defaultValue)
	{
		// So first convert from std::string default values to cv::String.
		std::vector<cv::String> defaultValue_cv;
		defaultValue_cv.reserve(defaultValue.size());
		for (auto& s : defaultValue)
			defaultValue_cv.push_back(s);

		// Then read in a vector of cv::String.
		std::vector<cv::String> cv_strings = _internalRead<std::vector<cv::String>, cv::FileNode::SEQ>(node, defaultValue_cv);

		// And convert to a vector of std::string.
		std::vector<std::string> std_strings;
		std_strings.reserve(cv_strings.size());
		for (auto& s : cv_strings)
			std_strings.push_back(s);

		return std_strings;
	}

#endif // OpenCV 3


	// Reads a std::vector value of varying type (defined by default value).
	template <typename Type>
	static std::vector<Type> read(const cv::FileNode& node, std::vector<Type> defaultValue)
	{
		return _internalRead<std::vector<Type>, cv::FileNode::SEQ>(node, defaultValue);
	}


private:
	template <typename Type, int CVType>
	static Type _internalRead(const cv::FileNode& node, Type defaultValue)
	{
		if (node.empty()) return defaultValue;

		// Check if the FileNode has a compatible type.
		if (node.type() == cv::FileNode::INT && CVType == cv::FileNode::FLOAT)
		{
			// Reading an integer is fine for floating-point settings.
		}
		else if (node.type() != CVType)
		{
			// Print a warning for type mismatches.
			static const char types[7][16] = { "None", "Int", "Real", "String", "Ref", "Sequence", "Mapping" };
			if (CVType < 7)
				LOG(WARNING) << "Warning: \'" << node.name() << " is not of type \'"
				             << types[CVType] << "\'. Using default value.";
			else
				LOG(WARNING) << "Warning: \'" << node.name() << " is not of type \'"
				             << CVType << "\'. Using default value.";
			return defaultValue;
		}

		// Read in the value.
		Type value;
		node >> value;
		return value;
	}
};

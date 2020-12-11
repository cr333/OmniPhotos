#include "DepthIO.hpp"
#include "cvutils.hpp"
#include "Logger.hpp"


using namespace std;


// Reads a COLMAP depth map into an OpenCV matrix.
cv::Mat1f readColmapDepthMapFile(const char* filename)
{
	FILE* pFile = fopen(filename, "rb");
	long lSize;
	char* buffer;
	size_t result;

	if (pFile == 0)
	{
		LOG(WARNING) << "Error in readColmapDepthMapFile: could not open " << filename;
		return cv::Mat1f();
	}

	// obtain file size:
	fseek(pFile, 0, SEEK_END);
	lSize = ftell(pFile);
	rewind(pFile);

	// allocate memory to contain the whole file:
	buffer = (char*)malloc(sizeof(char) * lSize);
	if (buffer == nullptr)
	{
		LOG(WARNING) << "Memory error.";
		exit(2);
	}

	// copy the file into the buffer:
	result = fread(buffer, 1, lSize, pFile);
	if (result != lSize)
	{
		LOG(WARNING) << "Reading error.";
		exit(3);
	}

	/* the whole file is now loaded in the memory buffer. */
	//1. Read header
	std::vector<char> aVec;
	std::vector<char> bVec;
	std::vector<char> cVec;

	aVec.clear();
	bVec.clear();
	cVec.clear();

	long left = 0;
	int delim = 0;

	char byte;
	while (true)
	{
		byte = buffer[left];
		if (byte == '&')
		{
			if (delim > 1)
			{
				break;
			}
			delim++;
		}
		else
		{
			switch (delim)
			{
				case 0: aVec.push_back(byte); break;
				case 1: bVec.push_back(byte); break;
				case 2: cVec.push_back(byte); break;
			}
		}
		left++;
	}
	left++; // skip the last "&"
	aVec.push_back('\0');
	bVec.push_back('\0');
	cVec.push_back('\0');
	//convert each char[4] to string to int -> works.
	int width = stoi(string(aVec.data()));
	int height = stoi(string(bVec.data()));
	//printf("Width: %d, Height: %d \n", width, height);
	//printf("Width: %s, Height: %s \n", a, b);
	//int channels = stoi(string(cVec.data()));
	//int size = width*height*channels;

	//long howManyBytesLeft = lSize - left;
	int count = 0;
	cv::Mat1f img(height, width);
	float realFloat = 0.0f;
	for (long hmar = left; hmar < lSize; hmar += 4)
	{
		char charFloat[4];
		byte = buffer[hmar];
		charFloat[0] = (unsigned char)byte;
		byte = buffer[hmar + 1];
		charFloat[1] = (unsigned char)byte;
		byte = buffer[hmar + 2];
		charFloat[2] = (unsigned char)byte;
		byte = buffer[hmar + 3];
		charFloat[3] = (unsigned char)byte;
		memcpy(&realFloat, charFloat, 4);
		//printf("Pixel: %d = %f \n", count, realFloat);

		int x, y;
		y = count / width;
		x = count % width;
		//printf("(%d,%d) = %f \n", y, x, realFloat);
		count++;
		img.at<float>(y, x) = realFloat;
	}

	// terminate
	fclose(pFile);
	free(buffer);

	return img;
}

// Reads a COLMAP depth map into an OpenCV matrix.
cv::Mat1f readColmapDepthMapFile(std::string filename)
{
	return readColmapDepthMapFile(filename.c_str());
}


// Reads a depth map from a colour-encoded image into an OpenCV matrix.
cv::Mat1f readRGBDepthMapFile(const std::string& filename, float scale_factor)
{
	// Load the image as a floating-point image.
	const cv::Mat3f depth_map_image = cv::convert(cv::imread(filename), CV_32F);

	// Reconstruct the depth from the BGR (!) values in the image.
	const auto channels = cv::split(depth_map_image);
	cv::Mat1f depth_map = (channels[0] * 65536.f + channels[1] * 256.f + channels[2]) * scale_factor;

	return depth_map;
}


// Reads a .dpt file (Sintel format) into an OpenCV matrix.
cv::Mat1f readSintelDptFile(const char* filename)
{
	if (filename == nullptr)
	{
		LOG(WARNING) << "Error in readSintelDptFile: empty filename.";
		return cv::Mat1f();
	}

	FILE* stream = fopen(filename, "rb");
	if (stream == 0)
	{
		LOG(WARNING) << "Error in readSintelDptFile: could not open " << filename << ".";
		return cv::Mat1f();
	}

	int width, height;
	float tag;

	if ((int)fread(&tag, sizeof(float), 1, stream) != 1
	    || (int)fread(&width, sizeof(int), 1, stream) != 1
	    || (int)fread(&height, sizeof(int), 1, stream) != 1)
	{
		LOG(WARNING) << "Error in readSintelDptFile: problem reading file " << filename << ".";
		return cv::Mat1f();
	}

	if (tag != 202021.25) // simple test for correct endian-ness
	{
		LOG(WARNING) << "Error in readSintelDptFile(" << filename << "): wrong tag (possibly due to big-endian machine?).";
		return cv::Mat1f();
	}

	// another sanity check to see that integers were read correctly (99999 should do the trick...)
	if (width < 1 || width > 99999)
	{
		LOG(WARNING) << "Error in readSintelDptFile(" << filename << "): illegal width " << width << ".";
		return cv::Mat1f();
	}

	if (height < 1 || height > 99999)
	{
		LOG(WARNING) << "Error in readSintelDptFile(" << filename << "): illegal height" << height << ".";
		return cv::Mat1f();
	}

	cv::Mat1f img(height, width);

	const size_t pixels = (size_t)width * (size_t)height;
	if (fread(img.data, sizeof(float), pixels, stream) != pixels)
	{
		LOG(WARNING) << "Error in readSintelDptFile(" << filename << "): file is too short.";
		return cv::Mat1f();
	}

	if (fgetc(stream) != EOF)
	{
		LOG(WARNING) << "Error in readSintelDptFile(" << filename << "): file is too long.";
		return cv::Mat1f();
	}

	fclose(stream);
	return img;
}

// Reads a .dpt file (Sintel format) into an OpenCV matrix.
cv::Mat1f readSintelDptFile(std::string filename)
{
	return readSintelDptFile(filename.c_str());
}


// Writes a depth map to a .dpt file (Sintel format).
void writeSintelDptFile(const char* filename, cv::Mat1f img)
{
	if (filename == nullptr)
	{
		LOG(ERROR) << "Error in writeSintelDptFile: empty filename.";
		return;
	}

	const char* dot = strrchr(filename, '.');
	if (dot == nullptr)
	{
		LOG(ERROR) << "Error in writeSintelDptFile: extension required in filename " << filename << ".";
		return;
	}

	if (strcmp(dot, ".dpt") != 0)
	{
		LOG(ERROR) << "Error in writeSintelDptFile: filename " << filename << " should have extension '.dpt'.";
		return;
	}

	int width = img.cols, height = img.rows, nBands = img.channels();

	if (nBands != 1)
	{
		LOG(ERROR) << "Error in writeSintelDptFile(" << filename << "): image must have 1 channel.";
		return;
	}

	FILE* stream = fopen(filename, "wb");
	if (stream == 0)
	{
		LOG(ERROR) << "Error in writeSintelDptFile: could not open " << filename;
		return;
	}

	// write the header
	fprintf(stream, "PIEH");
	if ((int)fwrite(&width, sizeof(int), 1, stream) != 1
	    || (int)fwrite(&height, sizeof(int), 1, stream) != 1)
	{
		LOG(ERROR) << "Error in writeSintelDptFile(" << filename << "): problem writing header.";
		return;
	}

	// write the rows
	for (int y = 0; y < height; y++)
	{
		if ((int)fwrite(img.row(y).data, sizeof(float), width, stream) != width)
		{
			LOG(ERROR) << "Error in writeSintelDptFile(" << filename << "): problem writing data.";
			return;
		}
	}

	fclose(stream);
}

// Writes a depth map to a .dpt file (Sintel format).
void writeSintelDptFile(std::string filename, cv::Mat1f img)
{
	writeSintelDptFile(filename.c_str(), img);
}


// Writes a colour-mapped, normalised log depth map as an image.
void writeNormalisedLogDepthMap(std::string filename, const cv::Mat1f depth_map, std::string color_map)
{
	// Save normalised log depth map.
	cv::Mat1b log_depth_map = cv::normalize(cv::log(depth_map), 0, 255, cv::NORM_MINMAX, CV_8U);

	//imwrite(filename_prefix + "-log-norm.png", log_depth_map_cv);
	cv::imwrite(filename, cv::applyColorMap(log_depth_map, color_map));
}


// Writes a colour-mapped, log depth map (within a given range) as an image.
void writeLogDepthMap(std::string filename, const cv::Mat1f depth_map, float min_depth, float max_depth, std::string color_map)
{
	// Scaled log depth map.
	const auto log_depth_map_cv = cv::log(depth_map) / log(10.f);
	float log_min_depth = log(min_depth) / log(10.f);
	float log_depth_range = log(max_depth / min_depth) / log(10.f);
	cv::Mat1b scaled_depth_map_cv = cv::convert((log_depth_map_cv - log_min_depth) / log_depth_range, CV_8U, 255, 0);

	//imwrite(filename_prefix + "-log-scaled.png", scaled_depth_map_cv);
	imwrite(filename, cv::applyColorMap(scaled_depth_map_cv, color_map));
}

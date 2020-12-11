#include "FlowIO.hpp"
#include "IOTools.hpp"


// Reads a .flo file (Middlebury format) into an OpenCV matrix.
cv::Mat2f readFloFile(const char* filename)
{
	if (filename == nullptr)
	{
		LOG(WARNING) << "Error in readFloFile: empty filename.";
		return cv::Mat2f();
	}

	FILE* stream = fopen(filename, "rb");
	if (stream == 0)
	{
		LOG(WARNING) << "Error in readFloFile: could not open " << filename << ".";
		return cv::Mat2f();
	}

	int width, height;
	float tag;

	if ((int)fread(&tag, sizeof(float), 1, stream) != 1
	    || (int)fread(&width, sizeof(int), 1, stream) != 1
	    || (int)fread(&height, sizeof(int), 1, stream) != 1)
	{
		LOG(WARNING) << "Error in readFloFile: problem reading file " << filename << ".";
		return cv::Mat2f();
	}

	if (tag != 202021.25) // simple test for correct endian-ness
	{
		LOG(WARNING) << "Error in readFloFile(" << filename << "): wrong tag (possibly due to big-endian machine?).";
		return cv::Mat2f();
	}

	// another sanity check to see that integers were read correctly (99999 should do the trick...)
	if (width < 1 || width > 99999)
	{
		char msg[64];
		sprintf("Error in readFloFile(%s): illegal width %d.\n", filename, width);
		LOG(WARNING) << msg;
		return cv::Mat2f();
	}

	if (height < 1 || height > 99999)
	{
		char msg[64];
		sprintf("Error in readFloFile(%s): illegal height %d.\n", filename, height);
		LOG(WARNING) << msg;
		return cv::Mat2f();
	}

	cv::Mat2f img(height, width);

	if ((int)fread(img.data, 2 * sizeof(float), (size_t)(width * height), stream) != width * height)
	{
		LOG(WARNING) << "Error in readFloFile(" << filename << "): file is too short.";
		return cv::Mat2f();
	}

	if (fgetc(stream) != EOF)
	{
		LOG(WARNING) << "Error in readFloFile(" << filename << "): file is too long.";
		return cv::Mat2f();
	}

	fclose(stream);
	return img;
}

// Reads a .flo file (Middlebury format) into an OpenCV matrix.
cv::Mat2f readFloFile(std::string filename)
{
	return readFloFile(filename.c_str());
}


// Writes a flow field to a .flo file (Middlebury format).
void writeFloFile(const char* filename, cv::Mat2f img)
{
	if (filename == nullptr)
	{
		LOG(ERROR) << "Error in writeFloFile: empty filename.";
		return;
	}

	const char* dot = strrchr(filename, '.');
	if (dot == nullptr)
	{
		LOG(ERROR) << "Error in writeFloFile: extension required in filename " << filename << ".";
		return;
	}

	if (strcmp(dot, ".flo") != 0)
	{
		LOG(ERROR) << "Error in writeFloFile: filename \'" << filename << "\' should have extension '.flo'.";
		return;
	}

	int width = img.cols, height = img.rows, nBands = img.channels();

	if (nBands != 2)
	{
		LOG(ERROR) << "Error in writeFloFile(" << filename  << "): image must have 2 bands.";
		return;
	}

	FILE* stream = fopen(filename, "wb");
	if (stream == 0)
	{
		LOG(ERROR) << "Error in writeFloFile: could not open " << filename << ".";
		return;
	}

	// write the header
	fprintf(stream, "PIEH");
	if ((int)fwrite(&width, sizeof(int), 1, stream) != 1
	    || (int)fwrite(&height, sizeof(int), 1, stream) != 1)
	{
		LOG(ERROR) << "Error in writeFloFile(" << filename << "): problem writing header.";
		return;
	}

	// write the rows
	size_t n = (size_t)(nBands * width);
	for (int y = 0; y < height; y++)
	{
		if ((int)fwrite(img.row(y).data, sizeof(float), n, stream) != n)
		{
			LOG(ERROR) << "Error in writeFloFile(" << filename << "): problem writing data.";
			return;
		}
	}

	fclose(stream);
}

// Writes a flow field to a .flo file (Middlebury format).
void writeFloFile(std::string filename, cv::Mat2f img)
{
	writeFloFile(filename.c_str(), img);
}


// Reads a .floss file (16-bit fixed-point format) into an OpenCV matrix.
//
// This file format is based on the Middlebury .flo format, but instead of using float32 uses int16
// to store scaled flow values. Specifically, flow values are multiplied by 8 and rounded to the
// nearest signed 16-bit integer (aka signed short or 'ss'). This represents all flow values in
// [-2048, +2048] with a precision of 0.125 pixels, which should be good enough.
//
// Note that float16 (IEEE 754-2008) would only have a precision of a half pixel in [512, 1024],
// and only a full pixel in [1024, 2048].
cv::Mat2f readFlossFile(const std::string filename)
{
	if (filename.empty())
	{
		LOG(WARNING) << "Error in readFlossFile: empty filename.";
		return cv::Mat2f();
	}

	FILE* stream = fopen(filename.c_str(), "rb");
	if (stream == nullptr)
	{
		LOG(WARNING) << "Error in readFlossFile: could not open " << filename << ".";
		return cv::Mat2f();
	}

	int width, height;
	char tag[5] = { 0 };

	if (fread(&tag, sizeof(char), 4, stream) != 4u
	    || fread(&width, sizeof(int), 1, stream) != 1u
	    || fread(&height, sizeof(int), 1, stream) != 1u)
	{
		LOG(WARNING) << "Error in readFlossFile: problem reading file " << filename << ".";
		return cv::Mat2f();
	}

	if (strcmp(tag, "SHRT") != 0) // simple test for correct endian-ness
	{
		LOG(WARNING) << "Error in readFlossFile(" << filename << "): wrong tag (possibly due to big-endian machine?).";
		return cv::Mat2f();
	}

	// another sanity check to see that integers were read correctly (99999 should do the trick...)
	if (width < 1 || width > 99999)
	{
		LOG(WARNING) << "Error in readFlossFile(" << filename << "): illegal width " << width << ".";
		return cv::Mat2f();
	}

	if (height < 1 || height > 99999)
	{
		LOG(WARNING) << "Error in readFlossFile(" << filename << "): illegal height " << height << ".";
		return cv::Mat2f();
	}

	cv::Mat2s shortFlow(height, width);
	const size_t pixels = (size_t)width * (size_t)height;
	if (fread(shortFlow.data, 2 * sizeof(short), pixels, stream) != pixels)
	{
		LOG(WARNING) << "Error in readFlossFile(" << filename << "): file is too short.";
		return cv::Mat2f();
	}

	if (fgetc(stream) != EOF)
	{
		LOG(WARNING) << "Error in readFlossFile(" << filename << "): file is too long.";
		return cv::Mat2f();
	}

	fclose(stream);

	cv::Mat2f flow;
	shortFlow.convertTo(flow, CV_32F, 1. / 8., 0.);

	return flow;
}


// Writes a flow field to a .floss file (16-bit fixed-point format).
bool writeFlossFile(const std::string filename, cv::Mat2f flow)
{
	if (filename.empty())
	{
		LOG(WARNING) << "Error in writeFlossFile: empty filename.";
		return false;
	}

	const auto extension = getFileExtension(filename);
	if (extension != ".floss")
	{
		LOG(WARNING) << "Error in writeFlossFile: filename \'" << filename << "\' should have extension '.floss'.";
		return false;
	}

	const int width = flow.cols;
	const int height = flow.rows;
	const int nBands = flow.channels();

	if (nBands != 2)
	{
		LOG(WARNING) << "Error in writeFlossFile(" << filename << "): image must have 2 bands.";
		return false;
	}

	FILE* stream = fopen(filename.c_str(), "wb");
	if (stream == nullptr)
	{
		LOG(WARNING) << "Error in writeFlossFile: could not open " << filename << ".";
		return false;
	}

	// write the header
	fprintf(stream, "SHRT");
	if (fwrite(&width, sizeof(int), 1, stream) != 1u || fwrite(&height, sizeof(int), 1, stream) != 1u)
	{
		LOG(WARNING) << "Error in writeFlossFile(" << filename << "): problem writing header.";
		return false;
	}

	// Convert single-precision floating-point flow field to signed short (16-bit) fixed-point flow.
	cv::Mat2s shortFlow;
	flow.convertTo(shortFlow, CV_16S, 8., 0.);

	// write the rows
	const size_t n = (size_t)nBands * (size_t)width;
	for (int y = 0; y < height; y++)
	{
		if (fwrite(shortFlow.row(y).data, sizeof(short), n, stream) != n)
		{
			LOG(WARNING) << "Error in writeFlossFile(" << filename << "): problem writing data.";
			return false;
		}
	}

	fclose(stream);
	return true;
}


// Reads a .F file (Barron format) into an OpenCV matrix.
cv::Mat2f readBarronFile(const char* filename)
{
	if (filename == nullptr)
	{
		LOG(WARNING) << "Error in readBarronFile: empty filename.";
		return cv::Mat2f();
	}

	FILE* stream = fopen(filename, "rb");
	if (stream == 0)
	{
		LOG(WARNING) << "Error in readBarronFile: could not open " << filename << ".";
		return cv::Mat2f();
	}

	float crop_width, crop_height;      // size after crop
	float width, height;                // size without crop
	float crop_offset_x, crop_offset_y; // crop offset

	if (fread(&crop_width, sizeof(float), 1, stream) != 1u
	    || fread(&crop_height, sizeof(float), 1, stream) != 1u
	    || fread(&width, sizeof(float), 1, stream) != 1u
	    || fread(&height, sizeof(float), 1, stream) != 1u
	    || fread(&crop_offset_x, sizeof(float), 1, stream) != 1u
	    || fread(&crop_offset_y, sizeof(float), 1, stream) != 1u)
	{
		LOG(WARNING) << "Error in readBarronFile: problem reading header of file " << filename << ".";
		return cv::Mat2f();
	}

	// sanity check to see that integers were read correctly (99999 should do the trick...)
	if (width < 1 || width > 99999)
	{
		char msg[128];
		sprintf("Error in readBarronFile(%s): illegal width %f.\n", filename, width);
		LOG(WARNING) << msg;
		return cv::Mat2f();
	}

	if (height < 1 || height > 99999)
	{
		char msg[128];
		sprintf("Error in readBarronFile(%s): illegal height %f.\n", filename, height);
		LOG(WARNING) << msg;
		return cv::Mat2f();
	}

	cv::Mat2f img((int)height, (int)width);

	if ((int)fread(img.data, 2 * sizeof(float), (size_t)(width * height), stream) != (int)(width * height))
	{
		LOG(WARNING) << "Error in readBarronFile(" << filename << "): file is too short.";
		return cv::Mat2f();
	}

	if (fgetc(stream) != EOF)
	{
		LOG(WARNING) << "Error in readBarronFile(" << filename << "): file is too long.";
		return cv::Mat2f();
	}

	fclose(stream);
	return img;
}

// Reads a .F file (Barron format) into an OpenCV matrix.
cv::Mat2f readBarronFile(std::string filename)
{
	return readBarronFile(filename.c_str());
}


// Writes a flow field to a .F file (Barron format).
void writeBarronFile(const char* filename, cv::Mat2f img)
{
	if (filename == nullptr)
	{
		LOG(ERROR) << "Error in writeBarronFile: empty filename.";
		return;
	}

	const char* dot = strrchr(filename, '.');
	if (dot == nullptr)
	{
		LOG(ERROR) << "Error in writeBarronFile: extension required in filename " << filename << ".";
		return;
	}

	if (strcmp(dot, ".F") != 0)
	{
		LOG(ERROR) << "Error in writeBarronFile: filename '" << filename << "' should have extension '.F'.";
		return;
	}

	float width = (float)img.cols, height = (float)img.rows;
	int nBands = img.channels();

	if (nBands != 2)
	{
		LOG(ERROR) << "Error in writeBarronFile(" << filename << "): image must have 2 bands.";
		return;
	}

	FILE* stream = fopen(filename, "wb");
	if (stream == 0)
	{
		LOG(ERROR) << "Error in writeBarronFile: could not open " << filename << ".";
		return;
	}

	// write the header
	float zero = 0.0f;
	if (fwrite(&width, sizeof(float), 1, stream) != 1u
	    || fwrite(&height, sizeof(float), 1, stream) != 1u
	    || fwrite(&width, sizeof(float), 1, stream) != 1u
	    || fwrite(&height, sizeof(float), 1, stream) != 1u
	    || fwrite(&zero, sizeof(float), 1, stream) != 1u
	    || fwrite(&zero, sizeof(float), 1, stream) != 1u)
	{
		LOG(ERROR) << "Error in writeBarronFile(" << filename << "): problem writing header.";
		return;
	}

	if ((int)fwrite(img.data, 2 * sizeof(float), (size_t)(width * height), stream) != (int)(width * height))
	{
		LOG(ERROR) << "Error in writeBarronFile(" << filename << "): problem writing data.";
		return;
	}

	fclose(stream);
}

// Writes a flow field to a .F file (Barron format).
void writeBarronFile(std::string filename, cv::Mat2f img)
{
	writeBarronFile(filename.c_str(), img);
}


// Reads a flow file (format depending on file extension) into an OpenCV matrix.
cv::Mat2f readFlowFile(const std::string filename)
{
	std::string extension = getFileExtension(filename);
	if (extension.empty())
	{
		LOG(WARNING) << "Error in readFlowFile: extension required in filename \'" << filename << "\'.";
		return cv::Mat2f();
	}

	// Use extension to pick reading function.
	if (extension == ".flo")   return readFloFile(filename);
	if (extension == ".floss") return readFlossFile(filename);
	if (extension == ".F")     return readBarronFile(filename);

	LOG(WARNING) << "Error in readFlowFile: extension \'" << extension << "\' not supported";
	return cv::Mat2f();
}


// Writes a flow field to a file (format depending on file extension).
void writeFlowFile(const std::string filename, cv::Mat2f flow)
{
	std::string extension = getFileExtension(filename);
	if (extension.empty())
	{
		LOG(ERROR) << "Error in writeFlowFile: extension required in filename \'" << filename << "\'.";
		return;
	}

	// Use extension to pick reading function.
	if      (extension == ".flo")   writeFloFile(filename, flow);
	else if (extension == ".floss") writeFlossFile(filename, flow);
	else if (extension == ".F")     writeBarronFile(filename, flow);
	else LOG(ERROR) << "Error in writeFlowFile: extension \'" << extension << "\' not supported.";
}


// Converts flow (relative displacements) to absolute coordinates suitable for cv::remap.
cv::Mat2f convertFlowToAbsolute(const cv::Mat2f flow)
{
	cv::Mat2f output(flow.rows, flow.cols);

	for (int y = 0; y < flow.rows; y++)
	{
		for (int x = 0; x < flow.cols; x++)
		{
			output[y][x] = cv::Vec2f((float)x, (float)y) + flow[y][x];
		}
	}

	return output;
}


// Converts flow (relative displacements) to absolute coordinates suitable for cv::remap.
cv::Mat2f convertFlowToRelative(const cv::Mat2f flow)
{
	cv::Mat2f output(flow.rows, flow.cols);

	for (int y = 0; y < flow.rows; y++)
	{
		for (int x = 0; x < flow.cols; x++)
		{
			output[y][x] = -cv::Vec2f((float)x, (float)y) + flow[y][x];
		}
	}

	return output;
}

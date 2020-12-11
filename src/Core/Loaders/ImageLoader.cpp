#include "ImageLoader.hpp"

#define STB_DXT_IMPLEMENTATION
#include "3rdParty/stb_dxt.h"

#include "Core/GL/GLFormats.hpp"
#include "Utils/ErrorChecking.hpp"
#include "Utils/Exceptions.hpp"
#include "Utils/Logger.hpp"
#include "Utils/Timer.hpp"
#include "Utils/Utils.hpp"
#include "Utils/cvutils.hpp"

#ifdef _OPENMP
	#include <omp.h>
#endif //_OPENMP

#include <iomanip>


// Initialise static member variable.
std::vector<std::string> ImageLoader::supportedTextureFormats;


ImageLoader::ImageLoader(std::vector<Camera*>* _cameras, bool _enableTextureCompression) :
    Loader(_cameras),
    enableTextureCompression(_enableTextureCompression)
{
	// Query the supported compressed texture formats and store if DXT1/DXT5 is supported.
	// Following the suggestion at https://www.khronos.org/opengl/wiki/Common_Mistakes,
	// we do this just once, to call `glGetIntegerv` as little as possible, and store
	// the result in the static variable `ImageLoader::supportedTextureFormats`.
	if (enableTextureCompression && supportedTextureFormats.empty())
	{
		// Uncompressed textures are always supported.
		supportedTextureFormats.push_back("GL_RGB");

		// Check the graphics card's GL_COMPRESSED_RGBA_S3TC_DXT5/1_EXT support.
		GLint numSupportedFormats = 0;
		glGetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS, &numSupportedFormats);
		if (numSupportedFormats <= 0)
			LOG(ERROR) << "Graphic card feature detection error.";

		// List supported texture compression formats and save the one's we're interested in.
		GLint* supportedFormatsList = new GLint[numSupportedFormats];
		glGetIntegerv(GL_COMPRESSED_TEXTURE_FORMATS, supportedFormatsList);
		for (int i = 0; i < numSupportedFormats; i++)
		{
			if (GL_COMPRESSED_RGBA_S3TC_DXT5_EXT == supportedFormatsList[i])
				supportedTextureFormats.push_back("DXT5");
			else if (GL_COMPRESSED_RGBA_S3TC_DXT1_EXT == supportedFormatsList[i])
				supportedTextureFormats.push_back("DXT1");
			else if (GL_COMPRESSED_RGB_S3TC_DXT1_NV == supportedFormatsList[i])
				supportedTextureFormats.push_back("DXT1");
		}
		delete[] supportedFormatsList;

		std::string msg;
		for (auto& it : supportedTextureFormats)
			msg = msg + " " + it;
		VLOG(1) << "Current graphics card '" << glGetString(GL_RENDERER) << "' supports the following texture compression formats:" << msg;
	}
}


ImageLoader::~ImageLoader()
{
	releaseGPUMemory();
}


// @param image 4-channel RGBA image
// @param alphaEnable 0 is DXT1, 1 is DXT5
void compressTextureInPlace(cv::Mat& image, int alphaEnable)
{
	if (image.channels() != 4)
	{
		LOG(ERROR) << "The DXT1/5 texture compression expects 4-channel RGBA input.";
		return;
	}

	// Note the DTX1 just uses half of the mat's memory.
	cv::Mat imageDXT = cv::Mat::zeros(image.rows, image.cols, CV_8UC1);
	if (!imageDXT.isContinuous())
		LOG(ERROR) << "OpenCV failed to allocate contiguous memory.";

	unsigned char* outputDataPtr = imageDXT.data;
	const unsigned int blockWidth = image.cols / 4;
	const unsigned int blockHeight = image.rows / 4;

	// Block-wise DXT1/DXT5 image compression.
	for (unsigned int j = 0; j < blockHeight; j++)
	{
		for (unsigned int i = 0; i < blockWidth; i++)
		{
			// Extract 4-by-4 pixel block.
			unsigned char block[64];
			for (int blockRowIndex = 0; blockRowIndex < 4; blockRowIndex++)
			{
				const int blockOffset = (j * 4 + blockRowIndex) * (blockWidth * 4 * 4) + i * 4 * 4;
				memcpy(&block[blockRowIndex * 4 * 4], image.data + blockOffset, 4 * 4);
			}

			// Compress the block.
			if (alphaEnable == 1) // DXT5
			{
				stb_compress_dxt_block(outputDataPtr, block, 1, STB_DXT_HIGHQUAL);
				outputDataPtr += 16;
			}
			else if (alphaEnable == 0) // DXT1
			{
				stb_compress_dxt_block(outputDataPtr, block, 0, STB_DXT_HIGHQUAL);
				outputDataPtr += 8;
			}
		}
	}

	image = imageDXT;
}


bool ImageLoader::checkTextureFormat()
{
	if (enableTextureCompression == false && imageTextureFormat != "GL_RGB")
	{
		std::string msg = "Error: Texture compression is disabled but texture format is not GL_RGB.";
		RUNTIME_EXCEPTION(msg);
	}

	if (imageTextureFormat != "GL_RGB" && imageTextureFormat != "DXT1" && imageTextureFormat != "DXT5")
	{
		std::string msg = "Error: Image texture format should be [GL_RGB|DXT1|DXT5].";
		RUNTIME_EXCEPTION(msg);
	}

	// Check if the GPU supports the selected format and fall back to uncompressed if not.
	if (imageTextureFormat == "DXT1" || imageTextureFormat == "DXT5")
	{
		// check the graphic card GL_COMPRESSED_RGBA_S3TC_DXT5_EXT support
		bool textureCompressSupport = false;
		for (int i = 0; i < ImageLoader::supportedTextureFormats.size(); i++)
		{
			if (imageTextureFormat == ImageLoader::supportedTextureFormats[i])
			{
				textureCompressSupport = true;
				break;
			}
		}

		if (textureCompressSupport)
		{
			VLOG(1) << "Current graphics card supports " << imageTextureFormat << " textures.";
		}
		else
		{
			LOG(WARNING) << imageTextureFormat << " texture compression is not supported. Disabling texture compression.";
			imageTextureFormat = "GL_RGB";
		}
	}

	// Check if image dimensions are supported (width/height a multiple of 4).
	if (imageTextureFormat == "DXT1" || imageTextureFormat == "DXT5")
	{
		cv::Mat image = (*cameras)[0]->getImage();
		if (image.rows % 4 != 0 || image.cols % 4 != 0)
		{
			LOG(WARNING) << "The RGB image size is " << image.size() << ". DXT1/5 texture compression requires the image size to be a multiple of 4. Disabling texture compression.";
			imageTextureFormat = "GL_RGB";
		}
	}

	return true;
}


bool ImageLoader::loadImages()
{
	if (!checkTextureFormat())
		return false;

	ScopedTimer timer;
	int imagesLoaded = 0;
	std::vector<Camera*>& camera_list = *cameras;

#ifdef _OPENMP
	#pragma omp parallel for shared(imagesLoaded) schedule(static, 1)
#endif
	for (int i = 0; i < camera_list.size(); ++i)
	{
		LOG(INFO) << "Loading image (" << (i + 1) << " of " << camera_list.size() << ")";
		Camera* camera = camera_list[i];
		if (camera->loadImageWithOpenCV())
		{
#ifdef _OPENMP
	#pragma omp atomic
#endif
			imagesLoaded++;

			// Compress images if texture compression using DXT1/5 is enabled and supported.
			if (imageTextureFormat == "DXT1" || imageTextureFormat == "DXT5")
			{
				LOG(INFO) << "Compressing texture image (" << (i + 1) << " of " << camera_list.size() << ")";

				// The input for texture compression must be RGBA.
				cv::Mat image = camera_list[i]->getImage();
				if (image.channels() == 3)
					cv::cvtColor(image, image, cv::COLOR_BGR2RGBA);

				if (imageTextureFormat == "DXT1")
					compressTextureInPlace(image, 0);
				else if (imageTextureFormat == "DXT5")
					compressTextureInPlace(image, 1);

				camera_list[i]->setImage(image);
			}
		}
		else
		{
			LOG(WARNING) << "Loading image '" << camera->imageName << "' failed.";
		}
	}

	LOG(INFO) << "Loaded " << imagesLoaded << " images in " << std::fixed << std::setprecision(2) << timer.getElapsedSeconds() << "s";
	return imagesLoaded == getImageCount();
}


void ImageLoader::loadTextures()
{
	if (!loadImages())
		LOG(ERROR) << "Failed to load all images";
}


void ImageLoader::initTextures()
{
	ErrorChecking::checkGLError();
	if (imageTexture)
	{
		imageTexture->layout.mem.elements = getImageCount();
	}
	else
	{
		GLMemoryLayout memLayout;
		if (imageTextureFormat == "GL_RGB")
			memLayout = GLMemoryLayout(getImageCount(), 3, "GL_BGR", "GL_UNSIGNED_BYTE", "GL_RGB8");
		else if (imageTextureFormat == "DXT5")
			memLayout = GLMemoryLayout(getImageCount(), 3, "GL_RGBA_DXT5", "", "GL_RGBA_DXT5");
		else if (imageTextureFormat == "DXT1")
			memLayout = GLMemoryLayout(getImageCount(), 3, "GL_RGBA_DXT1", "", "GL_RGBA_DXT1");

		GLTextureLayout texLayout = GLTextureLayout(memLayout, getImageDims(), "set in app", -1);
		imageTexture = new GLTexture(texLayout, "GL_TEXTURE_2D_ARRAY");
	}
	ErrorChecking::checkGLError();

	//setting up image textures
	if (!imageTexture->gl_ID)
	{
		glGenTextures(1, &imageTexture->gl_ID);
		ErrorChecking::checkGLError();
	}

	glBindTexture(GL_TEXTURE_2D_ARRAY, imageTexture->gl_ID);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	if (imageTextureFormat == "GL_RGB")
	{
		glTexImage3D(GL_TEXTURE_2D_ARRAY,
		             0, //level
		             getGLInternalFormat(imageTexture->layout.mem.internalFormat),
		             imageTexture->layout.resolution.x(),
		             imageTexture->layout.resolution.y(),
		             imageTexture->layout.mem.elements,
		             0, //border
		             getGLFormat(imageTexture->layout.mem.format),
		             getGLType(imageTexture->layout.mem.type),
		             NULL);
	}
	else if (imageTextureFormat == "DXT1" || imageTextureFormat == "DXT5")
	{
		// DXT1/DXT5 texture size unit is Byte
		unsigned int dxtDataSize = 0;
		if (imageTextureFormat == "DXT5")
			dxtDataSize = ((imageTexture->layout.resolution.x() + 3) / 4) * ((imageTexture->layout.resolution.y() + 3) / 4) * 16;
		else if (imageTextureFormat == "DXT1")
			dxtDataSize = ((imageTexture->layout.resolution.x() + 3) / 4) * ((imageTexture->layout.resolution.y() + 3) / 4) * 8;

		glCompressedTexImage3D(GL_TEXTURE_2D_ARRAY,
		                       0, //level
		                       getGLInternalFormat(imageTexture->layout.mem.internalFormat),
		                       imageTexture->layout.resolution.x(), // block's width to image width
		                       imageTexture->layout.resolution.y(), // block's height to image height
		                       imageTexture->layout.mem.elements,   // slice number
		                       0,
		                       dxtDataSize * imageTexture->layout.mem.elements,
		                       NULL);
	}
	ErrorChecking::checkGLError();

	if (projectionMatrixTexture)
	{
		projectionMatrixTexture->layout.mem.elements = getImageCount();
	}
	else
	{
		GLMemoryLayout memLayout = GLMemoryLayout(getImageCount(), 1, "GL_RED", "GL_FLOAT", "GL_R32F"); // these are settings from config.yaml
		GLTextureLayout texLayout = GLTextureLayout(memLayout, Eigen::Vector2i(4, 4), "set in app", -1);
		projectionMatrixTexture = new GLTexture(texLayout, "GL_TEXTURE_2D_ARRAY");
	}

	// Setting up projection matrices
	if (!projectionMatrixTexture->gl_ID)
		glGenTextures(1, &projectionMatrixTexture->gl_ID);

	glBindTexture(GL_TEXTURE_2D_ARRAY, projectionMatrixTexture->gl_ID);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage3D(GL_TEXTURE_2D_ARRAY,
	             0,
	             getGLInternalFormat(projectionMatrixTexture->layout.mem.internalFormat),
	             projectionMatrixTexture->layout.resolution.x(),
	             projectionMatrixTexture->layout.resolution.y(),
	             projectionMatrixTexture->layout.mem.elements,
	             0,
	             getGLFormat(projectionMatrixTexture->layout.mem.format),
	             getGLType(projectionMatrixTexture->layout.mem.type),
	             NULL);
	ErrorChecking::checkGLError();

	if (posViewTexture)
	{
		posViewTexture->layout.mem.elements = getImageCount();
		posViewTexture->layout.resolution = Eigen::Vector2i(2, getImageCount());
	}
	else
	{
		GLMemoryLayout memLayout = GLMemoryLayout(getImageCount(), 3, "GL_RGB", "GL_FLOAT", "GL_RGB16F"); // these are settings from config.yaml
		GLTextureLayout texLayout = GLTextureLayout(memLayout, Eigen::Vector2i(2, getImageCount()), "set in app", -1);
		posViewTexture = new GLTexture(texLayout, "GL_TEXTURE_2D");
	}

	if (!posViewTexture->gl_ID)
		glGenTextures(1, &posViewTexture->gl_ID);

	glBindTexture(GL_TEXTURE_2D, posViewTexture->gl_ID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D,
	             0,
	             getGLInternalFormat(posViewTexture->layout.mem.internalFormat),
	             posViewTexture->layout.resolution.x(),
	             posViewTexture->layout.resolution.y(),
	             0,
	             getGLFormat(posViewTexture->layout.mem.format),
	             getGLType(posViewTexture->layout.mem.type),
	             NULL);
	ErrorChecking::checkGLError();
}


void ImageLoader::fillTextures()
{
	ScopedTimer timer;
	for (int i = 0; i < getImageCount(); i++)
	{
		VLOG(1) << "Filling image texture (" << (i + 1) << " of " << getImageCount() << ")";
		Camera* cam = cameras->at(i);

		if (imageTextureFormat == "GL_RGB")
		{
			fillCameraTextureOpenCV(cam, i);
		}
		else if (imageTextureFormat == "DXT1" || imageTextureFormat == "DXT5")
		{
			cv::Mat img = cam->getImage();
			if (img.empty())
			{
				LOG(WARNING) << "Image '" << cam->imageName << "' is empty. Skipping texture upload.";
				return;
			}

			unsigned int dxtDataSize = 0;
			if (imageTextureFormat == "DXT5")
				dxtDataSize = ((img.cols + 3) / 4) * ((img.rows + 3) / 4) * 16;
			else if (imageTextureFormat == "DXT1")
				dxtDataSize = ((img.cols + 3) / 4) * ((img.rows + 3) / 4) * 8;

			glBindTexture(GL_TEXTURE_2D_ARRAY, imageTexture->gl_ID);
			glCompressedTexSubImage3D(GL_TEXTURE_2D_ARRAY,
			                          0,
			                          0,
			                          0,
			                          i,
			                          imageTexture->layout.resolution.x(), // block's width to image width
			                          imageTexture->layout.resolution.y(), // block's height to image height
			                          1,
			                          getGLFormat(imageTexture->layout.mem.format), // GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
			                          dxtDataSize,
			                          img.data);
		}

		// projection matrices
		fillProjectionTexture(cam, i);
	}

	//glBindTexture(GL_TEXTURE_2D_ARRAY, images);
	//glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
	ErrorChecking::checkGLError();

	LOG(INFO) << "Loaded images from CPU memory to GPU memory in "
	          << std::fixed << std::setprecision(2) << timer.getElapsedSeconds() << "s";

	updatePosViewTex();
}


void ImageLoader::releaseCPUMemory()
{
	// Images are stored inside Camera instances and not released here.
}


void ImageLoader::releaseGPUMemory()
{
	if (imageTexture)
	{
		delete imageTexture;
		imageTexture = nullptr;
	}

	if (projectionMatrixTexture)
	{
		delete projectionMatrixTexture;
		projectionMatrixTexture = nullptr;
	}

	if (posViewTexture)
	{
		delete posViewTexture;
		posViewTexture = nullptr;
	}
}


void ImageLoader::fillProjectionTexture(Camera* cam, int layer)
{
	Eigen::Matrix4f mat = cam->getProjection44();
	GLfloat* data = new GLfloat[4 * 4];
	memset(data, 0, 4 * 4 * sizeof(GLfloat));

	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 4; j++)
			data[i * 4 + j + 0] = (GLfloat)mat(i, j);

	glBindTexture(GL_TEXTURE_2D_ARRAY, projectionMatrixTexture->gl_ID);
	glTexSubImage3D(GL_TEXTURE_2D_ARRAY,
	                0,
	                0, 0, layer,
	                4, 4, 1,
	                GL_RED,
	                GL_FLOAT,
	                data);
	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
	delete[] data;
	ErrorChecking::checkGLError();
}


void ImageLoader::fillCameraTextureOpenCV(Camera* cam, int layer)
{
	cv::Mat img = cam->getImage();

	if (img.empty())
	{
		LOG(WARNING) << "Image '" << cam->imageName << "' is empty. Skipping texture upload.";
		return;
	}

	uploadImageToOpenGLTexture(img, imageTexture, layer);
}


void ImageLoader::uploadImageToOpenGLTexture(cv::Mat& img, GLTexture* texture, int layer)
{
	glBindTexture(GL_TEXTURE_2D_ARRAY, texture->gl_ID);
	glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, layer, img.cols, img.rows, 1,
	                getGLFormat(texture->layout.mem.format), // GL_RGB
	                getGLType(texture->layout.mem.type),     // GL_UNSIGNED_CHAR
	                (uchar*)img.ptr());
	//ErrorChecking::checkGLError();
}


void ImageLoader::updateProjectionTex()
{
	LOG(INFO) << "Update projection matrix textures.";
	int size = getImageCount();
	for (int i = 0; i < size; i++)
	{
		Camera* cam = cameras->at(i);
		fillProjectionTexture(cam, i);
		//ErrorChecking::checkGLError();
	}
}


void ImageLoader::setPosViewTex(int layer)
{
	const Camera* cam = cameras->at(layer);
	glTexSubImage2D(GL_TEXTURE_2D,
	                0,
	                0, layer,
	                1, 1,
	                GL_RGB, GL_FLOAT, cam->getCentre().data());
	ErrorChecking::checkGLError();
	glTexSubImage2D(GL_TEXTURE_2D,
	                0,
	                1, layer,
	                1, 1,
	                GL_RGB, GL_FLOAT, cam->getZDir().data());
	ErrorChecking::checkGLError();
}


void ImageLoader::updateSinglePosViewTex(int layer)
{
	glBindTexture(GL_TEXTURE_2D, posViewTexture->gl_ID);
	setPosViewTex(layer);
	glBindTexture(GL_TEXTURE_2D, 0);
}


void ImageLoader::updatePosViewTex()
{
	VLOG(1) << "Updating position and viewing directions texture.";

	glBindTexture(GL_TEXTURE_2D, posViewTexture->gl_ID);
	//glTexStorage2D(GL_TEXTURE_2D, 1, formats.er->internalFormat, 2, size);
	ErrorChecking::checkGLError();

	int size = posViewTexture->layout.mem.elements;
	for (int i = 0; i < size; i++)
	{
		setPosViewTex(i);
	}

	glBindTexture(GL_TEXTURE_2D, 0);
	ErrorChecking::checkGLError();
}

#include "FlowLoader.hpp"

#include "3rdParty/fs_std.hpp"

#include "Core/GL/GLFormats.hpp"

#include "Utils/ErrorChecking.hpp"
#include "Utils/FlowIO.hpp"
#include "Utils/Logger.hpp"
#include "Utils/Timer.hpp"
#include "Utils/cvutils.hpp"


FlowLoader::FlowLoader(bool _downsampleFlow, const Eigen::Vector2i& _imgDims)
{
	downsampleFlow = _downsampleFlow;
	imgDims = _imgDims;
	numberOfFlows = 1;
}


FlowLoader::FlowLoader(bool _downsampleFlow, const Eigen::Vector2i& _imgDims,
                       std::vector<std::string>* _forwardFlowFiles,
                       std::vector<std::string>* _backwardFlowFiles) :
    FlowLoader(_downsampleFlow, _imgDims)
{
	if (_forwardFlowFiles != nullptr && _backwardFlowFiles != nullptr)
	{
		assert(_forwardFlowFiles->size() == _backwardFlowFiles->size());

		for (size_t i = 0; i < _forwardFlowFiles->size(); i++)
		{
			forwardFlowFiles.push_back(_forwardFlowFiles->at(i));
			backwardFlowFiles.push_back(_backwardFlowFiles->at(i));
		}
		numberOfFlows = (int)_forwardFlowFiles->size();
	}

	if (!forwardFlows)
		forwardFlows = new std::vector<cv::Mat>(numberOfFlows);

	if (!backwardFlows)
		backwardFlows = new std::vector<cv::Mat>(numberOfFlows);
}


FlowLoader::~FlowLoader()
{
	releaseCPUMemory();
	releaseGPUMemory();
}


void FlowLoader::loadTextures()
{
	if (forwardFlowFiles.size() > 0 && backwardFlowFiles.size() > 0)
	{
		assert(forwardFlowFiles.size() == backwardFlowFiles.size());
		ScopedTimer timer;

		std::vector<cv::Mat>* forwardFlows_list = this->forwardFlows;
		std::vector<cv::Mat>* backwardFlows_list = this->backwardFlows;

#ifdef _OPENMP
	#pragma omp parallel for shared(forwardFlows_list, backwardFlows_list) schedule(static, 1)
#endif
		for (int i = 0; i < forwardFlowFiles.size(); i++)
		{
			LOG(INFO) << "Loading flow (" << (i + 1) << " of " << forwardFlowFiles.size() << ")";

			cv::Mat2f forwardFlow = readFlowFile(forwardFlowFiles[i]);
			cv::Mat2f backwardFlow = readFlowFile(backwardFlowFiles[i]);

			forwardFlows_list->at(i) = forwardFlow;
			backwardFlows_list->at(i) = backwardFlow;
		}

		LOG(INFO) << "Loaded " << (forwardFlowFiles.size() + backwardFlowFiles.size()) << " flow fields in "
		          << std::fixed << std::setprecision(2) << timer.getElapsedSeconds() << "s";
	}
}


void FlowLoader::initTextures()
{
	Eigen::Vector2i flowDims = imgDims;
	if (downsampleFlow)
		flowDims /= 2;

	// Forward flow
	ErrorChecking::checkGLError();
	if (forwardFlowTexture)
	{
		forwardFlowTexture->layout.mem.elements = numberOfFlows;
	}
	else
	{
		GLMemoryLayout memLayout = GLMemoryLayout(numberOfFlows, 2, "GL_RG", "GL_FLOAT", "GL_RG16F"); // these are settings from config.yaml
		GLTextureLayout texLayout = GLTextureLayout(memLayout, flowDims, "set in app", -1);
		forwardFlowTexture = new GLTexture(texLayout, "GL_TEXTURE_2D_ARRAY");
	}
	ErrorChecking::checkGLError();

	if (!forwardFlowTexture->gl_ID)
		glGenTextures(1, &forwardFlowTexture->gl_ID);
	ErrorChecking::checkGLError();

	glBindTexture(GL_TEXTURE_2D_ARRAY, forwardFlowTexture->gl_ID);
	ErrorChecking::checkGLError();

	glTexStorage3D(GL_TEXTURE_2D_ARRAY,
	               1,
	               getGLInternalFormat(forwardFlowTexture->layout.mem.internalFormat), // "GL_RG16F"
	               forwardFlowTexture->layout.resolution.x(),
	               forwardFlowTexture->layout.resolution.y(),
	               (GLsizei)forwardFlowTexture->layout.mem.elements);
	ErrorChecking::checkGLError();

	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	ErrorChecking::checkGLError();

	// Backward flow
	if (backwardFlowTexture)
	{
		backwardFlowTexture->layout.mem.elements = numberOfFlows;
	}
	else
	{
		GLMemoryLayout memLayout = GLMemoryLayout(numberOfFlows, 2, "GL_RG", "GL_FLOAT", "GL_RG16F"); // these are settings from config.yaml
		GLTextureLayout texLayout = GLTextureLayout(memLayout, flowDims, "set in app", -1);
		backwardFlowTexture = new GLTexture(texLayout, "GL_TEXTURE_2D_ARRAY");
	}
	ErrorChecking::checkGLError();

	if (!backwardFlowTexture->gl_ID)
		glGenTextures(1, &backwardFlowTexture->gl_ID);
	ErrorChecking::checkGLError();

	glBindTexture(GL_TEXTURE_2D_ARRAY, backwardFlowTexture->gl_ID);
	ErrorChecking::checkGLError();

	glTexStorage3D(GL_TEXTURE_2D_ARRAY,
	               1,
	               getGLInternalFormat(backwardFlowTexture->layout.mem.internalFormat), // "GL_RG16F"
	               backwardFlowTexture->layout.resolution.x(),
	               backwardFlowTexture->layout.resolution.y(),
	               (GLsizei)backwardFlowTexture->layout.mem.elements);
	ErrorChecking::checkGLError();

	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	ErrorChecking::checkGLError();
}


void FlowLoader::fillTextures()
{
	ScopedTimer timer;
	if (forwardFlowFiles.size() > 0)
	{
		assert(forwardFlowFiles.size() == backwardFlowFiles.size());

		for (int i = 0; i < forwardFlowFiles.size(); i++)
		{
			VLOG(1) << "Filling flow texture (" << (i + 1) << " of " << forwardFlowFiles.size() << ")";

			uploadFlowToOpenGLTexture(forwardFlows->at(i), forwardFlowTexture, i);
			uploadFlowToOpenGLTexture(backwardFlows->at(i), backwardFlowTexture, i);
		}

		ErrorChecking::checkGLError();
	}

	LOG(INFO) << "Loaded flows from CPU memory to GPU memory in "
	          << std::fixed << std::setprecision(2) << timer.getElapsedSeconds() << "s";
}


void FlowLoader::releaseCPUMemory()
{
	if (forwardFlows)
	{
		delete forwardFlows;
		forwardFlows = nullptr;
	}

	if (backwardFlows)
	{
		delete backwardFlows;
		backwardFlows = nullptr;
	}
}


void FlowLoader::releaseGPUMemory()
{
	if (forwardFlowTexture)
	{
		delete forwardFlowTexture;
		forwardFlowTexture = nullptr;
	}

	if (backwardFlowTexture)
	{
		delete backwardFlowTexture;
		backwardFlowTexture = nullptr;
	}
}


bool FlowLoader::checkAvailability()
{
	// 0) check number of files
	if (forwardFlowFiles.size() == 0)
	{
		LOG(WARNING) << "The forward flow files were not found. Were they computed?";
		return false;
	}

	if (backwardFlowFiles.size() == 0)
	{
		LOG(WARNING) << "The backward flow files were not found. Were they computed?";
		return false;
	}

	if (forwardFlowFiles.size() != backwardFlowFiles.size())
	{
		LOG(WARNING) << "Mismatching number of forward (" << forwardFlowFiles.size() << ") and backward flow files (" << backwardFlowFiles.size() << ").";
		return false;
	}

	// 1) check the optical flow files exist
	for (int i = 0; i < forwardFlowFiles.size(); i++)
	{
		if (!fs::exists(forwardFlowFiles[i]))
		{
			LOG(WARNING) << forwardFlowFiles[i] << " does not exist.";
			return false;
		}
	}

	for (int i = 0; i < backwardFlowFiles.size(); i++)
	{
		if (!fs::exists(backwardFlowFiles[i]))
		{
			LOG(WARNING) << backwardFlowFiles[i] << " does not exist.";
			return false;
		}
	}

	return true;
}


void FlowLoader::uploadFlowToOpenGLTexture(cv::Mat& flow, GLTexture* texture, int layer)
{
	glBindTexture(GL_TEXTURE_2D_ARRAY, texture->gl_ID);
	glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, layer, flow.cols, flow.rows, 1,
	                getGLFormat(texture->layout.mem.format), // GL_RG
	                getGLType(texture->layout.mem.type),     // GL_FLOAT
	                (void*)flow.ptr());
}

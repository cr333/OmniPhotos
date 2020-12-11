#pragma once

#include "Core/GL/GLTexture.hpp"
#include "Core/Loaders/Loader.hpp"


class FlowLoader: public Loader
{
public:
	FlowLoader(bool _downsampleFlow, const Eigen::Vector2i& _imgDims);
	FlowLoader(bool _downsampleFlow, const Eigen::Vector2i& _imgDims,
	           std::vector<std::string>* _forwardFlowFiles, std::vector<std::string>* _backwardFlowFiles);
	~FlowLoader();


	// Reads data from disk to CPU memory.
	void loadTextures() override;

	// Creates GPU textures.
	void initTextures() override;

	// Uploads data from CPU memory to GPU memory.
	void fillTextures() override;


	// Releases all resources in CPU memory.
	void releaseCPUMemory() override;

	// Releases all resources in GPU memory.
	void releaseGPUMemory() override;

	// Check the availability of optical flow files.
	bool checkAvailability();

	inline GLTexture* getForwardFlowsTexture() const { return forwardFlowTexture; }
	inline GLTexture* getBackwardFlowsTexture() const { return backwardFlowTexture; }

	inline std::vector<cv::Mat>* getForwardFlows() const { return forwardFlows; }
	inline std::vector<cv::Mat>* getBackwardFlows() const { return backwardFlows; }


private:
	int numberOfFlows = 0;

	// optical flow file paths
	std::vector<std::string> forwardFlowFiles;
	std::vector<std::string> backwardFlowFiles;

	// CPU memory resources
	std::vector<cv::Mat>* forwardFlows = nullptr;
	std::vector<cv::Mat>* backwardFlows = nullptr;

	// GPU texture resources
	GLTexture* forwardFlowTexture = nullptr;
	GLTexture* backwardFlowTexture = nullptr;

	Eigen::Vector2i imgDims;

	bool downsampleFlow = false;

	static void uploadFlowToOpenGLTexture(cv::Mat& flow, GLTexture* texture, int layer);
};

#pragma once

#include "FlowParameters.hpp"

#include "Core/Application.hpp"
#include "Core/Camera.hpp"

#include <opencv2/core/core.hpp>
#include <opencv2/video/tracking.hpp> // DISOpticalFlow

#include <string>


#ifdef USE_CUDA
	#include <opencv2/cudaoptflow.hpp>
	using cv::cuda::GpuMat;
#endif


class OpticalFlowApp : public Application
{
public:
	OpticalFlowApp(FlowMethod _method, bool _downsampleFlow = false);

	// Constructor to create DIS Optical Flow
	OpticalFlowApp(FlowMethod _method, int _preset, bool _downsampleFlow = false);

	virtual ~OpticalFlowApp();


	int init(FlowMethod _method, int _preset, BroxFlowParameters* params = nullptr);

	int init() override;
	void run() override;

	void setPair(std::string _left, std::string _right);
	void setPair(Camera& camLeft, Camera& camRight);

	void computeDISFlow(cv::Mat& left, cv::Mat& right, cv::Mat& flow);

	cv::Mat getFlow(bool forward);

	std::vector<std::string> getForwardFlows() const { return forwardFlows; }
	std::vector<std::string> getBackwardFlows() const { return backwardFlows; }

	// states
	bool readyToComputeFlowFields = false;
	bool flowFieldsComputed = false;

	// Options
	bool writeFlowIntoFile = false;
	bool writeColorCodedFlowToFile = false;
	bool downsampleFlow = false;
	bool convertToGrayscale = true;
	bool equirectWraparound = false;

	std::string outputDirectory;

	// DIS_flow preset values PRESET_ULTRAFAST:0, PRESET_FAST:1, PRESET_MEDIUM:3
	int disflowPreset;

	FlowMethod method;

	/** The file format used for storing optical flow fields (.flo, .floss, .F). */
	std::string fileExtension = ".floss";

	std::string pathToFlowLR;
	std::string pathToFlowRL;


private:
	bool prepareComputingFlow(bool loadFromDisk = true);

	void computeFlowsFarneback();
	void computeFarneback(cv::Mat& left, cv::Mat& right, cv::Mat& flow);

	// Compute DIS Optical Flow
	void computeFlowsDIS();

#ifdef USE_CUDA
	void prepareFlowCUDA();
	void computeFlowsCUDA();
	void computeFlowGPU(GpuMat& left, GpuMat& right, GpuMat& flow);
	void deleteCUDA();
#endif

	std::string left_pathToFile;
	std::string right_pathToFile;

	cv::Ptr<cv::DISOpticalFlow> disflow;

	// used to determine where to write flow fields
	std::string workingDirectory;

	// image pair
	cv::Mat imgL;
	cv::Mat imgR;

	// associated flows
	cv::Mat flowLR;
	cv::Mat flowRL;

#ifdef USE_CUDA
	GpuMat* imgLeftCUDA = nullptr;
	GpuMat* imgLeftCUDAf = nullptr;

	GpuMat* imgRightCUDA = nullptr;
	GpuMat* imgRightCUDAf = nullptr;

	GpuMat* flowLR_CUDA = nullptr;
	GpuMat* flowRL_CUDA = nullptr;

	cv::Ptr<cv::cuda::BroxOpticalFlow> brox;
#endif // USE_CUDA

	std::vector<std::string> forwardFlows;
	std::vector<std::string> backwardFlows;
};

#include "OpticalFlowApp.hpp"

#include "Utils/Exceptions.hpp"
#include "Utils/FlowIO.hpp"
#include "Utils/FlowVisualisation.hpp"
#include "Utils/Logger.hpp"
#include "Utils/Utils.hpp"
#include "Utils/cvutils.hpp"

#ifdef USE_CUDA
	#include <opencv2/cudaarithm.hpp>
	#include <opencv2/cudaoptflow.hpp>
using cv::cuda::GpuMat;
using namespace cv::cuda;
#endif


using namespace cv;
using namespace std;


OpticalFlowApp::OpticalFlowApp(FlowMethod _method, bool _downsampleFlow) :
    method(_method)
{
}


OpticalFlowApp::OpticalFlowApp(FlowMethod _method, int _preset, bool _downsampleFlow) :
    OpticalFlowApp(_method, _downsampleFlow)
{
	init(_method, _preset);
}


OpticalFlowApp::~OpticalFlowApp()
{
#ifdef USE_CUDA
	deleteCUDA();
#endif
}


int OpticalFlowApp::init(FlowMethod _method, int _preset, BroxFlowParameters* params)
{
	init();
	switch (method)
	{
		case FlowMethod::BroxCUDA:
		{
			if (!params)
			{
				LOG(INFO) << "Using standard Brox parameters since no argument was passed";
				params = new BroxFlowParameters();
			}

#ifdef USE_CUDA
			brox = cuda::BroxOpticalFlow::create(
			    params->alpha,
			    params->gamma,
			    params->scaleFactor,
			    params->innerIterations,
			    params->outerIterations,
			    params->solverIterations);
			LOG(INFO) << "Flow method: Brox CUDA";
#else
			RUNTIME_EXCEPTION("OpticalFlowApp::init(): You can't run Brox CUDA in OpenCV without CUDA support.");
#endif
			break;
		}

		case FlowMethod::DIS:
		{
			disflow = cv::DISOpticalFlow::create(_preset);
			LOG(INFO) << "Flow method: DIS CPU";
			break;
		}

		case FlowMethod::Farneback:
		{
			// Farneback: no presets, doesn't need to be created.
			LOG(INFO) << "Flow method: Farneback CPU";
			break;
		}
	}

	return 0;
}


int OpticalFlowApp::init()
{
	readyToComputeFlowFields = false;
	flowFieldsComputed = false;
	shouldShutdown = false;
	writeFlowIntoFile = false;
	writeColorCodedFlowToFile = false;
	return 0;
}


void OpticalFlowApp::run()
{
	if (!readyToComputeFlowFields)
	{
		LOG(WARNING) << "Not ready to compute flow fields.";
		return;
	}

	if (!flowFieldsComputed)
	{
		switch (method)
		{
			case FlowMethod::BroxCUDA:
			{
#ifdef USE_CUDA
				computeFlowsCUDA();
				deleteCUDA();
#endif
				break;
			}
			case FlowMethod::DIS:
			{
				computeFlowsDIS();
				break;
			}
			case FlowMethod::Farneback:
			{
				computeFlowsFarneback();
				break;
			}
		}

		if (writeFlowIntoFile)
		{
			if (equirectWraparound)
			{
				// Remove the wraparound padding from equirectangular images.
				// We have padded the input images by 1/8-th on the left and right, so now the flow
				// fields are "10/8-th" wide and we just crop off 1/10-th on the left and right.
				flowLR = flowLR.colRange(flowLR.cols / 10, (9 * flowLR.cols) / 10);
				flowRL = flowRL.colRange(flowRL.cols / 10, (9 * flowRL.cols) / 10);
			}

			writeFlowFile(pathToFlowLR, flowLR);
			forwardFlows.push_back(pathToFlowLR);
			writeFlowFile(pathToFlowRL, flowRL);
			backwardFlows.push_back(pathToFlowRL);

			if (writeColorCodedFlowToFile)
			{
				string pathLR = stripExtensionFromFilename(pathToFlowLR) + ".jpg";
				string pathRL = stripExtensionFromFilename(pathToFlowRL) + ".jpg";

				imwrite(pathLR, colourCodeFlow(flowLR));
				imwrite(pathRL, colourCodeFlow(flowRL));
			}
		}
	}
}


////////////////////////////////////////////////////////////////////


void OpticalFlowApp::setPair(string _left, string _right)
{
	left_pathToFile = _left;
	right_pathToFile = _right;
	readyToComputeFlowFields = prepareComputingFlow();

	string leftFile;
	splitFilename(_left, &workingDirectory, &leftFile);
	string rightFile;
	splitFilename(_right, &workingDirectory, &rightFile);

	// TB: you always assume first image left, second image right, forward flow from left to right
	if (writeFlowIntoFile)
	{
		leftFile = stripExtensionFromFilename(leftFile);
		pathToFlowLR = outputDirectory + "/" + leftFile + "-FlowToNext" + fileExtension;

		rightFile = stripExtensionFromFilename(rightFile);
		pathToFlowRL = outputDirectory + "/" + rightFile + "-FlowToPrevious" + fileExtension;
	}
}


void OpticalFlowApp::setPair(Camera& camLeft, Camera& camRight)
{
	imgL = camLeft.getImage().clone();
	imgR = camRight.getImage().clone();

	if (downsampleFlow)
	{
		cv::resize(imgL, imgL, cv::Size(0, 0), 0.5, 0.5, cv::INTER_LINEAR_EXACT);
		cv::resize(imgR, imgR, cv::Size(0, 0), 0.5, 0.5, cv::INTER_LINEAR_EXACT);
	}

	readyToComputeFlowFields = prepareComputingFlow(false);

	string leftFile;
	splitFilename(camLeft.imageName, &workingDirectory, &leftFile);
	string rightFile;
	splitFilename(camRight.imageName, &workingDirectory, &rightFile);

	// TB: you always assume first image left, second image right, forward flow from left to right
	if (writeFlowIntoFile)
	{
		leftFile = stripExtensionFromFilename(leftFile);
		pathToFlowLR = outputDirectory + "/" + leftFile + "-FlowToNext" + fileExtension;

		rightFile = stripExtensionFromFilename(rightFile);
		pathToFlowRL = outputDirectory + "/" + rightFile + "-FlowToPrevious" + fileExtension;
	}
}


cv::Mat OpticalFlowApp::getFlow(bool forward)
{
	if (forward)
		return flowLR;
	else
		return flowRL;
}


bool OpticalFlowApp::prepareComputingFlow(bool loadFromDisk)
{
	//try to read images from "left" and "right
	try
	{
		if (loadFromDisk)
		{
			imgL = imread(left_pathToFile, IMREAD_COLOR);
			imgR = imread(right_pathToFile, IMREAD_COLOR);
			if ((imgL.cols != imgR.cols) || (imgL.rows != imgR.rows))
				RUNTIME_EXCEPTION("OpticalFlowApp image dimensions wrong!");

			if (downsampleFlow)
			{
				cv::resize(imgL, imgL, Size(0, 0), 0.5, 0.5, INTER_LINEAR);
				cv::resize(imgR, imgR, Size(0, 0), 0.5, 0.5, INTER_LINEAR);
			}

			if (convertToGrayscale)
			{
				//convert to gray images
				cv::cvtColor(imgL, imgL, cv::COLOR_BGR2GRAY);
				cv::cvtColor(imgR, imgR, cv::COLOR_BGR2GRAY);
			}

			//TB: do other colour corrections here as well.
			// Or do all this stuff in dedicated dataset preparation step?
			//Should be done before
		}
		else
		{
			if (imgL.cols == 0 || imgR.cols == 0)
				RUNTIME_EXCEPTION("OpticalFlowApp: images not set.");
		}

		if (convertToGrayscale)
		{
			cv::cvtColor(imgL, imgL, cv::COLOR_RGB2GRAY);
			cv::cvtColor(imgR, imgR, cv::COLOR_RGB2GRAY);
		}

		if (equirectWraparound)
		{
			// Pad the input images for equirectangular wraparound (if desired).
			// Copy 1/8-th of the image from the right/left edge for wraparound padding
			// on the left/right side of the equirectangular image.
			imgL = cv::concat(
			    imgL.colRange((7 * imgL.cols) / 8, imgL.cols), // right 1/8-th of the image (wraparound #1)
			    imgL,                                          // full image in the middle
			    imgL.colRange(0, imgL.cols / 8), 1);           // left 1/8-th of the image (wraparound #2)
			imgR = cv::concat(
			    imgR.colRange((7 * imgR.cols) / 8, imgR.cols),
			    imgR,
			    imgR.colRange(0, imgR.cols / 8), 1);
		}

		if (method == FlowMethod::BroxCUDA)
		{
#ifdef USE_CUDA
			prepareFlowCUDA();
#endif
		}

		return true;
	}
	catch (cv::Exception& e)
	{
		LOG(WARNING) << "Exception caught: " << e.what();
		return false;
	}
	return true;
}


void OpticalFlowApp::computeFarneback(Mat& left, Mat& right, Mat& flow)
{
	cv::calcOpticalFlowFarneback(left, right, flow, 0.5, 3, 15, 3, 5, 1.2, 0);
	flowFieldsComputed = true;
}


void OpticalFlowApp::computeFlowsFarneback()
{
	computeFarneback(imgL, imgR, flowLR);
	computeFarneback(imgR, imgL, flowRL);
}


void OpticalFlowApp::computeFlowsDIS()
{
	computeDISFlow(imgL, imgR, flowLR);
	computeDISFlow(imgR, imgL, flowRL);
}


void OpticalFlowApp::computeDISFlow(Mat& left, Mat& right, Mat& flow)
{
	disflow->calc(left, right, flow);
	flowFieldsComputed = true;
}


#ifdef USE_CUDA

void OpticalFlowApp::prepareFlowCUDA()
{
	// copy to GPU matrix
	imgLeftCUDA = new GpuMat(imgL);
	imgRightCUDA = new GpuMat(imgR);

	imgLeftCUDAf = new GpuMat();
	imgLeftCUDA->convertTo(*imgLeftCUDAf, CV_32F, 1.0 / 255.0);

	imgRightCUDAf = new GpuMat();
	imgRightCUDA->convertTo(*imgRightCUDAf, CV_32F, 1.0 / 255.0);

	// initialize flow field
	flowLR_CUDA = new GpuMat(imgL.size(), CV_32FC2);
	flowRL_CUDA = new GpuMat(imgR.size(), CV_32FC2);
}


void OpticalFlowApp::computeFlowsCUDA()
{
	//compute left to right
	computeFlowGPU(*imgLeftCUDAf, *imgRightCUDAf, *flowLR_CUDA);
	//compute right to left
	computeFlowGPU(*imgRightCUDAf, *imgLeftCUDAf, *flowRL_CUDA);
	flowLR = Mat(*flowLR_CUDA);
	flowRL = Mat(*flowRL_CUDA);
}


void OpticalFlowApp::computeFlowGPU(GpuMat& left, GpuMat& right, GpuMat& flow)
{
	brox->calc(left, right, flow);
	flowFieldsComputed = true;
}


void OpticalFlowApp::deleteCUDA()
{
	if (imgLeftCUDA)
	{
		delete imgLeftCUDA;
		imgLeftCUDA = nullptr;
	}
	if (imgLeftCUDAf)
	{
		delete imgLeftCUDAf;
		imgLeftCUDAf = nullptr;
	}
	if (imgRightCUDA)
	{
		delete imgRightCUDA;
		imgRightCUDA = nullptr;
	}
	if (imgRightCUDAf)
	{
		delete imgRightCUDAf;
		imgRightCUDAf = nullptr;
	}
	if (flowLR_CUDA)
	{
		delete flowLR_CUDA;
		flowLR_CUDA = nullptr;
	}
	if (flowRL_CUDA)
	{
		delete flowRL_CUDA;
		flowRL_CUDA = nullptr;
	}
}

#endif // USE_CUDA

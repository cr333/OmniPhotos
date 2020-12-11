//#include "ssim.h"

#include "Utils/FlowIO.hpp"
#include "Utils/FlowVisualisation.hpp"
#include "Utils/IOTools.hpp"
#include "Utils/cvutils.hpp"

#include <tclap/CmdLine.h>

#include <string>
#include <opencv2/opencv.hpp>

using namespace cv;
using namespace std;


int main(int argc, char* argv[])
{
	// Set up command line parser TCLAP.
	TCLAP::CmdLine cmd("CompTool - Comparison tool.", ' ', "0.1");
	TCLAP::UnlabeledValueArg<string> sourceImageArg(      "source",   "Filename of source image.", true, "", "source file", cmd);
	TCLAP::UnlabeledValueArg<string> targetImageArg(      "target",   "Filename of target image.", true, "", "target file", cmd);
	TCLAP::UnlabeledValueArg<string> flowArg(             "flow",     "Filename of the flow field.", true, "", "flow file", cmd);
	TCLAP::ValueArg<string>          prefixArg(      "p", "prefix",   "The filename or path prefix of all input filenames.", false, "", "prefix", cmd);
	TCLAP::SwitchArg                 absFlowArg(     "a", "abs",      "Consider the flow to have absolute coordinates, like in a NN field.", cmd, false);
	TCLAP::SwitchArg                 backwardFlowArg("b", "backward", "The flow is going backward (reference frame is target image).", cmd, false);
	TCLAP::ValueArg<float>           resizeArg(      "r", "resize",   "Resize the input images (not flows) by this factor. [default: 1]", false, 1.0f, "factor", cmd);
	TCLAP::SwitchArg                 autosizeArg(    "",  "autosize", "Resize the input images to match the flow.", cmd, false);
	TCLAP::SwitchArg                 writeImagesArg( "",  "ab",       "Write out the (possibly resized) input images.", cmd, false);
	TCLAP::ValueArg<string>          flowVisArg(     "",  "flowvis",  "If specified, write a Middlebury visualisation of the flow to this file.", false, "", "filename", cmd);
	TCLAP::ValueArg<float>           maxFlowArg(     "",  "maxflow",  "Specifies the maximum flow used for scaling the Middlebury flow colours. [default: 100]", false, 100, "number", cmd);
	TCLAP::ValueArg<string>          warpedOutputArg("w", "warped",   "If specified, write the warped input image (depending on forward/backward flow) to this file.", false, "", "filename", cmd);
	TCLAP::ValueArg<string>          diffMapArg(     "d", "diffmap",  "If specified, write the difference image to this file.", false, "", "filename", cmd);
	TCLAP::ValueArg<float>           diffScaleArg(   "",  "diffscale", "Specifies the scale factor applied to the difference image. [default: 2]", false, 2.0, "factor", cmd);
	TCLAP::ValueArg<string>          colourMapArg(   "c", "cmap",     "Name of the colour map used for visualising the difference image. [default: none]", false, "", "name", cmd);
	cmd.parse(argc, (char const* const*)argv);


	//---- Loading all the inputs ------------------------------------------------------------------

	LOG(INFO) << "Loading input images ... ";
	string prefix = prefixArg.getValue();
	Mat a = imread(findFileWithPrefix(sourceImageArg.getValue(), prefix));
	Mat b = imread(findFileWithPrefix(targetImageArg.getValue(), prefix));

	if(a.empty() || b.empty())
	{
		LOG(WARNING) << "Error: File(s) not found. Exiting.";
		return -__LINE__;
	}

	// Resize input images if requested.
	if(resizeArg.getValue() != 1.0f)
	{
		float factor = resizeArg.getValue();
		char msg[64];
		sprintf(msg, "Resizing images by factor %.2f ... ", factor);
		LOG(INFO) << msg;
		a = resize(a, Size(0, 0), factor, factor, INTER_AREA);
		b = resize(b, Size(0, 0), factor, factor, INTER_AREA);
	}

	LOG(INFO) << "Loading input flow ... ";
	Mat flow = readFlowFile(findFileWithPrefix(flowArg.getValue(), prefix));
	if(flow.empty())
	{
		LOG(WARNING) << "Error: Flow not found. Exiting.";
		return -__LINE__;
	}
	if(!absFlowArg.getValue()) flow = convertFlowToAbsolute(flow);

	// Resize images to fit flow.
	if(autosizeArg.getValue())
	{
		char msg[64];
		sprintf(msg, "Resizing images to fit flow (%i x %i) ... ", flow.cols, flow.rows);
		LOG(INFO) << msg;
		a = resize(a, Size(flow.cols, flow.rows), 0, 0, INTER_AREA);
		b = resize(b, Size(flow.cols, flow.rows), 0, 0, INTER_AREA);
	}

	if(a.cols != flow.cols || b.cols != flow.cols || a.rows != flow.rows || b.rows != flow.rows)
	{
		LOG(WARNING) << "Error: The images have a different resolution to the flow.\n\tTry the --resize or --autosize arguments.";
		return -__LINE__;
	}


	//---- Warping, comparing and measuring --------------------------------------------------------

	if(writeImagesArg.getValue())
	{
		imwrite("a.png", a);
		imwrite("b.png", b);
	}
	//TB: I think here is something wrong.
	//remap always performs a backward warping,
	//https://docs.opencv.org/3.0.0/da/d54/group__imgproc__transform.html#gab75ef31ce5cdfb5c44b6da5f3b908ea4
	//dst(x,y)=src(map_x(x,y),map_y(x,y))
	Mat warped = remap(backwardFlowArg.getValue() ? b : a, flow, INTER_LANCZOS4, BORDER_REPLICATE);

	// Print evaluation metrics.
	Mat warpTarget = backwardFlowArg.getValue() ? a : b;
	char msg[32];
	sprintf(msg, "RMSE = %f", rmse(warped, warpTarget));
	LOG(INFO) << msg;
	sprintf(msg, "PSNR = %f", psnr(warped, warpTarget));
	LOG(INFO) << msg;
	//printf("SSIM = %f\n", calcSSIM(warped, warpTarget));


	//---- Writing outputs -------------------------------------------------------------------------

	// Writing warped output image.
	string warpedOutputFilename = warpedOutputArg.getValue();
	if(!warpedOutputFilename.empty())
	{
		string whichImage = (backwardFlowArg.getValue() ? "target" : "source");
		char msg[64];
		sprintf(msg, "Writing warped \"%s\" image to \"%s\" ... ", whichImage.c_str(), warpedOutputFilename.c_str());
		LOG(INFO) << msg;
		imwrite(warpedOutputFilename, warped);
	}

	// Writing difference map.
	string diffMapFilename = diffMapArg.getValue();
	if(!diffMapFilename.empty())
	{
		Mat diffMap = absdiff(warped, warpTarget);
		diffMap = convert(diffMap, diffMap.type(), diffScaleArg.getValue());

		// Apply colour map (optional).
		if(!colourMapArg.getValue().empty())
			diffMap = applyColorMap(diffMap, colourMapArg.getValue());

		LOG(INFO) << "Writing difference map to \"" << diffMapFilename  << "\" ...";
		imwrite(diffMapFilename, diffMap);
	}

	// Writing Middlebury flow visualisation.
	string flowVisFilename = flowVisArg.getValue();
	if(!flowVisFilename.empty())
	{
		LOG(INFO) << "Writing Middlebury flow visualisation to \"" << flowVisFilename << "\" ...";
		imwrite(flowVisFilename, flowColour(convertFlowToRelative(flow), maxFlowArg.getValue()));
	}
	
	return 0;

	//// Draw a circle of all flow colours (debug).
	//Mat2f flowtest(800, 800);

	//for(int y = 0; y < flowtest.rows; y++)
	//{
	//	for(int x = 0; x < flowtest.cols; x++)
	//	{
	//		flowtest(y, x) = Vec2f((2.2f * x) / flowtest.cols - 1.1f,
	//							   (2.2f * y) / flowtest.rows - 1.1f);
	//	}
	//}

	//Mat3b flowvis = flowColour(flowtest, 1.0f);
}

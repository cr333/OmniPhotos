#include "FlowVisualisation.hpp"

#include "Logger.hpp"

#include <opencv2/imgcodecs/imgcodecs.hpp>
#include <opencv2/imgproc/imgproc.hpp>


cv::Vec3f colourWheel(int k)
{
	// relative lengths of color transitions:
	// these are chosen based on perceptual similarity
	// (e.g. one can distinguish more shades between red and yellow
	//  than between yellow and green)
	int RY = 15;
	int YG = 6;
	int GC = 4;
	int CB = 11;
	int BM = 13;
	int MR = 6;

	int i = k;
	if (i < RY) return cv::Vec3f(1.f, float(i) / float(RY), 0.f);       else i -= RY;
	if (i < YG) return cv::Vec3f(1.f - float(i) / float(YG), 1.f, 0.f); else i -= YG;
	if (i < GC) return cv::Vec3f(0.f, 1.f, float(i) / float(GC));       else i -= GC;
	if (i < CB) return cv::Vec3f(0.f, 1.f - float(i) / float(CB), 1.f); else i -= CB;
	if (i < BM) return cv::Vec3f(float(i) / float(BM), 0.f, 1.f);       else i -= BM;
	if (i < MR) return cv::Vec3f(1.f, 0.f, 1.f - float(i) / float(MR)); //else i -= MR;

	return cv::Vec3f(0., 0., 0.);
}


cv::Vec3f flowColour(cv::Vec2f flow)
{
	float radius = (float)norm(flow);
	float angle = (float)(atan2(-flow[1], -flow[0]) / 3.14159265);
	// Note: The original Middlebury flow colour code uses (ncols - 1) == 54 in the line below,
	//       but this causes a colour discontinuity at flow (positive, +-epsilon). For flow
	//       (positive, -epsilon), angle is slightly less than 1.0 (but never quite reaches it),
	//       so k0 <= 53 (because int rounds down). With flow (positive, 0), angle = -1 and k0 = 0.
	//       k0 can never be 54, so this colour wheel position is skipped, leading to a discontinuty.
	//       By multiplying by 55, this problem is avoided.
	float fk = (angle + 1.0f) / 2.0f * 55.f;
	int k0 = (int)fk % 55;
	int k1 = (k0 + 1) % 55;
	float f = (float)(fk - floor(fk)); // fract(fk)
	//float f = fk - k0; // original code (equivalent)

	cv::Vec3f col = (1.f - f) * colourWheel(k0) + f * colourWheel(k1);
	if (radius <= 1.)
	{
		// increase saturation with radius
		col = cv::Vec3f(1., 1., 1.) - radius * (cv::Vec3f(1., 1., 1.) - col);
	}
	else
	{
		// out of range = darker
		col *= .75;
	}

	// Swizzle vector from RGB to BGR, to conform to OpenCV convention.
	col = cv::Vec3f(col[2], col[1], col[0]);

	return col;
}


cv::Mat3b flowColour(cv::Mat2f flow, float max_flow)
{
	float maxrad = -1;

	if (max_flow > 0)
	{
		// Use specified maximum flow for scale.
		maxrad = max_flow;
	}
	else // Determine motion range.
	{
		float maxx = -999, maxy = -999;
		float minx = 999, miny = 999;

		for (int y = 0; y < flow.rows; y++)
		{
			for (int x = 0; x < flow.cols; x++)
			{
				cv::Vec2f f = flow.at<cv::Vec2f>(y, x);

				maxx = std::max(maxx, f[0]);
				maxy = std::max(maxy, f[1]);
				minx = std::min(minx, f[0]);
				miny = std::min(miny, f[1]);
				maxrad = std::max(maxrad, (float)norm(f));
			}
		}

		DLOG(INFO) << "Max motion: " << maxrad << "; motion range: u = " << minx << " .. " << maxx << "; v = " << miny << " .. " << maxy;

		if (maxrad == 0) // if flow == 0 everywhere
			maxrad = 1;
	}

	cv::Mat3b result = cv::Mat3b(flow.rows, flow.cols);

	for (int y = 0; y < flow.rows; y++)
	{
		for (int x = 0; x < flow.cols; x++)
		{
			cv::Vec2f f = flow.at<cv::Vec2f>(y, x);
			result.at<cv::Vec3b>(y, x) = 255 * flowColour(f / maxrad);
		}
	}

	return result;
}


void writeTestFlowImage(std::string filename)
{
	cv::Mat2f test_flow(301, 301);
	for (int y = 0; y < test_flow.rows; y++)
	{
		for (int x = 0; x < test_flow.cols; x++)
		{
			test_flow(y, x) = cv::Vec2f((2.08f * x) / test_flow.cols - 1.04f, (2.08f * y) / test_flow.rows - 1.04f);
		}
	}

	cv::imwrite(filename, flowColour(test_flow));
}


cv::Vec3b computeColor(float fx, float fy)
{
	static bool first = true;

	// relative lengths of color transitions:
	// these are chosen based on perceptual similarity
	// (e.g. one can distinguish more shades between red and yellow
	//  than between yellow and green)
	const int RY = 15;
	const int YG = 6;
	const int GC = 4;
	const int CB = 11;
	const int BM = 13;
	const int MR = 6;
	const int NCOLS = RY + YG + GC + CB + BM + MR;
	static cv::Vec3i colorWheel[NCOLS];

	if (first)
	{
		int k = 0;

		for (int i = 0; i < RY; ++i, ++k)
			colorWheel[k] = cv::Vec3i(255, 255 * i / RY, 0);

		for (int i = 0; i < YG; ++i, ++k)
			colorWheel[k] = cv::Vec3i(255 - 255 * i / YG, 255, 0);

		for (int i = 0; i < GC; ++i, ++k)
			colorWheel[k] = cv::Vec3i(0, 255, 255 * i / GC);

		for (int i = 0; i < CB; ++i, ++k)
			colorWheel[k] = cv::Vec3i(0, 255 - 255 * i / CB, 255);

		for (int i = 0; i < BM; ++i, ++k)
			colorWheel[k] = cv::Vec3i(255 * i / BM, 0, 255);

		for (int i = 0; i < MR; ++i, ++k)
			colorWheel[k] = cv::Vec3i(255, 0, 255 - 255 * i / MR);

		first = false;
	}

	const float rad = sqrt(fx * fx + fy * fy);
	const float a = atan2(-fy, -fx) / (float)CV_PI;

	const float fk = (a + 1.0f) / 2.0f * (NCOLS - 1);
	const int k0 = static_cast<int>(fk);
	const int k1 = (k0 + 1) % NCOLS;
	const float f = fk - k0;

	cv::Vec3b pix;

	for (int b = 0; b < 3; b++)
	{
		const float col0 = colorWheel[k0][b] / 255.0f;
		const float col1 = colorWheel[k1][b] / 255.0f;

		float col = (1 - f) * col0 + f * col1;

		if (rad <= 1)
			col = 1 - rad * (1 - col); // increase saturation with radius
		else
			col *= .75; // out of range

		pix[2 - b] = static_cast<uchar>(255.0 * col);
	}

	return pix;
}


void drawOpticalFlow(const cv::Mat_<float>& flowx, const cv::Mat_<float>& flowy, cv::Mat& dst, float maxmotion)
{
	dst.create(flowx.size(), CV_8UC3);
	dst.setTo(cv::Scalar::all(0));

	// determine motion range:
	float maxrad = maxmotion;

	if (maxmotion <= 0)
	{
		maxrad = 1;
		for (int y = 0; y < flowx.rows; ++y)
		{
			for (int x = 0; x < flowx.cols; ++x)
			{
				cv::Point2f u(flowx(y, x), flowy(y, x));
				maxrad = std::max(maxrad, sqrt(u.x * u.x + u.y * u.y));
			}
		}
	}

	for (int y = 0; y < flowx.rows; ++y)
	{
		for (int x = 0; x < flowx.cols; ++x)
		{
			cv::Point2f u(flowx(y, x), flowy(y, x));
			dst.at<cv::Vec3b>(y, x) = computeColor(u.x / maxrad, u.y / maxrad);
		}
	}
}


cv::Mat colourCodeFlow(const cv::Mat& flow, bool verticalFlip)
{
	cv::Mat planes[2];
	cv::split(flow, planes);

	cv::Mat flowx(planes[0]);
	cv::Mat flowy(planes[1]);

	cv::Mat out;
	drawOpticalFlow(flowx, flowy, out, -1);
	if (verticalFlip)
		cv::flip(out, out, 0);
	return out;
}


void arrowedLineStackOverflow(cv::InputOutputArray img, cv::Point pt1, cv::Point pt2, const cv::Scalar& color,
                              int thickness, int line_type, int shift, double tipLength)
{
	const double tipSize = norm(pt1 - pt2) * tipLength; // Factor to normalize the size of the tip depending on the length of the arrow
	line(img, pt1, pt2, color, thickness, line_type, shift);
	const double angle = atan2((double)pt1.y - pt2.y, (double)pt1.x - pt2.x);
	cv::Point p(cvRound(pt2.x + tipSize * cos(angle + CV_PI / 4)),
	            cvRound(pt2.y + tipSize * sin(angle + CV_PI / 4)));
	line(img, p, pt2, color, thickness, line_type, shift);
	p.x = cvRound(pt2.x + tipSize * cos(angle - CV_PI / 4));
	p.y = cvRound(pt2.y + tipSize * sin(angle - CV_PI / 4));
	line(img, p, pt2, color, thickness, line_type, shift);
}


// superPixelRadius should be odd
cv::Mat overlayflowVectors(const cv::Mat& flow, int superPixelRadius)
{
	cv::Mat out = cv::Mat::zeros(flow.rows, flow.cols, CV_8UC3);

	int boxSide = 2 * superPixelRadius + 1;

	for (int y = 0; y < flow.rows; y += boxSide)
	{
		for (int x = 0; x < flow.cols; x += boxSide)
		{
			cv::Point pixel = { x, y };
			cv::Vec2f flowVec = flow.at<cv::Vec2f>(pixel);
			cv::Point flowAsPoint = pixel + cv::Point((int)flowVec[0], (int)flowVec[1]);
			arrowedLineStackOverflow(out, pixel, flowAsPoint, cv::Scalar(255, 255, 255));
		}
	}

	return out;
}

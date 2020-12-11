#pragma once

#include <opencv2/core/mat.hpp>


cv::Vec3f colourWheel(int k);
cv::Vec3f flowColour(cv::Vec2f flow);
cv::Mat3b flowColour(cv::Mat2f flow, float max_flow = -1);


// from OpenCV examples: optical_flow.cpp
cv::Vec3b computeColor(float fx, float fy);

// from OpenCV examples: optical_flow.cpp
void drawOpticalFlow(const cv::Mat_<float>& flowx, const cv::Mat_<float>& flowy,
                     cv::Mat& dst, float maxmotion = -1);

cv::Mat colourCodeFlow(const cv::Mat& flow, bool verticalFlip = false);

cv::Mat overlayflowVectors(const cv::Mat& flow, int superPixelRadius);

void arrowedLineStackOverflow(cv::InputOutputArray img,
                              cv::Point pt1,
                              cv::Point pt2,
                              const cv::Scalar& color,
                              int thickness = 1,
                              int line_type = 8,
                              int shift = 0,
                              double tipLength = 0.1);

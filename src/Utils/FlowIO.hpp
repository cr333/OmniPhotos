#pragma once

#include <opencv2/opencv.hpp>
#include <string>


// Reads a .flo file (Middlebury format) into an OpenCV matrix.
cv::Mat2f readFloFile(const char* filename);

// Reads a .flo file (Middlebury format) into an OpenCV matrix.
cv::Mat2f readFloFile(std::string filename);

// Writes a flow field to a .flo file (Middlebury format).
void writeFloFile(const char* filename, cv::Mat2f img);

// Writes a flow field to a .flo file (Middlebury format).
void writeFloFile(std::string filename, cv::Mat2f img);


// Reads a .floss file (16-bit fixed-point format) into an OpenCV matrix.
cv::Mat2f readFlossFile(const std::string filename);

// Writes a flow field to a .floss file (16-bit fixed-point format).
bool writeFlossFile(const std::string filename, cv::Mat2f flow);


// Reads a .F file (Barron format) into an OpenCV matrix.
cv::Mat2f readBarronFile(const char* filename);

// Writes a flow field to a .F file (Barron format).
void writeBarronFile(const char* filename, cv::Mat2f img);

// Writes a flow field to a .F file (Barron format).
void writeBarronFile(std::string filename, cv::Mat2f img);


// Reads a flow file (format depending on file extension) into an OpenCV matrix.
cv::Mat2f readFlowFile(const std::string filename);

// Writes a flow field to a file (format depending on file extension).
void writeFlowFile(const std::string filename, cv::Mat2f flow);


// Converts flow (relative displacements) to absolute coordinates suitable for cv::remap.
cv::Mat2f convertFlowToAbsolute(const cv::Mat2f flow);

// Converts flow (relative displacements) to absolute coordinates suitable for cv::remap.
cv::Mat2f convertFlowToRelative(const cv::Mat2f flow);

#pragma once

#include <opencv2/opencv.hpp>
#include <string>


// Reads a COLMAP depth map into an OpenCV matrix.
cv::Mat1f readColmapDepthMapFile(const char* filename);

// Reads a COLMAP depth map into an OpenCV matrix.
cv::Mat1f readColmapDepthMapFile(std::string filename);


// Reads a depth map from a colour-encoded image into an OpenCV matrix.
cv::Mat1f readRGBDepthMapFile(const std::string& filename, float scale_factor = (1.f / 65535.f));


// Reads a .dpt file (Sintel format) into an OpenCV matrix.
cv::Mat1f readSintelDptFile(const char* filename);

// Reads a .dpt file (Sintel format) into an OpenCV matrix.
cv::Mat1f readSintelDptFile(std::string filename);


// Writes a depth map to a .dpt file (Sintel format).
void writeSintelDptFile(const char* filename, cv::Mat1f img);

// Writes a depth map to a .dpt file (Sintel format).
void writeSintelDptFile(std::string filename, cv::Mat1f img);


// Writes a colour-mapped, normalised log depth map as an image.
void writeNormalisedLogDepthMap(std::string filename, const cv::Mat1f depth_map, std::string color_map = "inferno");

// Writes a colour-mapped, log depth map (within a given range) as an image.
void writeLogDepthMap(std::string filename, const cv::Mat1f depth_map, float min_depth, float max_depth, std::string color_map = "inferno");

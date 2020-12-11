#include <opencv2/opencv.hpp>

#define ALLCHANNEL -1
double calcSSIM(cv::Mat src1, cv::Mat src2, int channel = 0, int method=CV_BGR2YUV, cv::Mat mask=cv::Mat(), const double K1 = 0.01, const double K2 = 0.03, const int L = 255, const int downsamplewidth=256, const int gaussian_window=11, const double gaussian_sigma=1.5, cv::Mat* ssim_map=NULL);
double calcSSIMBB(cv::Mat src1, cv::Mat src2, int channel = 0, int method=CV_BGR2YUV, int boundx=0, int boundy=0, const double K1 = 0.01, const double K2 = 0.03, const int L = 255, const int downsamplewidth=256, const int gaussian_window=11, const double gaussian_sigma=1.5, cv::Mat* ssim_map=NULL);

double calcDSSIM(cv::Mat src1, cv::Mat src2, int channel = 0, int method=CV_BGR2YUV, cv::Mat mask=cv::Mat(), const double K1 = 0.01, const double K2 = 0.03, const int L = 255, const int downsamplewidth=256, const int gaussian_window=11, const double gaussian_sigma=1.5, cv::Mat* ssim_map=NULL);
double calcDSSIMBB(cv::Mat src1, cv::Mat src2, int channel = 0, int method=CV_BGR2YUV, int boundx=0, int boundy=0, const double K1 = 0.01, const double K2 = 0.03, const int L = 255, const int downsamplewidth=256, const int gaussian_window=11, const double gaussian_sigma=1.5, cv::Mat* ssim_map=NULL);
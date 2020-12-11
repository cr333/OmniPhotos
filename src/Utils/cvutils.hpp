#pragma once

#include <opencv2/opencv.hpp>
#include <stdio.h>
#include <string>
#include <vector>

#include "Utils/Logger.hpp"

namespace cv
{
	//---- Useful utility functions --------------------------------------------------------------------

	template <typename T1, typename T2>
	inline T1 clamp(T1 value, T2 lower, T2 upper)
	{
		return min(max(value, lower), upper);
	}

	template <typename T, int n>
	inline cv::Vec<T, n> clamp(cv::Vec<T, n> value, T lower, T upper)
	{
		Vec<T, n> result;
		for (int i = 0; i < n; i++)
			result[i] = min(max(value[i], lower), upper);
		return result;
	}

	inline std::string cvtype2string(int type)
	{
		std::string r;
		switch (type & CV_MAT_DEPTH_MASK)
		{
			case CV_8U:  r = "8U";   break;
			case CV_8S:  r = "8S";   break;
			case CV_16U: r = "16U";  break;
			case CV_16S: r = "16S";  break;
			case CV_32S: r = "32S";  break;
			case CV_32F: r = "32F";  break;
			case CV_64F: r = "64F";  break;
			default:     r = "User"; break;
		}

		return r + "C" + std::to_string(1 + (type >> CV_CN_SHIFT));
	}

	//---- Missing functionality -----------------------------------------------------------------------

	// Premultiplies each element in 'matrix' (e.g. a Vec3f) with the matrix 'M' (e.g. 3x3 matrix).
	// This can for example be used to multiply pixels in an RGB image with a 3x3 matrix.
	template <typename T, int n>
	inline cv::Mat_<cv::Vec<T, n>> multiply(const cv::Mat_<cv::Vec<T, n>> matrix, const cv::Mat1f M)
	{
		// First reshape input matrix from   w x h x n   to   n x (w * h) x 1.
		cv::Mat reshaped = matrix.reshape(1, matrix.cols * matrix.rows);

		// Then multiply each row with the transpose of M, so it appears like pre-multiplication.
		cv::Mat result = reshaped * M.t();

		// Finally return to original matrix shape.
		return result.reshape(n, matrix.rows);
	}

	// Computes the pointwise product of each element in 'matrix' (e.g. a Vec3f) with 'vector' (same type).
	// This can for example be used to scale or normalise flow fields.
	template <typename T, int n>
	inline cv::Mat_<cv::Vec<T, n>> multiply(const cv::Mat_<cv::Vec<T, n>> matrix, const cv::Vec<T, n> vector)
	{
		// First reshape input matrix from   w x h x n   to   n x (w * h) x 1.
		cv::Mat reshaped = matrix.reshape(1, matrix.cols * matrix.rows);

		// Then multiply each row with the vector as diagonal matrix.
		cv::Mat result = reshaped * cv::Mat::diag(cv::Mat(vector));

		// Finally return to original matrix shape.
		return result.reshape(n, matrix.rows);
	}


// Back-ported functionality from OpenCV 3.0.0-alpha
#if (CV_MAJOR_VERSION < 3)

	inline void decomposeEssentialMat(InputArray _E, OutputArray _R1, OutputArray _R2, OutputArray _t)
	{
		Mat E = _E.getMat().reshape(1, 3);
		CV_Assert(E.cols == 3 && E.rows == 3);

		Mat D, U, Vt;
		SVD::compute(E, D, U, Vt);

		if (determinant(U) < 0) U *= -1.;
		if (determinant(Vt) < 0) Vt *= -1.;

		Mat W = (Mat_<double>(3, 3) << 0, 1, 0, -1, 0, 0, 0, 0, 1);
		W.convertTo(W, E.type());

		Mat R1, R2, t;
		R1 = U * W * Vt;
		R2 = U * W.t() * Vt;
		t = U.col(2) * 1.0;

		R1.copyTo(_R1);
		R2.copyTo(_R2);
		t.copyTo(_t);
	}

#endif


// Ported functionality from 2.4.x.
#if (CV_MAJOR_VERSION >= 3)

	// Divides a multi-channel array into several single-channel arrays.
	// (From /include/opencv2/core/mat.hpp.)
	template <typename _Tp>
	inline void split(const Mat& src, std::vector<Mat_<_Tp>>& mv)
	{
		split(src, (std::vector<Mat>&)mv);
	}

#endif


	//---- Basic Python-like wrappers ------------------------------------------------------------------

	inline Mat absdiff(InputArray src1, InputArray src2)
	{
		Mat result;
		cv::absdiff(src1, src2, result);
		return result;
	}

	// Calculates the per-element sum of two arrays or an array and a scalar.
	// (Removed mask argument to avoid ambiguous function signature.)
	inline Mat add(InputArray src1, InputArray src2, /*InputArray mask = noArray(),*/ int dtype = -1)
	{
		Mat result;
		cv::add(src1, src2, result, noArray(), dtype);
		return result;
	}

	inline Mat convert(InputArray src, int rtype, double alpha = 1., double beta = 0.)
	{
		Mat result;
		src.getMat().convertTo(result, rtype, alpha, beta);
		return result;
	}

	inline Mat cvtColor(InputArray src, int code, int dstCn = 0)
	{
		Mat result;
		cv::cvtColor(src, result, code, dstCn);
		return result;
	}

	inline Mat dilate(InputArray src, InputArray kernel, Point anchor = Point(-1, -1), int iterations = 1,
	                  int borderType = BORDER_CONSTANT, const Scalar& borderValue = morphologyDefaultBorderValue())
	{
		Mat result;
		cv::dilate(src, result, kernel, anchor, iterations, borderType, borderValue);
		return result;
	}

	inline Mat erode(InputArray src, InputArray kernel, Point anchor = Point(-1, -1), int iterations = 1,
	                 int borderType = BORDER_CONSTANT, const Scalar& borderValue = morphologyDefaultBorderValue())
	{
		Mat result;
		cv::erode(src, result, kernel, anchor, iterations, borderType, borderValue);
		return result;
	}

	inline Mat exp(cv::InputArray src)
	{
		Mat result;
		cv::exp(src, result);
		return result;
	}

	inline Mat extractChannel(InputArray src, int coi)
	{
		Mat result;
		cv::extractChannel(src, result, coi);
		return result;
	}

	inline Mat flip(InputArray src, int flipCode)
	{
		Mat result;
		cv::flip(src, result, flipCode);
		return result;
	}

	inline Mat GaussianBlur(InputArray src, Size ksize, double sigmaX, double sigmaY = 0, int borderType = BORDER_DEFAULT)
	{
		Mat result;
		cv::GaussianBlur(src, result, ksize, sigmaX, sigmaY, borderType);
		return result;
	}

	inline Mat invert(InputArray src, int flags = DECOMP_LU)
	{
		Mat result;
		cv::invert(src, result, flags);
		return result;
	}

	// Calculates the natural logarithm of every array element.
	inline Mat log(InputArray src)
	{
		Mat result;
		cv::log(src, result);
		return result;
	}

	inline double max(InputArray src)
	{
		double maxVal;
		cv::minMaxIdx(src, nullptr, &maxVal);
		return maxVal;
	}

	inline Mat merge(InputArrayOfArrays mv)
	{
		Mat result;
		cv::merge(mv, result);
		return result;
	}

	inline double min(InputArray src)
	{
		double minVal;
		cv::minMaxIdx(src, &minVal, nullptr);
		return minVal;
	}

	// Calculates the per-element scaled product of two arrays.
	inline Mat multiply(InputArray src1, InputArray src2, double scale = 1, int dtype = -1)
	{
		Mat result;
		cv::multiply(src1, src2, result, scale, dtype);
		return result;
	}

	// Normalises the norm or value range of an array.
	inline Mat normalize(InputArray src, double alpha = 1, double beta = 0, int norm_type = NORM_L2, int dtype = -1, InputArray mask = noArray())
	{
		Mat result;
		cv::normalize(src, result, alpha, beta, norm_type, dtype, mask);
		return result;
	}

	inline Mat pow(InputArray src, double power)
	{
		Mat result;
		cv::pow(src, power, result);
		return result;
	}

	inline Mat remap(InputArray src, InputArray map1, InputArray map2, int interpolation,
	                 int borderMode = BORDER_CONSTANT, const Scalar& borderValue = Scalar())
	{
		cv::Mat result;
		cv::remap(src, result, map1, map2, interpolation, borderMode, borderValue);
		return result;
	}

	inline Mat remap(InputArray src, InputArray map, int interpolation, int borderMode = BORDER_CONSTANT,
	                 const Scalar& borderValue = Scalar())
	{
		cv::Mat result;
		cv::remap(src, result, map, cv::Mat(), interpolation, borderMode, borderValue);
		return result;
	}

	inline Mat resize(InputArray src, Size dsize, double fx = 0, double fy = 0, int interpolation = INTER_LINEAR)
	{
		if (src.getMat().data == nullptr) return src.getMat();
		Mat result;
		cv::resize(src, result, dsize, fx, fy, interpolation);
		return result;
	}

	// Splits a weakly typed multi-channel array into several weakly typed single-channel arrays.
	inline const std::vector<Mat> split(const Mat& src)
	{
		std::vector<Mat> result(src.channels());
		cv::split(src, result);
		return result;
	}

	// Splits a strongly typed multi-channel array into several strongly typed single-channel arrays.
	template <typename Type, int Channels>
	inline std::vector<Mat_<Type>> split(const Mat_<Vec<Type, Channels>>& src)
	{
		std::vector<Mat_<Type>> result(Channels);
		split<Type>(src, result);
		return result;
	}

	inline Mat sqrt(InputArray src)
	{
		Mat result;
		cv::sqrt(src, result);
		return result;
	}

	// Calculates the per-element difference between two arrays or array and a scalar.
	// (Removed mask argument to avoid ambiguous function signature.)
	inline Mat subtract(InputArray src1, InputArray src2, /*InputArray mask = noArray(),*/ int dtype = -1)
	{
		Mat result;
		cv::subtract(src1, src2, result, noArray(), dtype);
		return result;
	}

	inline cv::Mat threshold(InputArray src, double thresh, double maxval, int type = cv::THRESH_BINARY)
	{
		Mat result;
		cv::threshold(src, result, thresh, maxval, type);
		return result;
	}

	//---- More Python-like interfaces -----------------------------------------------------------------

	inline cv::Mat applyColorMap(cv::InputArray src, int colormap)
	{
		cv::Mat converted_src;
		switch (src.type() & CV_MAT_DEPTH_MASK)
		{
				// Signed types are not implemented and will raise an error below.
				//case CV_8S:
				//case CV_16S:
				//case CV_32S:

			case CV_16U:
				src.getMat().convertTo(converted_src, CV_8U, 1. / 256);
				break;

			// Assuming unit range [0, 1] input.
			case CV_32F:
			case CV_64F:
				src.getMat().convertTo(converted_src, CV_8U, 255);
				break;

			case CV_8U:
			default:
				converted_src = src.getMat();
				break;
		}

		if (converted_src.type() != CV_8UC1 && converted_src.type() != CV_8UC3)
		{
			char msg[64];
			std::sprintf(msg, "Error: Datatype '%s' not supported by applyColorMap.\n", cvtype2string(converted_src.type()).c_str());
			LOG(WARNING) << msg;
			return src.getMat();
		}

		cv::Mat dst;
		cv::applyColorMap(converted_src, dst, colormap);
		return dst;
	}


	inline int _translateColorMap(const std::string& name)
	{
		// clang-format off
		if(name == "autumn")           return cv::COLORMAP_AUTUMN;
		if(name == "bone")             return cv::COLORMAP_BONE;
		if(name == "jet")              return cv::COLORMAP_JET;
		if(name == "winter")           return cv::COLORMAP_WINTER;
		if(name == "rainbow")          return cv::COLORMAP_RAINBOW;
		if(name == "ocean")            return cv::COLORMAP_OCEAN;
		if(name == "summer")           return cv::COLORMAP_SUMMER;
		if(name == "spring")           return cv::COLORMAP_SPRING;
		if(name == "cool")             return cv::COLORMAP_COOL;
		if(name == "hsv")              return cv::COLORMAP_HSV;
		if(name == "pink")             return cv::COLORMAP_PINK;
		if(name == "hot")              return cv::COLORMAP_HOT;
		if(name == "parula")           return cv::COLORMAP_PARULA;
#if CV_VERSION_MAJOR > 4 || (CV_VERSION_MAJOR == 4 && CV_VERSION_MINOR >= 1) // The following were added in OpenCV 4.1
		if(name == "magma")            return cv::COLORMAP_MAGMA;
		if(name == "inferno")          return cv::COLORMAP_INFERNO;
		if(name == "plasma")           return cv::COLORMAP_PLASMA;
		if(name == "viridis")          return cv::COLORMAP_VIRIDIS;
		if(name == "cividis")          return cv::COLORMAP_CIVIDIS;
		if(name == "twilight")         return cv::COLORMAP_TWILIGHT;
		if(name == "twilight_shifted") return cv::COLORMAP_TWILIGHT_SHIFTED;
	#if CV_VERSION_MAJOR > 4 || (CV_VERSION_MAJOR == 4 && CV_VERSION_MINOR >= 2)
		if(name == "turbo")            return cv::COLORMAP_TURBO; // actually added in OpenCV 4.1.2, but let's say 4.2
	#endif
#endif
		return -1; // unmatched or unknown colour
		// clang-format on
	}


	inline cv::Mat applyColorMap(cv::InputArray src, std::string colormap)
	{
		int cm = _translateColorMap(colormap);
		if (cm == -1)
		{
			char msg[64];
			std::sprintf(msg, "Error: Colour map '%s' is not supported.\n", colormap.c_str());
			LOG(WARNING) << msg;
			return src.getMat();
		}

		return applyColorMap(src, cm);
	}


	// Returns evenly spaced values within a given interval.
	template <typename T>
	inline Mat_<T> arange(T start, T stop, T step = 0)
	{
		Mat_<T> result((int)ceil((stop - start) / step), 1);

		int index = 0;
		for (T x = start; x < stop; x += step)
			result(index++, 0) = x;

		return result;
	}

	// Returns evenly spaced values within a given interval [0, stop).
	template <typename T>
	inline Mat_<T> arange(T stop)
	{
		return arange(T(0), stop, T(1));
	}

	inline cv::Mat concat(cv::InputArray src1, cv::InputArray src2, int dimension)
	{
		cv::Mat result;
		cv::Mat src[] = { src1.getMat(), src2.getMat() };

		if (dimension == 0) cv::vconcat(src, 2, result);
		if (dimension == 1) cv::hconcat(src, 2, result);
		if (dimension == 2) cv::merge(  src, 2, result);

		return result;
	}

	inline cv::Mat concat(cv::InputArray src1, cv::InputArray src2, cv::InputArray src3, int dimension)
	{
		cv::Mat result;
		cv::Mat src[] = { src1.getMat(), src2.getMat(), src3.getMat() };

		if (dimension == 0) cv::vconcat(src, 3, result);
		if (dimension == 1) cv::hconcat(src, 3, result);
		if (dimension == 2) cv::merge(  src, 3, result);

		return result;
	}

	inline cv::Mat reduce(cv::InputArray src, int dimension, int rtype, int dtype = -1)
	{
		// Special code for reducing along the colour channel direction.
		if (dimension == 2)
		{
			// First reshape input matrix from   w x h x n   to   n x (w * h) x 1.
			Mat matrix = src.getMat();
			Mat reshaped = matrix.reshape(1, matrix.cols * matrix.rows);

			// Then reduce each row.
			Mat result;
			cv::reduce(reshaped, result, 1, rtype, dtype);

			// Finally return to original matrix shape (but single channel).
			result = result.reshape(1, matrix.rows);
			return result;
		}

		Mat result;
		cv::reduce(src, result, dimension, rtype, dtype);
		return result;
	}

	// Resizes an image to the given width and height.
	inline Mat resize(InputArray src, int width, int height, int interpolation = INTER_LINEAR)
	{
		if (src.getMat().data == nullptr) return src.getMat();
		Mat result;
		cv::resize(src, result, Size(width, height), 0., 0., interpolation);
		return result;
	}

	// Resizes an image using the given scale factor.
	inline Mat resize(InputArray src, double factor, int interpolation = INTER_LINEAR)
	{
		if (src.getMat().data == nullptr) return src.getMat();
		Mat result;
		cv::resize(src, result, Size(), factor, factor, interpolation);
		return result;
	}

	inline Mat mean(InputArray src, int dimension, int dtype = -1)
	{
		return reduce(src, dimension, REDUCE_AVG, dtype);
	}

	inline cv::Mat sum(cv::InputArray src, int dimension, int dtype = -1)
	{
		return reduce(src, dimension, REDUCE_SUM, dtype);
	}

	//---- Improved handling of BGRA images ------------------------------------------------------------

	inline cv::Mat imreadBGRA(std::string filename)
	{
		cv::Mat image = cv::imread(filename, -1);
		if (image.data == nullptr) return cv::Mat4f();

		// Add alpha channel if missing.
		cv::Mat image_BGRA;
		switch (image.channels())
		{
			case 1: image_BGRA = cvtColor(image, COLOR_GRAY2BGRA); break;
			case 3: image_BGRA = cvtColor(image, COLOR_BGR2BGRA); break;
			case 4: image_BGRA = image; break;
			default:
			{
				std::cerr << "Error: number of colour channels (" << image.channels() << ") not supported for '" << filename << "'." << std::endl;
				return cv::Mat4f();
			}
		}

		// Assert that images are now bytes with 4 channels.
		if (image_BGRA.type() != CV_8UC4)
		{
			char msg[64];
			sprintf(msg, "Error: unknown pixel type (%i) of '%s'.\n", image_BGRA.type(), filename.c_str());
			LOG(WARNING) << msg;
			return cv::Mat4f();
		}

		// Convert to 0..1 range.
		cv::Mat4f image_BGRA32F;
		image_BGRA.convertTo(image_BGRA32F, CV_32FC4, 1. / 255);

		// Convert to premultiplied alpha.
		if (image_BGRA32F.channels() == 4)
		{
			for (auto it = image_BGRA32F.begin(); it != image_BGRA32F.end(); it++)
			{
				auto alpha = (*it)[3];
				auto premultiplied_RGBA = (*it).mul(cv::Vec4f(alpha, alpha, alpha, 1.0f));
				*it = premultiplied_RGBA;
			}
		}

		//  TODO:
		//    ## Convert data type.
		//    if dtype is not None and image.dtype != dtype:
		//        if dtype in [np.uint8, np.uint16, np.uint32, np.uint64]:
		//            image = (np.iinfo(dtype).max * image + 0.5).astype(dtype)
		//        else:
		//            image = image.astype(dtype)

		return std::move(image_BGRA32F);
	}


	inline void imwriteBGRA(const std::string& filename, cv::InputArray image, const std::vector<int>& params = std::vector<int>())
	{
		// Convert premultiplied alpha to non-premultiplied alpha according to PNG spec.
		cv::Mat image_mat = image.getMat();
		if (image_mat.channels() == 4)
		{
			// Convert to float image.
			cv::Mat4f image_BGRA32F;
			if (image_mat.type() == CV_8UC4)
			{
				image_BGRA32F = convert(image_mat, CV_32F, 1. / 255);
			}
			else if (image_mat.type() == CV_64FC4)
			{
				image_BGRA32F = convert(image_mat, CV_32F);
			}
			else if (image_mat.type() == CV_32FC4)
			{
				image_BGRA32F = image_mat.clone();
			}
			else
			{
				LOG(ERROR) << "Error in imwriteBGRA: unsupported pixel type (" << std::to_string(image_mat.type()) << ").";
				return;
			}

			// Divide by alpha (with clamping).
			for (auto it = image_BGRA32F.begin(); it != image_BGRA32F.end(); it++)
			{
				float alpha = max((*it)[3], 1.0e-10f);
				cv::Vec4f non_premultiplied = (*it).mul(cv::Vec4f(1 / alpha, 1 / alpha, 1 / alpha, 1.0f));
				*it = clamp(non_premultiplied, 0.0f, 1.0f);
			}

			image_mat = image_BGRA32F;
		}

		// Convert data type back to bytes.
		if (image_mat.depth() == CV_32F || image_mat.depth() == CV_64F)
			image_mat = convert(image_mat, CV_8U, 255., 0.5);

		cv::imwrite(filename, image_mat, params);
	}


	// imshow with support for alpha
	inline void imshowA(const std::string& winname, cv::InputArray mat)
	{
		cv::Mat image = mat.getMat();

		// No data? -> show as mid-grey
		if (image.data == nullptr)
			image = 0.5f * cv::Mat3f::ones(1, 1);

		// Handling colour images with alpha.
		if (image.channels() == 4)
		{
			// First convert to float image.
			Mat4f float_image;
			if (image.depth() == CV_8U)
				float_image = convert(image, CV_32F, 1. / 255);
			else
				float_image = image;

			// Then compose on checkerboard background for display.
			Mat3f display_image(image.rows, image.cols);
			for (int y = 0; y < image.rows; y++)
			{
				for (int x = 0; x < image.cols; x++)
				{
					float bg_colour = 0.8f + 0.2f * (((x / 8) % 2) ^ ((y / 8) % 2));

					Vec4f pixel_in = float_image.at<Vec4f>(y, x);
					Vec4f pixel_bg = Vec4f(bg_colour, bg_colour, bg_colour, 1);
					Vec4f pixel_out = pixel_in + (1.f - pixel_in[3]) * pixel_bg; // over compositing

					display_image.at<Vec3f>(y, x) = Vec3f(&pixel_out[0]);
				}
			}
			image = display_image;
		}

		imshow(winname, image);
	}

	inline cv::Mat4f compose(cv::Mat4f foreground, cv::Mat4f background)
	{
		cv::Mat4f result = cv::Mat4f::zeros(foreground.rows, foreground.cols);

		for (int y = 0; y < result.rows; y++)
		{
			for (int x = 0; x < result.cols; x++)
			{
				auto F = foreground.at<Vec4f>(y, x);
				auto B = background.at<Vec4f>(y, x);

				result.at<Vec4f>(y, x) = F + (1 - F[3]) * B; // 'over' compositing
			}
		}

		return result;
	}

	//---- Image metrics -------------------------------------------------------------------------------

	// Computes the root mean squared error (difference) between matrices a and b.
	inline double rmse(cv::Mat a, cv::Mat b)
	{
		if (a.empty() || b.empty())
			return -1;

		cv::Mat diff = convert(a, CV_64F) - convert(b, CV_64F);
		return sqrt(mean(pow(diff, 2.0))[0]);
	}

	// Adapted from <http://docs.opencv.org/doc/tutorials/highgui/video-input-psnr-ssim/video-input-psnr-ssim.html>
	inline double psnr(const cv::Mat& I1, const cv::Mat& I2)
	{
		// Compute sum of squared errors (SSE).
		Mat s1 = convert(absdiff(I1, I2), CV_32F);
		s1 = s1.mul(s1);
		Scalar s = sum(s1);
		double sse = s.val[0] + s.val[1] + s.val[2];

		// For small values, return zero.
		if (sse <= 1e-10) return 0.0;

		double mse = sse / (double)(I1.channels() * I1.total());
		return 10.0 * log10(255. * 255. / mse);
	}

} // namespace cv

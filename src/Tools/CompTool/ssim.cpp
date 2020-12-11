// SSIM of C++ code for OpenCV2.x
// Norishige Fukushima, Nagoya Institute of Technology, Japan
// e-mail fukushima "at" nitech.ac.jp

//Refercence list for this code

/* [1]
* 
* http://denshikA.cc
* (japanese page)
* OpenCV is required
* http://opencv.jp/
* (japanese page)
*/

/*[2]
*
* The equivalent of Zhou Wang's SSIM matlab code using OpenCV.
* from http://www.cns.nyu.edu/~zwang/files/research/ssim/index.html
* The measure is described in :
* "Image quality assessment: From error measurement to structural similarity"
* C++ code by Rabah Mehdi. http://mehdi.rabah.free.fr/SSIM
*
* This implementation is under the public domain.
* @see http://creativecommons.org/licenses/publicdomain/
* The original work may be under copyrights. 
*
*/

/*
This DSSIM program has been created by Philipp Klaus Krause based on
Rabah Mehdi's C++ implementation of SSIM (http://mehdi.rabah.free.fr/SSIM).
Originally it has been created for the VMV '09 paper
"ftc - floating precision texture compression" by Philipp Klaus Krause.

The latest version of this program can probably be found somewhere at
http://www.colecovision.eu.

It can be compiled using g++ -I/usr/include/opencv -lcv -lhighgui dssim.cpp
Make sure OpenCV is installed (e.g. for Debian/ubuntu: apt-get install
libcv-dev libhighgui-dev).

DSSIM is described in
"Structural Similarity-Based Object Tracking in Video Sequences" by Loza et al.
however setting all Ci to 0 as proposed there results in numerical instabilities.
Thus this implementation used the Ci from the SSIM implementation.
SSIM is described in
"Image quality assessment: from error visibility to structural similarity" by Wang et al.
*/

/*
Copyright (c) 2005, Rabah Mehdi <mehdi.rabah@gmail.com>

Feel free to use it as you want and to drop me a mail
if it has been useful to you. Please let me know if you enhance it.
I'm not responsible if this program destroy your life & blablabla :)

Copyright (c) 2009, Philipp Klaus Krause <philipp@colecovision.eu>

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

/*
TODO: This needs porting to OpenCV 4, which doens't support IplImage anymore
============================================================================

#include "ssim.h"
static double getSSIM(IplImage* src1, IplImage* src2, IplImage* mask,
					  const double K1,
					  const double K2,
					  const int L,
					  const int downsamplewidth,
					  const int gaussian_window,
					  const double gaussian_sigma,
					  IplImage* dest)
{
	// default settings
	const double C1 = (K1 * L) * (K1 * L); //6.5025 C1 = (K(1)*L)^2;
	const double C2 = (K2 * L) * (K2 * L); //58.5225 C2 = (K(2)*L)^2;

	//　get width and height
	int x=src1->width, y=src1->height;

	//　distance (down sampling) setting
	int rate_downsampling = std::max(1, int((std::min(x,y) / downsamplewidth) + 0.5));

	int nChan=1, d=IPL_DEPTH_32F;

	//　size before down sampling
	CvSize size_L = cvSize(x, y);

	//　size after down sampling
	CvSize size = cvSize(x / rate_downsampling, y / rate_downsampling);

	//　image allocation
	cv::Ptr<IplImage> img1 = cvCreateImage( size, d, nChan);
	cv::Ptr<IplImage> img2 = cvCreateImage( size, d, nChan);


	//　convert 8 bit to 32 bit float value
	cv::Ptr<IplImage> img1_L = cvCreateImage( size_L, d, nChan);
	cv::Ptr<IplImage> img2_L = cvCreateImage( size_L, d, nChan);
	cvConvert(src1, img1_L);
	cvConvert(src2, img2_L);

	//　down sampling
	cvResize(img1_L, img1);
	cvResize(img2_L, img2);

	//　buffer alocation
	cv::Ptr<IplImage> img1_sq = cvCreateImage( size, d, nChan);
	cv::Ptr<IplImage> img2_sq = cvCreateImage( size, d, nChan);
	cv::Ptr<IplImage> img1_img2 = cvCreateImage( size, d, nChan);

	//　pow and mul
	cvPow( img1, img1_sq, 2 );
	cvPow( img2, img2_sq, 2 );
	cvMul( img1, img2, img1_img2, 1 );

	//　get sigma
	cv::Ptr<IplImage> mu1 = cvCreateImage( size, d, nChan);
	cv::Ptr<IplImage> mu2 = cvCreateImage( size, d, nChan);
	cv::Ptr<IplImage> mu1_sq = cvCreateImage( size, d, nChan);
	cv::Ptr<IplImage> mu2_sq = cvCreateImage( size, d, nChan);
	cv::Ptr<IplImage> mu1_mu2 = cvCreateImage( size, d, nChan);


	cv::Ptr<IplImage> sigma1_sq = cvCreateImage( size, d, nChan);
	cv::Ptr<IplImage> sigma2_sq = cvCreateImage( size, d, nChan);
	cv::Ptr<IplImage> sigma12 = cvCreateImage( size, d, nChan);

	//　allocate buffer
	cv::Ptr<IplImage> temp1 = cvCreateImage( size, d, nChan);
	cv::Ptr<IplImage> temp2 = cvCreateImage( size, d, nChan);
	cv::Ptr<IplImage> temp3 = cvCreateImage( size, d, nChan);

	//ssim map
	cv::Ptr<IplImage> ssim_map = cvCreateImage( size, d, nChan);


	//////////////////////////////////////////////////////////////////////////
	// 	// PRELIMINARY COMPUTING

	//　gaussian smooth
	cvSmooth( img1, mu1, CV_GAUSSIAN, gaussian_window, gaussian_window, gaussian_sigma );
	cvSmooth( img2, mu2, CV_GAUSSIAN, gaussian_window, gaussian_window, gaussian_sigma );

	//　get mu
	cvPow( mu1, mu1_sq, 2 );
	cvPow( mu2, mu2_sq, 2 );
	cvMul( mu1, mu2, mu1_mu2, 1 );

	//　calc sigma
	cvSmooth( img1_sq, sigma1_sq, CV_GAUSSIAN, gaussian_window, gaussian_window, gaussian_sigma );
	cvAddWeighted( sigma1_sq, 1, mu1_sq, -1, 0, sigma1_sq );

	cvSmooth( img2_sq, sigma2_sq, CV_GAUSSIAN, gaussian_window, gaussian_window, gaussian_sigma);
	cvAddWeighted( sigma2_sq, 1, mu2_sq, -1, 0, sigma2_sq );

	cvSmooth( img1_img2, sigma12, CV_GAUSSIAN, gaussian_window, gaussian_window, gaussian_sigma );
	cvAddWeighted( sigma12, 1, mu1_mu2, -1, 0, sigma12 );


	//////////////////////////////////////////////////////////////////////////
	// FORMULA

	// (2*mu1_mu2 + C1)
	cvScale( mu1_mu2, temp1, 2 );
	cvAddS( temp1, cvScalarAll(C1), temp1 );

	// (2*sigma12 + C2)
	cvScale( sigma12, temp2, 2 );
	cvAddS( temp2, cvScalarAll(C2), temp2 );

	// ((2*mu1_mu2 + C1).*(2*sigma12 + C2))
	cvMul( temp1, temp2, temp3, 1 );

	// (mu1_sq + mu2_sq + C1)
	cvAdd( mu1_sq, mu2_sq, temp1 );
	cvAddS( temp1, cvScalarAll(C1), temp1 );

	// (sigma1_sq + sigma2_sq + C2)
	cvAdd( sigma1_sq, sigma2_sq, temp2 );
	cvAddS( temp2, cvScalarAll(C2), temp2 );

	// ((mu1_sq + mu2_sq + C1).*(sigma1_sq + sigma2_sq + C2))
	cvMul( temp1, temp2, temp1, 1 );

	// ((2*mu1_mu2 + C1).*(2*sigma12 + C2))./((mu1_sq + mu2_sq + C1).*(sigma1_sq + sigma2_sq + C2))
	cvDiv( temp3, temp1, ssim_map, 1 );

	cv::Ptr<IplImage> stemp = cvCreateImage( size, IPL_DEPTH_8U, 1);
	cv::Ptr<IplImage> mask2 = cvCreateImage( size, IPL_DEPTH_8U, 1);

	cvConvertScale(ssim_map, stemp, 255.0, 0.0);
	cvResize(stemp,dest);
	cvResize(mask,mask2);

	CvScalar index_scalar = cvAvg( ssim_map, mask2 );

	// through observation, there is approximately 
	// 1% error max with the original matlab program

	return index_scalar.val[0];
}


double xcvCalcSSIM(IplImage* src, IplImage* dest, int channel, int method, IplImage* _mask,
				   const double K1,
				   const double K2,
				   const int L,
				   const int downsamplewidth,
				   const int gaussian_window,
				   const double gaussian_sigma,
				   IplImage* ssim_map
				   )
{  
	IplImage* mask;
	IplImage* __mask=cvCreateImage(cvGetSize(src),8,1);
	IplImage* smap=cvCreateImage(cvGetSize(src),8,1);

	cvSet(__mask,cvScalarAll(255));


	if(_mask==NULL)mask=__mask;
	else mask=_mask;
	IplImage* ssrc;
	IplImage* sdest;
	if(src->nChannels==1)
	{
		ssrc=cvCreateImage(cvGetSize(src),8,3);
		sdest=cvCreateImage(cvGetSize(src),8,3);
		cvCvtColor(src,ssrc,CV_GRAY2BGR);
		cvCvtColor(dest,sdest,CV_GRAY2BGR);
	}
	else
	{
		ssrc = cvCloneImage(src);
		sdest = cvCloneImage(dest);
		cvCvtColor(dest,sdest,method);
		cvCvtColor(src,ssrc,method);
	}

	IplImage* gray[4];
	IplImage* sgray[4];
	for(int i=0;i<4;i++)
	{
		gray[i] = cvCreateImage(cvGetSize(src),8,1);
		sgray[i] = cvCreateImage(cvGetSize(src),8,1);
	}

	cvSplit(sdest,gray[0],gray[1],gray[2],NULL);
	cvSplit(ssrc,sgray[0],sgray[1],sgray[2],NULL);

	double sn=0.0;

	if(channel==ALLCHANNEL)
	{
		for(int i=0;i<src->nChannels;i++)
		{
			sn+=getSSIM(sgray[i],gray[i],mask,K1,K2,L,downsamplewidth,gaussian_window,gaussian_sigma,smap);
		}
		sn/=(double)src->nChannels; 
	}
	else
	{
		sn	= getSSIM(sgray[channel],gray[channel],mask,K1,K2,L,downsamplewidth,gaussian_window,gaussian_sigma,smap);
	}

	for(int i=0;i<4;i++)
	{
		cvReleaseImage(&gray[i]);
		cvReleaseImage(&sgray[i]);
	}

	if(ssim_map!=NULL)cvCopy(smap,ssim_map);
	cvReleaseImage(&smap);
	cvReleaseImage(&__mask);
	cvReleaseImage(&ssrc);
	cvReleaseImage(&sdest);
	return sn;
}

double xcvCalcDSSIM(IplImage* src, IplImage* dest, int channel, int method, IplImage* _mask,
					const double K1,
					const double K2,
					const int L,
					const int downsamplewidth,
					const int gaussian_window,
					const double gaussian_sigma,
					IplImage* ssim_map
					)
{
	double ret = xcvCalcSSIM(src, dest, channel, method, _mask, K1,K2,L,downsamplewidth,gaussian_window,gaussian_sigma,ssim_map);
	if(ret==0)ret=-1.0;
	else ret =(1.0 / ret) - 1.0;
	return ret;
}
double xcvCalcSSIMBB(IplImage* src, IplImage* dest, int channel, int method, int boundx,int boundy,const double K1,	const double K2,	const int L, const int downsamplewidth, const int gaussian_window, const double gaussian_sigma, IplImage* ssim_map)
{
	IplImage* mask = cvCreateImage(cvGetSize(src),8,1);
	cvZero(mask);
	cvRectangle(mask,cvPoint(boundx,boundy),cvPoint(src->width-boundx,src->height-boundy),cvScalarAll(255),CV_FILLED);

	double ret = xcvCalcSSIM(src,dest,channel,method,mask,K1,K2,L,downsamplewidth,gaussian_window,gaussian_sigma,ssim_map);
	cvReleaseImage(&mask);
	return ret;
}

double xcvCalcDSSIMBB(IplImage* src, IplImage* dest, int channel, int method, int boundx,int boundy,const double K1,	const double K2,	const int L, const int downsamplewidth, const int gaussian_window, const double gaussian_sigma, IplImage* ssim_map)
{
	IplImage* mask = cvCreateImage(cvGetSize(src),8,1);
	cvZero(mask);
	cvRectangle(mask,cvPoint(boundx,boundy),cvPoint(src->width-boundx,src->height-boundy),cvScalarAll(255),CV_FILLED);

	double ret = xcvCalcSSIM(src,dest,channel,method,mask,K1,K2,L,downsamplewidth,gaussian_window,gaussian_sigma,ssim_map);
	cvReleaseImage(&mask);

	if(ret==0)ret=-1.0;
	else ret = (1.0 / ret) - 1.0;
	return ret;
}


double calcSSIM(cv::Mat src1, cv::Mat src2, int channel, int method, cv::Mat mask, const double K1, const double K2, const int L, const int downsamplewidth, const int gaussian_window, const double gaussian_sigma, cv::Mat* ssim_map)
{
	cv::Mat smap = (ssim_map == NULL ? cv::Mat() : *ssim_map);
	if(smap.empty()) smap.create(src1.size(), CV_8U);
	IplImage isrc1(src1), isrc2(src2), imask(mask), ismap(smap);
	
	if(mask.empty())
	{
		return xcvCalcSSIM(&isrc1, &isrc2, channel, method, NULL, K1, K2, L, downsamplewidth, gaussian_window, gaussian_sigma, &ismap);
	}
	else
	{
		return xcvCalcSSIM(&isrc1, &isrc2, channel, method, &imask, K1, K2, L, downsamplewidth, gaussian_window, gaussian_sigma, &ismap);
	}
}

double calcSSIMBB(cv::Mat src1, cv::Mat src2, int channel, int method, int boundx, int boundy, const double K1, const double K2, const int L, const int downsamplewidth, const int gaussian_window, const double gaussian_sigma, cv::Mat* ssim_map)
{
	cv::Mat smap = (ssim_map == NULL ? cv::Mat() : *ssim_map);
	if(smap.empty()) smap.create(src1.size(), CV_8U);
	IplImage isrc1(src1), isrc2(src2), ismap(smap);
	
	return xcvCalcSSIMBB(&isrc1, &isrc2, channel, method, boundx, boundy, K1, K2, L, downsamplewidth, gaussian_window, gaussian_sigma, &ismap);
}

double calcDSSIM(cv::Mat src1, cv::Mat src2, int channel, int method, cv::Mat mask, const double K1, const double K2, const int L, const int downsamplewidth, const int gaussian_window, const double gaussian_sigma, cv::Mat* ssim_map)
{
	cv::Mat smap = (ssim_map == NULL ? cv::Mat() : *ssim_map);
	if(smap.empty()) smap.create(src1.size(), CV_8U);
	IplImage isrc1(src1), isrc2(src2), imask(mask), ismap(smap);

	if(mask.empty())
	{
		return xcvCalcDSSIM(&isrc1, &isrc2, channel, method, NULL, K1, K2, L, downsamplewidth, gaussian_window, gaussian_sigma, &ismap);
	}
	else
	{
		return xcvCalcDSSIM(&isrc1, &isrc2, channel, method, &imask, K1, K2, L, downsamplewidth, gaussian_window, gaussian_sigma, &ismap);
	}
}

double calcDSSIMBB(cv::Mat src1, cv::Mat src2, int channel, int method, int boundx, int boundy, const double K1, const double K2, const int L, const int downsamplewidth, const int gaussian_window, const double gaussian_sigma, cv::Mat* ssim_map)
{
	cv::Mat smap = (ssim_map == NULL ? cv::Mat() : *ssim_map);
	if(smap.empty()) smap.create(src1.size(), CV_8U);
	IplImage isrc1(src1), isrc2(src2), ismap(smap);

	return xcvCalcDSSIMBB(&isrc1, &isrc2, channel, method, boundx, boundy, K1, K2, L, downsamplewidth, gaussian_window, gaussian_sigma, &ismap);
}
*/
#include"image.h"
#include "opencv/cv.h"
#include "opencv/highgui.h"
#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
IplImage* DeskewImage(IplImage* src)
{
	if(src == NULL)
		return NULL;

	float angle;
	IM_POINT impt[4];
	IM_RECT imrc;
	if (0 != auto_rotate(src, angle, impt, imrc, 0))
	{
		return NULL;
	}
	if(1)//用来多纠
	{
		imrc.left = imrc.left +10;
		imrc.top = imrc.top +10;
		imrc.right = imrc.right -10;
		imrc.bottom = imrc.bottom -15;
	}

	int dst_width  = imrc.right - imrc.left;
	int dst_height = imrc.bottom - imrc.top;

	IplImage *dst = cvCreateImage(cvSize(dst_width, dst_height),src->depth, src->nChannels);
	if (NULL == dst)
	{
		return NULL;
	}

	if (0 != image_rotate_crop(src, dst, angle, imrc))
	{
		cvReleaseImage(&dst);
		return NULL;
	}
	return dst;

}

int SaveImage(IplImage* image,char* filename)
{
	if(image == NULL)
		return -1;
	cvSaveImage(filename,image,0);
	return 0;
}

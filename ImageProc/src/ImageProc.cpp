
#include "opencv/cv.h"
#include "opencv/highgui.h"
#include "ImageProc.h"
#include <vector>
#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#define  WIDTH_DELBKG_COLOR			200
#define  HEIGTH_DELBKG_COLOR		200

using namespace cv;
using namespace std;

#define GRAY_CONTRAST_MAP(grayOut,grayIn)                                         \
{                                                                                 \
	if (grayIn < 28)                                                              \
	{                                                                             \
		grayOut = 0;                                                              \
	}                                                                             \
	else if (grayIn > 228)                                                        \
	{                                                                             \
		grayOut = 255;                                                            \
	}                                                                             \
	else if (grayIn < 128)                                                        \
	{                                                                             \
		grayOut = (grayIn-28) * (grayIn-28) * 128.0f / 10000;                     \
	}                                                                             \
	else                                                                          \
	{                                                                             \
		grayOut = (-128.0f) / 10000 * (grayIn-228) * (grayIn-228) + 255;          \
	}                                                                             \
                                                                                  \
} 

#define GRAY_IMP_BRIGHT(grayOut,grayIn)                                           \
{                                                                                 \
	if (grayIn < 0)                                                               \
	{                                                                             \
		grayIn = 0;                                                               \
	}                                                                             \
	if (grayIn > 255)                                                             \
	{                                                                             \
		grayIn = 255;                                                             \
	}                                                                             \
	grayOut = (255-grayIn) * (255-grayIn) * (255-grayIn)*((-1)/(255.0f*255.0f))  + 255.0f;                 \
	/*grayOut = (255-grayIn) * (255-grayIn)*((-1)/(255.0f))  + 255.0f; */                \
}

#define GRAY_HIGH_LIGHT(grayOut,grayIn)                                           \
{                                                                                 \
	if (grayIn < 0)                                                               \
	{                                                                             \
		grayIn = 0;                                                               \
	}                                                                             \
	if (grayIn > 230)                                                             \
	{                                                                             \
		grayIn = 255;                                                             \
	}                                                                             \
	else                                                                          \
	{                                                                             \
        grayOut = (255.0f/230.0f) * grayIn;                                       \
	}                                                                             \
}

enum DOC_TYPE
{
	DOC_A4PAPER = 0,
	DOC_BILL = 1,
	DOC_BCARD = 2,
	DOC_IDCARD = 3,
	DOC_UNDEFINE = 4,    
	DOC_CUSTOMER = 5
};
void my_sort(float *arr, int n)
{
    int i = 0;
    int j = 0;
    for (i = 0; i < n-1; i++)
    {
        for (j = 0; j < n-1 - i; j++)
        {
            if (*(arr+j) > *(arr+j+1))
            {
                float temp = *(arr+j);
                *(arr+j) = *(arr+j+1);
                *(arr+j+1) = temp;
            }
        }
    }
}
// GZM
int otsu(const uchar *bt_im_data, int width, int height, int widthstep)
{
    if (!bt_im_data
        || width <= 0
        || height <= 0
        )
        return 0;
    int threshold = 0;
    int hist[256];
    float prob[256];
    memset (&hist,0,256*sizeof(int));
    int x = 0;
    int y = 0;
    int k = 0;
    float prob_class1 = 0;
    float prob_class2 = 0;
    float mean1 = 0;
    float mean2 = 0;
    float sigma_b[256];
    memset (&sigma_b,0,256*sizeof(float));

    for (y = 0; y < height; y++ )
    {
        uchar* pbt = (uchar*)(bt_im_data + y*widthstep);
        for (x = 0; x < width; x++)
        {
            ( *(hist + *(pbt+x)) )++;
        }
    }


    // histgraph
    for (x = 0; x < 256; x++)
    {
        prob[x] = hist[x]/(float)(width*height);
    }

    // find the K value make the inter-clase Variance largest
    for (k = 10; k < 250; k++)
    {
        prob_class1 = prob_class2 = 0;
        // probability of class c1 and c2
        for (x = 0; x < k+1; x++)
        {
            prob_class1 += prob[x];
        }
        prob_class2 = fabs(1 - prob_class1);

        // average gray of C1
        for (x = 0; x < k+1; x++)
        {
            mean1 += x * prob[x];
        }
         mean1 /= prob_class1;

        // average gray of c2
        for (x = k+1; x < 256; x++)
        {
            mean2 += x * prob[x];
        }
        mean2 /= prob_class2;



        // inter-class variance
        sigma_b[k] = prob_class1*prob_class2*(mean1-mean2)*(mean1-mean2);

        // make the value zero each cycl
        mean1 = mean2  = 0;

    }

    for (k = 0;k < 256;k++)
    {
        if (sigma_b[k] > sigma_b[0])
        {
            sigma_b[0] = sigma_b[k];
            threshold = k;
        }

    }

    return threshold;

}

int image_map(IM_POINT * newPos,
			 IM_POINT * oldPos,
			 float angle,
			 int newHeight, int newWidth,
			 int oldHeight, int oldWidth,
			 int isGetSrcPost = 1)
{
	if (   !newPos
		|| !oldPos)
		return 0;
	float cos_ang = cos(angle);
	float sin_ang = sin(angle);
	int i = 0;
	isGetSrcPost = isGetSrcPost > 0 ? 1 : 0;
	
	if (isGetSrcPost == 1)
	{

		float N1 = -0.5 * newWidth * cos_ang - 0.5 * newHeight * sin_ang + 0.5 * oldWidth;
		float N2 =  0.5 * newWidth * sin_ang - 0.5 * newHeight * cos_ang + 0.5 * oldHeight;
		for (i = 0; i < 4; i++)
		{
			(*(oldPos+i)).x =        (*(newPos+i)).x * cos_ang + (*(newPos+i)).y * sin_ang + N1;
			(*(oldPos+i)).y = (-1) * (*(newPos+i)).x * sin_ang + (*(newPos+i)).y * cos_ang + N2;
		}

	}
	else 
	{

		float M1 = -0.5 * oldWidth * cos_ang + 0.5 * oldHeight * sin_ang + 0.5 * newWidth ;
		float M2 = -0.5 * oldWidth * sin_ang - 0.5 * oldHeight * cos_ang + 0.5 * newHeight ;
		for (i = 0; i < 4; i++)
		{
			(*(newPos+i)).x =  (*(oldPos+i)).x * cos_ang - (*(oldPos+i)).y * sin_ang + M1;
			(*(newPos+i)).y =  (*(oldPos+i)).x * sin_ang + (*(oldPos+i)).y * cos_ang + M2;
		}
		
	}
	return 1;


}

int image_rotate(IplImage * src, IplImage * dst, float angle)
{
    if (!src)
    {
        return 0;
    } 
	if (!dst)
	{
        return 0;
	}
    int src_width  = src->width;
    int src_height = src->height;
    double fAngle     = angle*180/3.141592653;
    float cos_ang  = cos(angle);
    float sin_ang  = sin(angle);
    int dst_width  = fabs(src_height*sin_ang) + fabs(src_width*cos_ang);
    int dst_height = fabs(src_height*cos_ang) + fabs(src_width*sin_ang);
    double map[6];
    CvMat map_matrix = cvMat(2, 3, CV_64FC1, map);

    CvPoint2D32f pt = cvPoint2D32f(src_width / 2, src_height / 2);
    cv2DRotationMatrix(pt,fAngle, 1.0, & map_matrix);

    map[2] += (dst_width - src_width) / 2;
    map[5] += (dst_height - src_height) / 2;
   
    cvWarpAffine(
        src,
        dst,
        &map_matrix,
        CV_INTER_LINEAR | CV_WARP_FILL_OUTLIERS,
        cvScalarAll(0)
        );
    /* cvGetQuadrangleSubPix(
       image,
        imgDst,
        &map_matrix
        );*/
  
    return 1;

}

int image_crop(IplImage * src, IplImage * dst, IM_RECT rect)
{
	if (!src)
	{
		return 0;
	}
	if (!dst)
	{
		return 0;
	}
	if (  rect.left < 0 
		|| rect.top < 0
		|| rect.right > src->width
		|| rect.bottom > src->height
		|| rect.left > rect.right
		|| rect.top > rect.bottom
		)
	{
		return 0;
	}
	if (   dst->width > src->width
		|| dst->height > src->height
		)
	{
		return 0;
	}
	if (   dst->depth != src->depth
		|| dst->nChannels != src->nChannels
		)
	{
		return 0;
	}

	CvRect cvrect = cvRect(rect.left, rect.top, rect.right - rect.left, rect.bottom -  rect.top);
	cvSetImageROI(src, cvrect);

	if (   cvrect.width != dst->width
		|| cvrect.height != dst->height
		)
	{
		return 0;
	}
	cvCopy(src, dst);
	cvResetImageROI(src);

	return 1;
}
//新算法过程概述:
//step1:图像缩放并灰度化, step2:计算有效图像区域边界, step3:计算阈值及重心, step4:标注连通区, step5:计算图像顶点及倾斜角度
int auto_rotate(const IplImage *pImgSrc, float &fAngle, IM_POINT *Pos, IM_RECT &rect, int iCropType)
{
	//bool bReverse = false;//xlc add 20180515
	if(pImgSrc==NULL || Pos==NULL) return RTN_PARAM_ERR;
	if(pImgSrc->nChannels!=3 && pImgSrc->nChannels!=1) return RTN_PARAM_ERR; 

	int iReVal=RTN_SUCCESS, thVal=0, iRatio=1;
	int lineLeft=0, lineTop=0, lineRight=0, lineBottom=0;
	int const POINT_NUM=4, MIN_TH=15;
	uchar *pImgMask = NULL;
	IplImage *pThumbSrc=NULL, *pImgGray=NULL;

	//创建小图
	if(pImgSrc->width>2047 || pImgSrc->height>2047)
	{
		pThumbSrc = cvCreateImage(cvSize(pImgSrc->width>>2, pImgSrc->height>>2), pImgSrc->depth, pImgSrc->nChannels);
		iRatio = 4;
	}
	else if(pImgSrc->width>1600 || pImgSrc->height>1600)
	{
		pThumbSrc = cvCreateImage(cvSize(pImgSrc->width>>1, pImgSrc->height>>1), pImgSrc->depth, pImgSrc->nChannels);
		iRatio = 2;
	}
	else 
	{
		pThumbSrc = cvCreateImage(cvSize(pImgSrc->width, pImgSrc->height), pImgSrc->depth, pImgSrc->nChannels);
		iRatio = 1;
	}

	if(pThumbSrc)
	{
		cvResize(pImgSrc, pThumbSrc);
		pImgGray = cvCreateImage(cvSize(pThumbSrc->width, pThumbSrc->height), pThumbSrc->depth, 1);
		if(pImgGray)
		{			
			if(pThumbSrc->nChannels==3)
			{
				//BackGroundClean(pThumbSrc, pImgGray);
				//cvCvtColor(pThumbSrc, pImgGray, CV_BGR2GRAY);

				//---------背景相减法----------------start
				double R = 0;
				double G = 0;
				double B = 0;
				char* pstart = NULL;

				pstart = pThumbSrc->imageData + 5*pThumbSrc->widthStep;
				for(int j=0;j<pThumbSrc->width;j++)
				{
					R = R + (BYTE)pstart [j*3] + (BYTE)pstart [j*3+1] + (BYTE)pstart [j*3+2];
				}

				for(int i=0;i<pThumbSrc->height;i++)
				{
					pstart = pThumbSrc->imageData + i*pThumbSrc->widthStep;
					G = G + (BYTE)pstart [5*3] + (BYTE)pstart [5*3+1] + (BYTE)pstart [5*3+2];
				}

				if (R/(pThumbSrc->width*3) <80 &&  G/(pThumbSrc->height*3) <80 )// 背景比较黑 沿用以前老的纠偏算法
				{
					cvCvtColor(pThumbSrc, pImgGray, CV_BGR2GRAY);					
				}
				else
				{
					R = 0;
					G = 0;
					B = 0;
					for(int i=0;i<pThumbSrc->height;i++)
					{
						pstart = pThumbSrc->imageData + i*pThumbSrc->widthStep;
						for(int j=0;j<pThumbSrc->width;j++)
						{
							R = R + (BYTE)pstart [j*3];
							G = G + (BYTE)pstart [j*3+1];
							B = B + (BYTE)pstart [j*3+2];
						}
					}
					R = R/(pThumbSrc->height*pThumbSrc->width);
					G = G/(pThumbSrc->height*pThumbSrc->width);
					B = B/(pThumbSrc->height*pThumbSrc->width);

					char* pstartGray = NULL;
					for(int i=0;i<pThumbSrc->height;i++)
					{
						pstart = pThumbSrc->imageData + i*pThumbSrc->widthStep;
						pstartGray = pImgGray->imageData + i*pImgGray->widthStep;
						for(int j=0;j<pThumbSrc->width;j++)
						{
							if (   abs( (BYTE)pstart [j*3] - R )   >= 50 
								|| abs( (BYTE)pstart [j*3+1] - G ) >= 50 
								|| abs( (BYTE)pstart [j*3+2] - B ) >= 50 )
							{	
								pstartGray[j] = 255;
							}
							else
							{
								pstartGray[j] = 0;
							}
						}
					}
				}
				//---------背景相减法----------------end
			}
			else cvCopy(pThumbSrc, pImgGray);
			//cvSmooth(pImgGray, pImgGray, CV_MEDIAN);//中值滤波去除噪点
			pImgMask = new uchar[pImgGray->widthStep*pImgGray->height];
		}
	}

	if(pThumbSrc && pImgGray && pImgMask)
	{	
		//N_Contrast(pImgGray, pImgGray, 8);
		//REVERSE://xlc add 20180515
		//if (bReverse)
		//{
		//	cv::Mat mtReverse = pImgGray;
		//	mtReverse = 255 - mtReverse;
		//}
		
		IM_POINT baryCenter;
		baryCenter.x = baryCenter.y = 0;
		//前景
		FindBorderLine(pImgGray, lineLeft, lineTop, lineRight, lineBottom);
		thVal = otusThreshold(pImgGray);

		if(0!= thVal && thVal<MIN_TH) iReVal=RTN_AUTO_ROTATE_FAILED;//20180517 add 0!= thVal
		if(iReVal==RTN_SUCCESS) iReVal=CmpBaryCenter(pImgGray, thVal, lineLeft, lineTop, lineRight, lineBottom, &baryCenter);
		//xlc add 2017-10-11
		if (baryCenter.x<lineLeft || baryCenter.x>lineRight)
		{
			baryCenter.x = (lineLeft+lineRight)/2;
		}


		if(iReVal==RTN_SUCCESS) iReVal=FG_ConnDetect(pImgGray, thVal, lineLeft, lineTop, lineRight, lineBottom, &baryCenter, pImgMask);


		if(iReVal==RTN_SUCCESS) iReVal=FindVertexPoints(pImgGray, pImgMask, 0, Pos, &fAngle);		//标志号0
		if(iReVal==RTN_SUCCESS)
		{
			for (int i=0; i<POINT_NUM; i++)
			{
				Pos[i].x*=iRatio; Pos[i].y*=iRatio;
			}

			//旋转四个角点, 计算矩形
			float const PI=3.1415926f;
			float M1=0, M2=0, fSin=0, fCos=0;
			IM_POINT pntRotate[POINT_NUM], pntTemp;
			int sortX[POINT_NUM], sortY[POINT_NUM], distMin=0, distTemp=0, tempVal=0;
			int widthRotate=0, heightRotate=0;

			widthRotate = fabs(pImgSrc->height*sin(fAngle))+fabs(pImgSrc->width*cos(fAngle));
			heightRotate = fabs(pImgSrc->height*cos(fAngle))+fabs(pImgSrc->width*sin(fAngle));

			fSin = sin(2*PI-fAngle);		//转化为按照顺时针旋转
			fCos = cos(2*PI-fAngle);
			M1 = -0.5*pImgSrc->width*fCos+0.5*pImgSrc->height*fSin+0.5*widthRotate;
			M2 = -0.5*pImgSrc->width*fSin-0.5*pImgSrc->height*fCos+0.5*heightRotate;
			for (int j=0; j<POINT_NUM; j++)
			{
				//计算旋转后坐标
				pntRotate[j].x = Pos[j].x*fCos-Pos[j].y*fSin+M1;
				pntRotate[j].y = Pos[j].x*fSin+Pos[j].y*fCos+M2;
			}

			if(iCropType == 1)		//多裁
			{
				//对四点坐标中的x, y进行排序, 从小到大
				for (int i=0; i<POINT_NUM; i++)
				{
					sortX[i] = pntRotate[i].x;
					sortY[i] = pntRotate[i].y;
				}

				for (int i=0; i<POINT_NUM-1; i++)
				{
					for (int j=i+1; j<POINT_NUM; j++)
					{
						if(sortX[i]>sortX[j])
						{
							tempVal = sortX[i];
							sortX[i] = sortX[j];
							sortX[j] = tempVal;
						}
					}
				}

				for (int i=0; i<POINT_NUM-1; i++)
				{
					for (int j=i+1; j<POINT_NUM; j++)
					{
						if(sortY[i]>sortY[j])
						{
							tempVal = sortY[i];
							sortY[i] = sortY[j];
							sortY[j] = tempVal;
						}
					}
				}
				//rect.left=((sortX[0]+sortX[1])>>1); rect.right=((sortX[2]+sortX[3])>>1);
				//rect.top=((sortY[0]+sortY[1])>>1); rect.bottom=((sortY[2]+sortY[3])>>1);
				rect.left=sortX[1]; rect.right=sortX[2];
				rect.top=sortY[1]; rect.bottom=sortY[2];
			}
			else
			{
				rect.left = rect.top = 0xffff;
				rect.right = rect.bottom = 0;
				for (int i=0; i<POINT_NUM; i++)
				{
					if(rect.left>pntRotate[i].x) rect.left=pntRotate[i].x;
					if(rect.top>pntRotate[i].y) rect.top=pntRotate[i].y;
					if(rect.right<pntRotate[i].x) rect.right=pntRotate[i].x;
					if(rect.bottom<pntRotate[i].y) rect.bottom=pntRotate[i].y;
				}
			}

			//调整Pos中的点排序, 以便上层程序绘制矩形
			float pntAngle[POINT_NUM];
			IM_POINT centerPnt;

			//计算中心点坐标
			centerPnt.x = centerPnt.y = 0;
			for (int i=0; i<POINT_NUM; i++)
			{
				centerPnt.x += Pos[i].x;
				centerPnt.y += Pos[i].y;
			}
			centerPnt.x /= POINT_NUM;
			centerPnt.y /= POINT_NUM;

			//计算各点与中心点形成的角度
			for (int i=0; i<POINT_NUM; i++)
			{
				if(Pos[i].x == centerPnt.x)
				{
					pntAngle[i] = (Pos[i].y>centerPnt.y ? 3*PI/2 : PI/2);
				}
				else
				{
					if(Pos[i].x > centerPnt.x)
					{
						if(Pos[i].y<centerPnt.y) pntAngle[i]=atan((centerPnt.y-Pos[i].y+0.0f)/(Pos[i].x-centerPnt.x));
						else pntAngle[i]=2*PI-atan((Pos[i].y-centerPnt.y+0.0f)/(Pos[i].x-centerPnt.x));
					}
					else
					{
						if(Pos[i].y<centerPnt.y) pntAngle[i]=PI-atan((centerPnt.y-Pos[i].y+0.0f)/(centerPnt.x-Pos[i].x));
						else pntAngle[i]=PI+atan((centerPnt.y-Pos[i].y+0.0f)/(Pos[i].x-centerPnt.x));
					}
				}
			}

			//按角度由小到大排序
			float fTemp=0;
			for (int i=0; i<POINT_NUM; i++)
			{
				fTemp=pntAngle[i]; tempVal=i;
				for (int j=i+1; j<POINT_NUM; j++)
				{
					if(pntAngle[j]<fTemp)
					{
						fTemp = pntAngle[j];
						tempVal = j;
					}
				}
				if(tempVal != i)
				{
					pntTemp = Pos[i];
					Pos[i] = Pos[tempVal];
					Pos[tempVal] = pntTemp;

					pntAngle[tempVal]=pntAngle[i];
					pntAngle[i] = fTemp;
				}
			}

			//计算Pos中各点间距，距离小于一定阈值，认为不构成四边形, 则识别失败
			int const TH_DIST=128;
			for (int i=0; i<POINT_NUM-1; i++)
			{
				for (int j=i+1; j<POINT_NUM; j++)
				{
					if(sqrt(0.0+(Pos[i].x-Pos[j].x)*(Pos[i].x-Pos[j].x)+
						(Pos[i].y-Pos[j].y)*(Pos[i].y-Pos[j].y))<TH_DIST)
					{
						iReVal = RTN_AUTO_ROTATE_FAILED;
						break;
					}
				}
				if(iReVal==RTN_AUTO_ROTATE_FAILED) break;
			}
		}

		//if ( false == bReverse && RTN_SUCCESS == iReVal &&
		//	(0 == rect.left || rect.right >= pImgSrc->width-1 ||  0 == rect.top || rect.bottom >= pImgSrc->height-1 ) )
		//{
		//	//极有可能是黑色银行卡 //xlc add 20180515
		//	fAngle = 0.0f;
		//	Pos[0].x = Pos[0].y = 0;
		//	Pos[1].x = Pos[1].y = 0;
		//	Pos[2].x = Pos[2].y = 0;
		//	Pos[3].x = Pos[3].y = 0;
		//	rect.left = rect.right = rect.top = rect.bottom = 0;
		//	bReverse = true;
		//	goto REVERSE;
		//}
	}
	else iReVal=RTN_MALLOC_ERR;

	if(pThumbSrc) cvReleaseImage(&pThumbSrc);
	if(pImgGray) cvReleaseImage(&pImgGray);
	if(pImgMask) delete pImgMask;

	return iReVal;
}

int image_rotate_crop(IplImage * src, IplImage *dst, float fAngle, IM_RECT rect)
{
	if(src==NULL || dst==NULL) return RTN_PARAM_ERR;
	if(rect.left<0 || rect.top<0) return RTN_PARAM_ERR;
	if(rect.left>rect.right || rect.top>rect.bottom) return RTN_PARAM_ERR;

	float fSin=sin(fAngle), fCos=cos(fAngle);
	int widthRotate=0, heightRotate=0;
	CvPoint2D32f center;    

	//旋转后图像大小
	widthRotate = fabs(src->height*fSin)+fabs(src->width*fCos);
	heightRotate = fabs(src->height*fCos)+fabs(src->width*fSin);

	//旋转中心
	center.x = src->width/2.0f+0.5f;
	center.y = src->height/2.0f+0.5f;

	//计算二维旋转的仿射变换矩阵  
	float m[6];              
	CvRect rcDest;
	CvMat M = cvMat(2, 3, CV_32F, m);

	rcDest.x=rect.left; rcDest.y=rect.top;
	rcDest.width=rect.right-rect.left;
	rcDest.height=rect.bottom-rect.top;
	cv2DRotationMatrix(center, 180*fAngle/3.1415926, 1, &M); //角度>0表示逆时针旋转
	
	//旋转图像，并用黑色填充其余值  
	IplImage *pImgRotate = cvCreateImage(cvSize(widthRotate, heightRotate), src->depth, src->nChannels);
	if(pImgRotate)
	{
		m[2] += (widthRotate-src->width)/2;
		m[5] += (heightRotate-src->height)/2;

		cvWarpAffine(src, pImgRotate, &M, CV_INTER_LINEAR+CV_WARP_FILL_OUTLIERS, cvScalarAll(0));
		cvSetImageROI(pImgRotate, rcDest);
		cvCopy(pImgRotate, dst);
		cvResetImageROI(pImgRotate);
		cvReleaseImage(&pImgRotate);
		return RTN_SUCCESS;
	}
	else
	{
		return RTN_AUTO_ROTATE_FAILED;
	}
}


int image_rotate_crop2(IplImage * src, IplImage *dst, float angle,IM_POINT * point, IM_RECT rect)
{
/*
	if (!dst || !src)
	{
		return 0;
	}
	if (   rect.left < 0
		|| rect.top < 0)
	{
		return 0;
	}
	if (   dst->nChannels != src->nChannels
		|| dst->depth != src->depth
		|| rect.right - rect.left != dst->width
		|| rect.bottom - rect.top != dst->height
		)
	{
		return 0;
	}

	angle *= -1;
	IM_RECT rc;
	IM_RECT rc2;
	rc.left = getPossRectL(point);
	rc.top = getPossRectT(point);
	rc.right = getPossRectR(point);
	rc.bottom = getPossRectB(point);

	int height = src->height;
	int width  = src->width;
	float cos_ang = cos(angle);
	float sin_ang = sin(angle);
	int newWidth  = fabs(height*sin_ang) + fabs(width*cos_ang);
	int newHeight = fabs(height*cos_ang) + fabs(width*sin_ang);
	IM_POINT newPos[4];
	IM_POINT Pos[4];
	
	Pos[0].x = rc.left;
	Pos[0].y = rc.top;
	Pos[1].x = rc.left;
	Pos[1].y = rc.bottom;
	Pos[2].x = rc.right;
	Pos[2].y = rc.bottom;
	Pos[3].x = rc.right;
	Pos[3].y = rc.top;
	image_map(newPos, Pos,angle,
			 newHeight, newWidth,
			 height, width,0
			 );
	rc2.left = getPossRectL(newPos);
	rc2.top  = getPossRectT(newPos);
	rc2.right= getPossRectR(newPos);
	rc2.bottom= getPossRectB(newPos);

	LONG temp1 = rc2.left;
	LONG temp2 = rc2.top;
	rc2.left = rect.left - rc2.left;
	rc2.top  = rect.top  - rc2.top;
	rc2.right = rect.right - temp1;
	rc2.bottom = rect.bottom - temp2;

	angle *= 180/3.141592653;

	CvRect cvrect;
	cvrect.x = rc.left;
	cvrect.y = rc.top;
	cvrect.width = rc.right-rc.left;
	cvrect.height = rc.bottom-rc.top;

	// 第一次裁剪 crop first
	IplImage * rctmp = cvCreateImage(cvSize(rc.right-rc.left,rc.bottom-rc.top), src->depth, src->nChannels);
	if (!rctmp)
	{
		return -1;
	}
	image_crop(src, rctmp, cvrect);

	cvSaveImage("F:\\srccorp.bmp",rctmp);

	// 旋转 rotate
	
    int src_height = rctmp->height;
    int src_width  = rctmp->width;
    int dst_width  = fabs(src_height*sin_ang) + fabs(src_width*cos_ang);
    int dst_height = fabs(src_height*cos_ang) + fabs(src_width*sin_ang);
    IplImage * img_rotate = cvCreateImage(
                                           cvSize(dst_width, dst_height),
                                           rctmp->depth, 
                                           rctmp->nChannels );
    if (!img_rotate)
	{
		cvReleaseImage(&rctmp);
	    return 0;
	}
    image_rotate(rctmp, img_rotate, angle);
	
	cvrect.x = rc2.left;
	cvrect.y = rc2.top;
	cvrect.width = rc2.right-rc2.left;
	cvrect.height = rc2.bottom-rc2.top;

    image_crop(img_rotate, dst, cvrect);
	
	cvReleaseImage(&rctmp);
	cvReleaseImage(&img_rotate);
*/
	return 0;
}

int getPosition(IplImage * imGray, IM_POINT * Pos,IM_RECT & rect, float fangle, int threshold, int iCropType)
{
	if (!imGray)
        return 1;
	//IM_RECT rect;
	int x = 0;
	int y = 0;
	int i = 0;
	const float K = 0.3;//ȷͶӰֽϵ
	float cos_ang = cos(fangle);
	float sin_ang = sin(fangle);
	int width = imGray->width;
	int height = imGray->height;
	int newWidth  = fabs(height*sin_ang) + fabs(width*cos_ang);
	int newHeight = fabs(height*cos_ang) + fabs(width*sin_ang);
	int widthstep = imGray->widthStep;
	int xMax = 0;
	int yMax = 0;
	int xBarycentre = 0;
	int yBarycentre = 0;
	int sumX = 1;
	int sumY = 1;
	int *projX = new int[newWidth+1];
	int *projY = new int[newHeight+1];
// 	if (!projX || !projY)
// 		return -1;
	memset(projX, 0, (newWidth+1)*sizeof(int));
	memset(projY, 0, (newHeight+1)*sizeof(int));

    uchar *pgrayData;
	rect.top=0;
	rect.left=0;
	rect.right = newWidth-1;
	rect.bottom = newHeight-1;
	IM_POINT newPos[4];


	if (1 != imGray->nChannels)
	{
		delete []projX;
		delete []projY;
		return -1;
	}
	else if (1 == imGray->nChannels)
	{
        pgrayData = (uchar*)imGray->imageData;
		for ( y = 0; y < height; y++)
		{
            //uchar* p = (uchar*)(psrcDate + y*image.GetEffWidth());
            uchar* p = (uchar*)(pgrayData + y*widthstep);
			for ( x = 0; x < width; x++)
			{
				*(p + x) = ( *(p + x    )  )
							 < threshold ? 0 : 1;
			}
			
		}
	}


	float M1 = -0.5 * width * cos_ang + 0.5 * height * sin_ang + 0.5 * newWidth ;
	float M2 = -0.5 * width * sin_ang - 0.5 * height * cos_ang + 0.5 * newHeight ;

	widthstep = imGray->widthStep;
	for (y = 0; y < height; y++)
	{
        uchar * bySrc = (uchar*)(pgrayData + y*widthstep);
		for (x = 0; x < width; x++)
		{
			int newX = x * cos_ang - y * sin_ang + M1;
			int newY = x * sin_ang + y * cos_ang + M2;
			*(projX+newX) += *(bySrc+x);
			*(projY+newY) += *(bySrc+x);
		}
	}

	*(projX + 0 ) = 0;
	*(projX + 1 ) = 0;
	*(projX + newWidth -1) = 0;
	*(projX + newWidth -2) = 0;
	*(projY + 0) = 0;
	*(projY + 1) = 0;
	*(projY + newHeight-1) = 0;
	*(projY + newHeight-2) = 0;


	for (i = 1; i < newWidth-1; i++)
	{	
		if (xMax < *(projX+i))
		{
			xMax = *(projX+i);
		}
	}
	
	for (i = 1; i < newHeight-1; i++)
	{
		if (yMax < *(projY+i))
		{
			yMax = *(projY+i);
		}
	}

	// 
	xBarycentre = 0;
	yBarycentre = 0;
	sumX = 1;
	sumY = 1;
	for (i = 1; i < newWidth-1; i++)
	{	
		xBarycentre += *(projX+i) * i;
		sumX        += *(projX+i);
	}
	
	for (i = 1; i < newHeight-1; i++)
	{
		yBarycentre += *(projY+i) * i;
		sumY        += *(projY+i);
	}
    xBarycentre = xBarycentre/sumX;
	yBarycentre = yBarycentre/sumY;

	if ( 0 == iCropType )//多裁剪 keep clean
	{

		if ( xBarycentre > 0 && xBarycentre < newWidth && yBarycentre > 0 && yBarycentre < newHeight )   // λЧ
		{
			for (i = xBarycentre; i > 2; i--) // λ
			{
				if (*(projX+i) < xMax*K 
					&& *(projX+i-1) < xMax*K && *(projX+i-2) < xMax*K
					&& (*(projX+i+1) > xMax*K  )
					)
				{ 
					int idx = i-newWidth*0.05;
					if (idx > 2 && *(projX+idx) > xMax*K)//如果遇到中间一条线的，略过次
					{
						;
					}
					else
					{
						rect.left = i;
						break;
					}
				}
			}
			for (i = xBarycentre; i < newWidth - 2; i++)// λ
			{
				if (*(projX+i) < xMax*K 
					&& *(projX+i+1) < xMax*K  && *(projX+i+2) < xMax*K
					&& (*(projX+i-1) > xMax*K  )
					)
				{
					int idx = i+newWidth*0.05;
					if ( idx < newWidth - 2 && *(projX+idx) > xMax*K)//如果遇到中间一条线的，略过次
					{
						;
					}
					else
					{
						rect.right = i;
						break;
					}
				}
			}
			for (i = yBarycentre; i > 2; i--)
			{
				
				if (*(projY+i) < yMax*K 
					&& *(projY+i-1) < yMax*K  && *(projY+i-2) < yMax*K
					&& (*(projY+i+1) > yMax*K )
					)
				{
					int idx = i-newHeight*0.05;
					if ( idx > 2 && *(projY+idx) > yMax*K)//如果遇到中间一条线的，略过次
					{
						;
					}
					else
					{
						rect.top = i;
						break;
					}
				}
				
			}
			for (i = yBarycentre; i < newHeight - 2; i++)
			{
				
				if (*(projY+i) < yMax*K 
					&& *(projY+i+1) < yMax*K  && *(projY+i+2) < yMax*K
					&& (*(projY+i-1) > yMax*K )
					)
				{
					int idx = i+newHeight*0.05;
					if ( idx < newHeight - 2 && *(projY+idx) > yMax*K)//如果遇到中间一条线的，略过次
					{
						;
					}
					else
					{
						rect.bottom = i;
						break;
					}
				}
				
			}

		}
		else  //重心偏离纸面，从外往里面排除找出边界
		{
			for (i = 1; i < newWidth-2; i++)
			{
				
				if (*(projX+i) > xMax*K 
					&& *(projX+i-1) < xMax*K
					&& (*(projX+i+1) > xMax*K  || *(projX+i+2) > xMax*K)
					)
				{

					rect.left = i;
					break;
				}
				
			}
			for (i = newWidth-1; i > 2; i--)
			{
				
				if (   *(projX+i) > xMax*K 
					&& *(projX+i+1) < xMax*K
					&& (*(projX+i-1) > xMax*K || *(projX+i-2) > xMax*K)
					)
				{
					rect.right = i;
					break;
				}
				
			}
			for (i = 1; i < newHeight-2; i++)
			{
				
				if (*(projY+i) > yMax*K 
					&& *(projY+i-1) < yMax*K
					&& (*(projY+i+1) > yMax*K || *(projY+i+2) > yMax*K)
					)
				{
					rect.top = i;
					break;
				}
				
			}
			for (i = newHeight-1; i > 2; i--)
			{
				
				if (   *(projY+i) > yMax*K 
					&& *(projY+i+1) < yMax*K
					&& (*(projY+i-1) > yMax*K || *(projY+i-2) > yMax*K)
					)
				{
					rect.bottom = i;
					break;
				}
				
			}

		}

		// ӿ
		while (rect.left < newWidth && projX[rect.left] < xMax*0.75)
			rect.left++;
		while (rect.right && projX[rect.right] < xMax*0.75)
			rect.right--;
		while (rect.top < newHeight && projY[rect.top] < yMax*0.75)
			rect.top++;
		while (rect.bottom && projY[rect.bottom] < yMax*0.75)
			rect.bottom--;
	}
	else// keep more doc 1 == iCropType
	{
		int xThreshold = newHeight*0.05 > 5 ? newHeight*0.01 : 5;
		int yThreshold = newWidth *0.05 > 5 ? newWidth *0.01 : 5;
		if ( xBarycentre > 0 && xBarycentre < newWidth && yBarycentre > 0 && yBarycentre < newHeight )   // λЧ
		{
			for (i = xBarycentre; i > 2; i--) // λ
			{
				if (*(projX+i) < xThreshold//1 
					//&& *(projX+i-1) < 1 && *(projX+i-2) < 1
					//&& (*(projX+i+1) > 1  )
					)
				{
					int idx = i-newWidth*0.05;
					if (idx > 2 && *(projX+idx) > xMax*K)//如果遇到中间一条线的，略过次
					{
						;
					}
					else
					{
						rect.left = i;
						break;
					}
				}
			}
			for (i = xBarycentre; i < newWidth - 2; i++)// λ
			{
				if (*(projX+i) < xThreshold
					//&& *(projX+i+1) < 1  && *(projX+i+2) < 1
					//&& (*(projX+i-1) > 1  )
					)
				{
					int idx = i+newWidth*0.05;
					if ( idx < newWidth - 2 && *(projX+idx) > xMax*K)//如果遇到中间一条线的，略过次
					{
						;
					}
					else
					{
						rect.right = i;
						break;
					}
				}
			}
			for (i = yBarycentre; i > 2; i--)
			{
				
				if (*(projY+i) < yThreshold
					//&& *(projY+i-1) < 1  && *(projY+i-2) < 1
					//&& (*(projY+i+1) > 1 )
					)
				{
					int idx = i-newHeight*0.05;
					if ( idx > 2 && *(projY+idx) > yMax*K)//如果遇到中间一条线的，略过次
					{
						;
					}
					else
					{
						rect.top = i;
						break;
					}
				}
				
			}
			for (i = yBarycentre; i < newHeight - 2; i++)
			{
				
				if (*(projY+i) < yThreshold
					//&& *(projY+i+1) < 1  && *(projY+i+2) < 1
					//&& (*(projY+i-1) > 1 )
					)
				{
					int idx = i+newHeight*0.05;
					if ( idx < newHeight - 2 && *(projY+idx) > yMax*K)//如果遇到中间一条线的，略过次
					{
						;
					}
					else
					{
						rect.bottom = i;
						break;
					}
				}
				
			}

		}
		else  // λԽͼƬ
		{
			for (i = 1; i < newWidth-2; i++)
			{
				
				if (*(projX+i) > xThreshold 
					//&& *(projX+i-1) < 1
					//&& (*(projX+i+1) > 1  || *(projX+i+2) > 1)
					)
				{
					rect.left = i;
					break;
				}
				
			}
			for (i = newWidth-1; i > 2; i--)
			{
				
				if (   *(projX+i) > xThreshold 
					//&& *(projX+i+1) < 1
					//&& (*(projX+i-1) > 1 || *(projX+i-2) > 1)
					)
				{
					rect.right = i;
					break;
				}
				
			}
			for (i = 1; i < newHeight-2; i++)
			{
				
				if (*(projY+i) > yThreshold 
					//&& *(projY+i-1) < 1
					//&& (*(projY+i+1) > 1 || *(projY+i+2) > 1)
					)
				{
					rect.top = i;
					break;
				}
				
			}
			for (i = newHeight-1; i > 2; i--)
			{
				
				if (   *(projY+i) > yThreshold 
					//&& *(projY+i+1) < 1
					//&& (*(projY+i-1) > 1 || *(projY+i-2) > 1)
					)
				{
					rect.bottom = i;
					break;
				}
				
			}

		}

    }

	//rect.bottom++;
	//rect.right++;

	delete []projX;
	delete []projY;

	if (   rect.bottom < rect.top 
		|| rect.left > rect.right 
		|| rect.right > newWidth 
		|| rect.bottom > newHeight)
	{
        return -1;
	}
	
	// pos[0,1,2,3]ֱΪϣ££ϵ
	newPos[0].x = rect.left;
	newPos[0].y = rect.top;
	newPos[1].x = rect.left;
	newPos[1].y = rect.bottom;
	newPos[2].x = rect.right;
	newPos[2].y = rect.bottom;
	newPos[3].x = rect.right;
	newPos[3].y = rect.top;
        image_map(newPos, Pos,fangle,
			 newHeight, newWidth,
			 height, width,1
			 );

    return 0;
	
}


int otsu (unsigned char *image, int rows, int cols, int x0, int y0, int dx, int dy)
{

	unsigned char *np; 
	int thresholdValue=1;
	int ihist[256];

	int i, j, k;  
	int n, n1, n2, gmin, gmax;
	double m1, m2, sum, csum, fmax, sb;

	memset(ihist, 0, sizeof(ihist));

	gmin=255; gmax=0;
	for (i = y0 + 1; i < y0 + dy - 1; i++) {
		np = &image[i*cols+x0+1];
		for (j = x0 + 1; j < x0 + dx - 1; j++) {
			ihist[*np]++;
			if(*np > gmax) gmax=*np;
			if(*np < gmin) gmin=*np;
			np++; 
		}
	}

	sum = csum = 0.0;
	n = 0;

	for (k = 0; k <= 255; k++) {
		sum += (double) k * (double) ihist[k]; 
		n   += ihist[k];             
	}

	fmax = -1.0;
	n1 = 0;
	for (k = 0; k < 255; k++) {
		n1 += ihist[k];
		if (!n1) { continue; }
		n2 = n - n1;
		if (n2 == 0) { break; }
		csum += (double) k *ihist[k];
		m1 = csum / n1;
		m2 = (sum - csum) / n2;
		sb = (double) n1 *(double) n2 *(m1 - m2) * (m1 - m2);
		if (sb > fmax) {
			fmax = sb;
			thresholdValue = k;
		}
	}

	return(thresholdValue);
}

int del_back_color(IplImage *imgSrc,IplImage *imgOut)
{
	if (NULL == imgSrc || NULL == imgOut || 
		imgSrc->nChannels != imgOut->nChannels ||
		imgSrc->width != imgOut->width ||
		imgSrc->height != imgOut->height)
	{
		return -1;
	}	
	cvCopy(imgSrc, imgOut);

	IplImage* imgGray = cvCreateImage(cvSize(imgSrc->width,imgSrc->height), imgSrc->depth, 1);
	if (imgSrc->nChannels >=3)
	{
		cvCvtColor(imgSrc,imgGray,CV_BGR2GRAY);	
		//N_Color2Gray(imgGray, imgSrc);
	}
	else
	{
		cvCopy(imgSrc, imgGray);
	}

	cvSmooth(imgGray,imgGray,CV_GAUSSIAN,3,3);//这个有必要 能去除 杂质点
	cvAdaptiveThreshold(imgGray, imgGray,255,0,0,35,15);//

	//去背景
	unsigned int iGray = 0;
	if (imgSrc->nChannels >=3)
	{
		uchar* pimgOut = NULL;
		uchar* pimgGray = NULL;
		for(int i=0;i<imgGray->height;i++)
		{
			pimgGray = (uchar*)(imgGray->imageData + i*imgGray->widthStep);
			pimgOut = (uchar*)(imgOut->imageData + i*imgOut->widthStep);
			for(int j=0; j<imgGray->width; j++)
			{
				if ( 0 != pimgGray[j] )
				{
					pimgOut[j*3] = 255;
					pimgOut[j*3 + 1] = 255;
					pimgOut[j*3 + 2] = 255;
				}
			}
		}
	}
	else
	{
		uchar* pimgOut = NULL;
		uchar* pimgGray = NULL;
		for(int i=0;i<imgGray->height;i++)
		{
			pimgGray = (uchar*)(imgGray->imageData + i*imgGray->widthStep);
			pimgOut = (uchar*)(imgOut->imageData + i*imgOut->widthStep);
			for(int j=0; j<imgGray->width; j++)
			{
				if ( 0 != pimgGray[j] )
				{
					pimgOut[j] = 255;
				}
			}
		}
	}

	cvReleaseImage(&imgGray);//忘记释放了 20180614

	//做下 unsharpMask操作
	cv::Mat mt(imgOut);
	ImageSharp(mt, mt,200);

	return 0;
}

void ImageSharp( cv::Mat &src, cv::Mat &dst,int nAmount )
{
	double sigma = 3;  	 
	int threshold = 1;  	 
	float amount = nAmount/100.0f;   	 
	cv::Mat imgBlurred;	
	GaussianBlur(src, imgBlurred, cv::Size(), sigma, sigma); 	 
	cv::Mat lowContrastMask = abs(src-imgBlurred)<threshold;	 
	dst = src*(1+amount)+imgBlurred*(-amount);	 src.copyTo(dst, lowContrastMask);
}

int compareImages(const IplImage * image1,const IplImage * image2)
{
	if ( !image1 || !image2 )
	{
		return -1;
	}
	if (    image1->width != image2->width  
		|| image1->height != image2->height
		|| image1->depth  != image2->depth
		|| image1->nChannels    != image2->nChannels
		 )
	{
	    return -1;
	}
	/*
	IplImage *gray1 = cvCreateImage(cvSize(image1->width, image1->height), image1->depth, 1);
	IplImage *gray2 = cvCreateImage(cvSize(image2->width, image2->height), image2->depth, 1);
	if (!gray1 || !gray2)
	{
		return -2;
	}
	*/
	int  width = 0;
	int  height = 0;
	width = image1->width;
	height = image2->height;

	/*if (image1->nChannels == 3)
	{
		bgrToY(image1, gray1);
		bgrToY(image2, gray2);
	}
	else if(image1->nChannels == 1)
	{
		cvCopy(image1, gray1);
		cvCopy(image2, gray2);
	}*/
	

	uchar *pdata1 = (uchar *)image1->imageData;
	uchar *pdata2 = (uchar *)image2->imageData;
	long deta = 0;
	const int T  = 20;
	
	if (image1->nChannels == 3)
	{
		for (int y = 0; y < height; y++)
		{
			uchar * p1 = (uchar*)(pdata1 +  (y)*image1->widthStep);
			uchar * p2 = (uchar*)(pdata2 +  (y)*image1->widthStep);
			for (int x = 0; x < width; x++)
			{
				int value = *(p1 + 3*x + 1) - *(p2 + 3*x + 1);
				if (abs(value) > T )
				{
					deta ++;
				}
			}
		}
	}
	else if(image1->nChannels == 1)
	{
		for (int y = 0; y < height; y++)
		{
			uchar * p1 = (uchar*)(pdata1 +  (y)*image1->widthStep);
			uchar * p2 = (uchar*)(pdata2 +  (y)*image2->widthStep);
			for (int x = 0; x < width; x++)
			{
				int value = *(p1 + x ) - *(p2 + x);
				if (abs(value) > T )
				{
					deta ++;
				}
			}
		}
	}

	if ( deta > width*height/50)
		return deta;
	else 
		return 0;
}

int bgrToY(const IplImage * bgr, IplImage * Y)
{

	if (!bgr || !Y)
	{
		return 1;
	}
	if (
		   bgr->nChannels != 3
		   || Y->nChannels != 1
		|| bgr->width != Y->width
		|| bgr->height != Y->height
		)
	{
		return 1;
	}
	int width = bgr->width;
	int height = bgr->height;
	int x = 0;
	int y = 0;
	for (y = 0; y < height; y++)
	{
		uchar *pSrc = (uchar*)(bgr->imageData + y * bgr->widthStep);	
		uchar *pDst = (uchar*)(Y->imageData + y * Y->widthStep);
		for (x = 0; x < width; x++)
		{
			*(pDst + x) = (
				            *(pSrc + 3*x    ) * 299
				          + *(pSrc + 3*x + 1) * 587
						  + *(pSrc + 3*x + 2) * 299
						  + 500
						  )/1000;
		}
	}
	return 0;
}

int auto_rotate_crop(const unsigned char * pSrcImageData, int iSrcWidth, int iSrcHeight, int nChannels, IM_POINT * Pos,
					 unsigned char * pDstImageData, int* iDstWidth, int* iDstHeight, int flag)
{
	if (!pSrcImageData || iSrcWidth < 0 || iSrcHeight < 0)
	{
		return RTN_PARAM_ERR;
	}
	if (1 != nChannels && 3 != nChannels)
	{
		return RTN_PARAM_ERR;
	}
	if (!iDstWidth || !iDstHeight)
	{
		return RTN_PARAM_ERR;
	}

	int width = iSrcWidth;
	int height = iSrcHeight;

	IplImage *src = cvCreateImage(cvSize(width,height), IPL_DEPTH_8U, nChannels);
	if (!src)
	{
		return RTN_MALLOC_ERR;
	}
	memcpy(src->imageData, pSrcImageData, src->widthStep * src->height );

	if (flag > 0)
	{
		flag = CROPTYPE_KEEP_MORE;
	}
	else
	{
		flag = CROPTYPE_KEEP_CLEAN;
	}

	float angle = 0;
	IM_POINT pos[4];
	IM_RECT  rect;
	int rtn = auto_rotate(src, angle, pos, rect, flag);
	if ( 0 == rtn )
	{
		cvReleaseImage(&src);
		// 纠偏裁边失败
        return RTN_AUTO_ROTATE_FAILED;
	}
	*iDstWidth  = rect.right - rect.left;
	*iDstHeight = rect.bottom - rect.top;
	(*(Pos + 0)).x = pos[0].x;
	(*(Pos + 0)).y = pos[0].y;
	(*(Pos + 1)).x = pos[1].x;
	(*(Pos + 1)).y = pos[1].y;
	(*(Pos + 2)).x = pos[2].x;
	(*(Pos + 2)).y = pos[2].y;
	(*(Pos + 3)).x = pos[3].x;
	(*(Pos + 3)).y = pos[3].y;

	if (!pDstImageData)
	{
		cvReleaseImage(&src);
		return RTN_SUCCESS;
	}
	IplImage *dst = NULL;
	dst = cvCreateImage(cvSize(*iDstWidth,*iDstHeight), IPL_DEPTH_8U, nChannels);
	if (!dst)
	{
		cvReleaseImage(&src);
		return RTN_MALLOC_ERR;
	}
	rtn = image_rotate_crop(src, dst, angle, rect);
	if (0 == rtn)
	{	
		cvReleaseImage(&dst);
		cvReleaseImage(&src);
		return RTN_AUTO_ROTATE_FAILED;
	}
	memcpy(pDstImageData, dst->imageData, dst->widthStep*dst->height);

	cvReleaseImage(&dst);
	cvReleaseImage(&src);
	return RTN_SUCCESS;

}

// 1 正面正立 2 正面倒立 
int detectIDNegPos(IplImage * src)
{
	if (!src)
	{
		return -1;
	}
	int result = -1;
	int rtn = -1;
	int width = src->width;
	int height = src->height;

	CvRect rect;

	IplImage * gray = cvCreateImage(cvSize(width, height), IPL_DEPTH_8U, 1);
	if (!gray)
	{
		//cvReleaseImage(&srccrop);
		return -2;
	}
	if (src->nChannels >= 3)
	{
		cvSplit(src, gray, NULL, NULL, NULL);
	}
	else if (src->nChannels == 1)
	{
		cvCopy(src, gray);
	}
	//cvSaveImage("F:/gray.bmp",gray);

	int threshold = otsu((uchar*)gray->imageData, gray->width, gray->height, gray->widthStep);

	height = gray->height;
	width  = gray->width;
	int widthStep = gray->widthStep;
	uchar * pData = (uchar *)gray->imageData;
	int x = 0;
	int y = 0;
	//////////////////////////////////////////////////
	////////////////////////////////
	for (y = 0; y < height; y++)
	{
		uchar *p = (uchar*)(pData + y*widthStep);
		for (x = 0; x < width; x++)
		{
			*(p + x) = *(p + x) >= threshold ? 0 : 1;
		}
	}

	int sumL = 1;
	int sumR = 1;
	pData = (uchar *)gray->imageData;
	for (y = 0.15 * height; y < 0.85 * height; y++)
	{
		uchar *p = (uchar*)(pData + y*widthStep);
		for (x = 0.1*width; x < 0.3*width; x++)
		{
			sumL += *(p + x);
		}
		for (x = 0.7*width; x < 0.9*width; x++)
		{
			sumR += *(p + x);
		}
	}
	int sum = 0.2*width * 0.7*height;

	if (   sumR > 0.5 * sum
		&& sumL < 0.1 * sum
	 )
	{
		result = 1;// 身份证正面，正像
	}
	else if (  sumL > 0.5 * sum
		    && sumR < 0.1 * sum
	 )
	{
		result = 2;// 身份证正面，倒像
	}
	else if ( ((float)sumR/(float)sumL) > 3)
	{
		result = 1;// 身份证正面，正像
	}
	else if (((float)sumL/(float)sumR) > 3)
	{
		result = 2;// 身份证正面，倒像
	}
	else
	{
		result = 3; // 身份证反面。暂不计正倒
	}

	if (3 != result)// 正面
	{
		if (2 == result)
		{
			rotate180(gray);
		}

		int nameText = 0;
		int idText = 0;
		int yearText = 0;
		// 姓名
		//CvRect rect;
		rect.x = 0.160 * gray->width;
		rect.y = 0.109  * gray->height;
		rect.width = 0.27 * gray->width;
		rect.height = 0.125 * gray->height;
		IplImage * name = cvCreateImage(cvSize(rect.width, rect.height), gray->depth, gray->nChannels);
		if (name)
		{
			image_crop(gray, name, rect);
			nameText = isTextInMidlle(name);
			//cvSaveImage("F:\\name.bmp",name);
			cvReleaseImage(&name);
		}
		
		rect.x = 0.160 * gray->width;
		rect.y = 0.36  * gray->height;
		rect.width = 0.120 * gray->width;
		rect.height = 0.125 * gray->height;
		IplImage * year = cvCreateImage(cvSize(rect.width, rect.height), gray->depth, gray->nChannels);
		if (year)
		{
			image_crop(gray, year, rect);
			yearText = isTextInMidlle(year);
			cvReleaseImage(&year);
		}
				
		rect.x = 0.315 * gray->width;
		rect.y = 0.786  * gray->height;
		rect.width = 0.620 * gray->width;
		rect.height = 0.175 * gray->height;
		IplImage * num = cvCreateImage(cvSize(rect.width, rect.height), gray->depth, gray->nChannels);
		if (num)
		{
			image_crop(gray, num, rect);
			idText = isTextInMidlle(num);
			cvReleaseImage(&num);
		}
		if (   1 != nameText
			&& 1 != idText
			&& 1 != yearText)
		{
			result = 5;// 非身份证
		}
	}
	else// 不是身份证正面
	{
		sumL = 1;
		sumR = 1;
		rect.x = 0.0525 * gray->width;
		rect.y = 0.0839 * gray->height;
		rect.width = 0.195 * gray->width;
		rect.height = 0.347 * gray->height;

		pData = (uchar *)gray->imageData;
		
		for (y = rect.y; y < rect.y + rect.height; y++)
		{
			uchar *p = (uchar*)(pData + y*widthStep);
			for (x = rect.x; x < rect.x + rect.width; x++)
			{
				sumL += *(p + x);	
			}
		}

		rect.x = gray->width - rect.x - rect.width;
		rect.y = gray->height - rect.y - rect.height;
		for (y = rect.y; y < rect.y + rect.height; y++)
		{
			uchar *p = (uchar*)(pData + y*widthStep);
			for (x = rect.x; x < rect.x + rect.width; x++)
			{
				sumR += *(p + x);
			}
		}

		if (sumL > 0 && sumR > 0)
		{
			sum = rect.width*rect.height + 1;
			if (
				sumL/(float)(sum) > 0.35
				&& sumR/(float)(sum) < 0.2
				)
			{
				result = 3;
			}
			if ( sumR/(float)(sum) > 0.35
				&& sumL/(float)(sum) < 0.2
				)
			{
				result = 4;
			}
			if (sumL/(float)sumR > 20)
			{
				result = 3;
			}
			if (sumR/(float)sumL > 20)
			{
				result = 4;
			}
		}
		else 
		{
			result = 5;
		}

		if (3 == result)
		{
			sumL = 0;
			sum = 0;
			rect.x = 0.0525 * gray->width;
			rect.y = 0.46 * gray->height;
			rect.width = 0.85 * gray->width;
			rect.height = 0.27 * gray->height;

			pData = (uchar *)gray->imageData;
			for (y = rect.y; y < rect.y + rect.height; y++)
			{
				uchar *p = (uchar*)(pData + y*widthStep);
				for (x = rect.x; x < rect.x + rect.width; x++)
				{
					sumL += *(p + x);
				}
			}
			sum = rect.width * rect.height+1;

			if (sumL/(float)sum > 0.15)//中间空白部分内容过多
			{
				result = 5;
			}
		}
		else if (4 == result)
		{
			sumL = 1;
			sum = 1;
			rect.x = 0.0525 * gray->width;
			rect.y = 0.328 * gray->height;
			rect.width = 0.85 * gray->width;
			rect.height = 0.27 * gray->height;

			pData = (uchar *)gray->imageData;
			for (y = rect.y; y < rect.y + rect.height; y++)
			{
				uchar *p = (uchar*)(pData + y*widthStep);
				for (x = rect.x; x < rect.x + rect.width; x++)
				{
					sumL += *(p + x);
				}
			}
			sum = rect.width * rect.height+1;

			if (sumL/(float)sum > 0.15)//中间空白部分内容过多
			{
				result = 5;
			}
		}

		

	}
	//printf("检验正反 %d %d %d",nameText,idText,yearText);

	cvReleaseImage(&gray);
	return result;
}

// 1 是
// 0 否
int isTextInMidlle(IplImage *image)
{
	if (!image)
	{
		return 0;
	}
	if (1 != image->nChannels)
	{
		return 0;
	}
	int x;
	int y;
	int width = image->width;
	int height = image->height;
	int widthstep = image->widthStep;
	int *projX = new int[width];
	int *projY = new int[height];
	if (!projX || !projY)
	{
		return 0;
	}
	memset(projX, 0, width*sizeof(int));
	memset(projY, 0, height*sizeof(int));
	uchar * pData = (uchar*)image->imageData;
	for (y = 0; y < height; y++)
	{
		uchar * p = (uchar*)(pData+(y)*widthstep);
		for (x = 0; x < width; x++)
		{
			int temp = *(p+x);
			*(projX+x) += *(p+x);
			*(projY+y) += *(p+x);
		}
	}

	int xMax = 0;
	int yMax = 0;
	int i = 0;
	for (i = 1; i < width-1; i++)
	{	
		if (xMax <  *(projX+i))
		{
			xMax = *(projX+i);
		}
	}
	for (i = 1; i < height-1; i++)
	{
		if (yMax <  *(projY+i))
		{
			yMax = *(projY+i);
		}
	}

	int xBarycentre = 0;
	int yBarycentre = 0;
	// 求重心
	getBarycentre(image, xBarycentre,yBarycentre);

	int xIdxL = 0;
	int xIdxR = width-1;
	int yIdxT = 0;
	int yIdxB = height - 1;
	// 判断分界点
	for (i = 1; i < width-1; i++)
	{	
		if (   0.2*xMax <  *(projX+i)
			&& 0.2*xMax >  *(projX+i-1) 
			)
		{
			xIdxL = i;
			break;
		}
	}
	for (i = width-1; i > 1; i--)
	{	
		if (   0.2*xMax <  *(projX+i)
			&& 0.2*xMax >  *(projX+i+1) 
			)
		{
			xIdxR = i;
			break;
		}
	}
	for (i = 1; i < height-1; i++)
	{	
		if (   0.2*yMax <  *(projY+i)
			&& 0.2*yMax >  *(projY+i-1) 
			)
		{
			yIdxT = i;
			break;
		}
	}
	for (i = height-1; i > 1; i--)
	{	
		if (   0.2*yMax <  *(projY+i)
			&& 0.2*yMax >  *(projY+i+1) 
			)
		{
			yIdxB = i;
			break;
		}
	}

	if (
		   xIdxL < xBarycentre
		&& xBarycentre < xIdxR
		&& yIdxT > 0.15 * height
		&& yIdxB < 0.85 * height
		//&& yIdxT < yIdxB
		&& yIdxT < yBarycentre
		&& yBarycentre < yIdxB
		)
	{
		return 1;
	}
	else
	{
		return 0;
	}

}

int rotate180(IplImage * src)
{
	IplImage *temp = cvCreateImage(cvSize(src->width, src->height), src->depth, src->nChannels);
	if (!temp)
	{
		return 0;
	}
	int width = src->width;
	int height = src->height;
	int x = 0;
	int y = 0;
	
	uchar *pSrc   = NULL;
	uchar *pTmp   = NULL;
	
	if (3 == src->nChannels)
	{
		for (int y = 0; y < height; y++)
		{
			pSrc = (uchar*)(src->imageData + (height - y - 1)*src->widthStep);
			pTmp = (uchar*)(temp->imageData + y*temp->widthStep);
			for (int x = 0; x < width; x++)
			{
				*(pTmp + 3*x    ) = *(pSrc + 3*(width - x - 1)    );
				*(pTmp + 3*x + 1) = *(pSrc + 3*(width - x - 1) + 1);
				*(pTmp + 3*x + 2) = *(pSrc + 3*(width - x - 1) + 2);

			}
		}
		cvCopy(temp,src);
	}
	else if (1 == src->nChannels)
	{
		
		for (int y = 0; y < height; y++)
		{
			pSrc = (uchar*)(src->imageData + (height - y - 1)*src->widthStep);
			pTmp = (uchar*)(temp->imageData + y*temp->widthStep);
			for (int x = 0; x < width; x++)
			{
				*(pTmp + x) = *(pSrc + (width - x - 1));

			}
		}
		cvCopy(temp,src);
		
	}

	return 1;

}


int image_crop(IplImage * src,IplImage *dst, CvRect rect)
{
	if (   !src 
		|| !dst
		)
	{
		return 0;
	}
	if (   dst->width != rect.width
		|| dst->height != rect.height
		)
	{
		return 0;
	}
	if (
		   rect.x > src->width
		|| rect.y > src->height
		)
	{
		return 0;
	}
	cvSetImageROI(src,rect);
	cvCopyImage(src,dst);
	cvResetImageROI(src);
	return 1;
}

int getBarycentre(IplImage *image, int & xBarycentre,int & yBarycentre)
{
	if (!image)
	{
		return -1;
	}
	if (1 != image->nChannels)
	{
		return 1;
	}
	int x;
	int y;
	int width = image->width;
	int height = image->height;
	int widthstep = image->widthStep;
	int *projX = new int[width];
	int *projY = new int[height];
	if (!projX || !projY)
	{
		return 2;
	}
	memset(projX, 0, width*sizeof(int));
	memset(projY, 0, height*sizeof(int));
	uchar * pData = (uchar*)image->imageData;
	for (y = 0; y < height; y++)
	{
		uchar * p = (uchar*)(pData+(y)*widthstep);
		for (x = 0; x < width; x++)
		{
			*(projX+x) += *(p+x);
			*(projY+y) += *(p+x);
		}
	}

	

	// 求重心
	xBarycentre = 0;
	yBarycentre = 0;
	int sumX = 1;
	int sumY = 1;
	int i = 0;
	for (i = 1; i < width-1; i++)
	{	
		xBarycentre += *(projX+i) * i;
		sumX        += *(projX+i);
	}
	
	for (i = 1; i < height-1; i++)
	{
		yBarycentre += *(projY+i) * i;
		sumY        += *(projY+i);
	}
    xBarycentre = xBarycentre/sumX;
	yBarycentre = yBarycentre/sumY;
	delete []projY;
	delete []projX;

	return 0;
}


int getRGBMax(IplImage * src, IplImage * gray)
{
	if (!src)
	{
		return 1;
	}
	if (!gray)
	{
		return 1;
	}
	if (src->width != gray->width
		|| src->height != gray->height
		)
	{
		return 1;
	}
	if (src->nChannels != 3 && gray->nChannels != 1)
	{
		return 1;
	}

	int x = 0;
	int y = 0;
	int width = src->width;
	int height = src->height;
	uchar *pDibbgr = NULL;
	uchar *pDibgray = NULL;
	pDibbgr = (uchar*)src->imageData;
	pDibgray = (uchar*)gray->imageData;

	
	for (y = 0; y < gray->height; y++)
	{
		uchar* p_in  = (uchar*)(pDibbgr +  (y)*src->widthStep);
		uchar* p_out = (uchar*)(pDibgray + (y)*gray->widthStep);
		for (x = 0; x < gray->width; x++)
		{
			int temp = *(p_in + 3*x    ) > *(p_in + 3*x + 1) ? *(p_in + 3*x    ) : *(p_in + 3*x + 1);
			temp = temp > *(p_in + 3*x + 2) ? temp : *(p_in + 3*x + 2);
			*(p_out + x) = temp;
			//int b = *(p_in + 3*x      );
			//int g = *(p_in + 3*x  + 1 );
			//int r = *(p_in + 3*x  + 2 );
			//if ( (b-g)>=5 && (b-r)>=5 )
			//{
			//	*(p_out + x) = 255;
			//}
		}
	}
	
	return 0;

}

//算法过程概述:
//step1:图像缩放并灰度化, step2:计算有效图像区域边界, step3:计算阈值, step4:标注连通区, step5:计算图像顶点及倾斜角度
int multiAutoRotate(IplImage *pImgSrc, int * pDocNum, int * ipRtn, float * pAngle, IM_POINT * pPoint, IM_RECT * pRect, int iCropType)
{
	if(pImgSrc==NULL || pDocNum==NULL || ipRtn==NULL || pAngle==NULL) return RTN_PARAM_ERR;
	if(pImgSrc->nChannels!=3 && pImgSrc->nChannels!=1) return RTN_PARAM_ERR; 

	int iReVal=RTN_SUCCESS, thVal=0, iRatio=1;
	int const MAX_DOC_COUNT=8;			//最多识别数目
	int lineLeft=0, lineTop=0, lineRight=0, lineBottom=0;
	int const RECT_POINT_NUM=4, MIN_TH=25;
	uchar *pImgMask = NULL;
	IplImage *pThumbSrc=NULL, *pImgGray=NULL;

	//创建小图
	if(pImgSrc->width>2047 || pImgSrc->height>2047)
	{
		pThumbSrc = cvCreateImage(cvSize(pImgSrc->width>>2, pImgSrc->height>>2), pImgSrc->depth, pImgSrc->nChannels);
		iRatio = 4;
	}
	else if(pImgSrc->width>1600 || pImgSrc->height>1600)
	{
		pThumbSrc = cvCreateImage(cvSize(pImgSrc->width>>1, pImgSrc->height>>1), pImgSrc->depth, pImgSrc->nChannels);
		iRatio = 2;
	}
	else 
	{
		pThumbSrc = cvCreateImage(cvSize(pImgSrc->width, pImgSrc->height), pImgSrc->depth, pImgSrc->nChannels);
		iRatio = 1;
	}

	if(pThumbSrc)
	{
		cvResize(pImgSrc, pThumbSrc);
		pImgGray = cvCreateImage(cvSize(pThumbSrc->width, pThumbSrc->height), pThumbSrc->depth, 1);
		if(pImgGray)
		{
			if(pThumbSrc->nChannels==3) cvCvtColor(pThumbSrc, pImgGray, CV_BGR2GRAY);
			else cvCopy(pThumbSrc, pImgGray);
			//cvSmooth(pImgGray, pImgGray, CV_MEDIAN);		//中值滤波去除噪点
			pImgMask = new uchar[pImgGray->widthStep*pImgGray->height];
		}
	}

	if(pThumbSrc && pImgGray && pImgMask)
	{
		//前景
		FindBorderLine(pImgGray, lineLeft, lineTop, lineRight, lineBottom);
		thVal = otusThreshold(pImgGray);
		if(thVal<MIN_TH) iReVal=RTN_AUTO_ROTATE_FAILED;
		if(iReVal==RTN_SUCCESS) iReVal=EdgeDetect(pImgGray, thVal, lineLeft, lineTop, lineRight, lineBottom, pImgMask, MAX_DOC_COUNT, pDocNum, pPoint, pAngle);
		if(*pDocNum==0) iReVal=RTN_AUTO_ROTATE_FAILED;
		if(iReVal==RTN_SUCCESS)
		{
			for (int i=0; i<(*pDocNum)*RECT_POINT_NUM; i++)
			{
				pPoint[i].x*=iRatio; pPoint[i].y*=iRatio;
			}

			//旋转四个角点, 计算矩形
			float const PI=3.1415926f;
			float M1=0, M2=0, fSin=0, fCos=0;
			IM_POINT pntRotate[RECT_POINT_NUM], pntTemp, *pPos=NULL;
			int sortX[RECT_POINT_NUM], sortY[RECT_POINT_NUM], distMin=0, distTemp=0, tempVal=0;
			int widthRotate=0, heightRotate=0;

			for (int i=0; i<(*pDocNum); i++)
			{
				ipRtn[i] = RTN_SUCCESS;

				pRect[i].left = pRect[i].top = 0xffff;
				pRect[i].right = pRect[i].bottom = 0;

				pPos = pPoint+i*RECT_POINT_NUM;
				widthRotate = fabs(pImgSrc->height*sin(pAngle[i]))+fabs(pImgSrc->width*cos(pAngle[i]));
				heightRotate = fabs(pImgSrc->height*cos(pAngle[i]))+fabs(pImgSrc->width*sin(pAngle[i]));

				fSin = sin(2*PI-pAngle[i]);		//转化为按照顺时针旋转
				fCos = cos(2*PI-pAngle[i]);
				M1 = -0.5*pImgSrc->width*fCos+0.5*pImgSrc->height*fSin+0.5*widthRotate;
				M2 = -0.5*pImgSrc->width*fSin-0.5*pImgSrc->height*fCos+0.5*heightRotate;
				for (int j=0; j<RECT_POINT_NUM; j++)
				{
					//计算旋转后坐标
					pntRotate[j].x = pPos[j].x*fCos-pPos[j].y*fSin+M1;
					pntRotate[j].y = pPos[j].x*fSin+pPos[j].y*fCos+M2;
				}

				if(iCropType == 1)		//多裁
				{
					//对四点坐标中的x, y进行排序, 从小到大
					for (int j=0; j<RECT_POINT_NUM; j++)
					{
						sortX[j] = pntRotate[j].x;
						sortY[j] = pntRotate[j].y;
					}

					for (int j=0; j<RECT_POINT_NUM-1; j++)
					{
						for (int k=j+1; k<RECT_POINT_NUM; k++)
						{
							if(sortX[j]>sortX[k])
							{
								tempVal = sortX[j];
								sortX[j] = sortX[k];
								sortX[k] = tempVal;
							}
						}
					}

					for (int j=0; j<RECT_POINT_NUM-1; j++)
					{
						for (int k=j+1; k<RECT_POINT_NUM; k++)
						{
							if(sortY[j]>sortY[k])
							{
								tempVal = sortY[j];
								sortY[j] = sortY[k];
								sortY[k] = tempVal;
							}
						}
					}
					//rect.left=((sortX[0]+sortX[1])>>1); rect.right=((sortX[2]+sortX[3])>>1);
					//rect.top=((sortY[0]+sortY[1])>>1); rect.bottom=((sortY[2]+sortY[3])>>1);
					pRect[i].left=sortX[1]; pRect[i].right=sortX[2];
					pRect[i].top=sortY[1]; pRect[i].bottom=sortY[2];
				}
				else
				{
					for (int j=0; j<RECT_POINT_NUM; j++)
					{
						if(pRect[i].left>pntRotate[j].x) pRect[i].left=pntRotate[j].x;
						if(pRect[i].top>pntRotate[j].y) pRect[i].top=pntRotate[j].y;
						if(pRect[i].right<pntRotate[j].x) pRect[i].right=pntRotate[j].x;
						if(pRect[i].bottom<pntRotate[j].y) pRect[i].bottom=pntRotate[j].y;
					}
				}

				//调整Pos中的点排序, 以便上层程序绘制矩形
				float pntAngle[RECT_POINT_NUM];
				IM_POINT centerPnt;

				//计算中心点坐标
				centerPnt.x = centerPnt.y = 0;
				for (int j=0; j<RECT_POINT_NUM; j++)
				{
					centerPnt.x += pPos[j].x;
					centerPnt.y += pPos[j].y;
				}
				centerPnt.x /= RECT_POINT_NUM;
				centerPnt.y /= RECT_POINT_NUM;

				//计算各点与中心点形成的角度
				for (int j=0; j<RECT_POINT_NUM; j++)
				{
					if(pPos[j].x == centerPnt.x)
					{
						pntAngle[j] = (pPos[j].y>centerPnt.y ? 3*PI/2 : PI/2);
					}
					else
					{
						if(pPos[j].x > centerPnt.x)
						{
							if(pPos[j].y<centerPnt.y) pntAngle[j]=atan((centerPnt.y-pPos[j].y+0.0f)/(pPos[j].x-centerPnt.x));
							else pntAngle[j]=2*PI-atan((pPos[j].y-centerPnt.y+0.0f)/(pPos[j].x-centerPnt.x));
						}
						else
						{
							if(pPos[j].y<centerPnt.y) pntAngle[j]=PI-atan((centerPnt.y-pPos[j].y+0.0f)/(centerPnt.x-pPos[j].x));
							else pntAngle[j]=PI+atan((centerPnt.y-pPos[j].y+0.0f)/(pPos[j].x-centerPnt.x));
						}
					}
				}

				//按角度由小到大排序
				float fTemp=0;
				for (int j=0; j<RECT_POINT_NUM; j++)
				{
					fTemp=pntAngle[j]; tempVal=j;
					for (int k=j+1; k<RECT_POINT_NUM; k++)
					{
						if(pntAngle[k]<fTemp)
						{
							fTemp = pntAngle[k];
							tempVal = k;
						}
					}
					if(tempVal != j)
					{
						pntTemp = pPos[j];
						pPos[j] = pPos[tempVal];
						pPos[tempVal] = pntTemp;

						pntAngle[tempVal]=pntAngle[j];
						pntAngle[j] = fTemp;
					}
				}
			}
		}
	}
	else iReVal=RTN_MALLOC_ERR;

	if(pThumbSrc) cvReleaseImage(&pThumbSrc);
	if(pImgGray) cvReleaseImage(&pImgGray);
	if(pImgMask) delete pImgMask;

	return iReVal;
}

static const IM_POINT neighborhood8[8] = 
   { {1, 0}, {1, 1}, {0, 1}, {-1, 1}, {-1, 0}, {-1, -1}, {0, -1}, {1, -1} };

int findContours(IplImage * image, std::vector< std::vector<IM_POINT> > & contours)
{
	if (!image)
	{
		return -1;
	}
	if (1 != image->nChannels)
	{
		return -1;
	}
	if (image->width < 1 || image->height < 1)
	{
		return -1;
	}

	IplImage *tmp = cvCreateImage(cvSize(image->width,image->height), image->depth, 1);
	if (!tmp)
	{
		return -1;
	}

	cvCopyImage(image, tmp);
	
	int width = tmp->width;
	int height = tmp->height;
	int * labelmap = new int[width * height];
	if (!labelmap)
	{
		return  -1;
	}
	memset(labelmap, 0, width * height*sizeof(int));

	int isInArea = 0; // 判断是否在某个连通区域内
	std::vector<IM_POINT> vec_tempPt;
	int coutoursNum = 0;
	int y = 0;
	int x = 0;
	int widthstep = tmp->widthStep;
	uchar * pBwData = (uchar*)(tmp->imageData);
	
	// 还是把周围一圈置黑吧，方便很多
	memset(pBwData, 0, width*sizeof(uchar));
	memset(pBwData + (height-1)*widthstep , 0, width*sizeof(uchar));
	for (y = 1; y < height-1; y++)
	{
		uchar *p = (uchar*)(pBwData + (y)*widthstep);
		*(p) = 0;
		*(p+width-1) = 0;
	}

	for (y = 1; y < height-1; y++)
	{
		isInArea = 0;
		uchar *p = (uchar*)(pBwData);;// + (height-1-y)*widthstep);
		for (x  = 1; x < width-1; x++)
		{

				if ( 0 == *(p + x + (y)*widthstep) 
					&& 0 == *(labelmap + y*width + x)
					)
				{
					continue;
				}
				if (   -1 == *(labelmap + y*width + x-1) // 左边点被标记轮廓外围-1
					&& 0   < *(labelmap + y*width + x)   // 当前点为连通区域
					)
				{
					isInArea = 1;
					continue;
				}
				if (   -1 == *(labelmap + y*width + x+1) // 左边点被标记轮廓外围-1
					&& 0   < *(labelmap + y*width + x)   // 当前点为连通区域
					)
				{
					isInArea = 0;
				}
				if (1 == isInArea)// 在其他连通域区域内部
				{
					continue;
				}

				// 找到一个连通域的开始
				if ( *(p + x + (y)*widthstep) > 0   // 当前点为白点
					&& 0 == *(labelmap + y*width + x)        // 当前点未被标记
					&& 0 == *(p + x + ((y-1))*widthstep)  // 当前点上面点为黑点
					)     
				{
					IM_POINT ptStart;
					ptStart.x = x;
					ptStart.y = y;
					vec_tempPt.push_back(ptStart);
					
					coutoursNum++;
					*(labelmap + y*width + x)  =  coutoursNum;
					
					if (x!=0)
					{
						*(labelmap + (y-1)*width + x) = -1;
					}
					
					int i = 0;// 领域标号
					int bFindACountour = 0;
					int xCur = x;
					int yCur = y;

					while (!bFindACountour) // 开始遍历
					{
						i = ((i+4) % 8 + 2)%8;
						int step = 0;//步骤计数器，防止在某一孤立点死循环
						for( ; i <= 8; i++)
						{
							step++;
							if (step > 9)
							{
								bFindACountour = 1;
								break;
							}
							i = i%8;
							//int j = (i+7)%8;
							int xNew = xCur + neighborhood8[i].x;
							int yNew = yCur + neighborhood8[i].y;
							if (xNew < 0)
							{
								xNew = 0;
							}
							if (xNew > width-1)
							{
								xNew = width-1;
							}
							if (yNew < 0)
							{
								yNew = 0;
							}
							if (yNew > height-1)
							{
								yNew = height-1;
							}
							if ( -1 == *(labelmap + yNew*width + xNew) )// 如果探测的区域已经被标记了，转到下一个
							{
								continue;
							}

							// 探索点未被标记并且有值，则为新的连通轮廓
							if (  
								  (     0 == *(labelmap + yNew*width + xNew)
								     || coutoursNum == *(labelmap + yNew*width + xNew) 
								   )  //这个判断要改进
								&& *(p + xNew + (yNew)*widthstep) > 0 
								)
							{
								IM_POINT pt;
								pt.x = xNew;
								pt.y = yNew;
								vec_tempPt.push_back(pt);
								*(labelmap + yNew*width + xNew)  =  coutoursNum;
								//*(labelmap + yNew*width + xNew)  =  coutoursNum;
								xCur = xNew;
								yCur = yNew;
								step = 0;
							
								if (xNew == ptStart.x && yNew == ptStart.y)
								{
									bFindACountour = 1;
								}
								break;
							}
							else if(0 == *(p + xNew + (yNew)*widthstep))// 如果不是白色的，探测的点标记-1
							{
								*(labelmap + yNew*width + xNew)  =  -1;
							}
							// 找到开头的点，形成一个闭合环
		
						}
					}// 一个联通域的结束

					int size = vec_tempPt.size();
					// 将一个轮廓序列加到容器中
					if (vec_tempPt.size() > image->width * image->height * 0.0002)
					{
						contours.push_back(vec_tempPt);
					}
					
					std::vector<IM_POINT>().swap(vec_tempPt);
					vec_tempPt.clear();
		
				}
		}// end x
	}// end y


	delete []labelmap;
	cvReleaseImage(&tmp);	

	return contours.size();
}

int getContoursMinRect(std::vector< std::vector<IM_POINT> > & contours, IM_RECT * rect, int index)
{
	if (index < 0 || index > contours.size())
	{
		return 1;
	}
	
	int left = contours.at(index).at(0).x;
	int right = contours.at(index).at(0).x;
	int top = contours.at(index).at(0).y;
	int bottom = contours.at(index).at(0).y;
	int size = contours.at(index).size();
	for (int i = 0; i < size; i++)
	{
		IM_POINT pt;
		pt = contours.at(index).at(i);
		;
		if (left > pt.x)
		{
			left = pt.x;
		}
		if (top > pt.y)
		{
			top = pt.y;
		}
		if (right < pt.x)
		{
			right = pt.x;
		}
		if (bottom < pt.y)
		{
			bottom = pt.y;
		}
	}
	if (left >= right || top >= bottom)
	{
		return 1;
	}

	if (rect)
	{
		rect->left = left;
		rect->top = top;
		rect->right = right;
		rect->bottom = bottom;
	}
	
	return 0;
}


int getContoursMinRectPoints(std::vector< std::vector<IM_POINT> > & contours, IM_POINT * points, int index)
{
	if (index < 0 || index > contours.size())
	{
		return 1;
	}
	
	IM_POINT pts[4];
	pts[0].x = contours.at(index).at(0).x;
	pts[0].y = contours.at(index).at(0).y;
	pts[1].x = contours.at(index).at(0).x;
	pts[1].y = contours.at(index).at(0).y;
	pts[2].x = contours.at(index).at(0).x;
	pts[2].y = contours.at(index).at(0).y;
	pts[3].x = contours.at(index).at(0).x;
	pts[3].y = contours.at(index).at(0).y;

	int left = contours.at(index).at(0).x;
	int right = contours.at(index).at(0).x;
	int top = contours.at(index).at(0).y;
	int bottom = contours.at(index).at(0).y;
	int size = contours.at(index).size();
	for (int i = 0; i < size; i++)
	{
		IM_POINT pt;
		pt = contours.at(index).at(i);

		if (left > pt.x)
		{
			left = pt.x;
			pts[0].x = pt.x;
			pts[0].y = pt.y;
		}
		if (top > pt.y)
		{
			top = pt.y;
			pts[1].x = pt.x;
			pts[1].y = pt.y;
		}
		if (right < pt.x)
		{
			right = pt.x;
			pts[2].x = pt.x;
			pts[2].y = pt.y;
		}
		if (bottom < pt.y)
		{
			bottom = pt.y;
			pts[3].x = pt.x;
			pts[3].y = pt.y;
		}
	}
	if (left >= right || top >= bottom)
	{
		return 1;
	}

	if (points)
	{
		points[0].x = pts[0].x;
		points[0].y = pts[0].y;
		points[1].x = pts[1].x;
		points[1].y = pts[1].y;
		points[2].x = pts[2].x;
		points[2].y = pts[2].y;
		points[3].x = pts[3].x;
		points[3].y = pts[3].y;
	}
	/*if (rect)
	{
		rect->left = left;
		rect->top = top;
		rect->right = right;
		rect->bottom = bottom;
	}*/
	
	return 0;
}

// 返回值 0 成功
// 返回值 非0 失败
int multiAutoRotateFixedArea(IplImage *image,int iDocNum,int * ipRtn, float * fAngle, IM_POINT * pPoint, IM_RECT * pRect, int iCropType)
{
	if (!image)
	{
		return 1;
	}

	int docNum = iDocNum;
	if (1 != docNum && 2 != docNum && 3 != docNum && 4 != docNum )
	{
		return 1;
	}
	int width = 0;
	int height = 0;
	switch (docNum)
	{
		case 1://一个区域
		{
			width = image->width;
			height = image->height;
			break;
		}
		case 2://左右两个区域
		{
			width = image->width/2;
			height = image->height;
			break;
		}
		case 3://左中右三个区域
		{
			width = image->width/3;
			height = image->height;
			break;
		}
		case 4://田字四区域
		{
			width = image->width/2;
			height = image->height/2;
			break;
		}
		default:
			break;

	}
	
	IplImage * imtmp = cvCreateImage(cvSize(width,height), image->depth, image->nChannels);
	float angle[4];
	IM_POINT point[16];
	IM_RECT rcRotate[4];
	int rtn[4];
	switch (docNum)
	{
		case 1://一个区域
		{
			cvCopy(image, imtmp);
			if (pPoint)
			{
				rtn[0] = auto_rotate(imtmp, angle[0], point, rcRotate[0], iCropType);
			}

			break;
		}
		case 2://左右两个区域
		case 3://左中右三个区域
		{
			for (int i = 0; i < docNum; i++)
			{
				CvRect rect;
				rect.x = i*image->width;
				rect.y = 0;
				rect.width = image->width;
				rect.height = image->height;
				image_crop(image, imtmp, rect);
				if (pPoint)
				{
					rtn[i] = auto_rotate(imtmp, angle[i], (point+i*4), rcRotate[i], iCropType);
				}

			
			}
	
			break;
		}
		
		case 4://田字四区域
		{
			for (int i = 0; i < docNum; i++)
			{
				CvRect rect;
				rect.x = (i%2)*image->width/2;
				rect.y = (i/2)*image->height/2;
				rect.width = image->width/2;
				rect.height = image->height/2;
				image_crop(image, imtmp, rect);

				if (pPoint)
				{
					rtn[i] = auto_rotate(imtmp, angle[i], (point+i*4), rcRotate[i], iCropType);
				}
			
			}
			break;
		}
		default:
			break;

	}

	
	
	
	for (int i = 0; i < docNum; i++)
	{
		CvRect rect;
		if (1 == docNum)
		{
		}
		else if (2 == docNum || 3 == docNum)
		{
			rect.x = i*image->width/2;
			rect.y = 0;
			rect.width = image->width;
			rect.height = image->height;
		}
		else if (4 == docNum)
		{
			rect.x = (i%2)*image->width/2;
			rect.y = (i/2)*image->height/2;
			rect.width = image->width/2;
			rect.height = image->height/2;
		}
	
		if ( ipRtn)
		{
			*(ipRtn+i) = rtn[i];
		}
		if (fAngle)
		{
			*(fAngle+i) = angle[i];
		}
		if (pPoint)
		{
			pPoint[ i*4 + 0].x = point[0+i*4].x + rect.x;
			pPoint[ i*4 + 0].y = point[0+i*4].y + rect.y;
			pPoint[ i*4 + 1].x = point[1+i*4].x + rect.x;
			pPoint[ i*4 + 1].y = point[1+i*4].y + rect.y;
			pPoint[ i*4 + 2].x = point[2+i*4].x + rect.x;
			pPoint[ i*4 + 2].y = point[2+i*4].y + rect.y;
			pPoint[ i*4 + 3].x = point[3+i*4].x + rect.x;
			pPoint[ i*4 + 3].y = point[3+i*4].y + rect.y;
		}
		if (pRect)
		{
			IM_POINT newPos[4];
			IM_POINT oldPos[4];
			oldPos[ 0].x = point[0+i*4].x + rect.x;
			oldPos[ 0].y = point[0+i*4].y + rect.y;
			oldPos[ 1].x = point[1+i*4].x + rect.x;
			oldPos[ 1].y = point[1+i*4].y + rect.y;
			oldPos[ 2].x = point[2+i*4].x + rect.x;
			oldPos[ 2].y = point[2+i*4].y + rect.y;
			oldPos[ 3].x = point[3+i*4].x + rect.x;
			oldPos[ 3].y = point[3+i*4].y + rect.y;

			int height = image->height;
			int width  = image->width;
			float cos_ang = cos(angle[i]);
			float sin_ang = sin(angle[i]);
			int newWidth  = fabs(height*sin_ang) + fabs(width*cos_ang);
			int newHeight = fabs(height*cos_ang) + fabs(width*sin_ang);
			image_map(
							  newPos, oldPos,angle[i],
							  newHeight, newWidth,
							  height, width,0
							 );
			rcRotate[i].left    = (newPos[0].x + newPos[1].x)/2;
			rcRotate[i].top     = (newPos[0].y + newPos[3].y)/2;
			rcRotate[i].right   = (newPos[2].x + newPos[3].x)/2;
			rcRotate[i].bottom  = (newPos[1].y + newPos[2].y)/2;

			pRect[i].left   = rcRotate[i].left;
			pRect[i].top    = rcRotate[i].top;
			pRect[i].right  = rcRotate[i].right;
			pRect[i].bottom = rcRotate[i].bottom;
		
		}
	
	}

	cvReleaseImage(&imtmp);
	return 0;
	
}

long getPossRectL(IM_POINT * point)
{
	long rc = (*(point+0)).x;
	for (int i = 0; i < 4; i++)
	{
		if ( (*(point+i)).x < rc )
		{
			rc = (*(point+i)).x;
		}
	}
	return rc;
}

long getPossRectT(IM_POINT * point)
{
	long rc = (*(point+0)).y;
	for (int i = 0; i < 4; i++)
	{
		if ((*(point+i)).y < rc)
		{
			rc = (*(point+i)).y;
		}
	}
	return rc;
}

long getPossRectR(IM_POINT * point)
{
	long rc = (*(point+0)).x;
	for (int i = 0; i < 4; i++)
	{
		if ((*(point+i)).x > rc)
		{
			rc = (*(point+i)).x;
		}
	}
	return rc;
}

long getPossRectB(IM_POINT * point)
{
	long rc = (*(point+0)).y;
	for (int i = 0; i < 4; i++)
	{
		if ((*(point+i)).y > rc)
		{
			rc = (*(point+i)).y;
		}
	}
	return rc;
}


int get_ex_rotate_angle(IplImage * src, float & fAngle,  int iDPI, IM_RECT rectSamplePerc, BOOL bWidthGreaterHeight, BOOL bParamIsValid )
{
	if (!src)
	{
		return 1;
	}
	if (iDPI < 1)
	{
		return 1;
	}
	
	long width = 0;
	long height = 0;
	long widthstep = 0;

	// 实际长宽
	float widthPhysics = 0;
	float heightPhysics = 0;

	uchar* pbgray = NULL;
	IplImage* gray = NULL;
	uchar* pDibbgr = NULL;
	uchar* pDibgray = NULL;

	int threshold = 192;

	int x;
	int y;

	// 投影
	int *projX = NULL;
	int *projY = NULL;

	long xBarycentre = 0;
	long yBarycentre = 0;
	long sumX = 1;
	long sumY = 1;
	long i = 0;
	long xStartHill = 0;
	long xEndHill = 0;
	long yStartHill = 0;
	long yEndHill = 0;
	long xHillIdx = 0;
	long yHillIdx = 0;
	long xMax = 0;
	long yMax = 0;
	

	DOC_TYPE iDocType = DOC_A4PAPER; //    0     A3,4 纸张       
	                 //    1     票据（长度比高度大约2倍）   
	                 //    2     名片    
	                 //    3     身份证

	//int resizeFac = 1;

	


	IplImage *pim = NULL;

	width  = src->width;
	height = src->height;

	pim = cvCreateImage(cvSize(width, height), src->depth, src->nChannels);
	if (!pim)
	{
		return -1;
	}
	cvCopy(src,pim);


	if (3 == pim->nChannels)
	{
       
	   gray = cvCreateImage(cvSize(width, height), pim->depth, 1);

     
	   getRGBMax(pim,gray);
	   
	}
	else if (1 == pim->nChannels)
	{
	 
	   gray = cvCreateImage(cvSize(width, height), pim->depth, 1);
	   cvCopy(pim, gray);
	}
	else 
	{
		if (pim)
		{
		    cvReleaseImage(&pim);
		}
		return -1;
	}

	threshold = otsu((uchar*)gray->imageData, gray->width, gray->height, gray->widthStep);
	threshold *= 1.1; 

	pDibgray = (uchar*)gray->imageData;
	width = gray->width;
	height = gray->height;
	projX = new int[width+1];
	projY = new int[height+1];
	if (!projX || !projY)
		return 0;
	memset(projX, 0, (width+1)*sizeof(int));
	memset(projY, 0, (height+1)*sizeof(int));

	// 防止文档边框有黑色线条干扰，考虑要不要吧gray的边框先抹掉。同时投影也减少了干扰

	for (y = 0; y < height*0.05; y++)
	{
		uchar * p = (uchar*)(pDibgray+(y)*widthstep);
		memset (p, 255, width);
	}
	for (y = height*0.95; y < height; y++)
	{
		uchar * p = (uchar*)(pDibgray+(y)*widthstep);
		memset (p, 255, width);
	}
	for (y = 0; y < height; y++)
	{
		uchar * p = (uchar*)(pDibgray+(y)*widthstep);
		for (x = 0; x < width*0.05; x++)
		{
			*(p + x) = 255;
		}
		for (x = width*0.95; x < width; x++)
		{
			*(p + x) = 255;
		}
	}

	widthstep = gray->widthStep;
	long lCoef = (width/2)*(width/2)+(height/2)*(height/2);
	for (y = 0; y < height; y++)
	{
		uchar * p = (uchar*)(pDibgray+(y)*widthstep);
		for (x = 0; x < width; x++)
		{
			float k = 0;
			int detaX = abs(long(x - width/2));
			int detaY = abs(long(y - height/2));
			k = 1 - 0.3f * ( ((float)(detaX*detaX + detaY*detaY))/lCoef );// 天啊，这里还需要强制转换一下？ int/float 有问题？ 
			int val = *(p + x) >= threshold*k ? 0 : 1;
			*(projX + x) += val;
			*(projY + y) += val;
			*(p + x) = val;

		}
	}

	//cvSaveImage("F:\\imagegray.bmp",gray);
	// 求重心
	xBarycentre = 0;
	yBarycentre = 0;
	sumX = 1;
	sumY = 1;
	xEndHill = width - 1;
	yEndHill = height - 1;
	for (i = width*0.05+1; i < width*0.95-1; i++)
	{	
		xBarycentre += *(projX+i) * i;
		sumX        += *(projX+i);
	}
	
	for (i = height*0.05+1; i < height*0.95-1; i++)
	{
		yBarycentre += *(projY+i) * i;
		sumY        += *(projY+i);
	}
    xBarycentre = xBarycentre/sumX;
	yBarycentre = yBarycentre/sumY;

	for (i = 1; i < (width-1)/2; i++)
	{	
		if ( *(projX+i) > 0.85*height)
		{
		    xStartHill = *(projX+i) ;
			break;
		}
	}
	for (i = width*0.95-1; i > (width-1)/2; i--)
	{	
		if ( *(projX+i) > 0.85*height)
		{
		    xEndHill = *(projX+i) ;
			break;
		}
	}
	for (i = 1; i < (height-1)/2; i++)
	{
		if ( *(projY+i) > 0.85*width)
		{
			yStartHill = *(projY+i);
			break;
		}
	}
	for (i = height*0.95-1; i > (height-1)/2; i--)
	{
		if ( *(projY+i) > 0.85*width)
		{
			yStartHill = *(projY+i);
			break;
		}
	}

	for (i = width*0.05+1; i < width*0.95-1; i++)
	{	
		if (xMax < *(projX+i))
		{
			xMax = *(projX+i);
			xHillIdx = i;
		}
	}
	
	for (i = height*0.05+1; i < height*0.95-1; i++)
	{
		if (yMax < *(projY+i))
		{
			yMax = *(projY+i);
			yHillIdx = i;
		}
	}


	width = gray->width;
	height = gray->height;
    int projXsum[2];
	int projYsum[2];
	memset( &projXsum, 0, 2*sizeof(int));
	memset( &projYsum, 0, 2*sizeof(int));

	long xlen = 0;
	long ylen = 0;

	widthPhysics = (float)gray->width / (float)iDPI;
	heightPhysics = (float)gray->height / (float)iDPI;
	widthPhysics *= 2.54;
	heightPhysics *= 2.54;//英寸转为厘米


	iDocType = DOC_UNDEFINE;// 未定义
	if ( width > height )
	{
		if (  fabs( (float)width / (float)height - 1.5 ) < 0.2
			&& fabs(widthPhysics - 29.5) < 3 
			&& fabs(heightPhysics - 21) < 2.5//( resizeFac*width > 0.8*iResWidth || resizeFac*height > 0.8*iResHeight )// 2000 1500
			)  // A4 纸的尺寸
		{
			iDocType = DOC_A4PAPER;
		}
		else if ( fabs( (float)width / (float)height - 1.6 ) < 0.2
			&& fabs(widthPhysics - 9) < 1.5 
			&& fabs(heightPhysics - 5.5) < 1
			//( resizeFac*width < 0.35*iResWidth || resizeFac*height < 0.35*iResHeight ))     //&& ( resizeFac*width < 1000 || resizeFac*height < 600 )) 
			)
		{
			iDocType = DOC_BCARD;// 名片或者身份证之类的吧
		}
		else if (
			   fabs(widthPhysics - 8.5) < 1.5 
			&& fabs(heightPhysics - 5.5) < 1
			)//(resizeFac*width < 0.38*iResWidth  || resizeFac*height < 0.30*iResHeight)                      //  900  550
        { 
			iDocType = DOC_BCARD;//或者身份证之类的吧
		}
		else if ( fabs( (float)width / (float)height - 2 ) < 0.2 )
		{
		    iDocType = DOC_BILL;                                 // 票据
		}
		else  
		{
			iDocType = DOC_UNDEFINE;// 未定义
		}
	}
	else
	{
		if (  fabs( (float)height / (float)width - 1.5 ) < 0.2
			&& fabs(widthPhysics - 21) < 2.5
			&& fabs(heightPhysics - 29.5) < 3         //&& ( resizeFac*height > 2000 || resizeFac*width > 1500 )
			)  // A4 纸的尺寸
		{
			iDocType = DOC_A4PAPER;
			
		}
		else if ( fabs( (float)height / (float)width - 1.6 ) < 0.2
			&& fabs(widthPhysics - 5.5) < 1
			&& fabs(heightPhysics - 9) < 1.5 
			)
			//&& ( resizeFac*height < 0.35*iResHeight || resizeFac*width < 0.35*iResWidth ))                 //&& ( resizeFac*height < 1000 || resizeFac*width < 600 ))
		{
			iDocType = DOC_BCARD;//或者身份证之类的吧

		}
		else if (
			   fabs(widthPhysics -  5.5) < 1
			&& fabs(heightPhysics - 8.5) < 1.5
			)
			//(resizeFac*height < 0.38*iResHeight || resizeFac*width < 0.30*iResWidth)                               //resizeFac*height < 900 || resizeFac*width < 550)
        { 
			iDocType = DOC_BCARD;//或者身份证之类的吧
		}
		else if ( fabs( (float)height / (float)width - 2 ) < 0.2 )
		{
		                                                   // 票据
			iDocType = DOC_BILL;
		}
		else  
		{
			iDocType = DOC_UNDEFINE;// 未定义
		}
	}

	if (bParamIsValid)
	{
		iDocType = DOC_CUSTOMER;// 用户定义
	}

	if (!bParamIsValid)
	{

		if (   width > height )
		{
			switch (iDocType)
			{
			case DOC_A4PAPER:
			case DOC_UNDEFINE:
				{

					IM_RECT rectL;
					IM_RECT rectR;
					long rectSumL = 0;
					long rectSumR = 0;
					rectL.left = width*0.05;
					rectL.right = width*0.125;
					rectL.top   = height*0.25;
					rectL.bottom = height*0.75;

					rectR.left = width*0.875;
					rectR.right = width*0.95;
					rectR.top   = height*0.25;
					rectR.bottom = height*0.75;
					rectSumL = getRectSum(gray, rectL);
					rectSumR = getRectSum(gray, rectR);

					if ((float)rectSumL/(float)rectSumR > 1.5)
					{
                        fAngle = 90;
						break;
					}
					else if ((float)rectSumR/(float)rectSumL > 1.5)
					{
                        fAngle = -90;
						break;
					}

					if (xBarycentre < width/2)
					{
						fAngle = 90;
					}
					else
					{
						fAngle = -90;
					}
					break;
				}
			case DOC_BILL:
				{

					/*if ( 0 != yStartHill)
					{
						if (yStartHill > abs ((LONG)(height - 1 - yEndHill)))
						{
							fAngle = 0;
						}
						else
						{
							fAngle = 180;
						}
					}*/
					IM_RECT rectT;
					IM_RECT rectB;
					long rectSumT = 0;
					long rectSumB = 0;
					rectT.left = width*0.25;
					rectT.right = width*0.75;
					rectT.top   = height*0.05;
					rectT.bottom = height*0.125;

					rectB.left = width*0.25;
					rectB.right = width*0.75;
					rectB.top   = height*0.875;
					rectB.bottom = height*0.95;
					rectSumT = getRectSum(gray, rectT);
					rectSumB = getRectSum(gray, rectB);


					if (rectSumT >= rectSumB)
					{
						fAngle = 0;
					}
					else 
					{
						fAngle = 180;
					}
					/*
					else // 没有找到直线吧
					{
						if (yBarycentre < height/2)
						{
							fAngle = 0;
						}
						else
						{
							fAngle = 180;
						}
					}
					*/
					break;
				}
			case DOC_BCARD:
			case DOC_IDCARD:
				{
					
					if (0 != yHillIdx)
					{
						if (yHillIdx > height/2)
						{
							fAngle = 0;
						}
						else 
						{
							fAngle = 180;
						}

					}
					else
					{
						if (yBarycentre > height/2)
						{
							fAngle = 180;
						}
						else 
						{
							fAngle = 0;
						}

					}
				}

			}
		}
		else
		{ // 竖放的
			switch (iDocType)
			{
			case DOC_A4PAPER:
			case DOC_UNDEFINE:
				{
					IM_RECT rectT;
					IM_RECT rectB;
					long rectSumT = 0;
					long rectSumB = 0;
					rectT.left = width*0.25;
					rectT.right = width*0.75;
					rectT.top   = height*0.05;
					rectT.bottom = height*0.125;

					rectB.left = width*0.25;
					rectB.right = width*0.75;
					rectB.top   = height*0.875;
					rectB.bottom = height*0.95;
					rectSumT = getRectSum(gray, rectT);
					rectSumB = getRectSum(gray, rectB);

					if ((float)rectSumT/(float)rectSumB > 1.5)
					{
                         fAngle = 0;
						 break;
					}
					else if ((float)rectSumB/(float)rectSumT > 1.5)
					{
						fAngle = 180;
						break;
					}

					if (yBarycentre < width/2)
					{
						fAngle = 180;
						
					}
					else
					{
						fAngle = 180;
					}
					break;
				}
			case DOC_BILL:
				{

					IM_RECT rectL;
					IM_RECT rectR;
					long rectSumL = 0;
					long rectSumR = 0;
					rectL.left = width*0.05;
					rectL.right = width*0.125;
					rectL.top   = height*0.25;
					rectL.bottom = height*0.75;

					rectR.left = width*0.875;
					rectR.right = width*0.95;
					rectR.top   = height*0.25;
					rectR.bottom = height*0.75;
					rectSumL = getRectSum(gray, rectL);
					rectSumR = getRectSum(gray, rectR);
					if (rectSumL >= rectSumR)
					{
						fAngle = 90;
					}
					else
					{
						fAngle = -90;
					}


					/*
					if ( 0 != xStartHill)
					{
						if (xStartHill > abs ((LONG)(width - 1 - xEndHill)))
						{
							fAngle = 90;
						}
						else
						{
							fAngle = -90;
						}
					}
					else // 没有找到直线吧
					{
						if (xBarycentre < width/2)
						{
							fAngle = 90;
						}
						else
						{
							fAngle = -90;
						}
					}
					*/
					break;
				}
			case DOC_BCARD:
			case DOC_IDCARD:
				{
					if (0 != xHillIdx)
					{
						if (xHillIdx > width/2)
						{
							fAngle = 90;
						}
						else 
						{
							fAngle = -90;
						}

					}
					else
					{
						if (xBarycentre > width/2)
						{
							fAngle = 90;
						}
						else 
						{
							fAngle = -90;
						}

					}
				}

			}
		}
	}
	else // 按照用户定义的来
	{
		if (     bWidthGreaterHeight && width > height    // 正常时候宽度大于长度
			|| ! bWidthGreaterHeight && width < height    // 正常时候高度大于宽度
			) 
		{
			        IM_RECT rectT;
					IM_RECT rectB;
					long rectSumT = 0;
					long rectSumB = 0;
					rectT.left = width*rectSamplePerc.left/100.0f;
					rectT.right = width*rectSamplePerc.right/100.0f;
					rectT.top   = height*rectSamplePerc.top/100.f;
					rectT.bottom = height*rectSamplePerc.bottom/100.0f;

					rectB.left = width*(100-rectSamplePerc.right)/100.0f;
					rectB.right = width*(100-rectSamplePerc.left)/100.0f;
					rectB.top   = height*(100-rectSamplePerc.bottom)/100.0f;
					rectB.bottom = height*(100-rectSamplePerc.top)/100.f;
					rectSumT = getRectSum(gray, rectT);
					rectSumB = getRectSum(gray, rectB);


					if (rectSumT >= rectSumB)
					{
						fAngle = 0;
					}
					else 
					{
						fAngle = 180;
					}

		}
		else if (      bWidthGreaterHeight && width < height    // 正常时候，宽度大于长度
			      || ! bWidthGreaterHeight && width > height    // 正常时候，高度大于宽度
			)
		{
			        IM_RECT rectL;
			        IM_RECT rectR;
					long rectSumL = 0;
					long rectSumR = 0;
					rectL.left = width*rectSamplePerc.top/100.0f;
					rectL.right = width*rectSamplePerc.bottom/100.0f;
					rectL.top   = height*(100-rectSamplePerc.right)/100.0f;
					rectL.bottom = height*(100-rectSamplePerc.left)/100.0f;

					rectR.left = width*(100-rectSamplePerc.bottom)/100.0f;
					rectR.right = width*(100-rectSamplePerc.top)/100.0f;
					rectR.top   = height*rectSamplePerc.left/100.0f;
					rectR.bottom = height*rectSamplePerc.right/100.0f;
					rectSumL = getRectSum(gray, rectL);
					rectSumR = getRectSum(gray, rectR);
					if (rectSumL >= rectSumR)
					{
						fAngle = 90;
					}
					else
					{
						fAngle = -90;
					}

		}

	}

	if (pim)
	{
		cvReleaseImage(&pim);
	}
	if (gray)
	{
		cvReleaseImage(&gray);
	}
	if (projX)
	{
		delete []projX;
	}
	if (projY)
	{
		delete []projY;
	}
	
	return 0;


}

int doExRotateAngle(float fAngle, IplImage * src)
{
	if (!src)
		return 1;

	long width = src->width;
	long height = src->height;
	long widthstep = src->widthStep;
	long newWidth = 0;
	long newHeight = 0;
	long newWidthstep = 0;
	
	if (   fabs(fAngle - 3.1415926535f/2.0f) < 1e-5
		|| fabs(fAngle - 90) < 1e-5
		)
	{
		imageRotateS(src, 90);
		

	}
	else if (fabs(fAngle - 3.1415926535f) < 1e-5
		|| fabs(fAngle - 180) < 1e-5 
		)
	{
		imageRotateS(src, 180);

	}
	else if (fabs(fAngle - (-3.1415926535f/2.0f)) < 1e-5
		|| fabs(fAngle + 90) < 1e-5
		)
	{
		imageRotateS(src, 270);
		
	}
	else
	{

	}

	return 0;

}

int imageRotateS(IplImage * src, int iAngle )
{
	if (!src)
	{
		return 1;
	}

	long width = src->width;
	long height = src->height;
	long widthstep = src->widthStep;
	//const long bpp = src->;
	long newWidth = 0;
	long newHeight = 0;
	long newWidthstep = 0;
	uchar  *pByte = NULL; 
	pByte = new uchar[widthstep*height];
	if (!pByte)
		return 0;
	memset(pByte, 0, widthstep * height * sizeof(uchar));
	long x;
	long y;
	IplImage * image = NULL;
	switch (iAngle)
	{
	    case 0:
		{
			break;
		}
		case 1:
		case 90:
			{
				newWidth = height;
				newHeight = width;
				int effX = 1;				
		
				image = cvCreateImage(cvSize(newWidth, newHeight), src->depth, src->nChannels);
				newWidthstep = image->widthStep;
				uchar *pSrc = (uchar *)src->imageData;
				widthstep = src->widthStep;

				uchar *pBibImage = (uchar *)image->imageData;
				if (3 == src->nChannels)
				{

					for (y = 0; y < newHeight; y++)
					{
						uchar * pDst = (uchar*)(pBibImage + (y)*newWidthstep);
						for (x = 0; x < newWidth; x ++)
						{
							int srcX = y;
							int srcY = newWidth - 1 - x;
							*(pDst + x*3 + 0 )  = *(pSrc + ( srcY)*widthstep + srcX *3 + 0 );
							*(pDst + x*3 + 1 )  = *(pSrc + ( srcY)*widthstep + srcX *3 + 1 );
							*(pDst + x*3 + 2 )  = *(pSrc + ( srcY)*widthstep + srcX *3 + 2 );
						}
					}
				}
				else
				{
					for (y = 0; y < newHeight; y++)
					{
						uchar * pDst = (uchar*)(pBibImage + (y)*newWidthstep);
						for (x = 0; x < newWidth; x ++)
						{
							int srcX = y;
							int srcY = newWidth - 1 - x;
							*(pDst + x )  = *(pSrc + (srcY)*widthstep + srcX );
						}
					}

				}
				
				break;
			}
		case 2:
		case 180:
			{
				newWidth = width;
				newHeight = height;
				int effX = 1;
				newWidthstep = widthstep;

				
				image = cvCreateImage(cvSize(newWidth, newHeight), src->depth, src->nChannels);
				newWidthstep = image->widthStep;
				uchar *pSrc = (uchar *)src->imageData;
				uchar *pBibImage = (uchar *)image->imageData;
				if (3 == src->nChannels)
				{

					for (y = 0; y < newHeight; y++)
					{
						uchar * pDst = (uchar*)(pBibImage + (y)*newWidthstep);
						for (x = 0; x < newWidth; x ++)
						{
							int srcX = newWidth - 1 - x;
							int srcY = newHeight - 1 - y;
							*(pDst + x*3 + 0 )  = *(pSrc + (srcY)*widthstep + srcX *3 + 0 );
							*(pDst + x*3 + 1 )  = *(pSrc + (srcY)*widthstep + srcX *3 + 1 );
							*(pDst + x*3 + 2 )  = *(pSrc + (srcY)*widthstep + srcX *3 + 2 );
						}
					}
				}
				else
				{
					for (y = 0; y < newHeight; y++)
					{
						uchar * pDst = (uchar*)(pBibImage + (y)*newWidthstep);
						for (x = 0; x < newWidth; x ++)
						{
							int srcX = newWidth - 1 - x;
							int srcY = newHeight - 1 - y;
							*(pDst + x )  = *(pSrc + (srcY)*widthstep + srcX );
						}
					}

				}
				
				break;

			}
		case 3:
		case -1:
		case 270:
		case -90:
			{
				newWidth = height;
				newHeight = width;
		

				image = cvCreateImage(cvSize(newWidth, newHeight), src->depth, src->nChannels);
				newWidthstep = image->widthStep;
				uchar *pSrc = (uchar *)src->imageData;
				widthstep = src->widthStep;

				uchar *pBibImage = (uchar *)image->imageData;
				if (3 == src->nChannels)
				{

					for (y = 0; y < newHeight; y++)
					{
						uchar * pDst = (uchar*)(pBibImage + (y)*newWidthstep);
						for (x = 0; x < newWidth; x ++)
						{
							int srcX = newHeight - 1 - y;
							int srcY = x;
							*(pDst + x*3 + 0 )  = *(pSrc + (srcY)*widthstep + srcX *3 + 0 );
							*(pDst + x*3 + 1 )  = *(pSrc + (srcY)*widthstep + srcX *3 + 1 );
							*(pDst + x*3 + 2 )  = *(pSrc + (srcY)*widthstep + srcX *3 + 2 );
						}
					}
				}
				else
				{
					for (y = 0; y < newHeight; y++)
					{
						uchar * pDst = (uchar*)(pBibImage + (y)*newWidthstep);
						for (x = 0; x < newWidth; x ++)
						{
							int srcX = newHeight - 1 - y;
							int srcY = x;
							*(pDst + x )  = *(pSrc + (srcY)*widthstep + srcX );
						}
					}
				}

				break;
			}
			

	}

	if (pByte)
	{
		delete []pByte;
	}
	if (image)
	{
		cvReleaseImage(&image);
	}

	return 0;
	
}


long getRectSum(IplImage * gray, IM_RECT rect)
{
	if (!gray)
	{
		return 1;
	}
	if (gray->nChannels != 1)
	{
		return 1;
	}
	if (   rect.left < 0
		|| rect.right > gray->width - 1
		|| rect.top < 0
		|| rect.bottom > gray->height - 1
		)
	{
		return 1;
	}

	long x;
	long y;
	uchar * pDibgray = NULL;
	long widthstep = gray->widthStep;
	long height    = gray->height;

	long yStart = rect.top;
	long yEnd   = rect.bottom;
	long xStart = rect.left;
	long xEnd   = rect.right;
	pDibgray     = (uchar*)gray->imageData;
	long sum = 1;
	for (y = yStart; y < yEnd; y++)
	{
		uchar * p = (uchar*)(pDibgray+(y)*widthstep);
		for (x = xStart; x < xEnd; x++)
		{
			if ( *(p + x) > 0)
			{
				sum++;
			}
		}
	}

	return sum;

}

long getGrayImageSum(IplImage * gray, int threshold )
{

	if (!gray)
	{
		return 1;
	}
	IplImage * image = cvCreateImage(cvSize(gray->width,gray->height), gray->depth, gray->nChannels);
	
	if (!image)
		return 1;
	if (1 != image->nChannels)
	{
		return 1;
	}

	int rtn = 1;
	int y = 0;
	int x = 0;
	int width = image->width;
	int height = image->height;

	uchar *pImData = (uchar *)image->imageData;
	for (y = 0; y < height; y++)
		{
			
			uchar* p  = (uchar*)(pImData +  (y)*image->widthStep);
			for (x = 0; x < width; x++)
			{
				if (*(p+x) > threshold)
				{
                   rtn++;
				}
			}
		}
	return rtn;
}

// 照片去底
int delBackground(IplImage * src, IplImage * dst)
{
	if (!src)
	{
		return RTN_PARAM_ERR;
	}
	if (src->nChannels != 3)
	{
		return RTN_PARAM_ERR;
	}
	if (dst->nChannels != 4)
	{
		return RTN_PARAM_ERR;
	}
	if (
		   src->height != dst->height
        || src->width  != dst->width
		)
	{
        return RTN_PARAM_ERR;
	}
	int width = src->width;
	int height = src->height;


	IplImage * temp = cvCreateImage(cvSize(width, height), src->depth, src->nChannels);
	if (!temp)
	{
		return RTN_MALLOC_ERR;
	}

	cvCopy(src, temp);

	CvScalar color1;
	CvScalar color2;
	CvScalar color3;
	CvScalar color4;

	int x = 0;
	int y = 0;
	uchar *pData = (uchar*)(temp->imageData);

	x = 26.0/342.0 * width;
	y = 97.0/487.0 * height;
	color1.val[0] = *(pData + y*temp->widthStep + 3 * x + 0);
	color1.val[1] = *(pData + y*temp->widthStep + 3 * x + 1);
	color1.val[2] = *(pData + y*temp->widthStep + 3 * x + 2);

	x = 323.0/342.0 * width;
	y = 97.0/487.0 * height;
	color2.val[0] = *(pData + y*temp->widthStep + 3 * x + 0);
	color2.val[1] = *(pData + y*temp->widthStep + 3 * x + 1);
	color2.val[2] = *(pData + y*temp->widthStep + 3 * x + 2);

	x = 26.0/342.0 * width;
	y = 326.0/487.0 * height;
	color3.val[0] = *(pData + y*temp->widthStep + 3 * x + 0);
	color3.val[1] = *(pData + y*temp->widthStep + 3 * x + 1);
	color3.val[2] = *(pData + y*temp->widthStep + 3 * x + 2);

	x = 323.0/342.0 * width;
	y = 326.0/487.0 * height;
	color4.val[0] = *(pData + y*temp->widthStep + 3 * x + 0);
	color4.val[1] = *(pData + y*temp->widthStep + 3 * x + 1);
	color4.val[2] = *(pData + y*temp->widthStep + 3 * x + 2);

	color1.val[0] = (color1.val[0] + color2.val[0] + color3.val[0] + color3.val[0])/4;
	color1.val[1] = (color1.val[1] + color2.val[1] + color3.val[1] + color3.val[1])/4;
	color1.val[2] = (color1.val[2] + color2.val[2] + color3.val[2] + color3.val[2])/4;

	memset(dst->imageData, 255, dst->height * dst->widthStep * sizeof(uchar));

	for (y = 0; y < height; y++)
	{
		uchar * p = (uchar*)(pData + y * temp->widthStep);
		uchar * pdst = (uchar*)(dst->imageData + y * dst->widthStep);
		for (x = 0; x < width; x++)
		{
			if (   fabs( *(p + 3*x) - color1.val[0]) < 35
				&& fabs( *(p + 3*x + 1) - color1.val[1]) < 30
				&& fabs( *(p + 3*x + 2) - color1.val[2]) < 35
				)
			{
				*(p + 3*x) = 255;
				*(p + 3*x + 1) = 255;
				*(p + 3*x + 2) = 255;
				*(pdst + 4*x + 3) = 0;

			}
			memcpy(pdst + 4*x,p + 3*x, 3*sizeof(uchar));
		}
	}

	cvReleaseImage(&temp);

	return RTN_SUCCESS;

}

// 去噪
int denoising(IplImage * src,IplImage * dst)
{

	if (!src || !dst)
	{
		return RTN_PARAM_ERR;
	}
	if (   src->width != dst->width
		|| src->height != dst->height
		|| src->nChannels != dst->nChannels)
	{
		return RTN_PARAM_ERR;
	}

	cvSmooth(src, dst);

	return RTN_SUCCESS;
}

//获取蓝色像素
int image_getBlue(IplImage* src, IplImage* dst )
{
	if (src->nChannels != 3)
	{
		return 0;
	}
	if (!dst)
	{
		dst = src;
	}
	if (   src->width != dst->width
		|| src->height != dst->height
		|| src->nChannels != dst->nChannels)
	{
		return 0;
	}

	IplImage* hsv = cvCreateImage( cvGetSize(src), 8, 3 );
	cvCvtColor(src, hsv, CV_BGR2HSV);

	for (int i = 0; i < src->height; i++)    
	{    
		  for (int j = 0; j < src->width; j++)    
		  {    
				CvScalar s = cvGet2D(hsv, i, j);
				if((s.val[0] > 90) && (s.val[0] < 150) &&(s.val[1] > 20) && (s.val[2] > 50))
				{
					cvSet2D(dst, i, j, cvGet2D(src, i, j));
				}
				else
				{
					cvSet2D(dst, i, j, cvScalar(255,255,255,0));
				}
		  }    
	} 
	cvReleaseImage(&hsv);

	return 1;
}

int highlight_remove(IplImage* src,IplImage* dst)
{
	int height=src->height;
	int width=src->width;
	int step=src->widthStep;
	int i=0,j=0;
	unsigned char R,G,B,MaxC;
	double alpha,beta,alpha_r,alpha_g,alpha_b,beta_r,beta_g,beta_b,temp=0,realbeta=0,minalpha=0;
	double gama,gama_r,gama_g,gama_b;
	unsigned char* srcData;
	unsigned char* dstData;
	for (i=0;i<height;i++)
	{
		srcData=(unsigned char*)src->imageData+i*step;
		dstData=(unsigned char*)dst->imageData+i*step;
		for (j=0;j<width;j++)
		{
			R=srcData[j*3];
			G=srcData[j*3+1];
			B=srcData[j*3+2];

			alpha_r=(double)R/(double)(R+G+B);
			alpha_g=(double)G/(double)(R+G+B);
			alpha_b=(double)B/(double)(R+G+B);
			alpha=max(max(alpha_r,alpha_g),alpha_b);
			MaxC=max(max(R,G),B);// compute the maximum of the rgb channels
			minalpha=min(min(alpha_r,alpha_g),alpha_b);                 beta_r=1-(alpha-alpha_r)/(3*alpha-1);
			beta_g=1-(alpha-alpha_g)/(3*alpha-1);
			beta_b=1-(alpha-alpha_b)/(3*alpha-1);
			beta=max(max(beta_r,beta_g),beta_b);//将beta当做漫反射系数，则有                 // gama is used to approximiate the beta
			gama_r=(alpha_r-minalpha)/(1-3*minalpha);
			gama_g=(alpha_g-minalpha)/(1-3*minalpha);
			gama_b=(alpha_b-minalpha)/(1-3*minalpha);
			gama=max(max(gama_r,gama_g),gama_b);   

			temp=(gama*(R+G+B)-MaxC)/(3*gama-1);
			//beta=(alpha-minalpha)/(1-3*minalpha)+0.08;
			//temp=(gama*(R+G+B)-MaxC)/(3*gama-1);
			dstData[j*3]=R-(unsigned char)(temp+0.5);
			dstData[j*3+1]=G-(unsigned char)(temp+0.5);
			dstData[j*3+2]=B-(unsigned char)(temp+0.5);   
		}
	} 
	return RTN_SUCCESS;
}


void FindBorderLine(IplImage *pImgGray, int &lineLeft, int &lineTop, int &lineRight, int &lineBottom)
{
	int tempWidth=(pImgGray->width>>2), tempHeight=(pImgGray->height>>2);
	int sumMin=0, sumTemp=0, barWidth=0;
	int const MAX_BAR_WIDTH=8;			//最大条形宽度
	uchar *pImgDataLoop=NULL;

	//step1: 寻找左边1/4区域每列加和最小位置
	sumMin = 0xffffff;
	for (int i=0; i<tempWidth; i++)
	{
		sumTemp = 0;
		barWidth = (i+MAX_BAR_WIDTH>tempWidth?tempWidth-i:MAX_BAR_WIDTH);
		for (int j=0; j<barWidth; j++)
		{
			pImgDataLoop = (uchar*)(pImgGray->imageData+i+j);
			for (int k=0; k<pImgGray->height; k++)
			{
				sumTemp += *pImgDataLoop;
				pImgDataLoop += pImgGray->widthStep;
			}
		}
		if(sumMin >= sumTemp/barWidth)
		{
			sumMin = sumTemp/barWidth;
			lineLeft = i;
		}
	}

	//step2: 寻找右边1/4区域加和最小者
	sumMin = 0xffffff;
	for (int i=0; i<tempWidth; i++)
	{
		sumTemp = 0;
		barWidth = (i+MAX_BAR_WIDTH>tempWidth?tempWidth-i:MAX_BAR_WIDTH);
		for (int j=0; j<barWidth; j++)
		{
			pImgDataLoop = (uchar*)(pImgGray->imageData+pImgGray->width-1-i-j);
			for (int k=0; k<pImgGray->height; k++)
			{
				sumTemp += *pImgDataLoop;
				pImgDataLoop += pImgGray->widthStep;
			}
		}
		if(sumMin >= sumTemp/barWidth)
		{
			sumMin = sumTemp/barWidth;
			lineRight = pImgGray->width-1-i;
		}
	}

	//step3: 寻找上部1/4区域加和最小者
	sumMin = 0xffffff;
	for (int i=0; i<tempHeight; i++)
	{
		sumTemp = 0;
		barWidth = (i+MAX_BAR_WIDTH>tempHeight?tempHeight-i:MAX_BAR_WIDTH);
		for (int j=0; j<barWidth; j++)
		{
			pImgDataLoop = (uchar*)(pImgGray->imageData+(i+j)*pImgGray->widthStep);
			for (int k=0; k<pImgGray->width; k++)
			{
				sumTemp += pImgDataLoop[k];
			}
		}

		if(sumMin >= sumTemp/barWidth)
		{
			sumMin = sumTemp/barWidth;
			lineTop = i;
		}
	}

	//step4: 寻找下部1/4区域加和最小者
	sumMin = 0xffffff;
	for (int i=0; i<tempHeight; i++)
	{
		sumTemp = 0;
		barWidth = (i+MAX_BAR_WIDTH>tempHeight?tempHeight-i:MAX_BAR_WIDTH);
		for (int j=0; j<barWidth; j++)
		{
			pImgDataLoop = (uchar*)(pImgGray->imageData+(pImgGray->height-1-i-j)*pImgGray->widthStep);
			for (int k=0; k<pImgGray->width; k++)
			{
				sumTemp += pImgDataLoop[k];
			}
		}

		if(sumMin >= sumTemp/barWidth)
		{
			sumMin = sumTemp/barWidth;
			lineBottom = pImgGray->height-1-i;
		}
	}
}
int otusThreshold(IplImage *pImgGray)
{
	/*最大类间方差法(otsu)原理:
	阈值将原图象分成前景，背景两个图象
	前景：用n1, csum, m1来表示在当前阈值下的前景的点数，质量矩，平均灰度
	后景：用n2, sum-csum, m2来表示在当前阈值下的背景的点数，质量矩，平均灰度
	当取最佳阈值时，背景应该与前景差别最大，关键在于如何选择衡量差别的标准
	而在otsu算法中这个衡量差别的标准就是最大类间方差
	*/
	int BestTh = 0;
	int const GRAY_NUM = 256;
	double *pHistogram=new double[GRAY_NUM];         //灰度直方图   
	double *pVariance=new double[GRAY_NUM];          //类间方差    
	double Pa=0.0;      //背景出现概率   
	double Pb=0.0;      //目标出现概率   
	double Wa=0.0;      //背景平均灰度值   
	double Wb=0.0;      //目标平均灰度值   
	double W0=0.0;      //全局平均灰度值   
	double dData1=0.0,  temp=0.0;   
	uchar *pGrayData = NULL;

	if(pHistogram && pVariance)
	{
		for(int i=0; i<GRAY_NUM; i++)   
		{   
			pHistogram[i] = 0.0;   
			pVariance[i] = 0.0;   
		}   
		for (int i=0; i<pImgGray->height; i++)
		{
			pGrayData = (uchar*)(pImgGray->imageData+i*pImgGray->widthStep);
			for (int j=0; j<pImgGray->width; j++)
			{
				pHistogram[pGrayData[j]]++;		//建立直方图  
			}
		}

		for(int i=0; i<GRAY_NUM; i++)     //计算全局平均灰度   
		{   
			pHistogram[i] /= (pImgGray->width*pImgGray->height);   
			W0 += i*pHistogram[i];   
		}   
		for(int i=0; i<GRAY_NUM; i++)     //对每个灰度值计算类间方差   
		{   
			Pa += pHistogram[i];   
			Pb = 1-Pa;   
			dData1 += i*pHistogram[i];    
			Wa = dData1/Pa;
			Wb = (W0-dData1)/Pb;
			pVariance[i] = Pa*Pb*(Wb-Wa)*(Wb-Wa);   
		}   
		//遍历每个方差，求取类间最大方差所对应的灰度值 
		for(int i=0; i<GRAY_NUM; i++)   
		{   
			if(pVariance[i]>temp)   
			{   
				temp = pVariance[i];   
				BestTh = i;   
			}   
		}
	}
	if(pHistogram) delete []pHistogram;
	if(pVariance) delete []pVariance;
	return BestTh;
}

//在边界内计算重心
//pImgGray: IN, 灰度图像
//thVal: IN, 阈值
//lineLeft: IN, 左边界
//lineRight: IN, 右边界
//lineTop: IN, 上边界
//lineBottom: IN, 下边界
//pPntBaryCenter: OUT, 重心点
int CmpBaryCenter(IplImage *pImgGray, int thVal, int lineLeft, int lineTop, int lineRight, int lineBottom, IM_POINT *pPntBaryCenter)
{
	if(pImgGray==NULL || pPntBaryCenter==NULL) return RTN_PARAM_ERR;

	UINT32 *pSumCol = new UINT32[lineRight-lineLeft];
	UINT32 *pSumRow = new UINT32[lineBottom-lineTop];
	UINT64 sumProduct=0, sumVal=0;
	uchar *pImgDataLoop = NULL;

	if(pSumCol==NULL || pSumRow==NULL)
	{
		if(pSumCol) delete pSumCol;
		if(pSumRow) delete pSumRow;
		return RTN_MALLOC_ERR;
	}

	pPntBaryCenter->x = pPntBaryCenter->y = 0xffff;
	memset(pSumCol,0,(lineRight-lineLeft)*sizeof(int));
	memset(pSumRow,0,(lineBottom-lineTop)*sizeof(int));
	//ZeroMemory(pSumCol, (lineRight-lineLeft)*sizeof(int));
	//ZeroMemory(pSumRow, (lineBottom-lineTop)*sizeof(int));

	//step1: 求和
	//求取行和
	for (int i=lineTop; i<lineBottom; i++)
	{
		pImgDataLoop = (uchar*)(pImgGray->imageData+i*pImgGray->widthStep);
		for (int j=lineLeft; j<lineRight; j++)
		{
			pSumRow[i-lineTop] += (pImgDataLoop[j]>thVal?255:0);
		}
	}
	//求列和
	for (int i=lineLeft; i<lineRight; i++)
	{
		pImgDataLoop = (uchar*)(pImgGray->imageData+lineTop*pImgGray->widthStep+i);
		for (int j=lineTop; j<lineBottom; j++)
		{
			pSumCol[i-lineLeft] += (*pImgDataLoop>thVal?255:0);
			pImgDataLoop += pImgGray->widthStep;
		}
	}

	//step2: 计算重心
	sumProduct = sumVal = 0;
	for (int i=0; i<lineRight-lineLeft; i++)
	{
		sumProduct += pSumCol[i]*(i+lineLeft);		//乘积
		sumVal += pSumCol[i];
	}
	if(sumVal>0) pPntBaryCenter->x=sumProduct/sumVal;

	sumProduct = sumVal = 0;
	for (int i=0; i<lineBottom-lineTop; i++)
	{
		sumProduct += pSumRow[i]*(i+lineTop);		//乘积
		sumVal += pSumRow[i];
	}
	if(sumVal>0) pPntBaryCenter->y=sumProduct/sumVal;
	delete[] pSumCol;//xlc add 20181127
	pSumCol = NULL;
	delete[] pSumRow;//xlc add 20181127
	pSumRow  = NULL;
	return (pPntBaryCenter->x==0xffff||pPntBaryCenter->y==0xffff)?RTN_AUTO_ROTATE_FAILED:RTN_SUCCESS;
}
//根据阈值标注前景连通区, 输入的重心参数作为种子点
//pImgGray: IN, 灰度图
//thVal: IN, 阈值
//lineLeft: IN, 左边界
//lineTop: IN, 右边界
//lineRight: IN, 上边界
//lineBottom: IN, 下边界
//pPntBaryCenter: IN, 重心位置
//pImgMask: OUT, 连通区标志图
int FG_ConnDetect(IplImage *pImgGray, int thVal, int lineLeft, int lineTop, int lineRight, int lineBottom, IM_POINT *pPntBaryCenter, uchar *pImgMask)
{
	if(pImgGray==NULL || pPntBaryCenter==NULL || pImgMask==NULL) return RTN_PARAM_ERR;

	int sumMax=0, counter=0, pos=0, tempVal=0, rngPixelCount=0;
	int seedLineLeft=0, seedLineRight=0, seedLineTop=0, seedLineBottom=0;
	int const WIN_WIDTH=(pImgGray->width>>3);			//窗口大小
	uchar *pImgData = (uchar*)pImgGray->imageData;
	structRngPoint *pRngQueue=NULL;
	structRngPoint *pRngPointBuf=NULL;

	//step1: 开辟内存
	rngPixelCount = pImgGray->width*pImgGray->height;
	pRngPointBuf = new structRngPoint[rngPixelCount];
	if(pRngPointBuf==NULL || pImgMask==NULL)
	{
		if(pRngPointBuf) delete pRngPointBuf;
		if(pImgMask) delete pImgMask;
		return RTN_MALLOC_ERR;
	}

	//step2: 初始化
	memset(pImgMask, 1, pImgGray->height*pImgGray->widthStep);
	for (int i=0; i<rngPixelCount; i++)
	{
		pRngPointBuf[i].pPrev = pRngPointBuf[i].pNext = NULL;
	}

	int xLeft = pPntBaryCenter->x-WIN_WIDTH;
	int xRight = pPntBaryCenter->x+WIN_WIDTH;
	int yTop = pPntBaryCenter->y-WIN_WIDTH;
	int yBottom = pPntBaryCenter->y+WIN_WIDTH;

	if(xLeft<lineLeft) xLeft=lineLeft;
	if(xRight>lineRight) xRight=lineRight;
	if(yTop<lineTop) yTop=lineTop;
	if(yBottom>lineBottom) yBottom=lineBottom;
/*
	//step3: 将窗口内大于阈值的点作为种子点, 当图像中有黑色矩形框线时，本方法容易将矩形边框当作图像边界
	for (int i=yTop; i<yBottom; i++)
	{
		pImgData = (uchar*)(pImgGray->imageData)+i*pImgGray->widthStep;
		for (int j=xLeft; j<xRight; j++)
		{
			if(pImgData[j] > thVal)
			{
				pRngPointBuf[counter].pixelPos = i*pImgGray->widthStep+j;
				if(pRngQueue == NULL)
				{
					pRngQueue = pRngPointBuf+counter;
				}
				else
				{
					pRngQueue->pNext = pRngPointBuf+counter;
					pRngQueue->pNext->pPrev = pRngQueue;
					pRngQueue = pRngQueue->pNext;
				}
				pImgMask[pRngPointBuf[counter].pixelPos] = 0;
				counter++;
			}
		}
	}
*/
	//step3: 将穿过重心点的X向及Y向点作为种子点， 此种方法当整个图像边缘存在“黑白黑”交错状况时，容易误判
	pImgData = (uchar*)(pImgGray->imageData)+pPntBaryCenter->y*pImgGray->widthStep;
	for (int i=lineLeft+1; i<lineRight-1; i++)
	{
		if(pImgData[i] > thVal)
		{
			//检查上下左右邻域是否符合条件
			if(pImgData[i-1]>thVal && pImgData[i+1]>thVal && 
				(pPntBaryCenter->y>0 && *(pImgData+i-pImgGray->widthStep)>thVal) &&
				(pPntBaryCenter->y+1<pImgGray->height && *(pImgData+i+pImgGray->widthStep)>thVal))
			{
				pRngPointBuf[counter].pixelPos = pPntBaryCenter->y*pImgGray->widthStep+i;
				if(pRngQueue == NULL)
				{
					pRngQueue = pRngPointBuf+counter;
				}
				else
				{
					pRngQueue->pNext = pRngPointBuf+counter;
					pRngQueue->pNext->pPrev = pRngQueue;
					pRngQueue = pRngQueue->pNext;
				}
				pImgMask[pRngPointBuf[counter].pixelPos] = 0;
				counter++;
			}
		}
	}

	//将重心点的Y向直线作为种子点, 自重心点向上
	pImgData = (uchar *)pImgGray->imageData+(lineTop+1)*pImgGray->widthStep+pPntBaryCenter->x;
	for (int i=lineTop+1; i<lineBottom-1; i++)
	{
		if(*pImgData > thVal)
		{
			//检查上下左右邻域是否符合条件
			if(*(pImgData-pImgGray->widthStep)>thVal && *(pImgData+pImgGray->widthStep)>thVal &&
				(pPntBaryCenter->x>0 && *(pImgData-1)>thVal) &&
				(pPntBaryCenter->x+1<pImgGray->width && *(pImgData+1)>thVal))
			{
				pRngPointBuf[counter].pixelPos = i*pImgGray->widthStep+pPntBaryCenter->x;
				if(pRngQueue == NULL)
				{
					pRngQueue = pRngPointBuf+counter;
				}
				else
				{
					pRngQueue->pNext = pRngPointBuf+counter;
					pRngQueue->pNext->pPrev = pRngQueue;
					pRngQueue = pRngQueue->pNext;
				}
				pImgMask[pRngPointBuf[counter].pixelPos] = 0;
				counter++;
			}
		}
		pImgData += pImgGray->widthStep;
	}
	
	if(counter == 0)			//无种子点
	{
		delete pRngPointBuf;
		return RTN_NO_SEED;
	}

	//step5: 按照先进后出操作进行标注
	pImgData = (uchar*)pImgGray->imageData;
	while(pRngQueue)
	{
		pos = pRngQueue->pixelPos;
		pRngQueue = pRngQueue->pPrev;

		//查看上下左右邻近是否符合条件
		tempVal = pos-1;
		//if((pos%pImgGray->widthStep)>0 && pImgMask[tempVal] && pImgData[tempVal]>thVal)		//left
		if((pos%pImgGray->widthStep)>lineLeft && pImgMask[tempVal] && pImgData[tempVal]>thVal)		//left
		{
			pRngPointBuf[counter].pixelPos = tempVal;
			if(pRngQueue == NULL)
			{
				pRngQueue = pRngPointBuf+counter;
			}
			else
			{
				pRngQueue->pNext = pRngPointBuf+counter;
				pRngQueue->pNext->pPrev = pRngQueue;
				pRngQueue = pRngQueue->pNext;
			}
			pImgMask[tempVal] = 0;
			counter++;
		}

		tempVal = pos+1;
		//if((pos%pImgGray->widthStep)+1<pImgGray->width && pImgMask[tempVal] && pImgData[tempVal]>thVal)		//right
		if((pos%pImgGray->widthStep)+1<lineRight && pImgMask[tempVal] && pImgData[tempVal]>thVal)		//right
		{
			pRngPointBuf[counter].pixelPos = tempVal;
			if(pRngQueue == NULL)
			{
				pRngQueue = pRngPointBuf+counter;
			}
			else
			{
				pRngQueue->pNext = pRngPointBuf+counter;
				pRngQueue->pNext->pPrev = pRngQueue;
				pRngQueue = pRngQueue->pNext;
			}
			pImgMask[tempVal] = 0;
			counter++;
		}

		tempVal = pos-pImgGray->widthStep;
		//if(tempVal>0 && pImgMask[tempVal] && pImgData[tempVal]>thVal)		//top
		if(tempVal>lineTop*pImgGray->widthStep && pImgMask[tempVal] && pImgData[tempVal]>thVal)		//top
		{
			pRngPointBuf[counter].pixelPos = tempVal;
			if(pRngQueue == NULL)
			{
				pRngQueue = pRngPointBuf+counter;
			}
			else
			{
				pRngQueue->pNext = pRngPointBuf+counter;
				pRngQueue->pNext->pPrev = pRngQueue;
				pRngQueue = pRngQueue->pNext;
			}
			pImgMask[tempVal] = 0;
			counter++;
		}

		tempVal = pos+pImgGray->widthStep;
		//if(tempVal<pImgGray->imageSize && pImgMask[tempVal] && pImgData[tempVal]>thVal)		//bottom
		if(tempVal<lineBottom*pImgGray->widthStep && pImgMask[tempVal] && pImgData[tempVal]>thVal)		//bottom
		{
			pRngPointBuf[counter].pixelPos = tempVal;
			if(pRngQueue == NULL)
			{
				pRngQueue = pRngPointBuf+counter;
			}
			else
			{
				pRngQueue->pNext = pRngPointBuf+counter;
				pRngQueue->pNext->pPrev = pRngQueue;
				pRngQueue = pRngQueue->pNext;
			}
			pImgMask[tempVal] = 0;
			counter++;
		}
	}

	delete pRngPointBuf;
	return RTN_SUCCESS;
}

//检测有效图像的边缘，计算角点及倾角
//pImgGray: IN, 灰度图
//thVal: IN, 阈值
//lineLeft: IN, 左边界
//lineTop: IN, 右边界
//lineRight: IN, 上边界
//lineBottom: IN, 下边界
//pImgMask: IN, 标志图
//docNum: OUT, 成功检测到的有效图像个数
//pPntsVertex: OUT, 各有效图像的四角顶点坐标
//pAngles: OUT, 各有效图像的倾角
int EdgeDetect(IplImage *pImgGray, int thVal, int lineLeft, int lineTop, int lineRight, int lineBottom, uchar *pImgMask, int maxDocNum, int *pDocNum, IM_POINT *pPntsVertex, float *pAngles)
{
	//int const MAX_DOC_NUM=8;			//能够检测出的最大有效图像数量
	int const CONN_POINTS_MIN_COUNT=48*48;		//连通区最少点数
	int const RECT_POINT_NUM=4;			//矩形顶点个数
	int ConnPixelCount=0, pos=0, tempVal=0, pixelCount=pImgGray->width*pImgGray->height;
	uchar *pGrayDataLoop=NULL, *pGrayData=NULL, *pMaskDataLoop=NULL;
	structRngPoint *pRngQueue=NULL;
	structRngPoint *pRngPointBuf=NULL;

	//step1: 开辟内存
	pRngPointBuf = new structRngPoint[pixelCount];
	if(pRngPointBuf==NULL) return RTN_MALLOC_ERR;

	//step2: 初始化
	*pDocNum = 0;
	memset(pImgMask,0,pixelCount);
	//ZeroMemory(pImgMask, pixelCount);
	pGrayData = (uchar*)pImgGray->imageData;
	for (int i=0; i<pixelCount; i++)
	{
		pRngPointBuf[i].pPrev = pRngPointBuf[i].pNext = NULL;
	}

	//step3: 在边界内遍历，寻找大于阈值的点，并以此点作为种子点，标注连通区域
	for (int i=lineTop; i<lineBottom; i++)
	{
		pMaskDataLoop = pImgMask+i*pImgGray->widthStep;
		pGrayDataLoop = (uchar*)(pImgGray->imageData+i*pImgGray->widthStep);
		for (int j=lineLeft; j<lineRight; j++)
		{
			if(pGrayDataLoop[j]>thVal && pMaskDataLoop[j]==0)
			{
				ConnPixelCount = 0;
				pRngQueue = pRngPointBuf;
				pRngPointBuf[ConnPixelCount].pixelPos = i*pImgGray->widthStep+j;
				pImgMask[pRngPointBuf[ConnPixelCount].pixelPos] = 1;
				ConnPixelCount++;

				//step4: 按照先进后出操作进行标注
				while(pRngQueue)
				{
					pos = pRngQueue->pixelPos;
					pRngQueue = pRngQueue->pPrev;

					//查看上下左右邻近是否符合条件
					tempVal = pos-1;
					if((pos%pImgGray->widthStep)>lineLeft+1 && pImgMask[tempVal]==0 && pGrayData[tempVal]>thVal)		//left
					{
						pRngPointBuf[ConnPixelCount].pixelPos = tempVal;
						if(pRngQueue == NULL)
						{
							pRngQueue = pRngPointBuf+ConnPixelCount;
						}
						else
						{
							pRngQueue->pNext = pRngPointBuf+ConnPixelCount;
							pRngQueue->pNext->pPrev = pRngQueue;
							pRngQueue = pRngQueue->pNext;
						}
						pImgMask[tempVal] = 1;
						ConnPixelCount++;
					}

					tempVal = pos+1;
					if((pos%pImgGray->widthStep)+1<lineRight && pImgMask[tempVal]==0 && pGrayData[tempVal]>thVal)		//right
					{
						pRngPointBuf[ConnPixelCount].pixelPos = tempVal;
						if(pRngQueue == NULL)
						{
							pRngQueue = pRngPointBuf+ConnPixelCount;
						}
						else
						{
							pRngQueue->pNext = pRngPointBuf+ConnPixelCount;
							pRngQueue->pNext->pPrev = pRngQueue;
							pRngQueue = pRngQueue->pNext;
						}
						pImgMask[tempVal] = 1;
						ConnPixelCount++;
					}

					tempVal = pos-pImgGray->widthStep;
					if(pos>(lineTop+1)*pImgGray->widthStep && pImgMask[tempVal]==0 && pGrayData[tempVal]>thVal)		//top
					{
						pRngPointBuf[ConnPixelCount].pixelPos = tempVal;
						if(pRngQueue == NULL)
						{
							pRngQueue = pRngPointBuf+ConnPixelCount;
						}
						else
						{
							pRngQueue->pNext = pRngPointBuf+ConnPixelCount;
							pRngQueue->pNext->pPrev = pRngQueue;
							pRngQueue = pRngQueue->pNext;
						}
						pImgMask[tempVal] = 1;
						ConnPixelCount++;
					}

					tempVal = pos+pImgGray->widthStep;
					if(tempVal<lineBottom*pImgGray->widthStep && pImgMask[tempVal]==0 && pGrayData[tempVal]>thVal)		//bottom
					{
						pRngPointBuf[ConnPixelCount].pixelPos = tempVal;
						if(pRngQueue == NULL)
						{
							pRngQueue = pRngPointBuf+ConnPixelCount;
						}
						else
						{
							pRngQueue->pNext = pRngPointBuf+ConnPixelCount;
							pRngQueue->pNext->pPrev = pRngQueue;
							pRngQueue = pRngQueue->pNext;
						}
						pImgMask[tempVal] = 1;
						ConnPixelCount++;
					}
				}	//end "while(pRngQueue)"

				//检查是否符合有效图像的条件
				if(ConnPixelCount > CONN_POINTS_MIN_COUNT)
				{
					//在Mask中标注连通号，以便于寻找顶点
					for (int k=0; k<ConnPixelCount; k++)
					{
						pImgMask[pRngPointBuf[k].pixelPos] = (2+(*pDocNum));		//+2用于区分前面的0和1
					}
					FindVertexPoints(pImgGray, pImgMask, 2+(*pDocNum), pPntsVertex+(*pDocNum)*RECT_POINT_NUM, pAngles+(*pDocNum));
					(*pDocNum)++;
				}

				//将标识连通区变量复位
				for (int k=0; k<ConnPixelCount; k++)
				{
					pRngPointBuf[k].pPrev = pRngPointBuf[k].pNext = NULL;
				}
			}	//end "if(pGrayDataLoop[j]>thVal && pMaskDataLoop[j]==0)"
			if(*pDocNum >= maxDocNum) break;
		}
		if(*pDocNum >= maxDocNum) break;
	}

	delete pRngPointBuf;
	return RTN_SUCCESS;
}
//根据边缘点寻找顶点、计算倾角
//pImgGray: IN, 灰度图像
//pImgMask: IN, 标识图
//connNo: IN, 连通序号, 程序依此寻找边缘
//pPntsVertex: OUT, 四角顶点
//pAngle: OUT, 倾角
int FindVertexPoints(IplImage *pImgGray, uchar *pImgMask, uchar connNo, IM_POINT *pPntsVertex, float *pAngle)
{
	int reCode = RTN_SUCCESS;
	int distTemp=0, distMax=0, validCount=0, edgePntCount=0;
	uchar *pMaskLoop=NULL;
	IM_POINT *pPntsEdge=NULL, tempPnt;

	//step1: 开辟内存
	pPntsEdge = new IM_POINT[pImgGray->width*2];
	if(pPntsEdge == NULL) return RTN_MALLOC_ERR;

	//step2: 从左向右扫描, 找相交点
	for (int i=0; i<pImgGray->width; i++)
	{
		tempPnt.x = tempPnt.y = 0xffff;

		//从上向下寻找首个标志为connNo的点
		pMaskLoop = pImgMask+i;
		for(int j=0; j<pImgGray->height; j++)
		{
			if(*pMaskLoop==connNo && *(pMaskLoop+pImgGray->width)==connNo && 
				*(pMaskLoop+pImgGray->width+pImgGray->width)==connNo)
			{
				tempPnt.x=i; tempPnt.y=j;
				pPntsEdge[edgePntCount].x = i;
				pPntsEdge[edgePntCount].y = j;
				edgePntCount++;
				break;
			}
			pMaskLoop += pImgGray->widthStep;
		}

		if(tempPnt.x != 0xffff)
		{
			//从下向上寻找首个标志为0的点
			pMaskLoop = pImgMask+(pImgGray->height-1)*pImgGray->widthStep+i;
			for(int j=pImgGray->height-1; j>=0; j--)
			{
				if(*pMaskLoop==connNo && *(pMaskLoop-pImgGray->width)==connNo && 
					*(pMaskLoop-pImgGray->width-pImgGray->width)==connNo)
				{
					pPntsEdge[edgePntCount].x = i;
					pPntsEdge[edgePntCount].y = j;
					edgePntCount++;
					break;
				}
				pMaskLoop -= pImgGray->widthStep;
			}
		}
	}

	//step3: 寻找顶点
	//查找距离最大的两点, 此两点必为顶点
	distMax = 0;
	for (int i=0; i<edgePntCount; i++)
	{
		for (int j=i+1; j<edgePntCount; j++)
		{
			distTemp = (pPntsEdge[i].x-pPntsEdge[j].x)*(pPntsEdge[i].x-pPntsEdge[j].x)+
				(pPntsEdge[i].y-pPntsEdge[j].y)*(pPntsEdge[i].y-pPntsEdge[j].y);
			if(distTemp > distMax)
			{
				distMax = distTemp;
				pPntsVertex[0] = pPntsEdge[i];
				pPntsVertex[1] = pPntsEdge[j];
			}
		}
	}

	//根据点到线的距离最大者确定第三个顶点
	double distMaxPntToLine=0.0, tempVal=0.0, a=0.0, b=0.0, c=0.0, s=0.0;
	a = sqrtl((pPntsVertex[0].x-pPntsVertex[1].x)*(pPntsVertex[0].x-pPntsVertex[1].x)+(pPntsVertex[0].y-pPntsVertex[1].y)*(pPntsVertex[0].y-pPntsVertex[1].y));
	for (int i=0; i<edgePntCount; i++)
	{
		b = sqrtl((pPntsEdge[i].x-pPntsVertex[0].x)*(pPntsEdge[i].x-pPntsVertex[0].x)+(pPntsEdge[i].y-pPntsVertex[0].y)*(pPntsEdge[i].y-pPntsVertex[0].y));
		c = sqrtl((pPntsEdge[i].x-pPntsVertex[1].x)*(pPntsEdge[i].x-pPntsVertex[1].x)+(pPntsEdge[i].y-pPntsVertex[1].y)*(pPntsEdge[i].y-pPntsVertex[1].y));
		s = (((int)sqrtl((a+b+c)*(a+b-c)*(a+c-b)*(b+c-a)))>>2);
		tempVal = s/a;
		if(distMaxPntToLine < tempVal)
		{
			pPntsVertex[2] = pPntsEdge[i];
			distMaxPntToLine = tempVal;
		}
	}

	//查找到第三顶点最大距离的点，以确定第四顶点
	distMax = 0;
	for (int i=0; i<edgePntCount; i++)
	{
		distTemp = (pPntsEdge[i].x-pPntsVertex[2].x)*(pPntsEdge[i].x-pPntsVertex[2].x)+
			(pPntsEdge[i].y-pPntsVertex[2].y)*(pPntsEdge[i].y-pPntsVertex[2].y);
		if(distMax < distTemp)
		{
			pPntsVertex[3] = pPntsEdge[i];
			distMax = distTemp;
		}
	}

	//step4: 计算倾角
	//根据三个顶点确定倾斜角度, 计算过程是: 对角线上的两个顶点已经得到, 分别计算第三顶点到此两点的距离, 以距离较大者的两点确定角度
	//pPntsVertex中的0,1元素形成一条对角线, 2,3元素形成另一条对角线
	validCount = 0;			//有效角度个数, 用于计算角度均值
	*pAngle = 0;
	distTemp = (pPntsVertex[2].x-pPntsVertex[0].x)*(pPntsVertex[2].x-pPntsVertex[0].x)+
		(pPntsVertex[2].y-pPntsVertex[0].y)*(pPntsVertex[2].y-pPntsVertex[0].y);
	if(distTemp > (pPntsVertex[2].x-pPntsVertex[1].x)*(pPntsVertex[2].x-pPntsVertex[1].x)+
		(pPntsVertex[2].y-pPntsVertex[1].y)*(pPntsVertex[2].y-pPntsVertex[1].y))
	{
		if(pPntsVertex[2].x != pPntsVertex[0].x)
		{
			*pAngle += atan((pPntsVertex[2].y-pPntsVertex[0].y+0.0f)/(pPntsVertex[2].x-pPntsVertex[0].x));
			validCount++;
		}
	}
	else
	{
		if(pPntsVertex[2].x != pPntsVertex[1].x)
		{
			*pAngle += atan((pPntsVertex[2].y-pPntsVertex[1].y+0.0f)/(pPntsVertex[2].x-pPntsVertex[1].x));
			validCount++;
		}
	}

	//另一条, 元素2,3形成对角线
	distTemp = (pPntsVertex[0].x-pPntsVertex[2].x)*(pPntsVertex[0].x-pPntsVertex[2].x)+
		(pPntsVertex[0].y-pPntsVertex[2].y)*(pPntsVertex[0].y-pPntsVertex[2].y);
	if(distTemp > (pPntsVertex[0].x-pPntsVertex[3].x)*(pPntsVertex[0].x-pPntsVertex[3].x)+
		(pPntsVertex[0].y-pPntsVertex[3].y)*(pPntsVertex[0].y-pPntsVertex[3].y))
	{
		if(pPntsVertex[0].x != pPntsVertex[2].x)
		{
			*pAngle += atan((pPntsVertex[0].y-pPntsVertex[2].y+0.0f)/(pPntsVertex[0].x-pPntsVertex[2].x));
			validCount++;
		}
	}
	else
	{
		if(pPntsVertex[0].x != pPntsVertex[3].x)
		{
			*pAngle += atan((pPntsVertex[0].y-pPntsVertex[3].y+0.0f)/(pPntsVertex[0].x-pPntsVertex[3].x));
			validCount++;
		}
	}
	if(validCount>0) (*pAngle)/=validCount;

	//调整旋转方向
	if(abs(*pAngle)>3.1415926/4)		//45度比较
	{
		*pAngle = ((*pAngle)>0 ? -(3.1415926/2-*pAngle) : (3.1415926/2+*pAngle));
	}

	//微调四个角点的坐标, 纠裁更准确
	AdjustVertexPoints(pPntsEdge, edgePntCount, pPntsVertex, pAngle, pImgGray->width, pImgGray->height);

	delete pPntsEdge;
	return reCode;

	////查找y坐标最小的两点
	//int xStart=0, xEnd=0, step=2;
	//int iTarget1=0, iTarget2=0;

	////查找y坐标最小的角点
	//tempPnt = pPntsVertex[0];
	//for (int i=0; i<4; i++)
	//{
	//	if(tempPnt.y > pPntsVertex[i].y) {
	//		iTarget1 = i;
	//		tempPnt = pPntsVertex[i];
	//	}
	//}

	////与iTarget1 Y坐标最近点
	//distTemp = 0xffff;
	//for (int i=0; i<4; i++)
	//{
	//	if(i!=iTarget1 && distTemp>abs(pPntsVertex[i].y-pPntsVertex[iTarget1].y)) {
	//		iTarget2 = i;
	//		distTemp = abs(pPntsVertex[i].y-pPntsVertex[iTarget1].y);
	//	}
	//}

	//if(pPntsVertex[iTarget1].x > pPntsVertex[iTarget2].x) {
	//	xStart = pPntsVertex[iTarget2].x;
	//	xEnd = pPntsVertex[iTarget1].x;
	//}
	//else {
	//	xStart = pPntsVertex[iTarget1].x;
	//	xEnd = pPntsVertex[iTarget2].x;
	//}
	//
	//xStart+=step; xEnd-=step;

	//float line[4];
	//CvMemStorage* storage = cvCreateMemStorage(0);
	//CvSeq* point_seq = cvCreateSeq(CV_32FC2, sizeof(CvSeq), sizeof(CvPoint2D32f), storage);

	////从左向右扫描, 从下向上寻找首个标志为0的点, 向Sequence中追加元素
	//for (int i=xStart; i<xEnd; i++)
	//{
	//	pMaskLoop = pImgMask+(pImgGray->height-1)*pImgGray->widthStep+i;
	//	for(int j=pImgGray->height-1; j>=0; j--)
	//	{
	//		if(*pMaskLoop==connNo && *(pMaskLoop-pImgGray->width)==connNo && 
	//			*(pMaskLoop-pImgGray->width-pImgGray->width)==connNo)
	//		{
	//			cvSeqPush(point_seq, &cvPoint2D32f(i, j));
	//			break;
	//		}
	//		pMaskLoop -= pImgGray->widthStep;
	//	}
	//}
	//if(point_seq->total > 0) {
	//	cvFitLine(point_seq, CV_DIST_L2, 0, 0.01, 0.01, line);
	//	*pAngle = atan(line[1]/line[0]);

	//	//调整旋转方向
	//	if(abs(*pAngle)>3.1415926/4)		//45度比较
	//	{
	//		*pAngle = ((*pAngle)>0 ? -(3.1415926/2-*pAngle) : (3.1415926/2+*pAngle));
	//	}

	//	//微调四个角点的坐标, 纠裁更准确
	//	AdjustVertexPoints(pPntsEdge, edgePntCount, pPntsVertex, pAngle, pImgGray->width, pImgGray->height);

	//	reCode = RTN_SUCCESS;
	//}
	//else {
	//	reCode = RTN_AUTO_ROTATE_FAILED;
	//}
	//cvClearSeq(point_seq);
	//cvReleaseMemStorage(&storage);
	
	//delete pPntsEdge;
	//return reCode;
}
//pPntsEdge: in, FindVertexPoints函数中得到的边缘坐标点
//edgePntCount: in, 边缘坐标点数量
//pPntsVertex: in, 角点坐标
//pAngle: in, 倾角大小
//imgWidth: in, 原图宽度
//imgHeight: in, 原图高度
void AdjustVertexPoints(IM_POINT *pPntsEdge, int edgePntCount, IM_POINT *pPntsVertex, float *pAngle, int imgWidth, int imgHeight)
{
	float const PI=3.1415926f;
	float M1=0, M2=0, fSin=0, fCos=0;
	IM_POINT tempPnt;
	int const POINT_NUM=4;
	int widthRotate=0, heightRotate=0, left=0, right=0, top=0, bottom=0;

	//将所有边缘点按照角度进行旋转，并查找边缘
	right = bottom = 0;
	left = top = 0xffff;

	widthRotate = fabs(imgHeight*sin(*pAngle))+fabs(imgWidth*cos(*pAngle));
	heightRotate = fabs(imgHeight*cos(*pAngle))+fabs(imgWidth*sin(*pAngle));

	fSin = sin(2*PI-*pAngle);		//转化为按照顺时针旋转
	fCos = cos(2*PI-*pAngle);
	M1 = -0.5*imgWidth*fCos+0.5*imgHeight*fSin+0.5*widthRotate;
	M2 = -0.5*imgWidth*fSin-0.5*imgHeight*fCos+0.5*heightRotate;
	for (int j=0; j<edgePntCount; j++)
	{
		//计算旋转后坐标
		tempPnt.x = pPntsEdge[j].x*fCos-pPntsEdge[j].y*fSin+M1;
		tempPnt.y = pPntsEdge[j].x*fSin+pPntsEdge[j].y*fCos+M2;
		if(tempPnt.x<left) left=tempPnt.x;
		if(tempPnt.y<top) top=tempPnt.y;
		if(tempPnt.x>right) right=tempPnt.x;
		if(tempPnt.y>bottom) bottom=tempPnt.y;
	}

	pPntsVertex[0].x=left; pPntsVertex[0].y=top;
	pPntsVertex[1].x=right; pPntsVertex[1].y=top;
	pPntsVertex[2].x=right; pPntsVertex[2].y=bottom;
	pPntsVertex[3].x=left; pPntsVertex[3].y=bottom;

	//将坐标旋转回去
	int tempWidth=widthRotate, tempHeight=heightRotate;
	widthRotate = fabs(tempHeight*sin(*pAngle))+fabs(tempWidth*cos(*pAngle));
	heightRotate = fabs(tempHeight*cos(*pAngle))+fabs(tempWidth*sin(*pAngle));

	fSin = sin(*pAngle);		//转化为按照顺时针旋转
	fCos = cos(*pAngle);
	M1 = -0.5*tempWidth*fCos+0.5*tempHeight*fSin+0.5*widthRotate;
	M2 = -0.5*tempWidth*fSin-0.5*tempHeight*fCos+0.5*heightRotate;
	for (int j=0; j<POINT_NUM; j++)
	{
		//计算旋转后坐标
		tempPnt.x = pPntsVertex[j].x*fCos-pPntsVertex[j].y*fSin+M1;
		tempPnt.y = pPntsVertex[j].x*fSin+pPntsVertex[j].y*fCos+M2;
		pPntsVertex[j] = tempPnt;
	}

	for (int j=0; j<POINT_NUM; j++)
	{
		//offset
		pPntsVertex[j].x += ((imgWidth-widthRotate)>>1)+2;		//补偿2个像素
		pPntsVertex[j].y += ((imgHeight-heightRotate)>>1)+2;
	}
}

////求两直线交点
//IM_POINT GetCrossPnt(IM_POINT *pLine1, IM_POINT *pLine2)
//{
//	double k1=0, k2=0;
//	IM_POINT pntCross;
//
//	//三种情况, 1.直线1为垂直线, 2.直线2为垂直线, 3.直线1和直线2均为非垂直线
//	if(pLine1[0].x == pLine1[1].x) {
//		k2 = (pLine2[0].y-pLine2[1].y)/(pLine2[0].x-pLine2[1].x);
//		pntCross.x = pLine1[0].x;
//		pntCross.y = pLine2[0].y-k2*(pLine2[0].x-pntCross.x);
//
//	}
//	else if(pLine2[0].x == pLine2[1].x) {
//		k1 = (pLine1[0].y-pLine1[1].y)/(pLine1[0].x-pLine1[1].x);
//		pntCross.x = pLine2[0].x;
//		pntCross.y = pLine1[0].y-k1*(pLine1[0].x-pntCross.x);
//	}
//	else {
//		k1 = (pLine1[0].y-pLine1[1].y)/(pLine1[0].x-pLine1[1].x);
//		k2 = (pLine2[0].y-pLine2[1].y)/(pLine2[0].x-pLine2[1].x);
//
//		//x=(k1*x0-k2*x2+y2-y0)/(k1-k2);
//		//y=y0+(x-x0)*k1;
//		pntCross.x = (k1*pLine1[0].x-k2*pLine2[0].x+pLine2[0].y-pLine1[0].y)/(k1-k2);
//		pntCross.y = pLine1[0].y+(pntCross.x-pLine1[0].x)*k1;
//	}
//	return pntCross;
//}


/*	调用第三个函数illumination_balance（）
	参数1：传入图像imgSrc
	参数2：输出图像imgDst
	请传入彩色rgb图像或是灰度图像.
	用法是int a=ill_balance_callback(imgSrc,imgDst,0)
*/

int illumination_balance(const IplImage *imgSrc,IplImage *imgDst,double brightness)
{
	//IplImage *imgSrcI=cvLoadImage("E:\\lean.jpg",-1);
	Mat imgSrcM(imgSrc,true);
	Mat imgDstM;

	Mat imgGray;
	Mat imgHls;
	vector<Mat> vHls;

	Mat imgTemp1=Mat::zeros(imgSrcM.size(),CV_64FC1);
	Mat imgTemp2=Mat::zeros(imgSrcM.size(),CV_64FC1);

	if(imgSrcM.channels()==1)
	{
		imgGray=imgSrcM.clone();
	}
	else if (imgSrcM.channels()==3)
	{
		cvtColor(imgSrcM, imgHls, CV_BGR2HLS);
		split(imgHls, vHls);
		imgGray=vHls.at(1);
	}
	else
	{
		return -1;
	}

	//imwrite("E:\\leanImgGray1.jpg",imgGray);
	imgGray.convertTo(imgTemp1,CV_64FC1);
	imgTemp1=imgTemp1+0.0001;
	log(imgTemp1,imgTemp1);

	GaussianBlur(imgTemp1, imgTemp2, Size(21, 21),-1,-1, BORDER_DEFAULT);//imgTemp2是低通滤波的结果
	imgTemp1 = (imgTemp1 - imgTemp2);//imgTemp1是对数减低通的高通
	addWeighted(imgTemp2, 0.47, imgTemp1, 0.55, 0, imgTemp1, -1);//imgTemp1是压制低频增强高频的结构

	exp(imgTemp1,imgTemp1);
	normalize(imgTemp1,imgTemp1,0,1,NORM_MINMAX);
	imgTemp1=imgTemp1*255;

	imgTemp1.convertTo(imgGray, CV_8UC1,1.8,-95);

	//imwrite("E:\\leanImgGray2.jpg",imgGray);
	if (imgSrcM.channels()==3)
	{
		vHls.at(1)=imgGray;
		merge(vHls,imgHls);
		cvtColor(imgHls, imgDstM, CV_HLS2BGR);
		//imwrite("E:\\leanImgGray3.jpg",imgDstM);
	}
	else if (imgSrcM.channels()==1)
	{
		imgDstM=imgGray.clone();
	}
	IplImage temp = (IplImage)imgDstM;
	//cvCopy(&(IplImage)imgDstM,imgDst);
	cvCopy(&temp,imgDst);
	//cvShowImage("jpg",imgDst);

	return 0;

}


/***********************************想找边框***************************************/

//投影函数xy平面
int hist_xy(const Mat imgsrc,int *ax,int *ay)
{
	if( !imgsrc.data)
	{
		return -1;
	}
	int xtemp;
	int ytemp;
	for (int i=0;i<imgsrc.rows;i++)
	{
		ax[i]=0;
		xtemp=0;
		for (int j=0;j<imgsrc.cols;j++)
		{
			if (imgsrc.at<uchar>(i,j)==255)
			{
				xtemp++;
			}
		}
		ax[i]=xtemp;
	}

	for (int j=0;j<imgsrc.cols;j++)
	{
		ay[j]=0;
		ytemp=0;
		for (int i=0;i<imgsrc.rows;i++)
		{
			if (imgsrc.at<uchar>(i,j)==255)
			{
				ytemp++;
			}
		}
		ay[j]=ytemp;
	}
}
//
////取左上角和右下角1/5的区域进入函数
////flags==1,传入左上角的
////flags==2,传入右下角的。
//int find_point(const Mat imgsrc,Point &pt,int flags)//flags必须为1或2，不能传入其他选项.
//{
//	if( !(flags==1 || flags==2) || !imgsrc.data)
//	{
//		return -1;
//	}
//	vector<Mat> vRGB;
//	split(imgsrc,vRGB);
//	Mat imgRed=Mat::zeros(imgsrc.size(),CV_8UC1);
//	imgRed=vRGB.at(2)-vRGB.at(1)-vRGB.at(0);
//	Mat imgbw=Mat::zeros(imgsrc.size(),CV_8UC1);
//	cv::threshold(imgRed,imgbw,CV_THRESH_OTSU,255,THRESH_BINARY);
//
//	/***************20161014*******************************************/
//	//解决光照不均时不可用
//	Mat imghsv;
//	cvtColor(imgsrc,imghsv,CV_BGR2HSV);
//	vector<Mat> vHSV;
//	split(imghsv,vHSV);
//	Mat imgh=vHSV.at(0);
//	Mat imgs=vHSV.at(1);
//	Mat imgv=vHSV.at(2);
//	for(int i=0;i<imghsv.rows;i++)
//	{
//		for(int j=0;j<imghsv.cols;j++)
//		{
//			if( imgh.at<uchar>(i,j)<40 && imgs.at<uchar>(i,j)>70 && imgv.at<uchar>(i,j)>60)
//			{
//				imgbw.at<uchar>(i,j)=255;
//			}
//			else
//			{
//				imgbw.at<uchar>(i,j)=0;
//			}
//		}
//	}
//	/***************20161014*******************************************/
//
//	int *xh=new int[imgbw.rows];
//	int *yh=new int[imgbw.cols];
//	hist_xy(imgbw,xh,yh);
//
//	if (flags==1)
//	{
//		for (int i=imgsrc.rows-3;i>3;i--)
//		{
//			if (xh[i-2]>100 && xh[i-1]>100 && xh[i]>100 && xh[i+1]>100 && xh[i+2]>100)
//			{
//				pt.y=i;
//				break;
//			}
//		}
//		for (int i=imgsrc.cols-3;i>3;i--)
//		{
//			if (yh[i-2]>100 && yh[i-1]>100 && yh[i]>100 && yh[i+1]>100 && yh[i+2]>100)
//			{
//				pt.x=i;
//				break;
//			}
//		}
//	}
//	if(flags==2)
//	{
//		for (int i=3;i<imgsrc.rows-3;i++)
//		{
//			if (xh[i-2]>100 && xh[i-1]>100 && xh[i]>100 && xh[i+1]>100 && xh[i+2]>100)
//			{
//				pt.y=i;
//				break;
//			}
//		}
//		for (int i=3;i<imgsrc.cols;i++)
//		{
//			if (yh[i-2]>100 && yh[i-1]>100 && yh[i]>100 && yh[i+1]>100 && yh[i+2]>100)
//			{
//				pt.x=i;
//				break;
//			}
//		}
//	}
//	delete xh;
//	delete yh;
//	return 0;
//}
//
////传入图像数据，返回上下两个点的坐标
//int find_rect(const Mat imgsrc,Point &ptstart,Point &ptend)
//{
//	if( !imgsrc.data)
//	{
//		return -1;
//	}
//	Mat imgrect1,imgrect2;
//	int rectx=ceil((double)imgsrc.cols/5);
//	int recty=ceil((double)imgsrc.rows/5);
//	imgrect1=imgsrc(Rect(0,0,rectx,recty));
//	imgrect2=imgsrc(Rect(imgsrc.cols-rectx,imgsrc.rows-recty,rectx,recty));
//	//imshow("ul",imgrect1);waitKey();
//	//imshow("dr",imgrect2);waitKey();
//
//	find_point(imgrect1,ptstart,1);
//	find_point(imgrect2,ptend,2);
//
//	ptend.x=imgsrc.cols-rectx+ptend.x;
//	ptend.y=imgsrc.rows-recty+ptend.y;
//
//	if(ptstart.x==0 || ptstart.y==0 || ptend.x==0 || ptend.y==0)
//	{
//		return -1;
//	}
//
//	return 0;
//}


//获取标定位置
//imgSrc: in, 单通道图像(红色通道图像)
//dirType: in, 加和方向, 0:y方向(自上而下), 1:x方向(从左向右)
//differType: in, 查找类型, 0:最大值查找, 1:最小值查找 
int FindRedFlag(IplImage imgSrc, int dirType, int differType)
{
	int arraySize = (dirType==0?imgSrc.height:imgSrc.width);
	int *pSumArray = new int[arraySize];
	int valMax=-0xfffffff, valMin=0xfffffff, differ=0, pos=0;
	uchar *pImgData = NULL;

	if(pSumArray)
	{
		//step1: 计算每行或每列数据之和
		if(dirType == 0)	//自上而下
		{
			//计算每行之和
			for (int i=0; i<imgSrc.height; i++)
			{
				pSumArray[i] = 0;
				pImgData = (uchar *)imgSrc.imageData+i*imgSrc.widthStep;
				for (int j=0; j<imgSrc.width; j++)
				{
					pSumArray[i] += pImgData[j];
				}
			}
		}
		else
		{
			//计算每列之和
			for (int i=0; i<imgSrc.width; i++)
			{
				pSumArray[i] = 0;
				pImgData = (uchar *)imgSrc.imageData+i;
				for (int j=0; j<imgSrc.height; j++)
				{
					pSumArray[i] += *pImgData;
					pImgData += imgSrc.widthStep;
				}
			}
		}

		//step2: 根据前后差值计算位置
		for (int i=0; i<arraySize-1; i++)
		{
			differ = pSumArray[i+1]-pSumArray[i];		//前后差值
			if(differType == 0)		//最大值
			{
				if(differ > valMax)	{pos=i; valMax=differ;}
			}
			else
			{
				if(differ < valMin)	{pos=i; valMin=differ;}
			}
		}

		delete []pSumArray;
	}

	return pos;
}
int find_rect(const IplImage *imgSrc,int &lineLeft, int &lineTop, int &lineRight, int &lineBottom)
{
	//Mat imgsrc(imgSrc,true);
	//Point upleft,downright;//坐标申请
	//find_rect(imgsrc,upleft,downright);
	//lineLeft=upleft.x;
	//lineTop=upleft.y;
	//lineRight=downright.x;
	//lineBottom=downright.y;
	//lineBottom=downright.y;
	/*if (lineBottom==0 || lineRight==0 || lineTop==0 || lineLeft==0)
	{
	return -1;
	}
	*/

	//截取原始图像左上角及右下角1/5大小图像
	Mat imgsrc(imgSrc,true);
	Mat imgrect1, imgrect2, imgRed;
	int rectx = ceil((double)imgsrc.cols/5);
	int recty = ceil((double)imgsrc.rows/5);
	imgrect1 = imgsrc(cv::Rect(0, 0, rectx, recty));
	imgrect2 = imgsrc(cv::Rect(imgsrc.cols-rectx, imgsrc.rows-recty, rectx, recty));

	//imgrect1分离红色通道数据并寻找标定位置
	vector<cv::Mat> channels;
	split(imgrect1, channels);
	imgRed = channels.at(2);
	lineTop = FindRedFlag(imgRed, 0, 0);
	lineLeft = FindRedFlag(imgRed, 1, 0);

	//imgrect2分离红色通道数据并寻找标定位置
	split(imgrect2, channels);
	imgRed = channels.at(2);
	lineBottom = imgsrc.rows-imgRed.rows+FindRedFlag(imgRed, 0, 1);
	lineRight = imgsrc.cols-imgRed.cols+FindRedFlag(imgRed, 1, 1);

	if (lineBottom<=lineTop || lineRight<=lineLeft)
	{
		return -1;
	}
	return 0;
}



//1.文字分行
int GetRowRects(IplImage* source, CvRect rcROI, vector<CvRect> &rows)
{
	if(source == NULL)
		return -1;

	if(rcROI.x < 0 || rcROI.y < 0 || rcROI.width > source->width || rcROI.height > source->height)
		return -1;

	int *projection = new int[rcROI.height]; //用于存储投影值
	memset(projection, 0, sizeof(int)*rcROI.height);

    //遍历每一行计算投影值
	for(int y = rcROI.y; y < rcROI.y + rcROI.height; y++)
	{
		for(int x = rcROI.x; x < rcROI.x + rcROI.width; x++)
		{
			CvScalar s = cvGet2D(source, y, x);
			if(s.val[0] < 10)
                projection[y - rcROI.y]++;
        }
    }
    //开始根据投影值识别分割点
    bool inLine = false;
    int start = 0;
    for (int i = 0; i < rcROI.height; ++i)
    {
        if (!inLine && (projection[i] > 0))
        {
            //由空白进入字符区域了，记录标记
            inLine = true;
            start = i;
        }
        else if (inLine && (i - start > 10) && (projection[i] == 0))
        {
            //由字符区域进入空白区域了
            inLine = false;
 
            //忽略高度太小的行，比如分隔线
            if (i - start > 10)
            {
                //记录下位置
				CvRect rc = {rcROI.x, rcROI.y + start , rcROI.width, i - start + 1};
				rows.push_back(rc);

#if _DEBUG
				//cvRectangleR(source, rc, cvScalar(0,0,0));//打开画框会导致文字分割失败
				//cvNamedWindow("分行",1);
				//cvShowImage("分行",source); 
#endif
            }
        }
    }
    delete projection;

	return 0;
}

//2.按行进行文字分块
int GetBlockRects(IplImage* source,  CvRect rcRow, vector<CvRect> &blocks)
{
	if(source == NULL)
		return -1;

	int *projection = new int[rcRow.width];
	memset(projection, 0, sizeof(int)*rcRow.width);

	for(int x = rcRow.x; x < rcRow.x + rcRow.width; x++)
	{
		for(int y = rcRow.y; y < rcRow.y + rcRow.height; y++)
		{
			CvScalar s = cvGet2D(source, y, x);
			if(s.val[0] < 10)
               projection[x - rcRow.x]++;
		}
	}
    //开始根据投影值识别分割点
    bool inLine = false;
    int start = 0;
    for (int i = 0; i < rcRow.width; ++i)
    {
        if (!inLine && (projection[i] > 0))
        {
            //由空白进入字符区域了，记录标记
            inLine = true;
            start = i;
        }
        else if (inLine && (i - start > 5) && (projection[i] == 0))
        {
            //由字符区域进入空白区域了
            inLine = false;
 
            //忽略高度太小的行，比如分隔线
            if (i - start > 5)
            {
                //记录下位置
				CvRect rc = {rcRow.x + start, rcRow.y , i - start + 1, rcRow.height};
				blocks.push_back(rc);

#if _DEBUG
				cvRectangleR(source, rc, cvScalar(0,0,0));
				cvNamedWindow("分割",1);
				cvShowImage("分割",source); 
#endif
            }
        }
    }
	delete projection;
	return 0;
}


//3.获取每个文字的区域
int GetTextBlock(IplImage* source,  CvRect rcROI, vector<CvRect> &blocks)
{
	if(source == NULL)
		return -1;

	if(rcROI.x < 0 || rcROI.y < 0 || rcROI.width > source->width || rcROI.height > source->height)
		return -1;

	IplImage *imgGray = cvCreateImage(cvGetSize(source),source->depth,1);

	cvCvtColor(source, imgGray,CV_BGR2GRAY);//得到灰度图  

	cvSetImageROI(imgGray, rcROI);
	cvThreshold(imgGray, imgGray, 0, 255, CV_THRESH_OTSU);//二值化控制提取的像素
	IplConvKernel* kernel = cvCreateStructuringElementEx(1, 3, 0,2, CV_SHAPE_RECT);//竖直方向腐蚀 
	cvErode(imgGray,imgGray, kernel);
	cvResetImageROI(imgGray);

	vector<CvRect> rows;
	GetRowRects(imgGray, rcROI, rows);//找到行区域
	vector<CvRect>::iterator iter;
	for(iter = rows.begin(); iter != rows.end(); iter++)
	{
		CvRect rcRow = *iter;

		vector<CvRect> rcBlocks;
		GetBlockRects(imgGray,  rcRow, rcBlocks);//找到文字区域

		blocks.insert(blocks.end(), rcBlocks.begin(), rcBlocks.end());//// 把rcBlocks加到blocks末尾
	}

	cvReleaseImage(&imgGray);

	return 0;
}

//4.除去空行
int deleteBlockSpace(IplImage* source, CvRect rcROI)
{
	if(source == NULL)
		return -1;

	if(rcROI.x < 0 || rcROI.y < 0 || rcROI.width > source->width || rcROI.height > source->height)
		return -1;

	vector<CvRect> blocks;
	int ret = GetTextBlock(source,  rcROI, blocks);

	if(ret == -1)
		return -1;

	//计算背景平均亮度
	IplImage* imgBkg = cvCreateImage(cvSize(rcROI.width, rcROI.height),source->depth,1);
		
	cvSetImageROI(source, rcROI);
	cvCvtColor(source, imgBkg,CV_BGR2GRAY);//得到灰度图 
	cvThreshold(imgBkg, imgBkg, 0, 255, CV_THRESH_OTSU);//二值化
	CvScalar avg = cvAvg(source, imgBkg);
	cvResetImageROI(source); 

#if _DEBUG
	cvShowImage("otsu",imgBkg); 
#endif 
	cvReleaseImage(&imgBkg);

	IplImage* imgGray = cvCreateImage(cvGetSize(source),source->depth,1);
	cvCvtColor(source, imgGray,CV_BGR2GRAY);//得到灰度图  
	cvSetImageROI(imgGray, rcROI);
	cvAdaptiveThreshold( imgGray, imgGray, 255,THRESH_BINARY , CV_ADAPTIVE_THRESH_MEAN_C ,13,10);//二值化
	cvResetImageROI(imgGray);

	vector<CvRect>::iterator iter;
	for(iter = blocks.begin(); iter != blocks.end(); iter++)
	{
		Rect rc = *iter;
		
		int *pCol = new int[ rc.height];
		memset(pCol, 0, sizeof(int)*rc.height);

		for(int y = rc.y; y < rc.y + rc.height; y++)
		{
			for(int x = rc.x; x < rc.x + rc.width; x++)
			{
				CvScalar s = cvGet2D(imgGray, y, x);
				if(s.val[0] < 10)
					pCol[y - rc.y]++;
			}
		}

		//最上面和最下面填充背景
		int nRow = rc.y;
		for(int y = rc.y; y < rc.y + rc.height; y++)
		{
			if(pCol[y - rc.y] != 0)
			{
				for(int x = rc.x; x < rc.x + rc.width; x++)
				{
					CvScalar s = cvGet2D(source, y , x );
					cvSet2D(source, nRow, x, s);//cvScalar(0,255,0))
				}
				++nRow;
			}
		}
		
		//多余的行填充背景
		while(nRow <  rc.y + rc.height)
		{
			for(int x = rc.x; x < rc.x + rc.width; x++)
			{
				cvSet2D(source, nRow, x , avg);
			}
			++nRow;
		}

		delete pCol;
	}
	cvReleaseImage(&imgGray);

	cvSetImageROI(source, rcROI);
	IplConvKernel* kernel = cvCreateStructuringElementEx(1, 1, 0,0, CV_SHAPE_RECT);
	cvErode(source,source, kernel);//腐蚀
	cvResetImageROI(source);

	return 0;
}

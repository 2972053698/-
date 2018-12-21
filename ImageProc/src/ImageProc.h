
#pragma once
typedef unsigned long       DWORD;
typedef int                 BOOL;
typedef unsigned char       BYTE;
typedef unsigned short      WORD;
typedef float               FLOAT;
typedef signed char         INT8, *PINT8;
typedef signed short        INT16, *PINT16;
typedef signed int          INT32, *PINT32;
typedef signed long long      INT64, *PINT64;
typedef unsigned char       UINT8, *PUINT8;
typedef unsigned short      UINT16, *PUINT16;
typedef unsigned int        UINT32, *PUINT32;
typedef unsigned long long    UINT64, *PUINT64;
#include "opencv/cv.h"
#include "opencv/highgui.h"
#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <vector>
#define min(a,b) ((a)<(b))?(a):(b)
#define max(a,b) ((a)>(b))?(a):(b)

typedef struct IM_POINT
{
    long  x;
    long  y;
}IM_POINT;

typedef  std::vector<IM_POINT>  CPOINT_LIST;

typedef struct IM_RECT
{
    long    left;
    long    top;
    long    right;
    long    bottom;
}IM_RECT;

#define CROPTYPE_KEEP_CLEAN 0
#define CROPTYPE_KEEP_MORE  1

#define RTN_MALLOC_ERR          -1
#define RTN_SUCCESS              0
#define RTN_PARAM_ERR            1
#define RTN_AUTO_ROTATE_FAILED   2
#define RTN_NO_SEED				 3


int auto_rotate_crop(const unsigned char * pSrcImageData, int iSrcWidth, int iSrcHeight, int nChannels, IM_POINT * Pos, unsigned char * pDstImageData, int* iDstWidth, int* iDstHeight, int flag);

// 获取自动纠偏各个参数
int auto_rotate(const IplImage *src,float &fangle, IM_POINT * Pos, IM_RECT & rect, int iCropType = 0);


// 全自动多张纠偏
int multiAutoRotate(IplImage *image,int * iDocNum,int * ipRtn, float * fAngle, IM_POINT * pPoint,IM_RECT * pRect, int iCropType);

// 固定区域自动纠偏
// 返回值 0 成功
// 返回值 非0 失败
int multiAutoRotateFixedArea(IplImage *image,int iDocNum,int * ipRtn, float * fAngle, IM_POINT * pPoint, IM_RECT * pRect, int iCropType);


// autoRotate内部调用，不对外
int getPosition(IplImage * imGray, IM_POINT * Pos,IM_RECT &rect, float fangle, int threshold,int iCropType);
  
/*
bool autoRotate(CxImage &image,float &fangle, IM_POINT * Point,RECT &rect);

bool getPosition(CxImage * imGray, IM_POINT * Pos,RECT &rect, float fangle, int threshold);
*/
// 暂时废弃
//bool autoCrop(CxImage & image,RECT & crop,int threshold);

// 图片旋转坐标映射
int image_map(IM_POINT * newPos,
			 IM_POINT * oldPos,
			 float angle,
			 int newHeight, int newWidth,
			 int oldHeight, int oldWidth,
			 int isGetSrcPost);
   
// 去底色
int del_back_color(IplImage *src,IplImage *dst);
void ImageSharp(cv::Mat &src, cv::Mat &dst,int nAmount); 

// 登记照片去底色 去红蓝背景
// IplImage * src ,3 == nchannels 
// IplImage * dst ,4 == nchannels 
int delBackground(IplImage * src, IplImage * dst);

// autoRotate获取参数后，可以调用这个实际旋转裁剪
int image_rotate_crop(IplImage * src, IplImage * dst, float angle, IM_RECT rect);

int image_rotate_crop2(IplImage * src, IplImage * dst, float angle,IM_POINT * point, IM_RECT rect);

int image_rotate(IplImage * src, IplImage * dst, float angle);



int image_crop(IplImage * src, IplImage * dst, IM_RECT rect);
// 比较两个图片是否相似
//bool CompareImages(CxImage *image1, CxImage *image2);

// 比较两个图片是否相似
//bool CompareImages2(CxImage &image1, CxImage &image2);

// 去噪点，未完成
int noiseFilter(IplImage * src);

// 大津法求阈值
int otsu(const unsigned char *btIm, int width, int height, int widthstep);

int image_getBlue(IplImage* src, IplImage* dst );

// 排序算法
void my_sort(float *arr, int n);

long getPossRectL(IM_POINT * point);

long getPossRectT(IM_POINT * point);

long getPossRectR(IM_POINT * point);

long getPossRectB(IM_POINT * point);

//周边光矫正
int improveCornerBrightness(IplImage * src/*, IplImage *dst*/);

int improveContrast(IplImage * src);

int bgrToY(const IplImage * bgr, IplImage * Y);

int compareImages(const IplImage * image1,const IplImage * image2);

// 身份证 1 正面正立 2 正面倒立 
int detectIDNegPos(IplImage * src);

int isTextInMidlle(IplImage *image);

int rotate180(IplImage * src);

int image_crop(IplImage * src,IplImage *dst, CvRect rect);

// 求重心
int getBarycentre(IplImage *image, int & xBarycentre,int & yBarycentre);

int getRGBMax(IplImage * src, IplImage * gray);

int findContours(IplImage * image, std::vector< std::vector<IM_POINT> > & contours);

int getContoursMinRect(std::vector< std::vector<IM_POINT> > & contours, IM_RECT * rect, int index);

int getContoursMinRectPoints(std::vector< std::vector<IM_POINT> > & contours, IM_POINT * points, int index);

long getRectSum(IplImage * gray, IM_RECT rect);

int imageRotateS(IplImage * src, int iAngle );

int get_ex_rotate_angle(IplImage * src, 
						float & fAngle,  
						int iDPI, 
						IM_RECT rectSamplePerc, 
						BOOL bWidthGreaterHeight, 
						BOOL bParamIsValid );

// 去噪
int denoising(IplImage * src,IplImage * dst);

int highlight_remove(IplImage* src,IplImage* dst);

	struct structRngPoint
	{
		int pixelPos;
		structRngPoint *pPrev, *pNext;

		structRngPoint()
		{
			pixelPos = 0;
			pPrev = pNext = NULL;
		}
	};

	//寻找四周边界线
	void FindBorderLine(IplImage *pImgGray, int &lineLeft, int &lineTop, int &lineRight, int &lineBottom);

	//计算阈值
	int otusThreshold(IplImage *pImgGray);

	//计算重心
	int CmpBaryCenter(IplImage *pImgGray, int thVal, int lineLeft, int lineTop, int lineRight, int lineBottom, IM_POINT *pPntBaryCenter);

	//标注连通区
	int FG_ConnDetect(IplImage *pImgGray, int thVal, int lineLeft, int lineTop, int lineRight, int lineBottom, IM_POINT *pPntBaryCenter, uchar *pImgMask);
	
	//计算角点和倾斜角度
	int FindVertexPoints(IplImage *pImgGray, uchar *pImgMask, uchar connNo, IM_POINT *pPntsVertex, float *pAngle);

	void AdjustVertexPoints(IM_POINT *pPntsEdge, int edgePntCount, IM_POINT *pPntsVertex, float *pAngle, int imgWidth, int imgHeight);

	//求两直线交点
	//IM_POINT GetCrossPnt(IM_POINT *pLine1, IM_POINT *pLine2);

	//多张
	int EdgeDetect(IplImage *pImgGray, int thVal, int lineLeft, int lineTop, int lineRight, int lineBottom, uchar *pImgMask, int maxDocNum, int *pDocNum, IM_POINT *pPntsVertex, float *pAngles);


	int illumination_balance(const IplImage *imgSrc,IplImage *imgDst,double brightness);

	int find_rect(const IplImage *imgSrc,int &lineLeft, int &lineTop, int &lineRight, int &lineBottom);

	int deleteBlockSpace(IplImage* source, CvRect rcROI);

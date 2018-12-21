
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
int auto_rotate_crop(const unsigned char * pSrcImageData, int iSrcWidth, int iSrcHeight, int nChannels, IM_POINT * Pos, unsigned char * pDstImageData, int* iDstWidth, int* iDstHeight, int flag);

int auto_rotate(const IplImage *src,float &fangle, IM_POINT * Pos, IM_RECT & rect, int iCropType = 0);


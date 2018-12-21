#ifndef _IMAGE_H_
#define _IMAGE_H_
#include "ImageProc.h"
#define DLL_PUBLIC __attribute__((visibility("default")))
#define DLL_HIDE   __attribute__((visibility("hidden")))
extern "C" DLL_PUBLIC IplImage* DeskewImage(IplImage* src);
extern "C" DLL_PUBLIC int SaveImage(IplImage* image,char* filename);
#endif

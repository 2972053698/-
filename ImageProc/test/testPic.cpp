#include "opencv/cv.h"
#include "opencv/highgui.h"
#include"image.h"
using namespace cv;
int main()
{
    IplImage * src = cvLoadImage("./src.jpg",CV_LOAD_IMAGE_UNCHANGED);
    IplImage* dst = DeskewImage(src);
    SaveImage(dst,"crop.jpg");
    cvReleaseImage(&src);
    cvReleaseImage(&dst);
    return 0;
}

#include <iostream> 
#include <opencv2/core/core.hpp> 
#include<opencv2/highgui/highgui.hpp> 
#include <opencv2/opencv.hpp>
#include"image.h"
using namespace cv; 
using namespace std;
int main() 
{ 
	//打开第一个摄像头
	VideoCapture cap(0);
	//判断摄像头是否打开
	if (!cap.isOpened())
	{
		cout<<"摄像头未成功打开"<<endl;
	}
		
	//创建窗口
	namedWindow("打开摄像头",1);
	
	for (;;)
	{	
		//创建Mat对象
		Mat frame;	
		//从cap中读取一帧存到frame中
		cap>>frame;
		IplImage img = IplImage(frame);
		IplImage* dst = DeskewImage(&img);
		Mat temp(dst);
		//判断是否读取到
		if (frame.empty())
		{
			break;
		}
		//显示摄像头读取到的图像
		imshow("打开摄像头",temp);
		cvReleaseImage(&dst);
		//等待300毫秒，如果按键则退出循环
		if (waitKey(300)>=0)
		{
			break;
		}
	}
	         
}


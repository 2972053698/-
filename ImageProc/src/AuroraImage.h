#ifndef __IMAGE_H__
#define __IMAGE_H__

class ELimageImpl
{
public:
	ELimageImpl(void);
	~ELimageImpl(void);

	int CreateFromFIBITMAP(FIBITMAP *dib);
	FIBITMAP *CreateFIBITMAP(void);

	int CreateFromDDB(HBITMAP bmp, HDC hdc);
	HBITMAP CreateDDB(HDC hdc);
	int CreateFromDIB(HBITMAP bmp);
	HBITMAP CreateDIB(void);

	int Create(const ELsize *size, int channels, 
		void *data, int step, int flag);
	int Copy(const ELimageImpl *image);
	int Release(void);
	int GetProperty(int flag);
	int SetProperty(int flag, int value);
	void *GetData(void);
	void *GetIPImage();
	
	int DrawText(HFONT font, const ELpoint *pt, const char *text, 
		ELcolor clr, int weight, ELimageImpl *dest);
	int Rotate(float angle, int flag, ELcolor clr, ELimageImpl *dest);
	int Crop(const ELrect *rect, ELimageImpl *dest);
	int Resize(const ELsize *size, ELimageImpl *dest);
	int Blend(const ELrect *rect, const ELimageImpl *image2, 
		const ELrect *rect2, int weight, ELimageImpl *dest);
	int CvtColor(int flag, ELimageImpl *dest);
	int Threshold(int threshold, ELimageImpl *dest);
	int AdaptiveThreshold(int flag, ELimageImpl *dest);
	int Flip(int flag, ELimageImpl *dest);
	int Smooth(int flag, ELimageImpl *dest);

	// aglo
	int Deskew(int flag, ELregion *region, ELimageImpl *dest);
	int DelBkColor(ELimageImpl *dest);
	int DelBkColorSeal(ELimageImpl *dest);//印章增强去底
	//白纸印章算法 ps:去底色印章加强
	//仅支持彩色图 imgOut 需要外部自己创建，且尺寸大小、位深度、颜色通道数 和 imgSrc一致
	int WhitePaperSeal(IplImage* imgSrc , IplImage* imgOut);
	//饱和度 仅支持彩色图 pos取值范围[-3,3] imgOut 需要外部自己创建，且尺寸大小、位深度、颜色通道数 和 imgSrc一致
	int Saturation(IplImage* imgSrc , IplImage* imgOut, float pos);
	//去除图片周围黑边
	int DeleteBlack(IplImage* cxImg, int blob);

	int DelBackground( ELimageImpl * dst);
	int Denoising(ELimageImpl * dst);

	int GetBlue(ELimageImpl * dst);
	int Morphology(LONG x, LONG y, LONG w, LONG h);

	int HighLightRremove(ELimageImpl *dest);

	int Compare(const ELimageImpl *image2);
	int Compare2(const ELimageImpl *image2);
	int Compare3(const ELimageImpl *image2);
	int Compare4(const ELimageImpl *image2);
	int Compare5(const ELimageImpl *image2);
	int Reverse(ELimageImpl *dest);
	int Rectify(int flag, ELimageImpl *dest);
	int MultiDeskew(int flag, ELregion *regionList, ELimageImpl **imageList);
	int Emboss(ELimageImpl * dst);
	int Sharpen(ELimageImpl * dst);
	int Dim(ELimageImpl * dst);

	int balance(ELimageImpl * dest);
	int	FindRect(ELrect *rect);

	int IdCardProcess();
	int FormProcess();

	IplImage *GetInternal(void){return m_imageImpl;};

	void BitmapToIplImage(Bitmap* pBitmap, IplImage* &pIplImg);
	void IplImageToBitmap(IplImage* pIplImg, Bitmap* &pBitmap);

	int Whiten(int flag = 0, int threshold = -1, float autoThresholdRatio = 0.85f, int aroundNum = 5, int lowestBrightness = 180);
	int EraseBlackEdge(int flag = 0);
	int Find_BLackLineRectEdge(ELrect * rect,int width ,int height,int flag);

	int meanStdDev(int *mean, int *dev);

	int Gamma(IplImage* imgSrc, float fGamma, IplImage* imgOut);

	//检测自动翻页算法
	//函数返回0，说明是背景;返回1，说明已经完成翻页动作，可以拍照取图
	int AutoShoot(BOOL isBgImg);
private:
	IplImage *m_imageImpl;
	
	unsigned int m_xDpi;
	unsigned int m_yDpi;	
};

#endif /* __IMAGE_H__ */
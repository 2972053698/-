#include "libs.h"
#include "image.h"
#include "INVOCR.h"

ELimageImpl::ELimageImpl(void)
{
	m_imageImpl = NULL;
	m_xDpi = 96;
	m_yDpi = 96;
}

ELimageImpl::~ELimageImpl(void)
{
	cvReleaseImage(&m_imageImpl);
}

int ELimageImpl::CreateFromFIBITMAP(FIBITMAP *dib)
{
	ASSERT(NULL == m_imageImpl);
	if (NULL == dib)
	{
		elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s) dib=%p", __FUNCTION__, "Paramters", dib);
		return -1;
	}

	FIBITMAP *dibTemp = NULL;
	unsigned int width = FreeImage_GetWidth(dib);
	unsigned int height = FreeImage_GetHeight(dib);
	unsigned char *data = FreeImage_GetBits(dib);
	unsigned int xDpi = (unsigned int)(FreeImage_GetDotsPerMeterX(dib) / 39.3700787f + 0.5f);
	unsigned int yDpi = (unsigned int)(FreeImage_GetDotsPerMeterY(dib) / 39.3700787f + 0.5f);

	unsigned int bpp = FreeImage_GetBPP(dib);
	if (24 != bpp)
	{
		dibTemp = FreeImage_ConvertTo24Bits(dib);
		if (NULL == dibTemp)
		{
			elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s)", __FUNCTION__, "FreeImage_ConvertTo24Bits");
			return -1;
		}

		data = FreeImage_GetBits(dibTemp);
	}

	int ret = -1;
	if (NULL != data)
	{
		CvSize size = cvSize((int)width, (int)height);
		IplImage* head = cvCreateImageHeader(size, IPL_DEPTH_8U, 3);
		if (NULL != head)
		{
			cvSetData(head, data, head->widthStep);
			m_imageImpl = cvCreateImage(size, IPL_DEPTH_8U, 3);
			if (NULL != m_imageImpl)
			{
				cvFlip(head, m_imageImpl);
				m_xDpi = xDpi;
				m_yDpi = yDpi;
				ret = 0;
			}
			else
			{
				elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s)", __FUNCTION__, "cvCreateImage");
			}

			cvReleaseImageHeader(&head);
		}
		else
		{
			elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s)", __FUNCTION__, "cvCreateImageHeader");
		}
	}
	else
	{
		elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s)", __FUNCTION__, "FreeImage_GetBits");
	}

	if (NULL != dibTemp)
	{
		FreeImage_Unload(dibTemp);
	}

	return ret;
}

FIBITMAP *ELimageImpl::CreateFIBITMAP(void)
{
	ASSERT(NULL != m_imageImpl);

	FIBITMAP *dib = FreeImage_Allocate(m_imageImpl->width,
		m_imageImpl->height, m_imageImpl->nChannels * IPL_DEPTH_8U);
	if (NULL != dib)
	{
		unsigned char *data = FreeImage_GetBits(dib);
		if (NULL != data)
		{
			CvSize size = cvSize((int)m_imageImpl->width, (int)m_imageImpl->height);
			IplImage* head = cvCreateImageHeader(size, IPL_DEPTH_8U, m_imageImpl->nChannels);
			if (NULL != head)
			{
				cvSetData(head, data, head->widthStep);
				cvFlip(m_imageImpl, head);
				cvReleaseImageHeader(&head);

				FreeImage_SetDotsPerMeterX(dib, (unsigned int)(m_xDpi * 39.3700787f + 0.5f));
				FreeImage_SetDotsPerMeterY(dib, (unsigned int)(m_yDpi * 39.3700787f + 0.5f));
			}
			else
			{
				elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s)", __FUNCTION__, "cvCreateImageHeader");
				FreeImage_Unload(dib);
				dib = NULL;
			}
		}
		else
		{
			elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s)", __FUNCTION__, "FreeImage_GetBits");
			FreeImage_Unload(dib);
			dib = NULL;
		}
	}
	else
	{
		elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s)", __FUNCTION__, "FreeImage_Allocate");
	}

	return dib;
}

int ELimageImpl::CreateFromDDB(HBITMAP bmp, HDC hdc)
{
	elWriteInfo(EL_INFO_LEVEL_INFO, "(%s) CreateFromDDB Start...", __FUNCTION__);
	ASSERT(NULL == m_imageImpl);
	if (NULL == bmp)
	{
		elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s) bmp=%p", __FUNCTION__, "Paramters", bmp);
		return -1;
	}

	BITMAP bm;
	if (0 == GetObject(bmp, sizeof(BITMAP), &bm))
	{
		elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s)", __FUNCTION__, "GetObject");
		return -1;
	}

	if (NULL != bm.bmBits)
	{
		elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s)", __FUNCTION__, "This is not a DDB");
		return -1;
	}

	// 仅仅处理24位和32位的情况
	if (24 != bm.bmBitsPixel && 32 != bm.bmBitsPixel)
	{
		elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s) BitsPixel=%d", __FUNCTION__, "BitsPixel", bm.bmBitsPixel);
		return -1;
	}

	BITMAPINFO bmi;
	memset(&bmi, 0, sizeof(BITMAPINFO));

	bmi.bmiHeader.biSize          = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth         = bm.bmWidth;     
	bmi.bmiHeader.biHeight        = -bm.bmHeight;
	bmi.bmiHeader.biPlanes        = 1;  
	bmi.bmiHeader.biBitCount      = bm.bmBitsPixel;   
	bmi.bmiHeader.biCompression   = BI_RGB;

	m_imageImpl = cvCreateImage(cvSize(bm.bmWidth, bm.bmHeight), 
		IPL_DEPTH_8U, bm.bmBitsPixel / 8);
	if (NULL == m_imageImpl)
	{
		elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s)", __FUNCTION__, "cvCreateImage");
		return -1;
	}

	if (bm.bmWidthBytes != m_imageImpl->widthStep)
	{
		elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s)", __FUNCTION__, "bm.bmWidthBytes != m_imageImpl->widthStep");
		cvReleaseImage(&m_imageImpl);
		return -1;
	}

	int ret;
	if (NULL == hdc)
	{
		HDC hdc2 = GetDC(NULL);
		ret = GetDIBits(hdc2, bmp, 0, bm.bmHeight, 
			m_imageImpl->imageData, &bmi, DIB_RGB_COLORS);
		ReleaseDC(NULL, hdc2);
	}
	else
	{
		ret = GetDIBits(hdc, bmp, 0, bm.bmHeight, 
			m_imageImpl->imageData, &bmi, DIB_RGB_COLORS);
	}

	if (0 == ret)
	{
		elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s)", __FUNCTION__, "GetDIBits");
		cvReleaseImage(&m_imageImpl);
		return -1;
	}

	if (4 == m_imageImpl->nChannels)
	{
		IplImage *imageTemp = cvCreateImage(cvGetSize(m_imageImpl), m_imageImpl->depth, 3);
		if (NULL == imageTemp)
		{
			elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s)", __FUNCTION__, "cvCreateImage");
			cvReleaseImage(&m_imageImpl);
			return -1;
		}

		cvCvtColor(m_imageImpl, imageTemp, CV_BGRA2BGR);
		cvReleaseImage(&m_imageImpl);
		m_imageImpl = imageTemp;
	}

	elWriteInfo(EL_INFO_LEVEL_INFO, "(%s) CreateFromDDB End.", __FUNCTION__);
	return 0;
}

HBITMAP ELimageImpl::CreateDDB(HDC hdc)
{
	ASSERT(NULL != m_imageImpl);

	HDC hdc2 = hdc;
	if (NULL == hdc2)
		hdc2 = GetDC(NULL);

	HBITMAP hBmpTemp = CreateCompatibleBitmap(hdc2, 1, 1);

	BITMAP bm;
	GetObject(hBmpTemp, sizeof(BITMAP), &bm);
	DeleteObject(hBmpTemp);

	if (24 != bm.bmBitsPixel && 32 != bm.bmBitsPixel)
	{
		if (hdc != hdc2)
			ReleaseDC(NULL, hdc2);
		return NULL;
	}

	IplImage *imageTemp = m_imageImpl;
	if (1 == m_imageImpl->nChannels)
	{
		imageTemp = cvCreateImage(cvGetSize(m_imageImpl), m_imageImpl->depth, 3);
		if (NULL == imageTemp)
		{
			elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s)", __FUNCTION__, "cvCreateImage");

			if (hdc != hdc2)
				ReleaseDC(NULL, hdc2);
			return NULL;
		}

		cvCvtColor(m_imageImpl, imageTemp, EL_CVT_GRAY2COLOR);
	}

	BITMAPINFO bmi;
	memset(&bmi, 0, sizeof(BITMAPINFO));

	bmi.bmiHeader.biSize          = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth         = imageTemp->width;     
	bmi.bmiHeader.biHeight        = -imageTemp->height;
	bmi.bmiHeader.biPlanes        = 1;  
	bmi.bmiHeader.biBitCount      = imageTemp->nChannels * imageTemp->depth; 
	bmi.bmiHeader.biCompression   = BI_RGB;
	bmi.bmiHeader.biSizeImage	  = imageTemp->widthStep * imageTemp->height;

	HBITMAP hBmp = CreateDIBitmap(hdc2, &bmi.bmiHeader, 
		CBM_INIT, imageTemp->imageData, &bmi, DIB_RGB_COLORS);
	if (NULL == hBmp)
	{
		DWORD dwErr = GetLastError();
		elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s) dwErr=%d", __FUNCTION__, "CreateDIBitmap", dwErr);
	}

	// 销毁临时DC
	if (hdc != hdc2)
		ReleaseDC(NULL, hdc2);
	//销毁临时图像
	if (imageTemp != m_imageImpl)
		cvReleaseImage(&imageTemp);

	return hBmp;
}

int ELimageImpl::CreateFromDIB(HBITMAP bmp)
{
	ASSERT(NULL == m_imageImpl);
	if (NULL == bmp)
	{
		elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s) bmp=%p", __FUNCTION__, "Paramters", bmp);
		return -1;
	}

	BITMAP bm;
	if (0 == GetObject(bmp, sizeof(BITMAP), &bm))
	{
		elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s)", __FUNCTION__, "GetObject");
		return -1;
	}

	if (NULL == bm.bmBits)
	{
		elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s)", __FUNCTION__, "This is not a DIB");
		return -1;
	}

	// 仅仅处理24位和32位的情况
	if (8 != bm.bmBitsPixel && 24 != bm.bmBitsPixel && 32 != bm.bmBitsPixel)
	{
		elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s) BitsPixel=%d", __FUNCTION__, "BitsPixel", bm.bmBitsPixel);
		return -1;
	}

	m_imageImpl = cvCreateImage(cvSize(bm.bmWidth, bm.bmHeight), 
		IPL_DEPTH_8U, bm.bmBitsPixel / 8);
	if (NULL == m_imageImpl)
	{
		elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s)", __FUNCTION__, "cvCreateImage");
		return -1;
	}
	
	if (bm.bmWidthBytes != m_imageImpl->widthStep)
	{
		elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s)", __FUNCTION__, "bm.bmWidthBytes != m_imageImpl->widthStep");
		cvReleaseImage(&m_imageImpl);
		return -1;
	}

	memcpy(m_imageImpl->imageData, bm.bmBits, bm.bmWidthBytes * bm.bmHeight);
	if (4 == m_imageImpl->nChannels)
	{
		IplImage *imageTemp = cvCreateImage(cvGetSize(m_imageImpl), m_imageImpl->depth, 3);
		if (NULL == imageTemp)
		{
			elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s)", __FUNCTION__, "cvCreateImage");
			cvReleaseImage(&m_imageImpl);
			return -1;
		}

		cvCvtColor(m_imageImpl, imageTemp, CV_BGRA2BGR);
		cvReleaseImage(&m_imageImpl);
		m_imageImpl = imageTemp;
	}

	return 0;
}

HBITMAP ELimageImpl::CreateDIB(void)
{
	ASSERT(NULL != m_imageImpl);

	BYTE bmiEx[sizeof(BITMAPINFO) + 255 * 4];  
	BITMAPINFO *bmi = (BITMAPINFO*)bmiEx; 
	memset(bmi, 0, sizeof(BITMAPINFO));

	bmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);    
	bmi->bmiHeader.biWidth = m_imageImpl->width;   
	bmi->bmiHeader.biHeight = -m_imageImpl->height;  
	bmi->bmiHeader.biPlanes = 1;   
	bmi->bmiHeader.biBitCount = m_imageImpl->nChannels * m_imageImpl->depth;   
	bmi->bmiHeader.biCompression = BI_RGB;
	bmi->bmiHeader.biSizeImage = m_imageImpl->widthStep * m_imageImpl->height;
  
	switch(bmi->bmiHeader.biBitCount)
	{  
	case 8:
		{
			for(int i = 0 ; i < 256 ; ++i)  
			{  
				bmi->bmiColors[i].rgbBlue = i;
				bmi->bmiColors[i].rgbGreen = i;  
				bmi->bmiColors[i].rgbRed= i;   
			}  
		}
		break;
	case 24:   
	case 32:
		{
			((DWORD*) bmi->bmiColors)[0] = 0x00FF0000; /* red mask */  
			((DWORD*) bmi->bmiColors)[1] = 0x0000FF00; /* green mask */  
			((DWORD*) bmi->bmiColors)[2] = 0x000000FF; /* blue mask */ 
		}
		break;   
	}
   
	void *pData = NULL;
	HBITMAP hBmp = CreateDIBSection(NULL, bmi, DIB_RGB_COLORS, &pData, NULL, 0);
	if (NULL != hBmp)
		memcpy(pData, m_imageImpl->imageData, m_imageImpl->widthStep * m_imageImpl->height);
	return hBmp;
}

int ELimageImpl::Create(const ELsize *size, int channels, void *data, int step, int flag)
{
	ASSERT(NULL == m_imageImpl);

	if (NULL == size)
	{
		elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s) size=%p", __FUNCTION__, "Paramters", size);
		return -1;
	}
	else if (size->width < 0 || size->height < 0)
	{
		elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s) width=%d,height=%d",
			__FUNCTION__, "Paramters", size->width, size->height);
		return -1;
	}

	if (EL_CHANNELS_GRAY != channels 
		&& EL_CHANNELS_COLOR != channels)
	{
		elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s) channels=%d", __FUNCTION__, "Paramters", channels);
		return -1;
	}

	if (NULL == data)
	{
		if (-1 != step || 0 != flag)
		{
			elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s) step=%d,flag=%d", __FUNCTION__, "Paramters", step, flag);
			return -1;
		}

		m_imageImpl = cvCreateImage(cvSize(size->width, size->height), IPL_DEPTH_8U, channels);
		if (NULL == m_imageImpl)
		{
			elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s)", __FUNCTION__, "cvCreateImage");
			return -1;
		}

		return 0;
	}

	if ((step < size->width * channels)
		|| (EL_CREATE_TOP_LEFT != flag && EL_CREATE_BOTTOM_LEFT != flag))
	{
		elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s) step=%d,width=%d,channels=%d,flag=%d", __FUNCTION__, 
			"Paramters", step, size->width, channels, flag);
		return -1;
	}

	IplImage *imageTemp = cvCreateImageHeader(cvSize(size->width, size->height), IPL_DEPTH_8U, channels);
	if (NULL == imageTemp)
	{
		elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s)", __FUNCTION__, "cvCreateImageHeader");
		return -1;
	}

	cvSetData(imageTemp, data, step);
	int ret = 0;
	m_imageImpl = cvCreateImage(cvSize(size->width, size->height), IPL_DEPTH_8U, channels);
	if (NULL == m_imageImpl)
	{
		elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s)", __FUNCTION__, "cvCreateImage");
		ret = -1;
	}
	else
	{
		if (EL_CREATE_TOP_LEFT == flag)
			cvCopy(imageTemp, m_imageImpl);
		else
			cvFlip(imageTemp, m_imageImpl);
	}

	cvReleaseImageHeader(&imageTemp);
	return ret;
}

int ELimageImpl::Copy(const ELimageImpl *image)
{
	if (NULL == image)
	{
		elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s) image=%p", __FUNCTION__, "Paramters", image);
		return -1;
	}

	ASSERT(NULL != image->m_imageImpl);
	// 向自己拷贝，直接返回
	if (this == image)
		return 0;

	if (NULL != m_imageImpl
		&& (m_imageImpl->width == image->m_imageImpl->width
		&& m_imageImpl->height == image->m_imageImpl->height
		&& m_imageImpl->depth == image->m_imageImpl->depth
		&& m_imageImpl->nChannels == image->m_imageImpl->nChannels))
	{
		cvCopy(image->m_imageImpl, m_imageImpl);
		return 0;
	}

	IplImage *imageTemp = cvCloneImage(image->m_imageImpl);
	if (NULL == imageTemp)
	{
		elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s)", __FUNCTION__, "cvCloneImage");
		return -1;
	}

	cvReleaseImage(&m_imageImpl);
	m_imageImpl = imageTemp;
	return 0;
}

int ELimageImpl::Release(void)
{
	ASSERT(NULL != m_imageImpl);
	cvReleaseImage(&m_imageImpl);
	return 0;
}

int ELimageImpl::GetProperty(int flag)
{
	ASSERT(NULL != m_imageImpl);
	switch (flag)
	{
	case EL_PROP_WIDTH:
		return m_imageImpl->width;
	case EL_PROP_HEIGHT:
		return m_imageImpl->height;
	case EL_PROP_CHANNELS:
		return m_imageImpl->nChannels;
	case EL_PROP_WIDTHSTEP:
		return m_imageImpl->widthStep;
	case EL_PROP_XDPI:
		return (int)m_xDpi;
	case EL_PROP_YDPI:
		return (int)m_yDpi;
	}

	elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s) flag=%d", __FUNCTION__, "Paramters", flag);
	return -1;
}

int ELimageImpl::SetProperty(int flag, int value)
{
	ASSERT(NULL != m_imageImpl);
	switch (flag)
	{
	case EL_PROP_XDPI:
		if (value > 0 && value < MAXWORD)
		{
			m_xDpi = value;
			return 0;
		}
		break;
	case EL_PROP_YDPI:
		if (value > 0 && value < MAXWORD)
		{
			m_yDpi = value;
			return 0;
		}
		break;
	}
	elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s) flag=%d,value=%d", __FUNCTION__, "Paramters", flag, value);
	return -1;
}

void *ELimageImpl::GetData(void)
{
	ASSERT(NULL != m_imageImpl);
	return m_imageImpl->imageData;
}

void *ELimageImpl::GetIPImage()
{
	return m_imageImpl;
}
  
wstring A2U(const string& str)//Ascii字符转  
{  
    wstring strDes;  
    if ( str.empty() )  
        goto __end;  
    int nLen=::MultiByteToWideChar(CP_ACP, 0, str.c_str(), str.size(), NULL, 0);  
    if ( 0==nLen )  
        goto __end;  
    wchar_t* pBuffer=new wchar_t[nLen+1];  
    memset(pBuffer, 0, nLen+1);  
    ::MultiByteToWideChar(CP_ACP, 0, str.c_str(), str.size(), pBuffer, nLen);  
    pBuffer[nLen]='\0';  
    strDes.append(pBuffer);  
    delete[] pBuffer;  
__end:  
    return strDes;  
} 

int ELimageImpl::DrawText(HFONT font, const ELpoint *pt, const char *text, ELcolor clr, int weight, ELimageImpl *dest)
{
	if (this == dest)
		return DrawText(font, pt, text, clr, weight, NULL);

	ASSERT(NULL != m_imageImpl);
	if (NULL == pt)
	{
		elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s) pt=%p", __FUNCTION__, "Paramters", pt);
		return -1;
	}

	if (NULL == text || '\0' == *text)
	{
		elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s) text=%p", __FUNCTION__, "Paramters", text);
		return -1;
	}

	if (weight < 0 || weight > 255)
	{
		elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s) weight=%d", __FUNCTION__, "Paramters", weight);
		return -1;
	}

	if (0 == weight)
	{
		HDC hdc = GetDC(NULL);
 		HDC hMemDC = CreateCompatibleDC(hdc);
 		HBITMAP hBmp = CreateDIB();
 		HBITMAP hOldBmp = (HBITMAP)SelectObject(hMemDC, hBmp);

 		HFONT hOldFont = NULL;
 		if (NULL != font)
 			hOldFont = (HFONT)SelectObject(hMemDC, font);
 		SetTextColor(hMemDC, clr);
 		SetBkMode(hMemDC, TRANSPARENT);
		
		int len = lstrlen(text);
 		
		wstring str = A2U(text);
		
		TextOutW(hMemDC, pt->x, pt->y, str.c_str(), str.length());
 		
		if (NULL != font)
 			SelectObject(hMemDC, hOldFont);

		ELimageImpl *imageTemp = (NULL == dest) ? this : dest;
		cvReleaseImage(&imageTemp->m_imageImpl);
		imageTemp->CreateFromDIB(hBmp);
		
 		DeleteObject(SelectObject(hMemDC, hOldBmp));
 		DeleteDC(hMemDC);
		ReleaseDC(NULL, hdc);
		return 0;
	}

	HDC hdc = GetDC(NULL);

	HDC hMemDC = CreateCompatibleDC(hdc);
	HBITMAP hBmp = CreateDIB();
	HBITMAP hOldBmp = (HBITMAP)SelectObject(hMemDC, hBmp);

	HDC hMemDC2 = CreateCompatibleDC(hdc);
	HBITMAP hBmp2 = CreateCompatibleBitmap(hdc, m_imageImpl->width, m_imageImpl->height);
	HBITMAP hOldBmp2 = (HBITMAP)SelectObject(hMemDC2, hBmp2);
	BitBlt(hMemDC2, 0, 0, m_imageImpl->width, m_imageImpl->height, hMemDC, 0, 0, SRCCOPY);

	HFONT hOldFont = NULL;
	if (NULL != font)
		hOldFont = (HFONT)SelectObject(hMemDC2, font);
	SetTextColor(hMemDC2, clr);
	SetBkMode(hMemDC2, TRANSPARENT);
	TextOut(hMemDC2, pt->x, pt->y, text, strlen(text));
	if (NULL != font)
		SelectObject(hMemDC2, hOldFont);

	BLENDFUNCTION bf;
	bf.BlendOp = AC_SRC_OVER;
	bf.BlendFlags = 0;
	bf.SourceConstantAlpha = (BYTE)(255 - weight);
	bf.AlphaFormat = 0;
	AlphaBlend(hMemDC, 0, 0, m_imageImpl->width, m_imageImpl->height, 
		hMemDC2, 0, 0, m_imageImpl->width, m_imageImpl->height, bf);

	ELimageImpl *imageTemp = (NULL == dest) ? this : dest;
	cvReleaseImage(&imageTemp->m_imageImpl);
	imageTemp->CreateFromDIB(hBmp);

	DeleteObject(SelectObject(hMemDC2, hOldBmp2));
	DeleteDC(hMemDC2);

	DeleteObject(SelectObject(hMemDC, hOldBmp));
	DeleteDC(hMemDC);
	ReleaseDC(NULL, hdc);
	return 0;
}

int ELimageImpl::Rotate(float angle, int flag, ELcolor clr, ELimageImpl *dest)
{
	if (this == dest)
		return Rotate(angle, flag, clr, NULL);

	ASSERT(NULL != m_imageImpl);
	if (EL_ROTATE_KEEP_SIZE != flag
		&& EL_ROTATE_KEEP_INFO != flag)
	{
		elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s) flag=%d", __FUNCTION__, "Paramters", flag);
		return -1;
	}

	float map[6];  
	CvMat mapMatrix = cvMat(2, 3, CV_32F, map);
	CvPoint2D32f center;

	IplImage *imageTemp;
	bool bCreate = false;

	if (EL_ROTATE_KEEP_SIZE == flag)
	{
		if (NULL != dest)
		{
			if (NULL != dest->m_imageImpl
				&& (dest->m_imageImpl->width == m_imageImpl->width
				&& dest->m_imageImpl->height == m_imageImpl->height
				&& dest->m_imageImpl->depth == m_imageImpl->depth
				&& dest->m_imageImpl->nChannels == m_imageImpl->nChannels))
			{
				imageTemp = dest->m_imageImpl;
			}
			else
			{
				imageTemp = cvCreateImage(cvGetSize(m_imageImpl), 
					m_imageImpl->depth, m_imageImpl->nChannels);
				if (NULL == imageTemp)
				{
					elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s)", __FUNCTION__, "cvCreateImage");
					return -1;
				}

				bCreate = true;
			}
		}
		else
		{
			imageTemp = m_imageImpl;
		}

		center.x = m_imageImpl->width / 2.0f;
		center.y = m_imageImpl->height / 2.0f;
		cv2DRotationMatrix(center, angle, 1.0, &mapMatrix);
	}
	else
	{
		double angle2 = angle  * CV_PI / 180.0;    
		double a = sin(angle2), b = cos(angle2);     
		int widthRotate = int(m_imageImpl->height * fabs(a) + m_imageImpl->width * fabs(b));    
		int heightRotate = int(m_imageImpl->width * fabs(a) + m_imageImpl->height * fabs(b));

		if (NULL != dest)
		{
			if (NULL != dest->m_imageImpl
				&& (dest->m_imageImpl->width == widthRotate
				&& dest->m_imageImpl->height == heightRotate
				&& dest->m_imageImpl->depth == m_imageImpl->depth
				&& dest->m_imageImpl->nChannels == m_imageImpl->nChannels))
			{
				imageTemp = dest->m_imageImpl;
			}
			else
			{
				imageTemp = cvCreateImage(cvSize(widthRotate, heightRotate), 
					m_imageImpl->depth, m_imageImpl->nChannels);
				if (NULL == imageTemp)
				{
					elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s)", __FUNCTION__, "cvCreateImage");
					return -1;
				}

				bCreate = true;
			}
		}
		else
		{
			if (m_imageImpl->width == widthRotate 
				&& m_imageImpl->height == heightRotate)
			{
				imageTemp = m_imageImpl;
			}
			else
			{
				imageTemp = cvCreateImage(cvSize(widthRotate, heightRotate), 
					m_imageImpl->depth, m_imageImpl->nChannels);
				if (NULL == imageTemp)
				{
					elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s)", __FUNCTION__, "cvCreateImage");
					return -1;
				}

				bCreate = true;
			}
		}

		center.x = m_imageImpl->width / 2.0f;
		center.y = m_imageImpl->height / 2.0f;
		cv2DRotationMatrix(center, angle, 1.0, &mapMatrix);    
		map[2] += (widthRotate - m_imageImpl->width) / 2.0f;    
		map[5] += (heightRotate - m_imageImpl->height) / 2.0f;
	}

	CvScalar scalar = cvScalar((clr >> 16) & 0xFF, (clr >> 8) & 0xFF, clr & 0xFF);
	cvWarpAffine(m_imageImpl, imageTemp, 
		&mapMatrix, CV_INTER_LINEAR | CV_WARP_FILL_OUTLIERS, scalar);
	if (bCreate)
	{
		if (NULL != dest)
		{
			cvReleaseImage(&dest->m_imageImpl);
			dest->m_imageImpl = imageTemp;
		}
		else
		{
			cvReleaseImage(&m_imageImpl);
			m_imageImpl = imageTemp;
		}
	}

	return 0;
}

int ELimageImpl::Crop(const ELrect *rect, ELimageImpl *dest)
{
	if (this == dest)
		return Crop(rect, NULL);

	ASSERT(NULL == m_imageImpl);


	if (NULL == rect
		|| (0 == rect->x
		&& 0 == rect->y
		&& m_imageImpl->width == rect->width
		&& m_imageImpl->height == rect->height))
	{
		if (NULL == dest)
			return 0;
		else
			return dest->Copy(this);
	}

	if (rect->x < 0 || rect->x >= m_imageImpl->width
		|| rect->y < 0 || rect->y >= m_imageImpl->height)
	{
		elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s) x=%d,y=%d", 
			__FUNCTION__, "Paramters", rect->x, rect->y);
		return -1;
	}

	if (rect->width <= 0 || rect->width > m_imageImpl->width
		|| rect->height <= 0 || rect->height > m_imageImpl->height)
	{
		elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s) width=%d,height=%d", 
			__FUNCTION__, "Paramters", rect->width, rect->height);
		return -1;
	}

	if (rect->x + rect->width > m_imageImpl->width
		|| rect->y + rect->height > m_imageImpl->height)
	{
		elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s) x=%d,y=%d,width=%d,height=%d", 
			__FUNCTION__, "Paramters", rect->x, rect->y, rect->width, rect->height);
		return -1;
	}
 
	if (NULL != dest
		&& NULL != dest->m_imageImpl
		&& rect->width == dest->m_imageImpl->width
		&& rect->height == dest->m_imageImpl->height
		&& m_imageImpl->depth == dest->m_imageImpl->depth
		&& m_imageImpl->nChannels == dest->m_imageImpl->nChannels)
	{
		cvSetImageROI(m_imageImpl, cvRect(rect->x, rect->y, rect->width, rect->height));
		cvCopy(m_imageImpl, dest->m_imageImpl);
		cvResetImageROI(m_imageImpl);
		return 0;
	}

	IplImage *imageTemp = cvCreateImage(cvSize(rect->width, rect->height),
		m_imageImpl->depth, m_imageImpl->nChannels);
	if (NULL == imageTemp)
	{
		elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s)", __FUNCTION__, "cvCreateImage");
		return -1;
	}
	cvSetImageROI(m_imageImpl, cvRect(rect->x, rect->y, rect->width, rect->height));
	cvCopy(m_imageImpl, imageTemp);
	cvResetImageROI(m_imageImpl);
	if (NULL == dest)
	{
		cvReleaseImage(&m_imageImpl);
		m_imageImpl = imageTemp;
		return 0;
	}

	cvReleaseImage(&dest->m_imageImpl);
	dest->m_imageImpl = imageTemp;
	return 0;
}

int ELimageImpl::Resize(const ELsize *size, ELimageImpl *dest)
{
	if (this == dest)
		return Resize(size, NULL);

	ASSERT(NULL != m_imageImpl);
	if (NULL == size
		|| (m_imageImpl->width == size->width
		&& m_imageImpl->height == size->height))
	{
		if (NULL == dest)
			return 0;
		else
			return dest->Copy(this);
	}

	if (size->width <= 0 || size->height <= 0)
	{
		elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s) width=%d, heigh=%d", 
			__FUNCTION__, "Paramters", size->width, size->height);
		return -1;
	}

	if (NULL != dest
		&& NULL != dest->m_imageImpl
		&& size->width == dest->m_imageImpl->width
		&& size->height == dest->m_imageImpl->height
		&& m_imageImpl->depth == dest->m_imageImpl->depth
		&& m_imageImpl->nChannels == dest->m_imageImpl->nChannels)
	{
		cvResize(m_imageImpl, dest->m_imageImpl);
		return 0;
	}

	IplImage *imageTemp = cvCreateImage(cvSize(size->width, size->height), 
		m_imageImpl->depth, m_imageImpl->nChannels);
	if (NULL == imageTemp)
	{
		elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s)", __FUNCTION__, "cvCreateImage");
		return -1;
	}

	cvResize(m_imageImpl, imageTemp);
	if (NULL == dest)
	{
		cvReleaseImage(&m_imageImpl);
		m_imageImpl = imageTemp;
		return 0;
	}

	cvReleaseImage(&dest->m_imageImpl);
	dest->m_imageImpl = imageTemp;
	return 0;
}

int ELimageImpl::Blend(const ELrect *rect, const ELimageImpl *image2,const ELrect *rect2, int weight, ELimageImpl *dest)
{
	if (this == dest)
		return Blend(rect, image2, rect2, weight, NULL);
	if (this == image2)
	{
		elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s)", __FUNCTION__, "Paramters1");
		return -1;
	}

	ASSERT(NULL != m_imageImpl);
	if (NULL == image2
		|| NULL == image2->m_imageImpl
		|| m_imageImpl->depth != image2->m_imageImpl->depth
		|| m_imageImpl->nChannels != image2->m_imageImpl->nChannels)
	{
		elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s)", __FUNCTION__, "Paramters2");
		return -1;
	}

	if (weight < 0 || weight > 255)
	{
		elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s)", __FUNCTION__, "Paramters3");
		return -1;
	}

	if (NULL != rect)
	{
		if (rect->x < 0 || rect->x >= m_imageImpl->width
			|| rect->y < 0 || rect->y >= m_imageImpl->height)
		{
			elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s)", __FUNCTION__, "Paramters4");
			return -1;
		}
		if (rect->width <= 0 || rect->width > m_imageImpl->width
			|| rect->height <= 0 || rect->height > m_imageImpl->height)
		{
			elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s)", __FUNCTION__, "Paramters5");
			return -1;
		}
		if (rect->x + rect->width > m_imageImpl->width
			|| rect->y + rect->height > m_imageImpl->height)
		{
			elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s)", __FUNCTION__, "Paramters6");
			return -1;
		}
	}

	if (NULL != rect2)
	{
		if (rect2->x < 0 || rect2->x >= image2->m_imageImpl->width
			|| rect2->y < 0 || rect2->y >= image2->m_imageImpl->height)
		{
			elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s)", __FUNCTION__, "Paramters7");
			return -1;
		}
		if (rect2->width <= 0 || rect2->width > image2->m_imageImpl->width
			|| rect2->height <= 0 || rect2->height > image2->m_imageImpl->height)
		{
			elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s)", __FUNCTION__, "Paramters8");
			return -1;
		}
		if (rect2->x + rect2->width > image2->m_imageImpl->width
			|| rect2->y + rect2->height > image2->m_imageImpl->height)
		{
			elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s)", __FUNCTION__, "Paramters9");
			return -1;
		}
	}

	IplImage *imageTemp = m_imageImpl;
	if (NULL != dest)
	{
		imageTemp = cvCloneImage(m_imageImpl);
		if (NULL == imageTemp)
		{
			elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s)", __FUNCTION__, "cvCloneImage");
			return -1;
		}
	}

	int width = (NULL == rect ? imageTemp->width : rect->width);
	int width2 = (NULL == rect2 ? image2->m_imageImpl->width : rect2->width);
	int height = (NULL == rect ? imageTemp->height : rect->height);
	int height2 = (NULL == rect2 ? image2->m_imageImpl->height : rect2->height);

	double alpha = weight / 255.0;
	if (width != width2 || height != height2)
	{
		IplImage *image2Temp = cvCreateImage(cvSize(width, height), 
			image2->m_imageImpl->depth, image2->m_imageImpl->nChannels);
		if (NULL == image2Temp)
		{
			if (NULL != dest)
				cvReleaseImage(&imageTemp);

			elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s)", __FUNCTION__, "cvCreateImage");
			return -1;
		}

		if (rect2)
			cvSetImageROI(image2->m_imageImpl, cvRect(rect2->x, rect2->y, rect2->width, rect2->height));
		cvResize(image2->m_imageImpl, image2Temp);
		if (rect2)
			cvResetImageROI(image2->m_imageImpl);

		if (rect)
			cvSetImageROI(imageTemp, cvRect(rect->x, rect->y, rect->width, rect->height));
		cvAddWeighted(image2Temp, 1 - alpha, imageTemp, alpha, 0.0, imageTemp);
		if (rect)
			cvResetImageROI(imageTemp);

		cvReleaseImage(&image2Temp);
	}
	else
	{
		if (rect)
			cvSetImageROI(imageTemp, cvRect(rect->x, rect->y, rect->width, rect->height));
		if (rect2)
			cvSetImageROI(image2->m_imageImpl, cvRect(rect2->x, rect2->y, rect2->width, rect2->height));
		cvAddWeighted(image2->m_imageImpl, 1 - alpha, imageTemp, alpha, 0.0, imageTemp);
		if (rect2)
			cvResetImageROI(image2->m_imageImpl);
		if (rect)
			cvResetImageROI(imageTemp);
	}

	if (NULL != dest)
	{
		cvReleaseImage(&dest->m_imageImpl);
		dest->m_imageImpl = imageTemp;
	}

	return 0;
}

int ELimageImpl::CvtColor(int flag, ELimageImpl *dest)
{
	if (this == dest)
		return CvtColor(flag, NULL);

	if (EL_CVT_COLOR2GRAY != flag && EL_CVT_GRAY2COLOR != flag) 
	{
		elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s) flag=%d", 
			__FUNCTION__, "Paramters", flag);
		return -1;
	}

	ASSERT(NULL != m_imageImpl);
	if ((EL_CVT_COLOR2GRAY == flag && 3 != m_imageImpl->nChannels)
		|| (EL_CVT_GRAY2COLOR == flag && 1 != m_imageImpl->nChannels))
	{
		elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s) flag=%d,channels=%d", 
			__FUNCTION__, "Paramters", flag, m_imageImpl->nChannels);
		return -1;
	}

	int destChannels = (EL_CVT_COLOR2GRAY == flag ? 1 : 3);
	if (NULL != dest
		&& NULL != dest->m_imageImpl
		&& dest->m_imageImpl->width == m_imageImpl->width
		&& dest->m_imageImpl->height == m_imageImpl->height
		&& dest->m_imageImpl->depth == m_imageImpl->depth
		&& destChannels == dest->m_imageImpl->nChannels)
	{
		cvCvtColor(m_imageImpl, dest->m_imageImpl, flag);  
		return 0;
	}
	
	

	IplImage *imageTemp = cvCreateImage(cvGetSize(m_imageImpl), m_imageImpl->depth, destChannels);
	if (NULL == imageTemp)
	{
		elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s)", __FUNCTION__, "cvCreateImage");
		return -1;
	}

	cvCvtColor(m_imageImpl, imageTemp, flag);
	IplImage *imageTempy = cvCreateImage(cvGetSize(m_imageImpl), m_imageImpl->depth, 1);
	cvSplit(imageTemp,imageTempy,0,0,0);

	if (NULL == dest)
	{
		cvReleaseImage(&m_imageImpl);
		cvReleaseImage(&imageTempy);
		m_imageImpl = imageTemp;
		return 0;
	}

	cvReleaseImage(&dest->m_imageImpl);
	dest->m_imageImpl = imageTemp;
	cvReleaseImage(&imageTempy);
	return 0;
}

int ELimageImpl::Threshold(int threshold, ELimageImpl *dest)
{
	if (this == dest)
		return Threshold(threshold, NULL);

	ASSERT(NULL != m_imageImpl);
	if (1 != m_imageImpl->nChannels)
	{
		elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s) channels=%d", 
			__FUNCTION__, "Paramters", m_imageImpl->nChannels);
		return -1;
	}

	if (threshold < 0 || threshold > 255)
	{
		elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s) threshold=%d", 
			__FUNCTION__, "Paramters", threshold);
		return -1;
	}

	if (NULL == dest)
	{
		cvThreshold(m_imageImpl, m_imageImpl, threshold, 255.0, CV_THRESH_BINARY);
		return 0;
	}

	if (NULL != dest
		&& NULL != dest->m_imageImpl
		&& dest->m_imageImpl->width == m_imageImpl->width
		&& dest->m_imageImpl->height == m_imageImpl->height
		&& dest->m_imageImpl->depth == m_imageImpl->depth
		&& dest->m_imageImpl->nChannels == m_imageImpl->nChannels)
	{
		cvThreshold(m_imageImpl, dest->m_imageImpl, threshold, 255.0, CV_THRESH_BINARY);
		return 0;
	}

	IplImage *imageTemp = cvCreateImage(cvGetSize(m_imageImpl), m_imageImpl->depth, m_imageImpl->nChannels);
	if (NULL == imageTemp)
	{
		elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s)", __FUNCTION__, "cvCreateImage");
		return -1;
	}

	cvThreshold(m_imageImpl, imageTemp, threshold, 255.0, CV_THRESH_BINARY);
	cvReleaseImage(&dest->m_imageImpl);
	dest->m_imageImpl = imageTemp;
	return 0;
}

int ELimageImpl::AdaptiveThreshold(int flag, ELimageImpl *dest)
{	
	if (this == dest)
		return AdaptiveThreshold(flag, NULL);

	ASSERT(NULL != m_imageImpl);
	if (1 != m_imageImpl->nChannels)
	{
		elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s) channels=%d,flag=%d", 
			__FUNCTION__, "Paramters", m_imageImpl->nChannels, flag);
		return -1;
	}
	if(flag ==1)
		flag = 3;
	if(flag < 1 ||flag > 100)
	{
		flag = 21;
	}
	if(flag%2 == 0)
	{
		flag = flag + 1;
	}	
	if (NULL == dest)
	{
		//cvThreshold(m_imageImpl, m_imageImpl, 0, 255.0, CV_THRESH_OTSU);	
		cvAdaptiveThreshold( m_imageImpl, m_imageImpl, 255,
			CV_ADAPTIVE_THRESH_MEAN_C ,
			CV_THRESH_BINARY ,
			flag,
			8);//31,15   10
		//cvSmooth(m_imageImpl, m_imageImpl, CV_GAUSSIAN,5, 5);	
		return 0;
	}
	if (NULL != dest
		&& NULL != dest->m_imageImpl
		&& dest->m_imageImpl->width == m_imageImpl->width
		&& dest->m_imageImpl->height == m_imageImpl->height
		&& dest->m_imageImpl->depth == m_imageImpl->depth
		&& dest->m_imageImpl->nChannels == m_imageImpl->nChannels)
	{
		//cvThreshold(m_imageImpl, dest->m_imageImpl, 0, 255.0, CV_THRESH_OTSU);
		cvSmooth(dest->m_imageImpl, dest->m_imageImpl, CV_GAUSSIAN,5, 5);
		cvAdaptiveThreshold( m_imageImpl, dest->m_imageImpl, 255,
		CV_ADAPTIVE_THRESH_MEAN_C ,
		CV_THRESH_BINARY ,
		flag,
		25);
		return 0;
	}

	IplImage *imageTemp = cvCreateImage(cvGetSize(m_imageImpl), m_imageImpl->depth, m_imageImpl->nChannels);
	if (NULL == imageTemp)
	{
		elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s)", __FUNCTION__, "cvCreateImage");
		return -1;
	}

	//cvThreshold(m_imageImpl, imageTemp, 0, 255.0, CV_THRESH_OTSU);
	cvSmooth(imageTemp, imageTemp, CV_GAUSSIAN,5, 5);
	cvAdaptiveThreshold( m_imageImpl, imageTemp, 255,
		CV_ADAPTIVE_THRESH_MEAN_C ,
		CV_THRESH_BINARY ,
		flag,
		25);
	

	cvReleaseImage(&dest->m_imageImpl);
	dest->m_imageImpl = imageTemp;
	return 0;
}

int ELimageImpl::Flip(int flag, ELimageImpl *dest)
{
	if (this == dest)
		return DelBkColor(NULL);

	ASSERT(NULL != m_imageImpl);
	if (EL_FLIP_HORIZONTAL != flag
		&& EL_FLIP_VERTICAL != flag
		&& EL_FLIP_BOTH != flag)
		return -1;

	if (NULL == dest)
	{
		cvFlip(m_imageImpl, m_imageImpl, flag);
		return 0;
	}

	IplImage * imageTemp = cvCreateImage(cvGetSize(m_imageImpl), 
		m_imageImpl->depth, m_imageImpl->nChannels);
	if (NULL == imageTemp)
	{
		elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s)", __FUNCTION__, "cvCreateImage");
		return -1;
	}

	cvFlip(m_imageImpl, dest->m_imageImpl, flag);

	cvReleaseImage(&dest->m_imageImpl);
	dest->m_imageImpl = imageTemp;
	return 0;
}

int ELimageImpl::Smooth(int flag, ELimageImpl *dest)
{
	if (this == dest)
		return Smooth(flag, NULL);

	if (0 != flag)
		return -1;

	ASSERT(NULL != m_imageImpl);
	if (NULL == dest)
	{
		//cvSmooth(m_imageImpl, m_imageImpl,CV_MEDIAN,3,3,0,0);  //中值
		//cvSmooth(m_imageImpl, m_imageImpl,CV_GAUSSIAN,1,1,0,0);  //高斯
		//cvSmooth(m_imageImpl, m_imageImpl,CV_BLUR,3,3,0,0);    //均值   
		//cvSmooth(m_imageImpl, m_imageImpl, CV_BILATERAL ,10,10);  //中值  

		
		//Gamma(m_imageImpl,1.5,m_imageImpl);
		
		IplImage * imageTemp = cvCreateImage(cvGetSize(m_imageImpl), 
		m_imageImpl->depth, m_imageImpl->nChannels);	
		cvSmooth(m_imageImpl, imageTemp,CV_MEDIAN,3,0,10,10); 	
		//cvSmooth(m_imageImpl, m_imageImpl,CV_BLUR,3,3,0,0);
		//cvSmooth(m_imageImpl, m_imageImpl,CV_GAUSSIAN,1,1,0,0); 
	
		
		//cvReleaseImage(&m_imageImpl);
		//m_imageImpl = imageTemp;
		return 0;
	}

	IplImage * imageTemp = cvCreateImage(cvGetSize(m_imageImpl), 
		m_imageImpl->depth, m_imageImpl->nChannels);
	if (NULL == imageTemp)
	{
		elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s)", __FUNCTION__, "cvCreateImage");
		return -1;
	}
	  cvSmooth(m_imageImpl,dest->m_imageImpl,CV_MEDIAN,3,3,0,0);
	cvReleaseImage(&dest->m_imageImpl);
	dest->m_imageImpl = imageTemp;
	return 0;
}

int ELimageImpl::Deskew(int flag, ELregion *region, ELimageImpl *dest)
{
	if (this == dest)
		return Deskew(flag, region, NULL);

	ASSERT(NULL != m_imageImpl);
	if (NULL != region && NULL != dest)
	{
		elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s)", __FUNCTION__, "Paramters");
		return -1;
	}

	float angle;
	IM_POINT impt[4];
	IM_RECT imrc;
	if (0 != auto_rotate(m_imageImpl, angle, impt, imrc, flag))
	{
		return -1;
	}

	if (NULL != region)
	{
		region->pt[0].x = impt[0].x;
		region->pt[0].y = impt[0].y;
		region->pt[1].x = impt[1].x;
		region->pt[1].y = impt[1].y;
		region->pt[2].x = impt[2].x;
		region->pt[2].y = impt[2].y;
		region->pt[3].x = impt[3].x;
		region->pt[3].y = impt[3].y;
		return 0;
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

	IplImage * imageTemp = cvCreateImage(cvSize(dst_width, dst_height), 
		m_imageImpl->depth, m_imageImpl->nChannels);
	if (NULL == imageTemp)
	{
		elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s)", __FUNCTION__, "cvCreateImage");
		return -1;
	}

	if (0 != image_rotate_crop(m_imageImpl, imageTemp, angle, imrc))
	{
		elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s)", __FUNCTION__, "image_rotate_crop");
		cvReleaseImage(&imageTemp);
		return -1;
	}

	if (NULL == dest)
	{
		cvReleaseImage(&m_imageImpl);
		m_imageImpl = imageTemp;
		return 0;
	}

	cvReleaseImage(&dest->m_imageImpl);
	dest->m_imageImpl = imageTemp;
	return 0;
}

int ELimageImpl::DelBkColor(ELimageImpl *dest)
{
	if (this == dest)
		return DelBkColor(NULL);

	ASSERT(NULL != m_imageImpl);
	if (NULL == dest)
	{
		if (0 != del_back_color(m_imageImpl, m_imageImpl))
		//if (0 != WhitePaperSeal(m_imageImpl, m_imageImpl))
		{
			elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s)", __FUNCTION__, "del_back_color");
			return -1;
		}

		return 0;
	}

	IplImage * imageTemp = cvCreateImage(cvGetSize(m_imageImpl), 
		m_imageImpl->depth, m_imageImpl->nChannels);
	if (NULL == imageTemp)
	{
		elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s)", __FUNCTION__, "cvCreateImage");
		return -1;
	}

	if (0 != del_back_color(m_imageImpl, imageTemp))
	//if (0 != WhitePaperSeal(m_imageImpl, imageTemp))
	{
		elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s)", __FUNCTION__, "del_back_color");
		cvReleaseImage(&imageTemp);
		return -1;
	}
	cvReleaseImage(&dest->m_imageImpl);
	dest->m_imageImpl = imageTemp;

	return 0;
}

int ELimageImpl::DelBkColorSeal( ELimageImpl *dest )
{
	if (this == dest)
		return DelBkColorSeal(NULL);

	ASSERT(NULL != m_imageImpl);
	if (NULL == dest)
	{
		if (0 != WhitePaperSeal(m_imageImpl, m_imageImpl))
		{
			elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s)", __FUNCTION__, "WhitePaperSeal");
			return -1;
		}

		return 0;
	}

	IplImage * imageTemp = cvCreateImage(cvGetSize(m_imageImpl), 
		m_imageImpl->depth, m_imageImpl->nChannels);
	if (NULL == imageTemp)
	{
		elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s)", __FUNCTION__, "cvCreateImage");
		return -1;
	}

	if (0 != WhitePaperSeal(m_imageImpl, imageTemp))
	{
		elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s)", __FUNCTION__, "WhitePaperSeal");
		cvReleaseImage(&imageTemp);
		return -1;
	}
	cvReleaseImage(&dest->m_imageImpl);
	dest->m_imageImpl = imageTemp;

	return 0;
}

int ELimageImpl::DelBackground( ELimageImpl * dest)
{
	if (this == dest)
		return DelBackground(NULL);

	ASSERT(NULL != m_imageImpl);
	if (NULL == dest)
	{
		if (0 != delBackground(m_imageImpl, m_imageImpl))
		{
			elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s)", __FUNCTION__, "delBackground");
			return -1;
		}

		return 0;
	}

	IplImage * imageTemp = cvCreateImage(cvGetSize(m_imageImpl), 
		m_imageImpl->depth, /*m_imageImpl->nChannels*/4);
	if (NULL == imageTemp)
	{
		elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s)", __FUNCTION__, "cvCreateImage");
		return -1;
	}

	if (0 != delBackground(m_imageImpl, imageTemp))
	{
		elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s)", __FUNCTION__, "delBackground");
		cvReleaseImage(&imageTemp);
		return -1;
	}

	cvReleaseImage(&dest->m_imageImpl);
	dest->m_imageImpl = imageTemp;
	return 0;
}

int ELimageImpl::Denoising(ELimageImpl * dest)
{
	if (this == dest)
		return Denoising(NULL);

	ASSERT(NULL != m_imageImpl);
	if (NULL == dest)
	{
		if (0 != denoising(m_imageImpl, m_imageImpl))
		{
			elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s)", __FUNCTION__, "denoising");
			return -1;
		}

		return 0;
	}

	IplImage * imageTemp = cvCreateImage(cvGetSize(m_imageImpl), 
		m_imageImpl->depth, m_imageImpl->nChannels);
	if (NULL == imageTemp)
	{
		elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s)", __FUNCTION__, "denoising");
		return -1;
	}

	if (0 != denoising(m_imageImpl, imageTemp))
	{
		elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s)", __FUNCTION__, "denoising");
		cvReleaseImage(&imageTemp);
		return -1;
	}

	cvReleaseImage(&dest->m_imageImpl);
	dest->m_imageImpl = imageTemp;
	return 0;
}

int ELimageImpl::GetBlue(ELimageImpl * dest)
{
	ASSERT(NULL != m_imageImpl);

	if (this == dest)
	{
		if (1 != image_getBlue(m_imageImpl, m_imageImpl))
		{
			elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s)", __FUNCTION__, "GetBlue");
			return -1;
		}
		return 0;
	}

	if (NULL == dest)
	{
		if (1 != image_getBlue(m_imageImpl, m_imageImpl))
		{
			elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s)", __FUNCTION__, "GetBlue");
			return -1;
		}
		return 0;
	}

	if (1 != image_getBlue(m_imageImpl, dest->m_imageImpl))
	{
		elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s)", __FUNCTION__, "GetBlue");
		return -1;
	}
	return 0;
}

int ELimageImpl::HighLightRremove(ELimageImpl *dest)
{
	if (this == dest)
		return HighLightRremove(NULL);

	ASSERT(NULL != m_imageImpl);
	if (NULL == dest)
	{
		if (0 != highlight_remove(m_imageImpl, m_imageImpl))
		{
			elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s)", __FUNCTION__, "denoising");
			return -1;
		}

		return 0;
	}

	IplImage * imageTemp = cvCreateImage(cvGetSize(m_imageImpl), 
		m_imageImpl->depth, m_imageImpl->nChannels);
	if (NULL == imageTemp)
	{
		elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s)", __FUNCTION__, "denoising");
		return -1;
	}

	if (0 != highlight_remove(m_imageImpl, imageTemp))
	{
		elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s)", __FUNCTION__, "denoising");
		cvReleaseImage(&imageTemp);
		return -1;
	}

	cvReleaseImage(&dest->m_imageImpl);
	dest->m_imageImpl = imageTemp;
	return 0;
}

int ELimageImpl::Compare(const ELimageImpl *image2)
{
	ASSERT(NULL != m_imageImpl);
	if (NULL == image2
		|| NULL == image2->m_imageImpl)
	{
		return -1;
	}

	int ret = compareImages(m_imageImpl, image2->m_imageImpl);
	if (ret < 0)
	{
		elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s)", __FUNCTION__, "compareImages");
		return -1;
	}

	// 为0表示相同，为1-100表示差异度
	return ret;
}

int ELimageImpl::meanStdDev(int *mean, int *dev)
{
	ASSERT(NULL != m_imageImpl);

	IplImage *imgTemp1 = NULL;

	if (3 == m_imageImpl->nChannels)
	{
		imgTemp1 = cvCreateImage(cvGetSize(m_imageImpl), m_imageImpl->depth, 1);
		cvCvtColor(m_imageImpl, imgTemp1, EL_CVT_COLOR2GRAY);

		CvScalar l_mean;
		CvScalar l_dev;
		double l_min;
		double l_max;

		cvAvgSdv( imgTemp1, &l_mean, &l_dev );
		cvMinMaxLoc(imgTemp1, &l_min, &l_max);

		*mean = l_mean.val[0];
		*dev = l_dev.val[0];

		cvReleaseImage(&imgTemp1);
	}

	return 0;
}


int ELimageImpl::Compare2(const ELimageImpl *image2)
{
	ASSERT(NULL != m_imageImpl);
	if (NULL == image2
		|| NULL == image2->m_imageImpl)
	{
		return -1;
	}

	IplImage *imgTemp1 = NULL;
	IplImage *imgTemp2 = NULL;

	if (3 == m_imageImpl->nChannels)
	{
		imgTemp1 = cvCreateImage(cvGetSize(m_imageImpl), m_imageImpl->depth, 1);
		cvCvtColor(m_imageImpl, imgTemp1, EL_CVT_COLOR2GRAY);
	}

	if (3 == image2->m_imageImpl->nChannels)
	{
		imgTemp2 = cvCreateImage(cvGetSize(image2->m_imageImpl), image2->m_imageImpl->depth, 1);
		cvCvtColor(image2->m_imageImpl, imgTemp2, EL_CVT_COLOR2GRAY);
	}

	int HistogramBins = 256;
	float HistogramRange1[2] = {0, 255};
	float *HistogramRange[1] = {&HistogramRange1[0]};

	CvHistogram *Histogram1 = cvCreateHist(1, &HistogramBins, CV_HIST_ARRAY, HistogramRange);
	CvHistogram *Histogram2 = cvCreateHist(1, &HistogramBins, CV_HIST_ARRAY, HistogramRange);

	cvCalcHist(&imgTemp1, Histogram1);
	cvCalcHist(&imgTemp2, Histogram2);

	cvNormalizeHist(Histogram1, 1);
	cvNormalizeHist(Histogram2, 1);

	double correl = cvCompareHist(Histogram1, Histogram2, CV_COMP_CORREL);
	double chisqr = cvCompareHist(Histogram1, Histogram2, CV_COMP_CHISQR);

	double intersect = cvCompareHist(Histogram1, Histogram2, CV_COMP_INTERSECT);
	double bhattacharyya = cvCompareHist(Histogram1, Histogram2, CV_COMP_BHATTACHARYYA);

	cvReleaseHist(&Histogram1);
	cvReleaseHist(&Histogram2);

	if (imgTemp1)
		cvReleaseImage(&imgTemp1);
	if (imgTemp2)
		cvReleaseImage(&imgTemp2);

	return (int)(bhattacharyya*100);
}

CvSeq* getImageContour(IplImage* srcIn)
{
	if (NULL == srcIn || 1 != srcIn->nChannels)
		return NULL;

	IplImage *src = cvCreateImage(cvGetSize(srcIn), srcIn->depth, 1);
	cvCopy(srcIn, src);

	CvMemStorage* mem = cvCreateMemStorage();
	if(!mem)
		return NULL;

	//二值化图像
	cvThreshold(src, src, 128, 255, CV_THRESH_BINARY);

	CvSeq* seq;
	cvFindContours(src, mem, &seq, sizeof(CvContour), CV_RETR_CCOMP);

	cvReleaseImage(&src);
	return seq;
}

int ELimageImpl::Compare3(const ELimageImpl *image2)
{
	ASSERT(NULL != m_imageImpl);
	if (NULL == image2
		|| NULL == image2->m_imageImpl)
	{
		return -1;
	}

	IplImage *imgTemp1 = m_imageImpl;
	if (3 == imgTemp1->nChannels)
	{
		imgTemp1 = cvCreateImage(cvGetSize(m_imageImpl), m_imageImpl->depth, 1);
		cvCvtColor(m_imageImpl, imgTemp1, EL_CVT_COLOR2GRAY);
	}

	IplImage *imgTemp2 = image2->m_imageImpl;
	if (3 == imgTemp2->nChannels)
	{
		imgTemp2 = cvCreateImage(cvGetSize(image2->m_imageImpl), image2->m_imageImpl->depth, 1);
		cvCvtColor(image2->m_imageImpl, imgTemp2, EL_CVT_COLOR2GRAY);
	}

	CvSeq *contour1, *contour2;
	contour1 = getImageContour(imgTemp1);
	contour2 = getImageContour(imgTemp2);

	double result = cvMatchShapes(contour1, contour2, 1);
	cvReleaseMemStorage(&contour1->storage);
	cvReleaseMemStorage(&contour2->storage);

	if (imgTemp1 != m_imageImpl)
		cvReleaseImage(&imgTemp1);
	if (imgTemp2 != image2->m_imageImpl)
		cvReleaseImage(&imgTemp2);

	return (int)(result*100);
}


int ELimageImpl::Compare4(const ELimageImpl *image2)
{
	ASSERT(NULL != m_imageImpl);
	if (NULL == image2
		|| NULL == image2->m_imageImpl)
	{
		return -1;
	}

	IplImage *imgTemp1 = m_imageImpl;
	if (3 == imgTemp1->nChannels)
	{
		imgTemp1 = cvCreateImage(cvGetSize(m_imageImpl), m_imageImpl->depth, 1);
		cvCvtColor(m_imageImpl, imgTemp1, EL_CVT_COLOR2GRAY);
		cvAdaptiveThreshold( imgTemp1, imgTemp1, 255,
			CV_ADAPTIVE_THRESH_MEAN_C ,
			CV_THRESH_BINARY ,
			27,
			15);
	}

	IplImage *imgTemp2 = image2->m_imageImpl;
	if (3 == imgTemp2->nChannels)
	{
		imgTemp2 = cvCreateImage(cvGetSize(image2->m_imageImpl), image2->m_imageImpl->depth, 1);
		cvCvtColor(image2->m_imageImpl, imgTemp2, EL_CVT_COLOR2GRAY);
		cvAdaptiveThreshold( imgTemp2, imgTemp2, 255,
			CV_ADAPTIVE_THRESH_MEAN_C ,
			CV_THRESH_BINARY ,
			27,
			15);
	}

	CvMemStorage* storage1 = cvCreateMemStorage();
	CvSeq* first_contour1 = NULL;

	int Nc = cvFindContours(
		imgTemp1,
		storage1,
		&first_contour1,
		sizeof(CvContour),
		CV_RETR_LIST
		);

	CvMemStorage* storage2 = cvCreateMemStorage();
	CvSeq* first_contour2 = NULL;

	int Nc2 = cvFindContours(
		imgTemp2,
		storage2,
		&first_contour2,
		sizeof(CvContour),
		CV_RETR_LIST
		);

	double n = cvMatchShapes(first_contour1,first_contour2,CV_CONTOURS_MATCH_I1,0);

	cvReleaseMemStorage(&storage1);
	cvReleaseMemStorage(&storage2);

	if (imgTemp1 != m_imageImpl)
		cvReleaseImage(&imgTemp1);
	if (imgTemp2 != image2->m_imageImpl)
		cvReleaseImage(&imgTemp2);

	return (int)(100*n);
}


int ELimageImpl::Compare5(const ELimageImpl *image2)
{
	ASSERT(NULL != m_imageImpl);

	IplImage *imgTemp1 = NULL;
	//IplImage *imgTemp2 = NULL;

	if (3 == m_imageImpl->nChannels)
	{
		imgTemp1 = cvCreateImage(cvGetSize(m_imageImpl), m_imageImpl->depth, 1);
		cvCvtColor(m_imageImpl, imgTemp1, EL_CVT_COLOR2GRAY);
	}

	IplImage* imgTemp2 = cvCreateImage(cvSize(320, 320), IPL_DEPTH_8U, 1);  
    //uchar r1, g1, b1;  
    for (int i = 0; i < imgTemp2->height; i++)  
    {  
        uchar *ptrImage = (uchar*)(imgTemp2->imageData + i * imgTemp2->widthStep);  
        //uchar *ptrDst = (uchar*)(img->imageData + i * img->widthStep);  
  
        for (int j = 0; j < imgTemp2->width; j++)  
        {   
            ptrImage[3 * j]=0;    
        }  
    }  


	int HistogramBins = 256;
	float HistogramRange1[2] = {0, 255};
	float *HistogramRange[1] = {&HistogramRange1[0]};
	float data[256] = {1};

	CvHistogram *Histogram1 = cvCreateHist(1, &HistogramBins, CV_HIST_ARRAY, HistogramRange);
	cvCalcHist(&imgTemp1, Histogram1);

	CvHistogram *Histogram2 = cvCreateHist(1, &HistogramBins, CV_HIST_ARRAY, HistogramRange);
	cvCalcHist(&imgTemp2, Histogram2);
	//CvHistogram *Histogram2 = cvMakeHistHeaderForArray(1, &HistogramBins,Histogram1,data,HistogramRange); 

	
	//cvCalcHist(&imgTemp2, Histogram2);
	//cvClearHist(Histogram2);
	

	cvNormalizeHist(Histogram1, 1);
	cvNormalizeHist(Histogram2, 1);

	double correl = cvCompareHist(Histogram1, Histogram2, CV_COMP_CORREL);
	double chisqr = cvCompareHist(Histogram1, Histogram2, CV_COMP_CHISQR);

	double intersect = cvCompareHist(Histogram1, Histogram2, CV_COMP_INTERSECT);
	double bhattacharyya = cvCompareHist(Histogram1, Histogram2, CV_COMP_BHATTACHARYYA);

	elWriteInfo(EL_INFO_LEVEL_ERROR, "%f %f %f %f", __FUNCTION__, correl, chisqr, intersect, bhattacharyya);

	cvReleaseHist(&Histogram1);
	cvReleaseHist(&Histogram2);

	if (imgTemp1)
		cvReleaseImage(&imgTemp1);
	if (imgTemp2)
		cvReleaseImage(&imgTemp2);

	return (int)(bhattacharyya*100);
}


int ELimageImpl::Reverse(ELimageImpl *dest)
{
	if (this == dest)
		return Reverse(NULL);

	ASSERT(NULL != m_imageImpl);
	// widthStep必须为4的整数倍
	ASSERT(0 == m_imageImpl->widthStep % 4);

	IplImage * imageTemp = m_imageImpl;
	if (NULL != dest)
	{
		imageTemp = cvCreateImage(cvGetSize(m_imageImpl), 
			m_imageImpl->depth, m_imageImpl->nChannels);
		if (NULL == imageTemp)
		{
			elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s)", __FUNCTION__, "cvCreateImage");
			return -1;
		}

		cvCopy(m_imageImpl, imageTemp);
	}

	// 图像反转
	int count = (imageTemp->widthStep / 4) * imageTemp->height;
	int *begin = (int *)imageTemp->imageData;
	int *end = begin + count;

	for (int *p = begin; p < end; ++p)
	{
		int temp = *p;
		temp = ~temp;
		*p = temp;
	}

	if (NULL != dest)
	{
		cvReleaseImage(&dest->m_imageImpl);
		dest->m_imageImpl = imageTemp;
	}

	return 0;
}

int ELimageImpl::Rectify(int flag, ELimageImpl *dest)
{
	if (this == dest)
		return Rectify(flag, NULL);

	ASSERT(NULL != m_imageImpl);

	int ret = -1;
	if (EL_RECTIFY_NONE == flag)
	{
		// 自动判断
		float angle;
		IM_RECT rect = {0};
		if (0 == get_ex_rotate_angle(m_imageImpl, angle, m_xDpi, rect, FALSE, FALSE))
			ret = Rotate(-angle, EL_ROTATE_KEEP_INFO, RGB(0, 0, 0), dest);
	}
	else if (EL_RECTIFY_IDCARD == flag)
	{
		ELimageImpl *pImage = this;
		ELimageImpl imgTemp;
		if (m_imageImpl->width < m_imageImpl->height)
		{
			Rotate(90.0f, EL_ROTATE_KEEP_INFO, RGB(0, 0, 0), &imgTemp);
			pImage = &imgTemp;
		}

		int retDetec = detectIDNegPos(pImage->m_imageImpl);
		if (1 == retDetec)
		{
			if (pImage != this)
			{
				Rotate(90.0f, EL_ROTATE_KEEP_INFO, RGB(0, 0, 0), dest);
			}
			else if (NULL != dest)
			{
				cvReleaseImage(&dest->m_imageImpl);
				dest->m_imageImpl = cvCloneImage(m_imageImpl);
			}

			ret = 0;
		}
		else if (2 == retDetec)
		{
			if (pImage != this)
			{
				Rotate(270.0f, EL_ROTATE_KEEP_INFO, RGB(0, 0, 0), dest);
			}
			else
			{
				Rotate(180.0f, EL_ROTATE_KEEP_INFO, RGB(0, 0, 0), dest);
			}

			ret = 0;
		}
		else if (3 == retDetec)
		{
			if (pImage != this)
			{
				Rotate(90.0f, EL_ROTATE_KEEP_INFO, RGB(0, 0, 0), dest);
			}
			else if (NULL != dest)
			{
				cvReleaseImage(&dest->m_imageImpl);
				dest->m_imageImpl = cvCloneImage(m_imageImpl);
			}

			ret = 0;
		}
		else if (4 == retDetec)
		{
			if (pImage != this)
			{
				Rotate(270.0f, EL_ROTATE_KEEP_INFO, RGB(0, 0, 0), dest);
			}
			else
			{
				Rotate(180.0f, EL_ROTATE_KEEP_INFO, RGB(0, 0, 0), dest);
			}

			ret = 0;
		}
	}

	return ret;
}

int ELimageImpl::MultiDeskew(int flag, ELregion *regionList, ELimageImpl **imageList)
{
	ASSERT(NULL != m_imageImpl);

	int rtn[40] = {0};
	float angle[40] = {0};
	IM_POINT point[40 * 4] = {0};
	IM_RECT rect[40] = {0};

	int docNum = 0;
	if (0 != multiAutoRotate(m_imageImpl, &docNum, rtn, angle, point, rect, flag))
	{
		return -1;
	}

	if (NULL != regionList)
	{
		for (int i = 0; i < docNum; ++i)
		{
			regionList[i].pt[0].x = point[i * 4].x;
			regionList[i].pt[0].y = point[i * 4].y;
			regionList[i].pt[1].x = point[i * 4 + 1].x;
			regionList[i].pt[1].y = point[i * 4 + 1].y;
			regionList[i].pt[2].x = point[i * 4 + 2].x;
			regionList[i].pt[2].y = point[i * 4 + 2].y;
			regionList[i].pt[3].x = point[i * 4 + 3].x;
			regionList[i].pt[3].y = point[i * 4 + 3].y;
		}
	}

	if (NULL != imageList)
	{
		for (int i = 0; i < docNum; ++i)
		{
			if (NULL != imageList[i])
			{
				cvReleaseImage(&imageList[i]->m_imageImpl);
				imageList[i]->m_imageImpl = NULL;

				int dst_width  = rect[i].right - rect[i].left;
				int dst_height = rect[i].bottom - rect[i].top;

				IplImage *imageTemp = cvCreateImage(cvSize(dst_width, dst_height), 
					m_imageImpl->depth, m_imageImpl->nChannels);
				if (NULL != imageTemp)
				{
					if (0 == image_rotate_crop(m_imageImpl, imageTemp, angle[i], rect[i]))
					{
						imageList[i]->m_imageImpl = imageTemp;
					}
					else
					{
						elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s)", __FUNCTION__, "image_rotate_crop");
						cvReleaseImage(&imageTemp);
					}
				}
			}
		}
	}

	return docNum;
}

int ELimageImpl::Emboss(ELimageImpl * dst)
{
	if (this == dst)
		return Emboss(NULL);

	ASSERT(NULL != m_imageImpl);
	IplImage * imageTemp = m_imageImpl;
	if (NULL != dst)
	{
		imageTemp = cvCreateImage(cvGetSize(m_imageImpl), 
			m_imageImpl->depth, m_imageImpl->nChannels);
		if (NULL == imageTemp)
		{
			elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s)", __FUNCTION__, "cvCreateImage");
			return -1;
		}

		cvCopy(m_imageImpl, imageTemp);
	}

	// 浮雕效果

	BYTE *pAll = (BYTE*)imageTemp->imageData;
	BYTE *pAll2 = (BYTE*)imageTemp->imageData + imageTemp->widthStep + imageTemp->nChannels;
	BYTE *pAllEnd = pAll + imageTemp->widthStep * (imageTemp->height - 1);

	int sizeLine = (imageTemp->width - 1) * imageTemp->nChannels;
	while (pAll < pAllEnd)
	{
		BYTE *pLine = pAll;
		BYTE *pLine2 = pAll2;
		BYTE *pLineEnd = pLine + sizeLine;

		while (pLine < pLineEnd)
		{
			int tmp = *pLine2 - *pLine + 128;
			if (tmp > 255)
			{
				*pLine = 255;
			}
			else if (tmp < 0)
			{
				*pLine = 0;
			}
			else
			{
				*pLine = tmp;
			}

			++pLine;
			++pLine2;
		}

		pAll += imageTemp->widthStep;
		pAll2 += imageTemp->widthStep;
	}

	if (NULL != dst)
	{
		cvReleaseImage(&dst->m_imageImpl);
		dst->m_imageImpl = imageTemp;
	}

	return 0;
}

int ELimageImpl::Sharpen(ELimageImpl * dst)
{
	if (this == dst)
		return Sharpen(NULL);

	ASSERT(NULL != m_imageImpl);
	IplImage * imageTemp = m_imageImpl;
	if (NULL != dst)
	{
		imageTemp = cvCreateImage(cvGetSize(m_imageImpl), 
			m_imageImpl->depth, m_imageImpl->nChannels);
		if (NULL == imageTemp)
		{
			elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s)", __FUNCTION__, "cvCreateImage");
			return -1;
		}

		cvCopy(m_imageImpl, imageTemp);
	}

	// 锐化

	float k[9] = {0, -1, 0, -1, 5, -1, 0, -1, 0};  //核
	CvMat km = cvMat(3, 3, CV_32FC1, k); 

	cvFilter2D(imageTemp, imageTemp, &km);

// 	BYTE *pAll = (BYTE*)imageTemp->imageData + imageTemp->widthStep + imageTemp->nChannels;
// 	BYTE *pAllLeft = (BYTE*)imageTemp->imageData + imageTemp->widthStep;
// 	BYTE *pAllTop = (BYTE*)imageTemp->imageData + imageTemp->nChannels;
// 	BYTE *pAllRight = (BYTE*)imageTemp->imageData + imageTemp->widthStep + 2 * imageTemp->nChannels;
// 	BYTE *pAllBottom = (BYTE*)imageTemp->imageData + imageTemp->widthStep * 2 + imageTemp->nChannels;
// 	BYTE *pAllEnd = pAll + imageTemp->widthStep * (imageTemp->height - 2);
// 
// 	int sizeLine = (imageTemp->width - 2) * imageTemp->nChannels;
// 	while (pAll < pAllEnd)
// 	{
// 		BYTE *pLine = pAll;
// 		BYTE *pLineLeft = pAllLeft;
// 		BYTE *pLineTop = pAllTop;
// 		BYTE *pLineRight = pAllRight;
// 		BYTE *pLineBottom = pAllBottom;
// 		BYTE *pLineEnd = pLine + sizeLine;
// 
// 		while (pLine < pLineEnd)
// 		{
// 			int tmp = *pLine * 4 - *pLineLeft - *pLineTop - *pLineRight - *pLineBottom;
// 			*pLine = (tmp + 4 * 255) >> 3;
// 
// 			++pLine;
// 			++pLineLeft;
// 			++pLineTop;
// 			++pLineRight;
// 			++pLineBottom;
// 		}
// 
// 		pAll += imageTemp->widthStep;
// 		pAllLeft += imageTemp->widthStep;
// 		pAllTop += imageTemp->widthStep;
// 		pAllRight += imageTemp->widthStep;
// 		pAllBottom += imageTemp->widthStep;
// 	}

// 	IplImage* redImage = cvCreateImage(cvGetSize(imageTemp), imageTemp->depth, 1);
// 	IplImage* greenImage = cvCreateImage(cvGetSize(imageTemp), imageTemp->depth, 1);
// 	IplImage* blueImage = cvCreateImage(cvGetSize(imageTemp), imageTemp->depth, 1);
// 
// 	cvSplit(imageTemp, blueImage, greenImage, redImage, NULL);
// 	cvEqualizeHist(redImage, redImage);
// 	cvEqualizeHist(greenImage, greenImage); 
// 	cvEqualizeHist(blueImage, blueImage);
// 	cvMerge(blueImage, greenImage, redImage, NULL, imageTemp);
// 
// 	cvReleaseImage(&blueImage);
// 	cvReleaseImage(&greenImage);
// 	cvReleaseImage(&redImage);

	if (NULL != dst)
	{
		cvReleaseImage(&dst->m_imageImpl);
		dst->m_imageImpl = imageTemp;
	}

	return 0;
}

int ELimageImpl::Dim(ELimageImpl * dst)
{
	if (this == dst)
		return Dim(NULL);

	ASSERT(NULL != m_imageImpl);
	IplImage * imageTemp = m_imageImpl;
	if (NULL != dst)
	{
		imageTemp = cvCreateImage(cvGetSize(m_imageImpl), 
			m_imageImpl->depth, m_imageImpl->nChannels);
		if (NULL == imageTemp)
		{
			elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s)", __FUNCTION__, "cvCreateImage");
			return -1;
		}

		cvCopy(m_imageImpl, imageTemp);
	}

	// 模糊

	float k[9];
	for (int i = 0; i < 9; ++i)
	{
		k[i] = 0.11f;
	}

	CvMat km = cvMat(3, 3, CV_32FC1, k);

	cvFilter2D(imageTemp, imageTemp, &km);

	if (NULL != dst)
	{
		cvReleaseImage(&dst->m_imageImpl);
		dst->m_imageImpl = imageTemp;
	}

	return 0;
}

int ELimageImpl::balance(ELimageImpl * dest)
{
	if (this == dest)
		return balance(NULL);

	ASSERT(NULL != m_imageImpl);
	if (NULL == dest)
	{
		if (0 != illumination_balance(m_imageImpl, m_imageImpl, 0))
		{
			elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s)", __FUNCTION__, "balance_callback");
			return -1;
		}

		return 0;
	}

	IplImage * imageTemp = cvCreateImage(cvGetSize(m_imageImpl), 
		m_imageImpl->depth, m_imageImpl->nChannels);
	if (NULL == imageTemp)
	{
		elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s)", __FUNCTION__, "cvCreateImage");
		return -1;
	}

	if (0 != illumination_balance(m_imageImpl, imageTemp, 0))
	{
		elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s)", __FUNCTION__, "delBackgrand");
		cvReleaseImage(&imageTemp);
		return -1;
	}

	cvReleaseImage(&dest->m_imageImpl);
	dest->m_imageImpl = imageTemp;
	return 0;
}



int	ELimageImpl::FindRect(ELrect *rect)
{	
	ASSERT(NULL != m_imageImpl);

	int Left = 0;
	int Top = 0;
	int Right = 0;
	int Bottom = 0;
	int ret = -1;

	ret = find_rect(m_imageImpl,Left, Top, Right, Bottom);

	if(-1 != ret)
	{
		rect->x = Left;
		rect->y = Top;
		rect->width = Right - Left;
		rect->height = Bottom - Top;
	}

	return ret;
}

int ELimageImpl::IdCardProcess()
{
	ASSERT(NULL != m_imageImpl);

	//初始化 GDI+   
	GdiplusStartupInput gdiplusStartupInput;
	GdiplusStartup( &m_gdiplusToken,&gdiplusStartupInput,NULL ); 

	Bitmap* pBitmap = NULL;
	IplImageToBitmap(m_imageImpl, pBitmap);

	if (NULL==pBitmap)
	{
		return -1;
	}
	
	IPFounctions ipf; 

	ipf.AdptiveBright(pBitmap,245.0f,242.10f,242.8f);	//自适应亮度增强 

	ipf.LineBrightAndContrast(pBitmap, 20, -5, 121);	//线性亮度增强 

	ipf.stretchHistogram(pBitmap);						//直方图均衡

	IplImage* pIplImg = NULL;

	BitmapToIplImage(pBitmap, pIplImg);
	cvReleaseImage(&m_imageImpl);
	m_imageImpl = pIplImg;

	//反初始化 GDI+
	GdiplusShutdown( m_gdiplusToken );

	return 0;
}

int ELimageImpl::FormProcess()
{
	ASSERT(NULL != m_imageImpl);

	//初始化 GDI+   
	GdiplusStartupInput gdiplusStartupInput;
	GdiplusStartup( &m_gdiplusToken,&gdiplusStartupInput,NULL ); 

	Bitmap* pBitmap = NULL;
	IplImageToBitmap(m_imageImpl, pBitmap);

	IPFounctions ipf; 
	ipf.FogRemove(pBitmap,46,0.7f);						//暗通道图像去雾 
	ipf.AdptiveBright(pBitmap,223.8f,216.10f,212.8f);	//自适应亮度增强 
	ipf.stretchHistogram(pBitmap);						//直方图均衡 
	ipf.LineBrightAndContrast(pBitmap, 10, -5, 121);	//线性亮度增强

	IplImage* pIplImg = NULL;

	BitmapToIplImage(pBitmap, pIplImg);
	cvReleaseImage(&m_imageImpl);
	m_imageImpl = pIplImg;

	//反初始化 GDI+
	GdiplusShutdown( m_gdiplusToken );

	return 0;
}

// pIplImage 需要外部释放 Mosesyuan
void ELimageImpl::BitmapToIplImage(Bitmap* pBitmap, IplImage* &pIplImg)
{
	if (!pBitmap)
	{
		return;
	}
	if(pIplImg)
	{
		cvReleaseImage(&pIplImg);
		pIplImg = NULL;
	}
	BitmapData bmpData;

	Gdiplus::Rect rect(0,0,pBitmap->GetWidth(),pBitmap->GetHeight());
	pBitmap->LockBits(&rect, ImageLockModeRead, PixelFormat24bppRGB, &bmpData);
	IplImage* tempImg = cvCreateImage(cvSize(pBitmap->GetWidth(), pBitmap->GetHeight()), IPL_DEPTH_8U, 3);
	BYTE* temp = (bmpData.Stride>0)?((BYTE*)bmpData.Scan0):((BYTE*)bmpData.Scan0+bmpData.Stride*(bmpData.Height-1));
	memcpy(tempImg->imageData, temp, abs(bmpData.Stride)*bmpData.Height);
	pBitmap->UnlockBits(&bmpData);
	pIplImg = tempImg;

	//判断Top-Down or Bottom-Up
	/*if (bmpData.Stride<0)        
		cvFlip(pIplImg, pIplImg);*/            
}

// pBitmap 同样需要外部释放！！
void ELimageImpl::IplImageToBitmap(IplImage* pIplImg, Bitmap* &pBitmap)
{
	if(!pIplImg)
		return;        
	BITMAPINFOHEADER bmih;
	memset(&bmih, 0, sizeof(BITMAPINFOHEADER));
	bmih.biSize = sizeof(BITMAPINFOHEADER);
	bmih.biWidth = pIplImg->width;
	bmih.biHeight = pIplImg->height;
	bmih.biPlanes = 1;
	bmih.biBitCount = pIplImg->depth*pIplImg->nChannels;
	bmih.biSizeImage = pIplImg->imageSize;
	BYTE* pData=new BYTE[bmih.biSizeImage];
	memcpy(pData, pIplImg->imageDataOrigin, pIplImg->imageSize);
	if (pBitmap)
	{
		delete pBitmap;
		pBitmap = NULL;
	}

	pBitmap = Gdiplus::Bitmap::FromBITMAPINFO((BITMAPINFO*)&bmih, pData);
}

//获取该点的灰度值，lineData:行数据地址，widthIdx：列坐标
int GetPointGray(unsigned char * lineData, int widthIdx)
{
	unsigned char R, G, B;
	R = lineData[widthIdx*3];
	G = lineData[widthIdx*3+1];
	B = lineData[widthIdx*3+2];
	int gray = (R*19595 + G*38469 + B*7472) >> 16;
	return gray;
}

/*
统计当前点邻域内的非白点比率
data:图片数据地址，width：图像宽度，height:图像高度，step：图像步长
widthIdx：当前点列坐标，heightIdx:当前点行坐标
arrowNum:当前点到邻域边界的点数量，即2*arrowNum+1为邻域的边长，realThreshold:判断非白点的阈值
*/
float GetNoWhiteRatioArround(unsigned char * data, int width, int height, int step, 
	int widthIdx, int heightIdx, int aroundNum, int realThreshold)
{
	int side = aroundNum*2 + 1;//邻域的边长
	int allCountAround = (side * side) - 1;//邻域中点的总数目为边长的平方，不考虑当前点，故减一
	int noWhitePointCount = 0;
	int tempHeightIdx, tempWidthIdx;
	for(tempHeightIdx = heightIdx - aroundNum; tempHeightIdx <= heightIdx + aroundNum; tempHeightIdx++)
	{
		//图像边界上的点，邻域是不完整的，计算这些点时，需要在总数量中扣除邻域中不在有效的图像坐标内的点的数量
		if (tempHeightIdx < 0 || tempHeightIdx >= height)
		{
			allCountAround -= side;//对于图像上下边界的点，扣除数量为边长值
			continue;
		}

		unsigned char * srcData = data + tempHeightIdx * step;
		for (tempWidthIdx = widthIdx - aroundNum; tempWidthIdx <= widthIdx + aroundNum; tempWidthIdx++)
		{
			if (tempWidthIdx < 0 || tempWidthIdx >= width)
			{
				allCountAround--;//对于图像左右边界的点，扣除数量为1
				continue;
			}

			if ((tempHeightIdx == heightIdx) && (tempWidthIdx == widthIdx))
			{
				continue;//不计算当前点
			}

			int gray = GetPointGray(srcData, tempWidthIdx);
			if (gray < realThreshold)
			{
				noWhitePointCount++;
			}
		}
	}
	float ration = (float)(noWhitePointCount / (float)(allCountAround));
	return ration;
}

int ELimageImpl::Whiten(int flag, int threshold, float autoThresholdRatio, 
	int aroundNum, int lowestBrightness)
{
	int imageHeight = m_imageImpl->height;
	int imageWidth = m_imageImpl->width;
	int imageStep = m_imageImpl->widthStep;
	int heightIdx = 0, widthIdx = 0;
	unsigned char* srcData;

	int realThreshold = threshold;
	if (-1 == threshold)//若用户传值为-1，将使用根据图像灰度自动计算出的阈值
	{
		int sampleHeight = imageHeight/4;
		int sampleWidth = 32;

		int sum = 0, count = 0;
		for (heightIdx = 0; heightIdx < sampleHeight; heightIdx++)
		{
			srcData = (unsigned char*)m_imageImpl->imageData+heightIdx*imageStep;

			//对左边界上采样范围内的点的数量和灰度进行统计
			for (int i = 0; i < sampleWidth; i++)
			{
				int gray = GetPointGray(srcData, i);
				sum += gray;
				count++;
			}

			//对右边界上采样范围内的点的数量和灰度进行统计
			for (int i = imageWidth - 1; i > (imageWidth -1 - sampleWidth); i--)
			{
				int gray = GetPointGray(srcData, i);
				sum += gray;
				count++;
			}
		}
		if (count == 0)
		{
			return -1;
		}
		//计算出的平均阈值并不能很好的筛选出非白点，需要乘以参数autoThresholdRatio，做适当的下调
		realThreshold = (int)( (sum/count) * autoThresholdRatio ); 
	}

	//创建两个数量为图像高度的数组，代表非白点区域在图像中的左边界和右边界
	int * leftBorder, * rightBorder;
	leftBorder = new int[imageHeight];
	rightBorder = new int[imageHeight];
	for (heightIdx = 0; heightIdx < imageHeight; heightIdx++)
	{
		//初始情况下，左边界值为图像宽度，右边界值为0，此时所有图像内容均视为白点
		leftBorder[heightIdx] = imageWidth;
		rightBorder[heightIdx] = 0;
	}

	//当当前点邻域内的非白点比率超过ThresholdRatio时，判定当前点需要设置为非白
	const float ThresholdRatio = 0.8f;

	for (heightIdx = 0; heightIdx < imageHeight; heightIdx++)
	{
		//从左到右遍历图像，查找非白点区域的左边界，查找到第一个非白点，则结束查找
		for (widthIdx = 0; widthIdx < imageWidth; widthIdx++)
		{
			float  noWhiteRatio = GetNoWhiteRatioArround((unsigned char *) m_imageImpl->imageData, 
				imageWidth, imageHeight, imageStep, widthIdx, heightIdx, aroundNum, realThreshold);
			if (noWhiteRatio > ThresholdRatio)
			{
				leftBorder[heightIdx] = widthIdx;  
				break;
			}
		}

		//从右到左遍历图像，查找非白点区域的右边界，查找到第一个非白点，则结束查找
		for (widthIdx = imageWidth - 1; widthIdx >= 0; widthIdx--)
		{
			float  noWhiteRatio = GetNoWhiteRatioArround((unsigned char *) m_imageImpl->imageData, 
				imageWidth, imageHeight, imageStep, widthIdx, heightIdx, aroundNum, realThreshold);
			if (noWhiteRatio > ThresholdRatio)
			{
				rightBorder[heightIdx] = widthIdx;
				break;
			}
		}
	}

	//根据找出的非白点区域边界，对边界外的像素点全部置白
	for (heightIdx = 0; heightIdx < imageHeight; heightIdx++)
	{
		srcData = (unsigned char*)m_imageImpl->imageData+heightIdx*imageStep;
		int left = leftBorder[heightIdx];
		int right = rightBorder[heightIdx];
		for (widthIdx = 0; widthIdx < imageWidth; widthIdx++)
		{
			if ((widthIdx < left) || (widthIdx > right))
			{
				srcData[widthIdx*3] = 255;
				srcData[widthIdx*3+1] = 255;
				srcData[widthIdx*3+2] = 255;
			}
		}
	}

	delete[] leftBorder;
	delete[] rightBorder;
	
	//获取图片的平均亮度，若小于最低值，自动调整至最低值
	CvScalar scalar;
	if (3 == m_imageImpl->nChannels)
	{
		IplImage *grayImage = cvCreateImage(cvGetSize(m_imageImpl),IPL_DEPTH_8U,1);  
		cvCvtColor(m_imageImpl,grayImage,CV_RGB2GRAY);  
		scalar = cvAvg(grayImage); 
		cvReleaseImage(&grayImage);  
	}
	else
	{
		scalar = cvAvg(m_imageImpl); 
	}
	double imageAverageBrightness = scalar.val[0]; 
	if (imageAverageBrightness < lowestBrightness)
	{
		cvConvertScale(m_imageImpl,m_imageImpl,lowestBrightness/imageAverageBrightness); 
	}
	return 0;
}

int ELimageImpl::EraseBlackEdge(int flag )
{	
	int imageHeight = m_imageImpl->height;
	int imageWidth = m_imageImpl->width;
	int imageStep = m_imageImpl->widthStep;
	int heightIdx = 0, widthIdx = 0;
	unsigned char* srcData;	
	if(flag <0 ||flag >imageHeight)
		flag = imageHeight/20;
	if(1)//flag == -1)
	{
		for(heightIdx = 0; heightIdx < imageHeight-2; heightIdx++)
		{
			srcData = (unsigned char*)m_imageImpl->imageData+heightIdx*imageStep;
			int endpoint = 0;int starpoint = 0;
			for (widthIdx = 0; widthIdx <=imageWidth; widthIdx++)
			{
				if(starpoint ==1 && (srcData[widthIdx*3] ==255||srcData[widthIdx*3+1] == 255||srcData[widthIdx*3+2] == 255))
				{
					endpoint++;
					if(endpoint>2)
					break;
				}
				if(srcData[widthIdx*3] !=255||srcData[widthIdx*3+1] != 255||srcData[widthIdx*3+2] != 255)
				{
					srcData[widthIdx*3]= 255;
					srcData[widthIdx*3+1] = 255;
					srcData[widthIdx*3+2] = 255;
					starpoint =1;
				}
			}
			int endpointe = 0;int starpointe = 0;
			for (widthIdx = imageWidth-1; widthIdx >0; widthIdx--)
			{
				if(starpointe ==1 && (srcData[widthIdx*3] ==255||srcData[widthIdx*3+1] == 255||srcData[widthIdx*3+2] == 255))
				{
					endpointe++;
					if(endpointe>2)
					break;
				}
				if(srcData[widthIdx*3] !=255||srcData[widthIdx*3+1] != 255||srcData[widthIdx*3+2] != 255)
				{
					srcData[widthIdx*3]= 255;
					srcData[widthIdx*3+1] = 255;
					srcData[widthIdx*3+2] = 255;
					starpointe =1;
				}
			}

		}
	}
	return 0;
	for (heightIdx = 0; heightIdx < imageHeight-2; heightIdx++)
	{
		srcData = (unsigned char*)m_imageImpl->imageData+heightIdx*imageStep;		
		for (widthIdx = 0; widthIdx <=imageWidth; widthIdx++)
		{	
			if(heightIdx < flag)
			{
				if(srcData[widthIdx*3] !=255||srcData[widthIdx*3+1] != 255||srcData[widthIdx*3+2] != 255)
				{
					srcData[widthIdx*3]= 255;
					srcData[widthIdx*3+1] = 255;
					srcData[widthIdx*3+2] = 255;				
				}	
			}
			else if(heightIdx  >imageHeight-flag )
			{				
				srcData[widthIdx*3]= 255;
				srcData[widthIdx*3+1] = 255;
				srcData[widthIdx*3+2] = 255;						
			}
			else
			{
				if(widthIdx < flag)
				{
					srcData[widthIdx*3]= 255;
					srcData[widthIdx*3+1] = 255;
					srcData[widthIdx*3+2] = 255;
				}
				if(widthIdx > imageWidth-flag )
				{
					srcData[widthIdx*3]= 255;
					srcData[widthIdx*3+1] = 255;
					srcData[widthIdx*3+2] = 255;
				}
			}				
		}
	}		
	return 0;
}
  

// 内轮廓填充     
// 参数:     
// 1. pBinary: 输入二值图像，单通道，位深IPL_DEPTH_8U。    
// 2. dAreaThre: 面积阈值，当内轮廓面积小于等于dAreaThre时，进行填充。     
//void FillInternalContours(IplImage *pBinary, double dAreaThre,IplImage *pBin0)     
//{     
//    double dConArea;     
//    CvSeq *pContour = NULL;     
//    CvSeq *pConInner = NULL;     
//    CvMemStorage *pStorage = NULL;     
//    // 执行条件     
//    if (pBinary)     
//    {     
//        // 查找所有轮廓     
//        pStorage = cvCreateMemStorage(0);     
//        cvFindContours(pBinary, pStorage, &pContour, sizeof(CvContour), CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE);     
//        // 填充所有轮廓     
//        cvDrawContours(pBinary, pContour, CV_RGB(255, 255, 255), CV_RGB(255, 255, 255), 2, CV_FILLED, 8, cvPoint(0, 0));    
//        // 外轮廓循环     
//		
//        int wai = 0;    
//        int nei = 0;    
//		vector<CvRect> rects;
//        for (; pContour != NULL; pContour = pContour->h_next)     
//        {     
//            wai++;    
//            // 内轮廓循环     
//            for (pConInner = pContour->v_next; pConInner != NULL; pConInner = pConInner->h_next)     
//            {     
//                nei++;    
//                // 内轮廓面积     
//                dConArea = fabs(cvContourArea(pConInner, CV_WHOLE_SEQ));    
//                printf("%f\n", dConArea);     
//            }    
//          
//
//			CvBox2D  box2D = cvMinAreaRect2( pContour );
//	if(box2D.size.width>650 &&box2D.size.width<730)
//			{
//			 CvPoint2D32f point[4];
//			 int i;
//			 for ( i=0; i<4; i++)
//			 {
//			 point[i].x = 0;
//			 point[i].y = 0;
//			 }
//			  cvBoxPoints(box2D, point); //计算二维盒子顶点
//			 CvPoint pt[4];
//			 for ( i=0; i<4; i++)
//			 {
//			 pt[i].x = (int)point[i].x;
//			 pt[i].y = (int)point[i].y;
//			 }
//			 cvLine( pBinary, pt[0], pt[1],CV_RGB(0,0,0), 2, 8, 0 );
//			 cvLine( pBinary, pt[1], pt[2],CV_RGB(0,0,0), 2, 8, 0 );
//			 cvLine( pBinary, pt[2], pt[3],CV_RGB(0,0,0), 2, 8, 0 );
//			 cvLine( pBinary, pt[3], pt[0],CV_RGB(0,0,0), 2, 8, 0 );
//
//			   //旋转中心为图像中心  
//    CvPoint2D32f center;    
//    center.x=float (pBinary->width/2.0+0.5);  
//    center.y=float (pBinary->height/2.0+0.5);  
//    //计算二维旋转的仿射变换矩阵  
//    float m[6];              
//    CvMat M = cvMat( 2, 3, CV_32F, m );  
//	cv2DRotationMatrix( center, box2D.angle,1, &M);  
//    //变换图像，并用黑色填充其余值  
//    cvWarpAffine(pBin0,pBin0, &M,CV_INTER_LINEAR+CV_WARP_FILL_OUTLIERS,cvScalarAll(0) );  	  
//	 cvWarpAffine(pBinary,pBinary, &M,CV_INTER_LINEAR+CV_WARP_FILL_OUTLIERS,cvScalarAll(0) );  	 
//	
//	 CvMemStorage *pStorager = cvCreateMemStorage(0);    
//	 CvSeq *pContourr = NULL; 
//        cvFindContours(pBinary, pStorager, &pContourr, sizeof(CvContour), CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE);  
//		 // 填充所有轮廓     
//        cvDrawContours(pBinary, pContourr, CV_RGB(255, 255, 255), CV_RGB(255, 255, 255), 2, CV_FILLED, 8, cvPoint(0, 0));    
//		
//		for (; pContourr != NULL; pContourr = pContourr->h_next)    
//		{
//			CvRect rect = cvBoundingRect(pContourr,0);  
//			if(rect.width > box2D.size.width -50 &&  rect.width < box2D.size.width  +50)		
//			{				
//				IplImage* Img_dst = cvCreateImage(cvSize(rect.width,rect.height),IPL_DEPTH_8U,1);
//				myCutOut(pBin0,Img_dst,rect.x,rect.y,rect.width,rect.height);
//				cvSaveImage("D://1111.bmp",Img_dst);
//				 cvNamedWindow("imgq");    
//				cvShowImage("imgq", Img_dst); 
//				 cvWaitKey(-1);  			
//
//			}
//		}
//	
//			}
//
//
//
//			//cvMinAreaRect2(); 
//
//			//if(rect.height>400 && rect.height<650)
//            //cvRectangle(pBinary, cvPoint(rect.x, rect.y), cvPoint(rect.x + rect.width, rect.y + rect.height),CV_RGB(0,0, 0), 3, 8, 0);  
//        }     
//  
//        printf("wai = %d, nei = %d", wai, nei);    
//        cvReleaseMemStorage(&pStorage);     
//        pStorage = NULL;     
//    }     
//}  


IplImage * FillInternalContours(IplImage *pImage, IplImage *PBinaryTemp, int width,int height,ELrect *rec)     
{     
    double dConArea;     
    CvSeq *pContour = NULL;            
    CvMemStorage *pStorage = NULL; 	
    if (PBinaryTemp)     
    {     
        // 查找所有轮廓     
        pStorage = cvCreateMemStorage(0);     
        cvFindContours(PBinaryTemp, pStorage, &pContour, sizeof(CvContour), CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE);  		
        int wai = 0;    
        int nei = 0;  
		
		vector<CvBox2D> cvboxlist;
        for (; pContour != NULL; pContour = pContour->h_next)     
        {     
            wai++;  
			CvBox2D  box2D = cvMinAreaRect2( pContour );
			if(box2D.size.height>height-100 && box2D.size.width>height-200)
			cvboxlist.push_back(box2D);
			else{
			continue;
			}
			if((box2D.size.height>height-30 )&&(box2D.size.height<height+30)&& (box2D.size.width>width-30) &&(box2D.size.width<width+30)||(box2D.size.height>width-30 )&&(box2D.size.height<width+30)&& (box2D.size.width>height-30) &&(box2D.size.width<height+30))
			{
				if(box2D.angle < 0  && -box2D.angle  > 45)
				box2D.angle +=90; 
				 CvPoint2D32f point[4];
				 int i;
				 for ( i=0; i<4; i++)
				 {
				 point[i].x = 0;
				 point[i].y = 0;
				 }
				 cvBoxPoints(box2D, point); //计算二维盒子顶点
				 CvPoint pt[4];
				 for ( i=0; i<4; i++)
				 {
				 pt[i].x = (int)point[i].x;
				 pt[i].y = (int)point[i].y;
				 }		

				 cvLine( PBinaryTemp, pt[0], pt[1],CV_RGB(255,255,255), 2, 8, 0 );
				 cvLine( PBinaryTemp, pt[1], pt[2],CV_RGB(255,255,255), 2, 8, 0 );
				 cvLine( PBinaryTemp, pt[2], pt[3],CV_RGB(255,255,255), 2, 8, 0 );
				 cvLine( PBinaryTemp, pt[3], pt[0],CV_RGB(255,255,255), 2, 8, 0 );
				/*  cvNamedWindow("imgq");    
				cvShowImage("imgq", PBinaryTemp); 
				cvSaveImage("d:/234.bmp",PBinaryTemp);
				 cvWaitKey(-1);  */	
			    //旋转中心为图像中心  
				CvPoint2D32f center; 
				center.x=float ((pt[0].x + pt[3].x)/2.0+0.5);  
				center.y=float (pt[3].y = pt[3].y/2.0+0.5);  
				//计算二维旋转的仿射变换矩阵  
				float m[6];              
				CvMat M = cvMat( 2, 3, CV_32F, m );  
				cv2DRotationMatrix( center, box2D.angle,1, &M);  
				//变换图像，并用黑色填充其余值  			  
				cvWarpAffine(pImage,pImage, &M,CV_INTER_LINEAR+CV_WARP_FILL_OUTLIERS,cvScalarAll(0) ); 	
			/*	cvNamedWindow("aaaf");
				cvShowImage("aaaf",pImage);*/
				
				cvWaitKey(-1);
				IplImage* PBinaryTemp0 = cvCreateImage(cvSize(pImage->width, pImage->height), pImage->depth, 1);
				cvCvtColor(pImage, PBinaryTemp0, EL_CVT_COLOR2GRAY);			
				cvAdaptiveThreshold( PBinaryTemp0, PBinaryTemp0, 255,
						CV_ADAPTIVE_THRESH_MEAN_C ,
						CV_THRESH_BINARY ,
						41,
						8);


				CvMemStorage *pStorager = cvCreateMemStorage(0);    
				CvSeq *pContourr = NULL; 				
				cvFindContours(PBinaryTemp0, pStorager, &pContourr, sizeof(CvContour), CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE);
				/*cvDrawContours(PBinaryTemp0, pContourr, CV_RGB(255, 255, 255), CV_RGB(255, 255, 255), 2, 5, 8, cvPoint(0, 0));  				
				cvNamedWindow("aaadf");
				cvShowImage("aaadf",PBinaryTemp0);
				
				cvWaitKey(-1);*/
				cvReleaseImage(&PBinaryTemp0);
				for (; pContourr != NULL; pContourr = pContourr->h_next)    
				{
					CvRect rect = cvBoundingRect(pContourr,0);
					vector<CvRect>relist;
					if(rect.height >1400 && rect.width > 1400)
					{
						relist.push_back(rect);
					}

					if((((rect.height > height-30) && ( rect.height < height +30))&&((rect.width > width -30) &&  (rect.width < width +30)))  ||(((rect.height > width-30) && ( rect.height < width +30))&&((rect.width > height -30) && ( rect.width < height +30))))		
					{						
						CvRect cropRect = {rect.x, rect.y, rect.width,rect.height};
						if(rec != NULL)
						{
						rec->x = rect.x;
						rec->y = rect.y;
						rec->width = rect.height;
						rec->height = rect.height;
						}
						IplImage * imagetemp = cvCreateImage(cvSize(rect.width, rect.height), pImage->depth, pImage->nChannels);
						cvSetImageROI(pImage, cropRect);
						cvCopy(pImage, imagetemp);													  
						cvResetImageROI(pImage);				
						cvReleaseMemStorage(&pStorager);  
						cvReleaseMemStorage(&pStorage); 
						return imagetemp;			
					}
				}			
				cvReleaseMemStorage(&pStorager);  
			}
			int a ;
		 } 
        cvReleaseMemStorage(&pStorage);     
        pStorage = NULL;     
    }  
	return 0;
}  

int ELimageImpl::Find_BLackLineRectEdge(ELrect * rect,int width ,int height,int flag)
{
	ASSERT(NULL != m_imageImpl);   
	IplImage *imageTemp = cvCreateImage(cvGetSize(m_imageImpl), m_imageImpl->depth, 1);
	if (NULL == imageTemp)
	{
		elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(%s)", __FUNCTION__, "cvCreateImage");
		return -1;
	}

	cvCvtColor(m_imageImpl, imageTemp, EL_CVT_COLOR2GRAY);

	cvAdaptiveThreshold( imageTemp, imageTemp, 255,
			CV_ADAPTIVE_THRESH_MEAN_C ,
			CV_THRESH_BINARY ,
			41,
			8);
  
  
	IplImage* image  = NULL;
	image = FillInternalContours(m_imageImpl,imageTemp,width,height,rect);  
	if(image == NULL)
	{
		 cvReleaseImage(&imageTemp);   
		 return -1;
	}
    cvReleaseImage(&imageTemp);   
	cvReleaseImage(&m_imageImpl);   
	m_imageImpl = image;	
	return 0;
}

int ELimageImpl::Morphology(LONG x, LONG y, LONG w, LONG h)
{
	if(NULL == m_imageImpl)
		return -1;

	/*INVOCR ROI;
	RECT rc = {left, top, right, bottom};
	return ROI.Morphology(m_imageImpl, rc, 3, 1, true);*/

	CvRect rcROI = cvRect((int)x, (int)y, (int)w, (int)h);
	deleteBlockSpace(m_imageImpl,rcROI);
	
	return 0;
}
int ELimageImpl::Gamma( IplImage* imgSrc, float fGamma, IplImage* imgOut )
{
	if (NULL == imgSrc || NULL == imgOut || 
		imgSrc->nChannels != imgOut->nChannels ||
		imgSrc->width != imgOut->width ||
		imgSrc->height != imgOut->height)
	{
		return -1;
	}

	if (fGamma <= 0.0f) return -1;
	fGamma = 1/fGamma;

	unsigned char lut[256];  
	for( int i = 0; i < 256; i++ )  
	{  
		lut[i] = (unsigned char)(pow((float)(i/255.0), fGamma) * 255.0f);  
	} 

	//if (fGamma <= 0.0f) return false;
	//double dinvgamma = 1/fGamma;
	//double dMax = pow(255.0, dinvgamma) / 255.0;
	//int cTable[256]; //<nipper>
	//for (int i=0;i<256;i++)	
	//{
	//	cTable[i] = (int)max(0,min(255,(int)( pow((double)i, dinvgamma) / dMax)));
	//}

	int i = 0;
	int j = 0;
	uchar* pstart = NULL;

	if (1 == imgOut->nChannels)//灰度图
	{
		for(i=0; i<imgOut->height; i++)
		{
			pstart = (uchar*)(imgOut->imageData + i*imgOut->widthStep);
			for(j=0; j<imgOut->width; pstart++,j++)
				*pstart = lut[*pstart];	
		}
	}
	else if (imgOut->nChannels >=3 )// 彩色图
	{
		pstart = NULL;

		for(i=0; i<imgOut->height; i++)
		{
			pstart = (uchar*)(imgOut->imageData + i*imgOut->widthStep);
			for(j=0; j<imgOut->width*imgOut->nChannels; pstart++,j++)
				*pstart = lut[*pstart];	
		}
	}

	return 0;
}

int ELimageImpl::AutoShoot(BOOL isBgImg)
{
	elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(AutoPage Enter)", __FUNCTION__);
	
	Sleep(200);
	int iRet = -1;

	static bool bFirstImg = true;		//是否是第一帧前景图
	static bool bCapture=false;			//是否已经拍照
	static IplImage* imgFront = NULL;	//保留前一张图片
	static IplImage* imgBack = NULL;	//保留背景图片
	float fCount=0.0f;					//记录差别超过阈值的点数

	if(NULL == m_imageImpl)
	{
		return iRet;
	}

	if (NULL == m_imageImpl || 2 == m_imageImpl->nChannels || m_imageImpl->nChannels > 3)
	{
		return iRet;
	}

	if(isBgImg)
	{//获取背景图片
		if(imgBack)
		{
			cvReleaseImage(&imgBack);
			imgBack = NULL;
		}

		elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(imgBack)", __FUNCTION__);

		imgBack = cvCreateImage(cvSize(m_imageImpl->width,m_imageImpl->height), m_imageImpl->depth, 1);
		if (3 == m_imageImpl->nChannels)
		{
			cvCvtColor(m_imageImpl, imgBack,CV_BGR2GRAY);
		}
		else if (1 == m_imageImpl->nChannels)
		{
			cvCopy(m_imageImpl, imgBack);
		}

		bFirstImg=true;
		return iRet;
	}

	if (bFirstImg)
	{//获取前一张图
		elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(imgFront)", __FUNCTION__);
		if (imgFront)
		{
			cvReleaseImage(&imgFront);
			imgFront=NULL;
		}
		imgFront = cvCreateImage(cvSize(m_imageImpl->width,m_imageImpl->height), m_imageImpl->depth, 1);
		if (3 == m_imageImpl->nChannels)
		{
			cvCvtColor(m_imageImpl, imgFront,CV_BGR2GRAY);
		}
		else if (1 == m_imageImpl->nChannels)
		{
			cvCopy(m_imageImpl, imgFront);
		}
		bFirstImg=false;
		return iRet;
	}

	if (m_imageImpl->width != imgFront->width || m_imageImpl->height != imgFront->height)
	{//前一张图尺寸改变
		elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(SizeChanged)", __FUNCTION__);

		cvReleaseImage(&imgFront);
		imgFront = NULL;

		imgFront = cvCreateImage(cvSize(m_imageImpl->width,m_imageImpl->height), m_imageImpl->depth, 1);
		if (3 == m_imageImpl->nChannels)
		{
			cvCvtColor(m_imageImpl, imgFront,CV_BGR2GRAY);
		}
		else if (1 == m_imageImpl->nChannels)
		{
			cvCopy(m_imageImpl, imgFront);
		}
		return iRet;
	}
	
	//获取当前比较图像
	IplImage* imgGray = cvCreateImage(cvSize(m_imageImpl->width,m_imageImpl->height), m_imageImpl->depth, 1);
	if (3 == m_imageImpl->nChannels)
	{
		cvCvtColor(m_imageImpl, imgGray,CV_BGR2GRAY);
	}
	else if (1 == m_imageImpl->nChannels)
	{
		cvCopy(m_imageImpl, imgGray);
	}

	//和前一张图片进行比较
	for (int i=0;i<imgGray->width;i++)
	{
		for (int j=0;j<imgGray->height;j++)
		{
			if ( abs(CV_IMAGE_ELEM(imgGray,uchar,j,i)-CV_IMAGE_ELEM(imgFront,uchar,j,i)) >= 20  )
			{
				fCount++;
			}			
		}
	}
	static float preFrontDif=-1;
	float frontDif = fCount/(imgGray->width*imgGray->height);
	if (-1==preFrontDif)
	{
		preFrontDif=frontDif;
		cvCopy(imgGray, imgFront);
		cvReleaseImage(&imgGray);
		imgGray=NULL;
		return iRet;
	}
	elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(frontDif=%f,preFrontDif=%f)", __FUNCTION__,frontDif,preFrontDif);
	
	if (frontDif > 0.15f)
	{//正在翻页
		preFrontDif=frontDif;
		cvCopy(imgGray, imgFront);
		cvReleaseImage(&imgGray);
		imgGray=NULL;			
		bCapture=false;			//翻页后判断不是背景要拍照
		elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(Set bCapture=false)", __FUNCTION__);
		return iRet;//有差别，判断为正在翻动的过程
	}

	elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(frontDif-preFrontDif=%f)", __FUNCTION__,abs(frontDif-preFrontDif));
	if (abs(preFrontDif-frontDif)>0.005f)
	{//曝光还不稳定
		preFrontDif=frontDif;
		cvCopy(imgGray, imgFront);
		cvReleaseImage(&imgGray);
		imgGray=NULL;	
		elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(Exposure!)", __FUNCTION__);
		return iRet;
	}
	preFrontDif=frontDif;

	//如果背景存在,将当前图像和背景比较
	if (imgBack && imgGray->width==imgBack->width && imgGray->height == imgBack->height)
	{
		for (int i=0;i<imgGray->width;i++)
		{
			for (int j=0;j<imgGray->height;j++)
			{
				if ( abs(CV_IMAGE_ELEM(imgGray,uchar,j,i)-CV_IMAGE_ELEM(imgBack,uchar,j,i)) >= 20  )
				{
					fCount++;
				}			
			}
		}	
		float backDif=fCount/(imgGray->width*imgGray->height);
		elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(backDif=%f)", __FUNCTION__,backDif);
		if ( backDif < 0.05f)//认为是背景
		{//认为是背景图
			elWriteInfo(EL_INFO_LEVEL_ERROR, "It is background!");
			cvCopy(imgGray, imgBack);//更新背景图
			cvCopy(imgGray,imgFront);//更新前一张图
			cvReleaseImage(&imgGray);//释放当前比较图像
			imgGray=NULL;
			iRet=0;
			return iRet;
		}
	}
	else
	{
		elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(backImage Error)", __FUNCTION__);
	}

	if (!bCapture)
	{
		iRet = 1;
		bCapture=true;
	}

	cvCopy(imgGray, imgFront);
	cvReleaseImage(&imgGray);
	elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(Will Capture...)", __FUNCTION__);
	elWriteInfo(EL_INFO_LEVEL_ERROR, "(%s)(AutoPage Outer)", __FUNCTION__);
	return iRet;
}

int ELimageImpl::WhitePaperSeal( IplImage* imgSrc , IplImage* imgOut )
{
	if (NULL == imgSrc || NULL == imgOut || 
		imgSrc->nChannels != imgOut->nChannels ||
		imgSrc->width != imgOut->width ||
		imgSrc->height != imgOut->height ||
		imgSrc->nChannels < 3)
	{
		return -1;
	}

	Saturation(imgSrc , imgSrc, 0.3);//HSV  饱和度 不能设置的 过大，会颜色变化 太大。

	//1 自适应二值化留下目标
	IplImage* imgGray = cvCreateImage(cvSize(imgSrc->width,imgSrc->height), imgSrc->depth, 1);

	cvCvtColor(imgSrc,imgGray,CV_BGR2GRAY);
	cvAdaptiveThreshold(imgGray, imgGray,255,0,0,31,15);//51-->31

	uchar* pimgSrc = NULL;

	//3留下目标
	cvCopy(imgSrc, imgOut);

	//
	IplImage* hsv = cvCreateImage( cvGetSize(imgSrc), 8, 3 );
	cvCvtColor(imgSrc,hsv,CV_BGR2HSV);
	CvScalar s;


	uchar* pimgGray = NULL;
	for(int i=0;i<imgOut->height;i++)
	{
		pimgSrc = (uchar*)(imgOut->imageData + i*imgOut->widthStep);
		pimgGray = (uchar*)(imgGray->imageData + i*imgGray->widthStep); 

		for(int j=0; j<imgOut->width; j++)
		{
			s = cvGet2D(hsv, i, j);
			if ( 255==pimgGray[j] && (pimgSrc[j*3] + pimgSrc[j*3+1] + pimgSrc[j*3+2])/3 >30
				&& ! (((s.val[0] >= 0) && (s.val[0] <= 10) ||  (s.val[0] >= 156) && (s.val[0] <= 180) )
				&& (s.val[1] >= 128)/*43 old*/ 
				&& (s.val[2] >= 128) )/*46 old*/
				)
			{
				pimgSrc[j*3] = 255;
				pimgSrc[j*3+1] = 255;
				pimgSrc[j*3+2] = 255;
			}
		}
	}	

	//红色增强

	UINT8 * pInImgData;
	int iR = 0;
	const float fZoom = 1.3f;

	for (int i=0; i<imgOut->height; i++)
	{	
		for (int j=0; j<imgOut->width; j++)
		{
			pInImgData = (UINT8 *)imgOut->imageData + i*imgOut->widthStep+j*3;

			if (pInImgData[2] < 200  && pInImgData[2] > 40  //对于 低红色不进行红色增强，否则容易 出现 杂质红20180905
				&& pInImgData[2] >pInImgData[0]  && pInImgData[2] > pInImgData[1] )//红色分量判断
			{
				iR = pInImgData[2] * fZoom;//红色扩大
				if (iR >255)
				{
					iR = 255;
				}

				pInImgData[0] = pInImgData[0]/fZoom;//蓝色减小
				pInImgData[1] = pInImgData[1]/fZoom;//绿色减小
				pInImgData[2] = iR;//红色扩大
			}
		}
	}


	cv::Mat mt(imgOut);
	mt.convertTo(mt, -1, 1, -30);//降低亮度

	//4对比度增强	
	int cTable[256]; //<nipper>
	for (int i=0;i<256;i++)	{
		cTable[i] = (int)max(0,min(255,(int)((i-128)*2.2f + 128.5f)));//2.2->2.4
	}

	uchar* pstart = NULL;
	for(int i=0; i<imgOut->height; i++)
	{
		pstart = (uchar*)(imgOut->imageData + i*imgOut->widthStep);
		for(int j=0; j<imgOut->width*imgOut->nChannels; pstart++,j++)
			*pstart = cTable[*pstart];	
	}

	//resize 下 再还原,改善锯齿现象
	//IplImage* imgBig = cvCreateImage(cvSize(imgOut->width*2.1, imgOut->height*2.1), 
	//	imgOut->depth, imgOut->nChannels);
	//cvResize(imgOut, imgBig);
	//cvResize(imgBig, imgOut);
	//cvReleaseImage(&imgBig);

	cvReleaseImage(&imgGray);
	cvReleaseImage(&hsv);	
	DeleteBlack( imgOut, 0 );//去除周围黑边20181023 ADD
	return 0;
}

int ELimageImpl::Saturation( IplImage* imgSrc , IplImage* imgOut, float pos )
{
	if (NULL == imgSrc || NULL == imgOut || 
		imgSrc->nChannels != imgOut->nChannels ||
		imgSrc->width != imgOut->width ||
		imgSrc->height != imgOut->height ||
		imgSrc->nChannels<3)
	{
		return -1;
	}

	if(pos>3)
	{
		pos = 3;
	}
	else if (pos<-3)
	{
		pos = -3;
	}

	float fValue = 1.0f;
	if (pos >= 0)
	{
		fValue = pos + 1.0f;
	}
	else
	{	
		fValue = (4-abs(pos))*0.25f;
	}

	cvCopy(imgSrc, imgOut);
	cvCvtColor(imgOut, imgOut, CV_BGR2HSV);

	uchar* pstart = NULL;
	float h = 0.0f;
	float s = 0.0f;
	float v = 0.0f;
	int k = 0 ;
	for(int i=0;i<imgOut->height;i++)
	{
		pstart = (uchar*)(imgOut->imageData + i*imgOut->widthStep);
		for(int j=0;j<imgOut->width;j++)
		{
			k = j*3+1;
			if ( (uchar)pstart [k]*fValue <=255)
			{
				(uchar)pstart [k] = (uchar)pstart [k]*fValue;
			}
			else
			{
				(uchar)pstart [k] = 255;
			}
		}
	}

	cvCvtColor(imgOut,imgOut,CV_HSV2BGR);

	return 0;
}

int ELimageImpl::DeleteBlack( IplImage* cxImg, int blob )
{
	//blob=0 代表找黑色斑块，-1 代表找白色斑块
	if (NULL == cxImg || 3 != cxImg->nChannels)
	{
		return -1;
	}

	if( 0 != blob)
	{
		blob = 0;
	}

	IplImage *dst_gray = cvCreateImage(cvGetSize(cxImg),cxImg->depth,1);//灰度图
	cvCvtColor(cxImg,dst_gray,CV_BGR2GRAY);//得到灰度图

	IplImage* grayZero = cvCreateImage(cvGetSize(cxImg),cxImg->depth,1);//灰度图
	cvCopy(dst_gray, grayZero);

	cvSmooth(dst_gray,dst_gray,CV_GAUSSIAN,3,3);//这个有必要 能去除 杂质点
	cvAdaptiveThreshold(dst_gray, dst_gray,255,0,0,101,15);


	int x_Sign=0; 	
	int	i_Temp=0;
	int	x_Temp=0;
	int	y_Temp=0;

	int iTempLeft,iTempRight,iTempTop,iTempBot;
	iTempLeft=iTempRight=iTempTop=iTempBot=0;

	char*	p_Data;//BYTE -> char*
	p_Data=dst_gray->imageData;

	int	i_Wide=dst_gray->width; //取得原图的数据区宽度
	int i_Widebyte=dst_gray->widthStep/*(i_Wide+3)/4*4*/;
	int	i_Height=dst_gray->height; //取得原图的数据区高度	

	int *p_Temp=new int[i_Wide*i_Height];//开辟一个临时内存区由于标记从 0000开始 如 0000 11 222222 3
	memset(p_Temp,-1,i_Wide*i_Height*sizeof(int));

	int i = 0;
	int j = 0;
	int iStepSize=1000;//每次申请内存递增量
	int  i_ArrRowSize=iStepSize;//
	int  (*p_BlobPos)[5]=new int [i_ArrRowSize][5];
	if (p_BlobPos==NULL)
	{
		return -1;
	}

	const int i_Pos=-1;
	memset(p_BlobPos, i_Pos, sizeof(int)*i_ArrRowSize*5);

	char*	p_DataTemp = NULL;
	int iTempNum = 0;


	char* chZero = grayZero->imageData;
	char* chZeroTemp = NULL;
	for(j=0;j<i_Height;j++)	// 高度
	{ 
		chZeroTemp = chZero+j*i_Widebyte;
		p_DataTemp = p_Data+j*i_Widebyte;
		for( i=0;i<i_Wide;i++,chZeroTemp++,p_DataTemp++)	// 宽度
		{
			if (* ((BYTE*)chZeroTemp) < 20)
			{
				*p_DataTemp = 0;
			}
		}
	}
	cvReleaseImage(&grayZero);

	for(j=0;j<i_Height;j++)	// 高度
	{ 
		p_DataTemp = p_Data+j*i_Widebyte;
		for( i=0;i<i_Wide;i++,p_DataTemp++)	// 宽度
		{
			if(*p_DataTemp==blob)//若当前点为黑点 //
			{				
				if(j!=0 && i!=(i_Wide-1) && *(p_DataTemp-i_Widebyte+1)==blob)//右上 *(p_DataTemp-i_Widebyte+1)
				{
					iTempNum = *(p_Temp+(j-1)*i_Wide+i+1);
					*(p_Temp+(j)*i_Wide+i)=iTempNum;

					if (i<p_BlobPos[iTempNum][1])
					{
						p_BlobPos[iTempNum][1]=i;
					}
					else
					{
						if (i>p_BlobPos[iTempNum][2])
							p_BlobPos[iTempNum][2]=i;
					}

					if (j<p_BlobPos[iTempNum][3])
					{
						p_BlobPos[iTempNum][3]=j;
					}
					else
					{
						if (j>p_BlobPos[iTempNum][4])
							p_BlobPos[iTempNum][4]=j;	
					}			

					x_Temp=iTempNum;			
					//接下来的左前和左上不能同时都满足20171114 测试 没出现这种情况
					if(i!=0 && *(p_DataTemp-1)==blob && *(p_Temp+j*i_Wide+i-1)!=x_Temp)//左前
					{						
						y_Temp=*(p_Temp+j*i_Wide+i-1);//记录下来y_Temp	
						//						v_Join.Add(y_Temp);							

						iTempLeft =p_BlobPos[y_Temp][1];
						iTempRight =p_BlobPos[y_Temp][2];
						iTempTop =p_BlobPos[y_Temp][3];
						iTempBot =p_BlobPos[y_Temp][4];

						if((iTempRight-iTempLeft)*(iTempBot-iTempTop) <//20171230 增加判断
							(p_BlobPos[x_Temp][2]-p_BlobPos[x_Temp][1])*(p_BlobPos[x_Temp][4]-p_BlobPos[x_Temp][3]) )
						{
							p_BlobPos[y_Temp][0]=i_Pos;
							p_BlobPos[y_Temp][1]=i_Pos;
							p_BlobPos[y_Temp][2]=i_Pos;
							p_BlobPos[y_Temp][3]=i_Pos;
							p_BlobPos[y_Temp][4]=i_Pos;

							if (iTempLeft<p_BlobPos[x_Temp][1])
							{
								p_BlobPos[x_Temp][1]=iTempLeft;
							}

							if (iTempRight>p_BlobPos[x_Temp][2])
							{
								p_BlobPos[x_Temp][2]=iTempRight;
							}

							if (iTempTop<p_BlobPos[x_Temp][3])
							{
								p_BlobPos[x_Temp][3]=iTempTop;
							}

							if (iTempBot>p_BlobPos[x_Temp][4])
							{
								p_BlobPos[x_Temp][4]=iTempBot;	
							}

							for(int m=iTempTop;m<=iTempBot;m++)
								for(int n=iTempLeft;n<=iTempRight;n++)
								{
									if(*(p_Temp+(m)*i_Wide+n)==y_Temp)
									{
										*(p_Temp+(m)*i_Wide+n)=x_Temp;						
									}
								}
						}
						else
						{
							if (iTempLeft<p_BlobPos[x_Temp][1])
							{
								p_BlobPos[y_Temp][1]=iTempLeft;
							}
							else
							{
								p_BlobPos[y_Temp][1]=p_BlobPos[x_Temp][1];
							}

							if (iTempRight>p_BlobPos[x_Temp][2])
							{
								p_BlobPos[y_Temp][2]=iTempRight;
							}
							else
							{
								p_BlobPos[y_Temp][2]=p_BlobPos[x_Temp][2];
							}

							if (iTempTop<p_BlobPos[x_Temp][3])
							{
								p_BlobPos[y_Temp][3]=iTempTop;
							}
							else
							{
								p_BlobPos[y_Temp][3]=p_BlobPos[x_Temp][3];
							}

							if (iTempBot>p_BlobPos[x_Temp][4])
							{
								p_BlobPos[y_Temp][4]=iTempBot;	
							}
							else
							{
								p_BlobPos[y_Temp][4]=p_BlobPos[x_Temp][4];	
							}

							for(int m=p_BlobPos[x_Temp][3];m<=p_BlobPos[x_Temp][4];m++)
								for(int n=p_BlobPos[x_Temp][1];n<=p_BlobPos[x_Temp][2];n++)
								{
									if(*(p_Temp+(m)*i_Wide+n)==x_Temp)
									{
										*(p_Temp+(m)*i_Wide+n)=y_Temp;						
									}
								}

								p_BlobPos[x_Temp][0]=i_Pos;
								p_BlobPos[x_Temp][1]=i_Pos;
								p_BlobPos[x_Temp][2]=i_Pos;
								p_BlobPos[x_Temp][3]=i_Pos;
								p_BlobPos[x_Temp][4]=i_Pos;
						}

					}//end//左前
					else if(i!=0 && *(p_DataTemp-i_Widebyte-1)==blob && *(p_Temp+(j-1)*i_Wide+i-1)!=x_Temp)//左上
					{
						y_Temp=*(p_Temp+(j-1)*i_Wide+i-1);					
						//						v_Join.Add(y_Temp);	

						iTempLeft =p_BlobPos[y_Temp][1];
						iTempRight =p_BlobPos[y_Temp][2];
						iTempTop =p_BlobPos[y_Temp][3];
						iTempBot =p_BlobPos[y_Temp][4];

						if((iTempRight-iTempLeft)*(iTempBot-iTempTop) <//20171230 增加判断
							(p_BlobPos[x_Temp][2]-p_BlobPos[x_Temp][1])*(p_BlobPos[x_Temp][4]-p_BlobPos[x_Temp][3]) )
						{
							p_BlobPos[y_Temp][0]=i_Pos;
							p_BlobPos[y_Temp][1]=i_Pos;
							p_BlobPos[y_Temp][2]=i_Pos;
							p_BlobPos[y_Temp][3]=i_Pos;
							p_BlobPos[y_Temp][4]=i_Pos;

							if (iTempLeft<p_BlobPos[x_Temp][1])
							{
								p_BlobPos[x_Temp][1]=iTempLeft;
							}

							if (iTempRight>p_BlobPos[x_Temp][2])
							{
								p_BlobPos[x_Temp][2]=iTempRight;
							}

							if (iTempTop<p_BlobPos[x_Temp][3])
							{
								p_BlobPos[x_Temp][3]=iTempTop;
							}

							if (iTempBot>p_BlobPos[x_Temp][4])
							{
								p_BlobPos[x_Temp][4]=iTempBot;	
							}

							for(int m=iTempTop;m<=iTempBot;m++)
								for(int n=iTempLeft;n<=iTempRight;n++)
								{
									if(*(p_Temp+(m)*i_Wide+n)==y_Temp)
									{
										*(p_Temp+(m)*i_Wide+n)=x_Temp;									
									}
								}
						}
						else
						{
							if (iTempLeft<p_BlobPos[x_Temp][1])
							{
								p_BlobPos[y_Temp][1]=iTempLeft;
							}
							else
							{
								p_BlobPos[y_Temp][1]=p_BlobPos[x_Temp][1];
							}

							if (iTempRight>p_BlobPos[x_Temp][2])
							{
								p_BlobPos[y_Temp][2]=iTempRight;
							}
							else
							{
								p_BlobPos[y_Temp][2]=p_BlobPos[x_Temp][2];
							}

							if (iTempTop<p_BlobPos[x_Temp][3])
							{
								p_BlobPos[y_Temp][3]=iTempTop;
							}
							else
							{
								p_BlobPos[y_Temp][3]=p_BlobPos[x_Temp][3];
							}

							if (iTempBot>p_BlobPos[x_Temp][4])
							{
								p_BlobPos[y_Temp][4]=iTempBot;	
							}
							else
							{
								p_BlobPos[y_Temp][4]=p_BlobPos[x_Temp][4];	
							}

							for(int m=p_BlobPos[x_Temp][3];m<=p_BlobPos[x_Temp][4];m++)
								for(int n=p_BlobPos[x_Temp][1];n<=p_BlobPos[x_Temp][2];n++)
								{
									if(*(p_Temp+(m)*i_Wide+n)==x_Temp)
									{
										*(p_Temp+(m)*i_Wide+n)=y_Temp;						
									}
								}

								p_BlobPos[x_Temp][0]=i_Pos;
								p_BlobPos[x_Temp][1]=i_Pos;
								p_BlobPos[x_Temp][2]=i_Pos;
								p_BlobPos[x_Temp][3]=i_Pos;
								p_BlobPos[x_Temp][4]=i_Pos;		

						}

					}//end//左上
				}
				else if(j!=0 && *(p_DataTemp-i_Widebyte)==blob)//正上
				{
					iTempNum = *(p_Temp+(j-1)*i_Wide+i);
					*(p_Temp+j*i_Wide+i)=iTempNum;

					if (i<p_BlobPos[iTempNum][1])
					{
						p_BlobPos[iTempNum][1]=i;
					}
					else
					{
						if (i>p_BlobPos[iTempNum][2])
							p_BlobPos[iTempNum][2]=i;
					}

					if (j<p_BlobPos[iTempNum][3])
					{
						p_BlobPos[iTempNum][3]=j;
					}
					else
					{
						if (j>p_BlobPos[iTempNum][4])
							p_BlobPos[iTempNum][4]=j;	
					}


					x_Temp=iTempNum;				
				}
				else if(j!=0 && i!=0 && *(p_DataTemp-i_Widebyte-1)==blob)//左上
				{
					iTempNum = *(p_Temp+(j-1)*i_Wide+i-1);
					*(p_Temp+(j)*i_Wide+i)=iTempNum;

					if (i<p_BlobPos[iTempNum][1])
					{
						p_BlobPos[iTempNum][1]=i;
					}
					else
					{
						if (i>p_BlobPos[iTempNum][2])
							p_BlobPos[iTempNum][2]=i;
					}

					if (j<p_BlobPos[iTempNum][3])
					{
						p_BlobPos[iTempNum][3]=j;
					}
					else
					{
						if (j>p_BlobPos[iTempNum][4])
							p_BlobPos[iTempNum][4]=j;	
					}				

					x_Temp=iTempNum;				
				}
				else if( i!=0 && *(p_DataTemp-1)==blob)//左前
				{
					iTempNum = *(p_Temp+(j)*i_Wide+i-1);
					*(p_Temp+(j)*i_Wide+i)=iTempNum;

					if (i<p_BlobPos[iTempNum][1])
					{
						p_BlobPos[iTempNum][1]=i;
					}
					else
					{
						if (i>p_BlobPos[iTempNum][2])
							p_BlobPos[iTempNum][2]=i;
					}

					if (j<p_BlobPos[iTempNum][3])
					{
						p_BlobPos[iTempNum][3]=j;
					}
					else
					{
						if (j>p_BlobPos[iTempNum][4])
							p_BlobPos[iTempNum][4]=j;	
					}

					x_Temp=iTempNum;	
				}
				else//没有
				{
					i_Temp=x_Sign;
					++x_Sign;

					if (x_Sign<=i_ArrRowSize)
					{				
						p_BlobPos[i_Temp][0]=i_Temp;
						p_BlobPos[i_Temp][1]=i;
						p_BlobPos[i_Temp][2]=i;
						p_BlobPos[i_Temp][3]=j;
						p_BlobPos[i_Temp][4]=j;
					}
					else
					{	
						//申请内存					
						if (i_ArrRowSize>100000)
						{
							delete[] p_Temp;                    
							delete[] p_BlobPos; 
							return -2;
						}

						int *iArrayTemp=new int[i_ArrRowSize*5];//保存数据							
						memcpy(iArrayTemp,p_BlobPos,5*i_ArrRowSize*sizeof(int));							

						delete[] p_BlobPos; 
						p_BlobPos=NULL;

						i_ArrRowSize+=iStepSize;
						p_BlobPos = new int[i_ArrRowSize][5];

						if (p_BlobPos==NULL)
						{
							return -1;
						}							

						memset(p_BlobPos, i_Pos, 5*i_ArrRowSize*sizeof(int));

						memcpy(p_BlobPos, iArrayTemp, 5*(i_ArrRowSize-iStepSize)*sizeof(int));			

						delete[] iArrayTemp;
						iArrayTemp=NULL;

						p_BlobPos[i_Temp][0]=i_Temp;
						p_BlobPos[i_Temp][1]=i;
						p_BlobPos[i_Temp][2]=i;
						p_BlobPos[i_Temp][3]=j;
						p_BlobPos[i_Temp][4]=j;
					}
					*(p_Temp+(j)*i_Wide+i)=i_Temp;

				}			
			}// 每列
		}//end 每行	
	}	

	char* pimgColorData = cxImg->imageData;
	char* pColorTemp = NULL;
	int i_WidebyteColor = cxImg->widthStep;

	int i_Total=0;
	for (int i_T=0;i_T<i_ArrRowSize;i_T++)
	{
		if (p_BlobPos[i_T][0]!=i_Pos)
		{
			if (0 == p_BlobPos[i_T][1] ||
				0 == p_BlobPos[i_T][3] ||
				i_Height - 1 == p_BlobPos[i_T][4])
			{				
				for (int n=p_BlobPos[i_T][3]; n<=p_BlobPos[i_T][4]; n++)
					for (int m=p_BlobPos[i_T][1]; m<=p_BlobPos[i_T][2]; m++)					
					{
						if (p_BlobPos[i_T][0] == p_Temp[n*i_Wide+m])
						{
							pColorTemp = pimgColorData+n*i_WidebyteColor+m*3;
							*pColorTemp = 255;
							*(++pColorTemp) = 255;
							*(++pColorTemp) = 255;
						}
					}				
			}
		}
	}

	cvReleaseImage(&dst_gray);
	delete[] p_Temp;
	p_Temp = NULL;
	delete[] p_BlobPos;
	p_BlobPos = NULL;

	return 0;
}

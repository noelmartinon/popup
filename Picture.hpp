#include <windows.h>
#include <olectl.h>
#include <Wincodec.h> //IWICBitmapSource

#ifdef USE_GDIPLUS
    #include <gdiplus.h>
    #pragma comment (lib, "gdiplus.lib")
#endif

#pragma comment (lib, "Windowscodecs") //WICConvertBitmapSource

#ifndef max
    #define max(a,b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
    #define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

HBITMAP __stdcall ResourceToBitmap(HMODULE hModule, LPCTSTR lpName, LPCTSTR lpType);
HBITMAP __stdcall FileToBitmap(LPCTSTR lpFileName);
HBITMAP __stdcall PictureToBitmap(LPBYTE pmem, DWORD nSize);
bool SaveBMPFile(char *filename, HBITMAP bitmap, HDC bitmapDC, int width, int height);
HBITMAP __stdcall CreateBitmapMask(HBITMAP hbmColour, COLORREF crTransparent);
void DrawTransparent(HDC hdc, int x, int y, HBITMAP hBitmap, COLORREF crColour);
HBITMAP ReplaceColor(HBITMAP hBmp, HDC hBmpDC, COLORREF cOldColor, COLORREF cNewColor, COLORREF cTolerance);
double GetBitmapLuminance(HBITMAP hBmp, HDC hBmpDC, HRGN hRgn);
HRGN BitmapToRegion(HBITMAP hBmp, HDC hBmpDC, COLORREF cTransparentColor, COLORREF cTolerance);
HBITMAP ResizeBmp(HBITMAP hBmpSrc, SIZE newSize);
HBITMAP LoadBitmapFromICOFile(LPCTSTR lpFileName);

// For PNG image format :
IStream * CreateStreamOnResource(LPCTSTR lpName, LPCTSTR lpType);
IWICBitmapSource * LoadBitmapFromStream(IStream * ipImageStream);
HBITMAP CreateHBITMAP(IWICBitmapSource * ipBitmap);
HBITMAP LoadPNGResource(LPCTSTR lpName, LPCTSTR lpType);
IStream * CreateStreamOnFile(LPCTSTR pszPathName);
HBITMAP LoadPNGFile(LPCTSTR lpName);

//  ===========================================================================
//  Create a new picture object from a resource that is in bmp, wmf, ico, gif, jpg or png format
//  ===========================================================================
HBITMAP __stdcall ResourceToBitmap(HMODULE hModule, LPCTSTR lpName, LPCTSTR lpType)
{
    HGLOBAL hgbl;
    HRSRC hRsrc;

    hRsrc = FindResource(hModule, lpName, lpType);
    hgbl = LoadResource(hModule, hRsrc);
    if(hgbl)
    {
        HBITMAP hbmp = PictureToBitmap((BYTE*)LockResource(hgbl), SizeofResource(hModule, hRsrc));
        FreeResource(hgbl);
        return hbmp;
    }
    return 0;
}

//  ===========================================================================
//  Create a new picture object from the contents of a file that is in bmp, wmf, ico, gif, jpg or png format
//  ===========================================================================
#ifdef USE_GDIPLUS
// GDI+ has built-in encoders and decoders that support the following file types: BMP, GIF, JPEG, PNG, TIFF
// GDI+ also has built-in decoders that support the following file types: WMF, EMF, ICON
HBITMAP __stdcall FileToBitmap(LPCTSTR lpFileName)
{
    HBITMAP hbmp = NULL;

    const size_t cSize = strlen(lpFileName)+1;
    wchar_t* wcs = new wchar_t[cSize];
    mbstowcs (wcs, lpFileName, cSize);

    // Initialize GDI+
    static Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    static ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    // Need to use 'new' operator instead of 'Gdiplus::Bitmap bitmap(wcs, false);'
    // else bitmap stay in memory and GdiplusShutdown generates an error "abnormal program termination"

    Gdiplus::Bitmap *bitmap = new Gdiplus::Bitmap(wcs, false);
    bitmap->GetHBITMAP(0, &hbmp);
    delete bitmap;
    delete[] wcs;

    // Release GDI+
    Gdiplus::GdiplusShutdown(gdiplusToken);

    return hbmp;
}
#else
HBITMAP __stdcall FileToBitmap(LPCTSTR lpFileName)
{
    HBITMAP hbmp = 0;
    HANDLE hProcessHeap = GetProcessHeap();
    HANDLE hFile = CreateFile(lpFileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if(hFile == INVALID_HANDLE_VALUE) return 0;
    DWORD dwFileSize = GetFileSize(hFile, 0), dwRead;
    LPVOID lpMem = HeapAlloc(hProcessHeap, 0, dwFileSize);

    if(ReadFile(hFile, lpMem, dwFileSize, &dwRead, 0) && dwRead == dwFileSize)
        hbmp = PictureToBitmap((LPBYTE)lpMem, dwFileSize);

    HeapFree(hProcessHeap, 0, lpMem);
    CloseHandle(hFile);

    // Try icon format
    if (!hbmp) hbmp=LoadBitmapFromICOFile(lpFileName);
    
    /*if (hbmp){
        BITMAP bm;
        GetObject(hbmp, sizeof(BITMAP), &bm);
        char str[128];
        sprintf(str, "%ld x %ld", bm.bmWidth, bm.bmHeight);
        MessageBox(NULL, str, "Debug - Info FileToBitmap()", MB_ICONINFORMATION | MB_OK);
    }
    */
            
    return hbmp;
}
#endif

//  ===========================================================================
//  Create a new picture object from the contents of a stream that must be in bmp, wmf, ico, gif, jpg or png format
//  ===========================================================================
HBITMAP __stdcall PictureToBitmap(LPBYTE pmem, DWORD nSize)
{
    HRESULT hr;
    CoInitialize(0);
    HBITMAP hbmp_dst = 0;
    HGLOBAL hgbl =(HGLOBAL)GlobalAlloc(GMEM_FIXED, nSize);
  
    memcpy(hgbl, pmem, nSize);
  
    IStream* stream = 0;
    hr = CreateStreamOnHGlobal(hgbl, TRUE, &stream);
    if(!SUCCEEDED(hr) || !stream)
    {
        stream->Release();
        GlobalFree(hgbl);
        CoUninitialize();
        return hbmp_dst;
    }
  
    IPicture* picture = 0;
    hr = OleLoadPicture(stream, nSize, 0, IID_IPicture, (void**)&picture);
    if(!SUCCEEDED(hr) || !picture)
    {
        // Object does not support the IID_IPicture specified interface
        // so trying PNG format (IWICBitmapDecoder() with CLSID_WICPngDecoder)

        // load the bitmap with WIC
        IWICBitmapSource * ipBitmap = LoadBitmapFromStream(stream);
        if (ipBitmap == NULL)
        {
            stream->Release();
            GlobalFree(hgbl);  
            return NULL;
        }

        // create a HBITMAP containing the image
        hbmp_dst = CreateHBITMAP(ipBitmap);
        ipBitmap->Release();

        stream->Release();
        GlobalFree(hgbl);
        CoUninitialize();
        return hbmp_dst;
    }
  
    HBITMAP hbmp_src;
    picture->get_Handle((OLE_HANDLE *)&hbmp_src);
    if(!SUCCEEDED(hr) || !picture)
    {
        picture->Release();
        stream->Release();
        GlobalFree(hgbl);
        CoUninitialize();
        return hbmp_dst;
    }
  
    BITMAP bmp;
    GetObject(hbmp_src, sizeof bmp, &bmp);
    hbmp_dst = (HBITMAP)CopyImage(hbmp_src, IMAGE_BITMAP, 0, 0, 0);
  
    picture->Release();
    stream->Release();
    GlobalFree(hgbl);
    CoUninitialize();
    return hbmp_dst;
}

//  ===========================================================================
//  Save an HBITMAP to a BMP file
//  ===========================================================================
bool SaveBMPFile(char *filename, HBITMAP bitmap, HDC bitmapDC, int width, int height){
	bool Success=0;
	HDC SurfDC=NULL;
	HBITMAP OffscrBmp=NULL;
	HDC OffscrDC=NULL;
	LPBITMAPINFO lpbi=NULL;
	LPVOID lpvBits=NULL;
	HANDLE BmpFile=INVALID_HANDLE_VALUE;
	BITMAPFILEHEADER bmfh;
	if ((OffscrBmp = CreateCompatibleBitmap(bitmapDC, width, height)) == NULL)
		return 0;
	if ((OffscrDC = CreateCompatibleDC(bitmapDC)) == NULL)
		return 0;
	HBITMAP OldBmp = (HBITMAP)SelectObject(OffscrDC, OffscrBmp);
	BitBlt(OffscrDC, 0, 0, width, height, bitmapDC, 0, 0, SRCCOPY);
	if ((lpbi = (LPBITMAPINFO)(new char[sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD)])) == NULL) 
		return 0;
	ZeroMemory(&lpbi->bmiHeader, sizeof(BITMAPINFOHEADER));
	lpbi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	SelectObject(OffscrDC, OldBmp);
	if (!GetDIBits(OffscrDC, OffscrBmp, 0, height, NULL, lpbi, DIB_RGB_COLORS))
		return 0;
	if ((lpvBits = new char[lpbi->bmiHeader.biSizeImage]) == NULL)
		return 0;
	if (!GetDIBits(OffscrDC, OffscrBmp, 0, height, lpvBits, lpbi, DIB_RGB_COLORS))
		return 0;
	if ((BmpFile = CreateFile(filename,
          		GENERIC_WRITE,
          		0, NULL,
          		CREATE_ALWAYS,
          		FILE_ATTRIBUTE_NORMAL,
          		NULL)) == INVALID_HANDLE_VALUE)
		return 0;
	DWORD Written;
	bmfh.bfType = 19778;
	bmfh.bfReserved1 = bmfh.bfReserved2 = 0;
	if (!WriteFile(BmpFile, &bmfh, sizeof(bmfh), &Written, NULL))
		return 0;
	if (Written < sizeof(bmfh)) 
		return 0; 
	if (!WriteFile(BmpFile, &lpbi->bmiHeader, sizeof(BITMAPINFOHEADER), &Written, NULL)) 
		return 0;
	if (Written < sizeof(BITMAPINFOHEADER)) 
		return 0;
	int PalEntries;
	if (lpbi->bmiHeader.biCompression == BI_BITFIELDS) 
		PalEntries = 3;
	else PalEntries = (lpbi->bmiHeader.biBitCount <= 8) ?
					  (int)(1 << lpbi->bmiHeader.biBitCount) : 0;
	if(lpbi->bmiHeader.biClrUsed) 
	PalEntries = lpbi->bmiHeader.biClrUsed;
	if(PalEntries){
	if (!WriteFile(BmpFile, &lpbi->bmiColors, PalEntries * sizeof(RGBQUAD), &Written, NULL)) 
		return 0;
		if (Written < PalEntries * sizeof(RGBQUAD)) 
			return 0;
	}
	bmfh.bfOffBits = SetFilePointer(BmpFile, 0, 0, FILE_CURRENT);
	if (!WriteFile(BmpFile, lpvBits, lpbi->bmiHeader.biSizeImage, &Written, NULL)) 
		return 0;
	if (Written < lpbi->bmiHeader.biSizeImage) 
		return 0;
	bmfh.bfSize = SetFilePointer(BmpFile, 0, 0, FILE_CURRENT);
	SetFilePointer(BmpFile, 0, 0, FILE_BEGIN);
	if (!WriteFile(BmpFile, &bmfh, sizeof(bmfh), &Written, NULL))
		return 0;
	if (Written < sizeof(bmfh)) 
		return 0;
	CloseHandle(BmpFile);
	return 1;
}

//  ===========================================================================
//  Create an HBITMAP mask from an existing HBITMAP
//  ===========================================================================
HBITMAP __stdcall CreateBitmapMask(HBITMAP hbmColour, COLORREF crTransparent)
{
    HDC hdcMem, hdcMem2;
    HBITMAP hbmMask;
    BITMAP bm;

    // Create monochrome (1 bit) mask bitmap.
    GetObject(hbmColour, sizeof(BITMAP), &bm);
    hbmMask = CreateBitmap(bm.bmWidth, bm.bmHeight, 1, 1, NULL);

    // Get some HDCs that are compatible with the display driver
    hdcMem = CreateCompatibleDC(0);
    hdcMem2 = CreateCompatibleDC(0);

    SelectObject(hdcMem, hbmColour);
    SelectObject(hdcMem2, hbmMask);

    // Set the background colour of the colour image to the colour
    // you want to be transparent.
    SetBkColor(hdcMem, crTransparent);

    // Copy the bits from the colour image to the B+W mask... everything
    // with the background colour ends up white while everythig else ends up
    // black...Just what we wanted.
    BitBlt(hdcMem2, 0, 0, bm.bmWidth, bm.bmHeight, hdcMem, 0, 0, SRCCOPY);

    // Take our new mask and use it to turn the transparent colour in our
    // original colour image to black so the transparency effect will
    // work right.
    BitBlt(hdcMem, 0, 0, bm.bmWidth, bm.bmHeight, hdcMem2, 0, 0, SRCINVERT);

    // Clean up.
    DeleteDC(hdcMem);
    DeleteDC(hdcMem2);

    return hbmMask;
}

//  ===========================================================================  
//  Draw an HBITMAP to a device context according to a color of transparency
//  ===========================================================================
void DrawTransparent(HDC hdc, int x, int y, HBITMAP hBitmap, COLORREF crColour)
{
	COLORREF crOldBack = SetBkColor(hdc, RGB(255, 255, 255));
	COLORREF crOldText = SetTextColor(hdc, RGB(0, 0, 0));
	HDC dcImage, dcTrans;

	// Create two memory dcs for the image and the mask
	dcImage=CreateCompatibleDC(hdc);
	dcTrans=CreateCompatibleDC(hdc);

	// Select the image into the appropriate dc
	HBITMAP pOldBitmapImage = (HBITMAP)SelectObject(dcImage, hBitmap);

	// Create the mask bitmap
	BITMAP bitmap;
	GetObject(hBitmap, sizeof(BITMAP), &bitmap);
	HBITMAP bitmapTrans=CreateBitmap(bitmap.bmWidth, bitmap.bmHeight, 1, 1, NULL);

	// Select the mask bitmap into the appropriate dc
	HBITMAP pOldBitmapTrans = (HBITMAP)SelectObject(dcTrans, bitmapTrans);

	// Build mask based on transparent colour
	SetBkColor(dcImage, crColour);
	BitBlt(dcTrans, 0, 0, bitmap.bmWidth, bitmap.bmHeight, dcImage, 0, 0, SRCCOPY);

	// Do the work - True Mask method - cool if not actual display
	BitBlt(hdc, x, y, bitmap.bmWidth, bitmap.bmHeight, dcImage, 0, 0, SRCINVERT);
	BitBlt(hdc, x, y, bitmap.bmWidth, bitmap.bmHeight, dcTrans, 0, 0, SRCAND);
	BitBlt(hdc, x, y, bitmap.bmWidth, bitmap.bmHeight, dcImage, 0, 0, SRCINVERT);

	// Restore settings
	SelectObject(dcImage, pOldBitmapImage);
	SelectObject(dcTrans, pOldBitmapTrans);
	SetBkColor(hdc, crOldBack);
	SetTextColor(hdc, crOldText);
}

//  ===========================================================================
//  ReplaceColor
//
//  Author    : Dimitri Rochette drochette@coldcat.fr
//              Noël Martinon for the addition of "cTolerance", 2018
//  Specials Thanks to Joe Woodbury for his comments and code corrections
//
//  Includes  : Only <windows.h>
//
//  hBmp         : Source Bitmap
//  cOldColor : Color to replace in hBmp
//  cNewColor : Color used for replacement
//  hBmpDC    : DC of hBmp ( could be NULL if hBmp is not selected )
//
//  Retcode   : HBITMAP of the modified bitmap or NULL for errors
//
//  ===========================================================================
#define COLORREF2RGB(Color) (Color & 0xff00) | ((Color >> 16) & 0xff) \
                                 | ((Color << 16) & 0xff0000)

HBITMAP ReplaceColor(HBITMAP hBmp, HDC hBmpDC, COLORREF cOldColor, COLORREF cNewColor, COLORREF cTolerance = 0x101010)
{
    HBITMAP RetBmp=NULL;
    if (hBmp)
    {
        HDC BufferDC=CreateCompatibleDC(NULL);    // DC for Source Bitmap
        if (BufferDC)
        {
            HBITMAP hTmpBitmap = (HBITMAP) NULL;
            if (hBmpDC)
                if (hBmp == (HBITMAP)GetCurrentObject(hBmpDC, OBJ_BITMAP))
                {
                    hTmpBitmap = CreateBitmap(1, 1, 1, 1, NULL);
                    SelectObject(hBmpDC, hTmpBitmap);
                }

            HGDIOBJ PreviousBufferObject=SelectObject(BufferDC,hBmp);
            // here BufferDC contains the bitmap
            
            HDC DirectDC=CreateCompatibleDC(NULL); // DC for working
            if (DirectDC)
            {
                // Get bitmap size
                BITMAP bm;
                GetObject(hBmp, sizeof(bm), &bm);

                // create a BITMAPINFO with minimal initilisation 
                // for the CreateDIBSection
                BITMAPINFO RGB32BitsBITMAPINFO; 
                ZeroMemory(&RGB32BitsBITMAPINFO,sizeof(BITMAPINFO));
                RGB32BitsBITMAPINFO.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
                RGB32BitsBITMAPINFO.bmiHeader.biWidth=bm.bmWidth;
                RGB32BitsBITMAPINFO.bmiHeader.biHeight=bm.bmHeight;
                RGB32BitsBITMAPINFO.bmiHeader.biPlanes=1;
                RGB32BitsBITMAPINFO.bmiHeader.biBitCount=32;

                // pointer used for direct Bitmap pixels access
                UINT * ptPixels;    

                HBITMAP DirectBitmap = CreateDIBSection(DirectDC,
                                       (BITMAPINFO *)&RGB32BitsBITMAPINFO, 
                                       DIB_RGB_COLORS,
                                       (void **)&ptPixels, 
                                       NULL, 0);
                if (DirectBitmap)
                {
                    // here DirectBitmap!=NULL so ptPixels!=NULL no need to test
                    HGDIOBJ PreviousObject=SelectObject(DirectDC, DirectBitmap);
                    BitBlt(DirectDC,0,0,
                                   bm.bmWidth,bm.bmHeight,
                                   BufferDC,0,0,SRCCOPY);

                    // here the DirectDC contains the bitmap

                    // Convert COLORREF to RGB (Invert RED and BLUE)
                    cOldColor=COLORREF2RGB(cOldColor);
                    cNewColor=COLORREF2RGB(cNewColor);

                    // Keep on hand highest and lowest values for the "transparent" pixels
					BYTE lr = GetRValue(cOldColor);
					BYTE lg = GetGValue(cOldColor);
					BYTE lb = GetBValue(cOldColor);
                    BYTE hr = min(0xff, lr + GetRValue(cTolerance));
					BYTE hg = min(0xff, lg + GetGValue(cTolerance));
					BYTE hb = min(0xff, lb + GetBValue(cTolerance));

					COLORREF cToleranceColor=RGB(hr, hg, hb);

                    // After all the inits we can do the job : Replace Color
                    for (int i=((bm.bmWidth*bm.bmHeight)-1);i>=0;i--)
                    {
                        if (ptPixels[i]>=cOldColor && GetRValue(ptPixels[i])<=hr && GetGValue(ptPixels[i])<=hg && GetBValue(ptPixels[i])<=hb)
                        ptPixels[i]=cNewColor;
                    }
                    // little clean up
                    // Don't delete the result of SelectObject because it's 
                    // our modified bitmap (DirectBitmap)
                       SelectObject(DirectDC,PreviousObject);

                    // finish
                    RetBmp=DirectBitmap;
                }
                // clean up
                DeleteDC(DirectDC);
            }
            if (hTmpBitmap)
            {
                SelectObject(hBmpDC, hBmp);
                DeleteObject(hTmpBitmap);
            }
            SelectObject(BufferDC,PreviousBufferObject);
            // BufferDC is now useless
            DeleteDC(BufferDC);
        }
    }
    return RetBmp;
}

//  ===========================================================================
//  Get luminance of a bitmap optionally according to a region
//  ===========================================================================
double GetBitmapLuminance(HBITMAP hBmp, HDC hBmpDC, HRGN hRgn=NULL)
{
    double luminanceY=0;
    double nbPixels=0;
    COLORREF crPix;

    HDC hMemDC = (!hBmpDC)?CreateCompatibleDC(NULL):hBmpDC;
    SelectObject(hMemDC, hBmp);

    BITMAP bm;
    GetObject(hBmp, sizeof(bm), &bm);
    long width = bm.bmWidth;
    long height = bm.bmHeight;

    for (int xx=0; xx<width; xx++)
        for (int yy=0; yy<height; yy++)
        {
            if (hRgn && !PtInRegion(hRgn, xx, yy)) continue;
            crPix = GetPixel(hMemDC,xx,yy);
            luminanceY += ((double)0.299 * GetRValue(crPix) +
                                (double)0.587 * GetGValue(crPix) +
                                (double)0.114 * GetBValue(crPix)) / 255; // see https://en.wikipedia.org/wiki/HSL_and_HSV#Lightness
            nbPixels++;
        }


    if (!hBmpDC) DeleteDC(hMemDC);
    return (nbPixels>0)?luminanceY/nbPixels:0;

}

//  ===========================================================================
//	BitmapToRegion :	Create a region from the "non-transparent" pixels of a bitmap
//	Author :			Jean-Edouard Lachand-Robert, June 1998.
//                      Noël Martinon for the addition of "hBmpDC" and correction of "pixels comparison GetxValue()", 2018
//
//	hBmp :				Source bitmap
//  hBmpDC :            Source hdc of the bitmap when hBmp is created with CreateBitmap(). Not needed with LoadBitmap()
//	cTransparentColor :	Color base for the "transparent" pixels (default is black)
//	cTolerance :		Color tolerance for the "transparent" pixels.
//
//	A pixel is assumed to be transparent if the value of each of its 3 components (blue, green and red) is
//	greater or equal to the corresponding value in cTransparentColor and is lower or equal to the
//	corresponding value in cTransparentColor + cTolerance.
//  ===========================================================================
HRGN BitmapToRegion (HBITMAP hBmp, HDC hBmpDC = NULL, COLORREF cTransparentColor = 0, COLORREF cTolerance = 0x101010)
{
	HRGN hRgn = NULL;

	if (hBmp)
	{
		// Create a memory DC inside which we will scan the bitmap content
		HDC hMemDC = (!hBmpDC)?CreateCompatibleDC(NULL):hBmpDC;
		if (hMemDC)
		{
			// Get bitmap size
			BITMAP bm;
			GetObject(hBmp, sizeof(bm), &bm); 

			// Create a 32 bits depth bitmap and select it into the memory DC 
			BITMAPINFOHEADER RGB32BITSBITMAPINFO = {	
					sizeof(BITMAPINFOHEADER),	// biSize 
					bm.bmWidth,					// biWidth; 
					bm.bmHeight,				// biHeight; 
					1,							// biPlanes;
					32,							// biBitCount 
					BI_RGB,						// biCompression; 
					0,							// biSizeImage; 
					0,							// biXPelsPerMeter;
					0,							// biYPelsPerMeter; 
					0,							// biClrUsed; 
					0							// biClrImportant; 
			};
			VOID * pbits32; 
			HBITMAP hbm32 = CreateDIBSection(hMemDC, (BITMAPINFO *)&RGB32BITSBITMAPINFO, DIB_RGB_COLORS, &pbits32, NULL, 0);
			if (hbm32)
			{
				HBITMAP holdBmp = (HBITMAP)SelectObject(hMemDC, hbm32);

				// Create a DC just to copy the bitmap into the memory DC
				HDC hDC = CreateCompatibleDC(hMemDC);
				if (hDC)
				{
					// Get how many bytes per row we have for the bitmap bits (rounded up to 32 bits)
					BITMAP bm32;
					GetObject(hbm32, sizeof(bm32), &bm32);
					while (bm32.bmWidthBytes % 4)
						bm32.bmWidthBytes++;

					// Copy the bitmap into the memory DC
					HBITMAP holdBmp = (HBITMAP)SelectObject(hDC, hBmp);
					BitBlt(hMemDC, 0, 0, bm.bmWidth, bm.bmHeight, hDC, 0, 0, SRCCOPY);

					// For better performances, we will use the ExtCreateRegion() function to create the
					// region. This function take a RGNDATA structure on entry. We will add rectangles by
					// amount of ALLOC_UNIT number in this structure.
					#define ALLOC_UNIT	100
					DWORD maxRects = ALLOC_UNIT;
					HANDLE hData = GlobalAlloc(GMEM_MOVEABLE, sizeof(RGNDATAHEADER) + (sizeof(RECT) * maxRects));
					RGNDATA *pData = (RGNDATA *)GlobalLock(hData);
					pData->rdh.dwSize = sizeof(RGNDATAHEADER);
					pData->rdh.iType = RDH_RECTANGLES;
					pData->rdh.nCount = pData->rdh.nRgnSize = 0;
					SetRect(&pData->rdh.rcBound, MAXLONG, MAXLONG, 0, 0);

					// Keep on hand highest and lowest values for the "transparent" pixels
					BYTE lr = GetRValue(cTransparentColor);
					BYTE lg = GetGValue(cTransparentColor);
					BYTE lb = GetBValue(cTransparentColor);
					BYTE hr = min(0xff, lr + GetRValue(cTolerance));
					BYTE hg = min(0xff, lg + GetGValue(cTolerance));
					BYTE hb = min(0xff, lb + GetBValue(cTolerance));
					
					COLORREF cToleranceColor=RGB(hr, hg, hb);

					// Scan each bitmap row from bottom to top (the bitmap is inverted vertically)
					BYTE *p32 = (BYTE *)bm32.bmBits + (bm32.bmHeight - 1) * bm32.bmWidthBytes;
					for (int y = 0; y < bm.bmHeight; y++)
					{
						// Scan each bitmap pixel from left to right
						for (int x = 0; x < bm.bmWidth; x++)
						{
							// Search for a continuous range of "non transparent pixels"
							int x0 = x;
							LONG *p = (LONG *)p32 + x;
							while (x < bm.bmWidth)
							{
								if (*p>=cTransparentColor && *p<=cToleranceColor) break;

                                BYTE b = (BYTE)(((DWORD)(*p)) >> 16);// cannot use GetRValue(*p) because it give blue value ( (*p) is RGB and not COLORREF type);
								if (b >= lr && b <= hr)
								{
                                    b = (BYTE)(((DWORD)(*p)) >> 8);// here GetGValue(*p) may be used
									if (b >= lg && b <= hg)
									{
                                        b = ((BYTE)(*p));// cannot use GetBValue(*p); because it give red value!
										if (b >= lb && b <= hb)
											// This pixel is "transparent"
											break;
									}

								}
								p++;
								x++;
							}

							if (x > x0)
							{
								// Add the pixels (x0, y) to (x, y+1) as a new rectangle in the region
								if (pData->rdh.nCount >= maxRects)
								{
									GlobalUnlock(hData);
									maxRects += ALLOC_UNIT;
									hData = GlobalReAlloc(hData, sizeof(RGNDATAHEADER) + (sizeof(RECT) * maxRects), GMEM_MOVEABLE);
									pData = (RGNDATA *)GlobalLock(hData);
								}
								RECT *pr = (RECT *)&pData->Buffer;
								SetRect(&pr[pData->rdh.nCount], x0, y, x, y+1);
								if (x0 < pData->rdh.rcBound.left)
									pData->rdh.rcBound.left = x0;
								if (y < pData->rdh.rcBound.top)
									pData->rdh.rcBound.top = y;
								if (x > pData->rdh.rcBound.right)
									pData->rdh.rcBound.right = x;
								if (y+1 > pData->rdh.rcBound.bottom)
									pData->rdh.rcBound.bottom = y+1;
								pData->rdh.nCount++;

								// On Windows98, ExtCreateRegion() may fail if the number of rectangles is too
								// large (ie: > 4000). Therefore, we have to create the region by multiple steps.
								if (pData->rdh.nCount == 2000)
								{
									HRGN h = ExtCreateRegion(NULL, sizeof(RGNDATAHEADER) + (sizeof(RECT) * maxRects), pData);
									if (hRgn)
									{
										CombineRgn(hRgn, hRgn, h, RGN_OR);
										DeleteObject(h);
									}
									else
										hRgn = h;
									pData->rdh.nCount = 0;
									SetRect(&pData->rdh.rcBound, MAXLONG, MAXLONG, 0, 0);
								}
							}
						}

						// Go to next row (remember, the bitmap is inverted vertically)
						p32 -= bm32.bmWidthBytes;
					}

					// Create or extend the region with the remaining rectangles
					HRGN h = ExtCreateRegion(NULL, sizeof(RGNDATAHEADER) + (sizeof(RECT) * maxRects), pData);
					if (hRgn)
					{
						CombineRgn(hRgn, hRgn, h, RGN_OR);
						DeleteObject(h);
					}
					else
						hRgn = h;

					// Clean up
					SelectObject(hDC, holdBmp);
					DeleteDC(hDC);
				}

				DeleteObject(SelectObject(hMemDC, holdBmp));
			}

			if (!hBmpDC) DeleteDC(hMemDC);
		}
	}

	return hRgn;
}
//  ===========================================================================
//  Load ico file to hbitmap
//  ===========================================================================
HBITMAP LoadBitmapFromICOFile(LPCTSTR lpFileName)
{
    HICON hIcon=(HICON)LoadImage(NULL, lpFileName, IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_LOADFROMFILE);
    if (!hIcon) return NULL;

    ICONINFO iconInfo;
    GetIconInfo(hIcon,&iconInfo);

    HBITMAP hBitmap;
    hBitmap = (HBITMAP)::CopyImage(iconInfo.hbmColor, IMAGE_BITMAP, 0, 0, LR_DEFAULTSIZE);

    /*
    ICONINFO iconInfo;
    GetIconInfo(hIcon,&iconInfo);
    BITMAP bmp;
    GetObject(iconInfo.hbmColor, sizeof(bmp), &bmp);

    HWND hDesktopWnd = ::GetDesktopWindow();
	HDC hDesktopDC = ::GetDC(hDesktopWnd);
	HDC hdcBmp = CreateCompatibleDC(hDesktopDC);
	HDC hdcMem = CreateCompatibleDC(hDesktopDC);

    hBitmap = CreateCompatibleBitmap(hDesktopDC, bmp.bmWidth,bmp.bmHeight);
    ReleaseDC(hDesktopWnd,hDesktopDC);
    HBITMAP hBitmapBackup = (HBITMAP)SelectObject(hdcBmp, hBitmap);
    //DrawIconEx(hdcBmp, 0, 0, hIcon, bmp.bmWidth,bmp.bmHeight, 0, (HBRUSH)RGB(255, 255, 255), DI_NORMAL);

    SelectObject(hdcMem, iconInfo.hbmColor);
    BitBlt(hdcBmp,0,0,bmp.bmWidth,bmp.bmHeight,hdcMem,0,0,SRCCOPY);

    SelectObject(hdcBmp, hBitmapBackup);
    */

    return hBitmap;
}

//  ===========================================================================
//  Create a stream object initialized with the data from an executable resource.
//  ===========================================================================
//  Source: http://faithlife.codes/blog/2008/09/displaying_a_splash_screen_with_c_part_i/
//
//  MIT-style license
//  Copyright 2007-2008 Logos Bible Software
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
//  associated documentation files (the “Software”), to deal in the Software without restriction, including
//  without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to
//  the following conditions:
//
//  The above copyright notice and this permission notice shall be included in all copies or substantial
//  portions of the Software.
//
//  THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
//  FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
//  OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
//  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
//  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//  ===========================================================================
IStream * CreateStreamOnResource(LPCTSTR lpName, LPCTSTR lpType)
{
    // initialize return value
    IStream * ipStream = NULL;

    // find the resource
    HRSRC hrsrc = FindResource(NULL, lpName, lpType);
    if (hrsrc == NULL)
        return NULL;

    // load the resource
    DWORD dwResourceSize = SizeofResource(NULL, hrsrc);
    HGLOBAL hglbImage = LoadResource(NULL, hrsrc);
    if (hglbImage == NULL)
        return NULL;

    // lock the resource, getting a pointer to its data
    LPVOID pvSourceResourceData = LockResource(hglbImage);
    if (pvSourceResourceData == NULL)
        return NULL;

    // allocate memory to hold the resource data
    HGLOBAL hgblResourceData = GlobalAlloc(GMEM_MOVEABLE, dwResourceSize);
    if (hgblResourceData == NULL)
        return NULL;

    // get a pointer to the allocated memory
    LPVOID pvResourceData = GlobalLock(hgblResourceData);
    if (pvResourceData == NULL)
    {
        GlobalFree(hgblResourceData);
        return NULL;
    }

    // copy the data from the resource to the new memory block
    CopyMemory(pvResourceData, pvSourceResourceData, dwResourceSize);
    GlobalUnlock(hgblResourceData);

    // create a stream on the HGLOBAL containing the data
    if (SUCCEEDED(CreateStreamOnHGlobal(hgblResourceData, TRUE, &ipStream)))
        return ipStream;

    // couldn't create stream; free the memory
    GlobalFree(hgblResourceData);

    // no need to unlock or free the resource
    return ipStream;
}

//  ===========================================================================
//  Load a PNG image from the specified stream (using Windows Imaging Component).
//  ===========================================================================
//  Source: http://faithlife.codes/blog/2008/09/displaying_a_splash_screen_with_c_part_i/
//
//  MIT-style license
//  Copyright 2007-2008 Logos Bible Software
//
//  For details about license see above
//  ===========================================================================
IWICBitmapSource * LoadBitmapFromStream(IStream * ipImageStream)
{
    // initialize return value
    IWICBitmapSource * ipBitmap = NULL;

    // load WIC's PNG decoder
    IWICBitmapDecoder * ipDecoder = NULL;
    if (FAILED(CoCreateInstance(CLSID_WICPngDecoder, NULL, CLSCTX_INPROC_SERVER, __uuidof(ipDecoder), reinterpret_cast<void**>(&ipDecoder))))
        return NULL;
 
    // load the PNG
    if (FAILED(ipDecoder->Initialize(ipImageStream, WICDecodeMetadataCacheOnLoad)))
    {
        ipDecoder->Release();
        return NULL;
    }
 
    // check for the presence of the first frame in the bitmap
    UINT nFrameCount = 0;
    if (FAILED(ipDecoder->GetFrameCount(&nFrameCount)) || nFrameCount != 1)
    {
        ipDecoder->Release();
        return NULL;
    }
 
    // load the first frame (i.e., the image)
    IWICBitmapFrameDecode * ipFrame = NULL;
    if (FAILED(ipDecoder->GetFrame(0, &ipFrame)))
    {
        ipDecoder->Release();
        return NULL;
    }

    // convert the image to 32bpp BGRA format with pre-multiplied alpha
    //   (it may not be stored in that format natively in the PNG resource,
    //   but we need this format to create the DIB to use on-screen)
    WICConvertBitmapSource(GUID_WICPixelFormat32bppPBGRA, ipFrame, &ipBitmap);
    ipFrame->Release();

    ipDecoder->Release();
    return ipBitmap;
}

//  ===========================================================================
//  Create a 32-bit DIB from the specified WIC bitmap.
//  ===========================================================================
//  Source: http://faithlife.codes/blog/2008/09/displaying_a_splash_screen_with_c_part_i/
//
//  MIT-style license
//  Copyright 2007-2008 Logos Bible Software
//
//  For details about license see above
//  ===========================================================================
HBITMAP CreateHBITMAP(IWICBitmapSource * ipBitmap)
{
    // initialize return value
    HBITMAP hbmp = NULL;
 
    // get image attributes and check for valid image
    UINT width = 0;
    UINT height = 0;
    if (FAILED(ipBitmap->GetSize(&width, &height)) || width == 0 || height == 0)
        return NULL;
 
    // prepare structure giving bitmap information (negative height indicates a top-down DIB)
    BITMAPINFO bminfo;
    ZeroMemory(&bminfo, sizeof(bminfo));
    bminfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bminfo.bmiHeader.biWidth = width;
    bminfo.bmiHeader.biHeight = -((LONG) height);
    bminfo.bmiHeader.biPlanes = 1;
    bminfo.bmiHeader.biBitCount = 32;
    bminfo.bmiHeader.biCompression = BI_RGB;
 
    // create a DIB section that can hold the image
    void * pvImageBits = NULL;
    HDC hdcScreen = GetDC(NULL);
    hbmp = CreateDIBSection(hdcScreen, &bminfo, DIB_RGB_COLORS, &pvImageBits, NULL, 0);
    ReleaseDC(NULL, hdcScreen);
    if (hbmp == NULL)
        return NULL;
 
    // extract the image into the HBITMAP
    const UINT cbStride = width * 4;
    const UINT cbImage = cbStride * height;
    if (FAILED(ipBitmap->CopyPixels(NULL, cbStride, cbImage, static_cast<BYTE *>(pvImageBits))))
    {
        // couldn't extract image; delete HBITMAP
        DeleteObject(hbmp);
        hbmp = NULL;
    }

    return hbmp;
}

//  ===========================================================================
//  Load the PNG containing the image form resource into a HBITMAP.
//  ===========================================================================
//  Original code is LoadSplashImage() function
//  Source: http://faithlife.codes/blog/2008/09/displaying_a_splash_screen_with_c_part_i/
//
//  MIT-style license
//  Copyright 2007-2008 Logos Bible Software
//
//  For details about license see above
//  ===========================================================================
HBITMAP LoadPNGResource(LPCTSTR lpName, LPCTSTR lpType="PNG")
{
    HBITMAP hbmpSplash = NULL;
    CoInitialize(0);

    // load the PNG image data into a stream
    IStream * ipImageStream = CreateStreamOnResource(lpName, lpType);
    if (ipImageStream == NULL)
        return NULL;

    // load the bitmap with WIC
    IWICBitmapSource * ipBitmap = LoadBitmapFromStream(ipImageStream);
    if (ipBitmap == NULL)
    {
        ipImageStream->Release();
        return NULL;
    }

    // create a HBITMAP containing the image
    hbmpSplash = CreateHBITMAP(ipBitmap);
    ipBitmap->Release();

    ipImageStream->Release();
    return hbmpSplash;
}

//  ===========================================================================
//  Create a stream object initialized with the data from a file.
//  ===========================================================================
IStream * CreateStreamOnFile(LPCTSTR pszPathName)
{
    HANDLE hFile = ::CreateFile(pszPathName,
                                FILE_READ_DATA,
                                FILE_SHARE_READ,
                                NULL, 
                                OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL);
    if ( !hFile )
        return NULL;

    DWORD len = ::GetFileSize( hFile, NULL); // only 32-bit of the actual file size is retained
    if (len == 0)
        return NULL;

    HGLOBAL hGlobal = ::GlobalAlloc(GMEM_MOVEABLE, len);
    if ( !hGlobal )
    {
        ::CloseHandle(hFile);
        return NULL;
    }

    char* lpBuffer = reinterpret_cast<char*> ( ::GlobalLock(hGlobal) );
    DWORD dwBytesRead = 0;

    while ( ::ReadFile(hFile, lpBuffer, 4096, &dwBytesRead, NULL) )
    {
        lpBuffer += dwBytesRead;
        if (dwBytesRead == 0)
            break;
        dwBytesRead = 0;
    }

    ::CloseHandle(hFile);

	
    ::GlobalUnlock(hGlobal);

    // don't delete memory on object's release
    IStream* pStream = NULL;
    if ( ::CreateStreamOnHGlobal(hGlobal,FALSE,&pStream) != S_OK )
    {
        ::GlobalFree(hGlobal);
        return NULL;
    }

    return pStream;
}

//  ===========================================================================
//  Create a new picture object from a 'PNG' file
//  ===========================================================================
HBITMAP LoadPNGFile(LPCTSTR lpName)
{
    HBITMAP hbmpSplash = NULL;
    CoInitialize(0);

    // load the PNG image data into a stream
    IStream * ipImageStream = CreateStreamOnFile(lpName);
    if (ipImageStream == NULL)
        return NULL;

    // load the bitmap with WIC
    IWICBitmapSource * ipBitmap = LoadBitmapFromStream(ipImageStream);
    if (ipBitmap == NULL)
    {
        ipImageStream->Release();
        return NULL;
    }

    // create a HBITMAP containing the image
    hbmpSplash = CreateHBITMAP(ipBitmap);
    ipBitmap->Release();

    ipImageStream->Release();
    return hbmpSplash;
}

//  ===========================================================================
//  Resize a bitmap
//  ===========================================================================
HBITMAP ResizeBmp(HBITMAP hBmpSrc, SIZE newSize)
{
    // current size
    BITMAP bmpInfo;
    GetObject(hBmpSrc, sizeof(BITMAP), &bmpInfo);
    SIZE oldSize;
    oldSize.cx = bmpInfo.bmWidth;
    oldSize.cy = bmpInfo.bmHeight;
    
    // select src in DC
    HDC hdc = GetDC(NULL);
    HDC hDCSrc = CreateCompatibleDC(hdc);
    HBITMAP hOldBmpSrc = (HBITMAP)SelectObject(hDCSrc, hBmpSrc);
    
    // create bitmap dest and select it in DC
    HDC hDCDst = CreateCompatibleDC(hdc);
    HBITMAP hBmpDst = CreateCompatibleBitmap(hdc, newSize.cx, newSize.cy);
    HBITMAP hOldBmpDst = (HBITMAP)SelectObject(hDCDst, hBmpDst);
                  
    // resize
    StretchBlt(hDCDst, 0, 0, newSize.cx, newSize.cy, hDCSrc, 0, 0, oldSize.cx, oldSize.cy, SRCCOPY);
    
    // free resources
    SelectObject(hDCSrc, hOldBmpSrc);
    SelectObject(hDCDst, hOldBmpDst);
    DeleteDC(hDCSrc);
    DeleteDC(hDCDst);
    ReleaseDC(NULL, hdc);

    return hBmpDst;
}
//  ===========================================================================
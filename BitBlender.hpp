// Refont of Paul Watt's BitBlender helper functions in a single file
// by Noël MARTINON
// NOTA : original namespace 'article' is renamed to 'bitblender'

/* BitBlender.h ***************************************************************
Author      Paul Watt
Date:       7/21/2011
Purpose:    Bit Blender is a collection of helper functions that simplifies the 
            use of GradientFill and AlphaBlend.  These functions also extend
            the capabilities of the original API to allow radial gradients, 
            arbitrary gradients at an angle, and alpha-blend support.

Copyright 2011 Paul Watt
******************************************************************************/
#ifndef BITBLENDER_H_INCLUDED
#define BITBLENDER_H_INCLUDED

/* Includes ******************************************************************/
#include <windows.h>
#include <algorithm>
#include <vector>

// targetver.h
#pragma once

// The following macros define the minimum required platform.  The minimum required platform
// is the earliest version of Windows, Internet Explorer etc. that has the necessary features to run 
// your application.  The macros work by enabling all features available on platform versions up to and 
// including the version specified.

// Modify the following defines if you have to target a platform prior to the ones specified below.
// Refer to MSDN for the latest info on corresponding values for different platforms.
#ifndef WINVER                          // Specifies that the minimum required platform is Windows Vista.
#define WINVER 0x0600           // Change this to the appropriate value to target other versions of Windows.
#endif

#ifndef _WIN32_WINNT            // Specifies that the minimum required platform is Windows Vista.
#define _WIN32_WINNT 0x0600     // Change this to the appropriate value to target other versions of Windows.
#endif

#ifndef _WIN32_WINDOWS          // Specifies that the minimum required platform is Windows 98.
#define _WIN32_WINDOWS 0x0410 // Change this to the appropriate value to target Windows Me or later.
#endif

#ifndef _WIN32_IE                       // Specifies that the minimum required platform is Internet Explorer 7.0.
#define _WIN32_IE 0x0700        // Change this to the appropriate value to target other versions of IE.
#endif
// END targetver.h

// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

#define _CRT_NON_CONFORMING_SWPRINTFS
#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX

#include <algorithm>
using std::min;
using std::max;

// Windows Header Files:
#include <windows.h>

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

/* Constants *****************************************************************/
/* Common Color Definitions **************************************************/
const COLORREF k_white          = RGB(0xFF, 0xFF, 0xFF);
const COLORREF k_lightGray      = RGB(0xED, 0xED, 0xED);
const COLORREF k_gray           = RGB(0xC0, 0xC0, 0xC0);
const COLORREF k_darkGray       = RGB(0x80, 0x80, 0x80);
const COLORREF k_black          = RGB(0x00, 0x00, 0x00);
const COLORREF k_cpDarkOrange   = RGB(0xFF, 0x99, 0x00);
const COLORREF k_cpLightOrange  = RGB(0xFF, 0xE2, 0xA8);
const COLORREF k_cpDarkGreen    = RGB(0x48, 0x8E, 0x00);
const COLORREF k_cpLightGreen   = RGB(0x76, 0xAB, 0x40);
const COLORREF k_red2           = RGB(0xFF, 0x1A, 0x00);
const COLORREF k_red            = RGB(0xFF, 0x00, 0x00);
const COLORREF k_green          = RGB(0x00, 0xFF, 0x00);
const COLORREF k_blue           = RGB(0x00, 0x00, 0xFF);
const COLORREF k_lightPurple    = RGB(0x70, 0x70, 0xA0);
const COLORREF k_hilightPurple  = RGB(0x80, 0x80, 0xC0);
const COLORREF k_redBase        = RGB(0xFF, 0x40, 0x60);
const COLORREF k_deepRed        = RGB(0x60, 0x00, 0x00);
const COLORREF k_greenBase      = RGB(0x30, 0xC0, 0x40);
const COLORREF k_deepGreen      = RGB(0x00, 0x60, 0x00);
const COLORREF k_blueBase       = RGB(0x40, 0x80, 0xFF);
const COLORREF k_deepBlue       = RGB(0x00, 0x00, 0x60);

const COLORREF k_blueGlass      = RGB(0x40,0x50,0x70);
const COLORREF k_blueGlassEdge  = RGB(0x30,0x40,0x55);

// Color Palette colors.
const COLORREF k_brightRed      = RGB(237,28,36);
const COLORREF k_brightYellow   = RGB(255,242,0);
const COLORREF k_yellowGreen    = RGB(168,230,29);
const COLORREF k_brightGreen    = RGB(34,217,76);
const COLORREF k_brightCyan     = RGB(100,183,239);
const COLORREF k_indigo         = RGB(47,54,153);
const COLORREF k_violet         = RGB(111,49,152);

const COLORREF k_softRed        = RGB(255,163,177);
const COLORREF k_softOrange     = RGB(245,228,156);
const COLORREF k_softGreen      = RGB(211,249,188);
const COLORREF k_periwinkle     = RGB(112,154,209);
const COLORREF k_lavender       = RGB(0xC0, 0xC0, 0xE0);
// END stdafx.h


/* AutoGDI.h ******************************************************************
Author:    Paul Watt
Date:      7/21/2011 11:10:01 PM
Purpose:   A few GDI helper objects and functions for use with C++ win32 GDI 
           development in the absence of MFC or WTL.
Copyright 2011 Paul Watt
*******************************************************************************/
#ifndef AutoObj_H_INCLUDED
#define AutoObj_H_INCLUDED

/* Includes ******************************************************************/
#include <windows.h>

namespace bitblender
{
/* Class Definitions *********************************************************/
// These objects are defined below to allow object cleanup on the stack.
// AutoBitmap   Manages HBITMAP handles
// AutoBrush    Manages HBRUSH handles
// AutoFont     Manages HFONT handles
// AutoPen      Manages HPEN handles
// AutoRgn      Manages HRGN handles 
// AutoPalette  Manages HPALETTE handles

// AutoSaveDC   Takes a snapshot of the current DC.  Restores when leaves scope.

// MemDCBuffer Creates and manages a memory DC buffer for double buffer painting.

/* Utility Functions *********************************************************/
/******************************************************************************
Date:       8/14/2011
Purpose:    Calculates the Height and Width of a rectangle.
Parameters: rc[in]: A RECT struct where left <= right and top <= bottom.
Return:     Returns a SIZE struct with the calculated height and width.
*******************************************************************************/
inline
SIZE GetRectSize(const RECT& rc) 
{
  SIZE sz = {rc.right - rc.left, rc.bottom - rc.top};
  return sz;
}

/******************************************************************************
Date:       8/14/2011
Purpose:    Calculates the center point of a rectangle.
Parameters: rc[in]: A RECT struct where left <= right and top <= bottom.
            sz[in]: Size has already been calculated for the rectangle.
Return:     Returns the center point of the rectangle.
*******************************************************************************/
inline
POINT GetRectCenter(const RECT& rc, const SIZE& sz) 
{
  POINT pt = {rc.left + sz.cx/2, rc.top + sz.cy/2};
  return pt;
}

/******************************************************************************
Date:       8/14/2011
Purpose:    Calculates the center point of a rectangle.
Parameters: rc[in]: A RECT struct where left <= right and top <= bottom.
Return:     Returns the center point of the rectangle.
*******************************************************************************/
inline
POINT GetRectCenter(const RECT& rc) 
{
  return GetRectCenter(rc, GetRectSize(rc));
}

/* GDI HANDLE Resource Management Object **************************************
Purpose:    This class provides the template to cleanup any of the GDI objects
            that should be freed with a call to ::DeleteObject.
            This object is meant to be created on the stack.  When the object
            goes out of scope, it will automatically clean up the GDI resources.

Note:       This object is only intended to be declared on the stack.
Do not declare this object with "static"
******************************************************************************/
template <typename T>
class AutoObj
{
public:
  T   handle;

  /* Default Constructor *****************************************************/
  // Use Attach to assign a handle to an empty Auto object.
  AutoObj ()  
    : handle(NULL)          { }

  /* explicit Handle constructor *********************************************/
  // Wouldn't it be a bitch for the compiler to convert one of your
  // GDI Object Handles into one of these objects (secretly), 
  // and destroy it without telling you.  That's why this is defined explicit.
  explicit
    AutoObj (T in)  
    : handle(in)            { }

  /* Destructor **************************************************************/
  ~AutoObj()                { ::DeleteObject(handle);}

  /* GDI Object Handle Conversion Operator ***********************************/
  operator T() const        { return handle;} 

  /* Public ******************************************************************
  Purpose:    Attach an unmanaged handle to this auto-object 
              for resource management.
  Parameters: in[in]: The input handle to be managed.
  Return:     If the object is not currently managing a handle, the input
              handle will become managed by this object and true is returned.
              If false is returned, this object is already managing a handle.
  ***************************************************************************/
  bool Attach(T in)         { if (NULL == handle)
                                handle = in;
                              return handle == in;
                            }

  /* Public ******************************************************************
  Purpose:    Detach a managed handle from this auto-object.
              The caller then takes responsibility for resource management
              of the GDI Object.
  Return:     The managed handle is returned.
              If there is currently no handle, then NULL will be returned.
  ***************************************************************************/
  T Detach()                { T retVal = handle;
                              handle = NULL;
                              return retVal;
                            }

private:
  // Make this object non-copyable by hiding these functions"
  AutoObj(const AutoObj&);            // Copy Constructor
  AutoObj& operator=(const AutoObj&); // Assignment Operator
  // Prohibit some other nonsensical functions.
  bool Attach(T in) const;            
  T    Detach()     const;
};


/* DC Snapshot manager ********************************************************
Purpose:    Takes a snapshot of the current DC state.  When the object goes
            out of scope, it will restore the context of the DC.

            The restoration can be forced sooner by calling Restore.

Note:       This object is only intended to be declared on the stack.
            Do not declare this object with "static"
******************************************************************************/
class AutoSaveDC
{
public:
  /* Construction ************************************************************/
  explicit
    AutoSaveDC(HDC hdc)     
    : m_hdc(hdc)            { m_ctx = ::SaveDC(hdc);}
   ~AutoSaveDC()            { Restore();}

 /* Methods ******************************************************************/
 void Restore()             { if (0 != m_ctx)
                              {
                                ::RestoreDC(m_hdc, m_ctx);
                                m_ctx = 0;
                                m_hdc = 0;
                              }
                            }
private:
  /* Members *****************************************************************/
  HDC m_hdc;                // HDC to restore
  int m_ctx;                // Saved context

  /* Methods *****************************************************************/
  // No default constructor.
  AutoSaveDC();
  // Make this object non-copyable by hiding these functions"
  AutoSaveDC(const AutoSaveDC&);            // Copy Constructor
  AutoSaveDC& operator=(const AutoSaveDC&); // Assignment Operator
};

/* Class **********************************************************************
Purpose:    Implements a double-buffer from an input DC Handle.
            A size should be specified to indicate the size of the backup buffer.
Construction:            
            However, if the size is omitted, and a Memory DC is passed in, 
            the constructor will attempt to create a duplicate buffer the 
            size of the buffer selected into the input Memory DC.

            If the size is omitted, and the input DC is not a Memory DC, all
            input directed to this object will simply write directly to the 
            original DC.

IsBuffered: Return true if this object maintains a double buffer, false otherwise.
Flush:      Writes the data from the backup buffer into the specified buffer.
            
*****************************************************************************/
class MemDCBuffer
{
public:
  /* Construction ************************************************************/
  explicit 
    MemDCBuffer(HDC hdc)                       
    : m_buffer(BufferInit_(hdc))            { }

    MemDCBuffer(HDC hdc, int cx, int cy)   
    : m_buffer(BufferInit_(hdc, cx, cy))    { }

   ~MemDCBuffer()                          { if (IsBuffered())
                                                ::DeleteDC(m_memDC);
                                            }

  /* Operators ***************************************************************/
  operator HDC()                            { return m_memDC;}

  /* Status ******************************************************************/
  bool IsBuffered() const                   { return NULL != (HBITMAP)m_buffer
                                                  && m_isSelected;
                                            }

  /* Methods *****************************************************************/
  void Flush(HDC hdc)                       { Flush(hdc, 0, 0, m_bm.bmWidth, m_bm.bmHeight);}
  void Flush(HDC hdc, const RECT& rc)       { Flush(hdc, 
                                                    rc.left, rc.top, 
                                                    rc.right - rc.left, rc.bottom - rc.top);
                                            }
  void Flush( HDC hdc, 
              int x, int y, 
              int cx, int cy)               { ::BitBlt( hdc, x, y, cx, cy,
                                                        m_memDC, 0, 0, SRCCOPY);
                                            }

  HBITMAP BorrowImage()                     { ::SelectObject(m_memDC, m_hOldBmp);
                                              m_isSelected = false;
                                              return m_buffer;
                                            }
  void    ReturnImage()                     { ::SelectObject(m_memDC, m_buffer);
                                              m_isSelected = true;
                                            }

private:
  /* Members *****************************************************************/
  HDC               m_memDC;
  AutoObj<HBITMAP>  m_buffer;
  BITMAP            m_bm;
  HBITMAP           m_hOldBmp;
  bool              m_isSelected;

  /* Methods *****************************************************************/
  HBITMAP BufferInit_(HDC hdc, int cx=0, int cy=0)
  { 
    if ( 0 == cx 
      || 0 == cy)
    {
      AutoObj<HBITMAP> tempBmp(::CreateCompatibleBitmap(hdc, 1, 1));
      // Attempt to create a duplicate buffer assuming
      // the specified hdc is a memory DC.
      m_hOldBmp = (HBITMAP)::SelectObject(hdc, tempBmp);
      if (NULL == m_hOldBmp)
      {
        // This is not a memory DC.
        // Double buffering will not be performed.
        m_memDC = hdc;
        m_isSelected = false;
        return NULL;    
      }

      // Get the dimensions of the existing bitmap.
      ::GetObject(m_hOldBmp, sizeof(BITMAP), &m_bm);
      cx = m_bm.bmWidth;
      cy = m_bm.bmHeight;
    }

    m_memDC = ::CreateCompatibleDC(hdc);
    HBITMAP hBitmapBuffer = ::CreateCompatibleBitmap(hdc, cx, cy);
    ::GetObject(hBitmapBuffer, sizeof(BITMAP), &m_bm);
    m_hOldBmp = (HBITMAP)::SelectObject(m_memDC, hBitmapBuffer);
    m_isSelected = true;

    return hBitmapBuffer;
  }

  /* Prohibited Access functions *********************************************/
  MemDCBuffer();
  MemDCBuffer(const MemDCBuffer&);
  MemDCBuffer& operator=(const MemDCBuffer&);
};

/******************************************************************************
Date:       8/27/2011
Purpose:    Makes a copy of the specified bitmap which will be compatible with
            the supplied DC.
Parameters: hdc[in]: DC to make the new bitmap compatible with.
            hBmp[in]: BITMAP handle, must not be selected into any memDC.
Return:     A new bitmap handle that contains a copy of hBmp.
            The caller is responsible to call ::DeleteObject on this handle.
*******************************************************************************/
inline
HBITMAP CopyBitmap(HDC hdc, HBITMAP hBmp)
{
  HDC memInDC  = ::CreateCompatibleDC(hdc);
  HDC memOutDC = ::CreateCompatibleDC(hdc);

  BITMAP bm;
  ::GetObject(hBmp, sizeof(BITMAP), &bm);
  HBITMAP hOutputBmp = ::CreateCompatibleBitmap(hdc, bm.bmWidth, bm.bmHeight);

  ::SelectObject(memInDC, hBmp);
  ::SelectObject(memOutDC, hOutputBmp);

  ::BitBlt(memOutDC, 0, 0, bm.bmWidth, bm.bmHeight,
           memInDC, 0, 0, SRCCOPY);

  ::DeleteDC(memInDC);
  ::DeleteDC(memOutDC);

  return hOutputBmp;
}

/* Typedefs ******************************************************************/
typedef AutoObj<HBITMAP>    AutoBitmap;
typedef AutoObj<HBRUSH>     AutoBrush;
typedef AutoObj<HFONT>      AutoFont;
typedef AutoObj<HPEN>       AutoPen;
typedef AutoObj<HRGN>       AutoRgn;
typedef AutoObj<HPALETTE>   AutoPalette;

} // namespace bitblender

#endif
// END AutoGDI.h

namespace bitblender
{                     

/* Constants *****************************************************************/
/* Constants *****************************************************************/
const double k_pi         = 3.1415926535897932384626433832795;
const double k_pi_2       = 0.5 * k_pi;
const double k_pi_4       = 0.25* k_pi;
const double k_3pi_2      = 1.5 * k_pi;
const double k_2pi        = 2.0 * k_pi;

const double k_degToRad   = k_pi / 180.0;
const double k_RadToDeg   = 180.0 / k_pi;


// These constants are common levels of translucency.
const BYTE k_opaque       = 0xFF;
const BYTE k_transparent  = 0x00;

const BYTE k_alpha100     = k_opaque;       //100%, 0xFF == 255
const BYTE k_alpha80      = 0xCC;           // 80%, 0xCC == 204
const BYTE k_alpha75      = 0xC0;           // 75%, 0xC0 == 192
const BYTE k_alpha60      = 0x99;           // 60%, 0x99 == 153
const BYTE k_alpha50      = 0x80;           // 50%, 0x80 == 128
const BYTE k_alpha40      = 0x66;           // 40%, 0x66 == 102
const BYTE k_alpha25      = 0x40;           // 25%, 0x40 == 64
const BYTE k_alpha20      = 0x33;           // 20%, 0x33 == 51
const BYTE k_alpha0       = k_transparent;  //  0%, 0x00 == 0

/* Utilities *****************************************************************/
inline COLOR16 ToColor16(BYTE byte)   { return byte << 8;}
inline COLOR16 RVal16(COLORREF color) { return ToColor16(GetRValue(color));}
inline COLOR16 GVal16(COLORREF color) { return ToColor16(GetGValue(color));}
inline COLOR16 BVal16(COLORREF color) { return ToColor16(GetBValue(color));}


/* Functions *****************************************************************/
bool RectGradient(HDC,const RECT&,COLORREF,COLORREF,BOOL,BYTE,BYTE);
bool RectGradient(HDC,const RECT&,COLORREF,COLORREF,BOOL);
bool RadialGradient(HDC,int,int,int,COLORREF,COLORREF,size_t,BYTE,BYTE);
bool RadialGradient(HDC,int,int,int,COLORREF,COLORREF,size_t);
bool AngularGradient(HDC,const RECT&,double,COLORREF,COLORREF,BYTE,BYTE);
bool AngularGradient(HDC,const RECT&,double,COLORREF,COLORREF);

bool CombineAlphaChannel(HDC, HBITMAP, HBITMAP);

void GetColorDiff(COLORREF, COLORREF, int&, int&, int&);

BLENDFUNCTION GetBlendFn(BYTE, bool);

/* Bit Manipulation Functions ************************************************/
bool PreBlendAlphaBitmap(HDC, HBITMAP);
bool ShiftColorChannelsLeft (HDC,HBITMAP);
bool ShiftColorChannelsRight(HDC,HBITMAP);
bool InvertBitmap(HDC, HBITMAP);
bool ColorToGrayscale(HDC, HBITMAP);
bool GrayscaleToColor(HDC, HBITMAP, COLORREF);

/******************************************************************************
Date:       8/26/2011
Purpose:    Template that will extract the pixels from a bitmap into a DIB, and
            call a specified function on each bit.
            The bitmap manipulated with this function must be 32-bit color depth.
Parameters: hdc[in]: DC to be compatible with.
            hBmp[in]: Bitmap to shift the color channels for.
            fnT[in]: Functor that will modify a 32-bit number however is desired.

*******************************************************************************/
template <typename fnT>
bool ManipulateDIBits(HDC hdc, HBITMAP hBmp, fnT fn)
{
  // Attempt to extract the BITMAP from the current DC.
  BITMAP bm;
  ::GetObject(hBmp, sizeof(BITMAP), &bm);

  // zero the memory for the bitmap info 
  BITMAPINFO bmi;
  ZeroMemory(&bmi, sizeof(BITMAPINFO));

  // setup bitmap info  
  bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bmi.bmiHeader.biWidth = bm.bmWidth;
  bmi.bmiHeader.biHeight = bm.bmHeight;
  bmi.bmiHeader.biPlanes = bm.bmPlanes;
  bmi.bmiHeader.biBitCount = bm.bmBitsPixel;
  bmi.bmiHeader.biCompression = BI_RGB;
  bmi.bmiHeader.biSizeImage = bm.bmWidth * bm.bmHeight * 4;

  std::vector<UINT32> bits;
  bits.resize(bm.bmWidth * bm.bmHeight, 0x0);
  if (!::GetDIBits(hdc, hBmp, 0, bm.bmHeight, (void**)&bits[0], &bmi, DIB_RGB_COLORS))
    return false;

  // Perform the manipulation.
  std::for_each(bits.begin(), bits.end(), fn);

  // Transfer the data back to the input device.
  return 0
      != ::SetDIBits(hdc, hBmp, 0, bm.bmHeight, (void**)&bits[0], &bmi, DIB_RGB_COLORS);
}

/* Inline Function Implementations *******************************************/
/******************************************************************************
Date:       7/21/2011
Purpose:    Helper function to simplify defining and drawing a gradient fill
            in a rectangular area.  This function will create all of the 
            internal structures for each call to GradientFill.  
            If the same gradient definition will be used repeatedly, !!! will
            provide better performance.
Parameters: hDC[in]:  The device context to write to.
            rect[in]: The rectangle coordinates to fill with the gradient.
            c1[in]: The color to use at the start of the gradient.
            c2[in]: The color to use at the end of the gradient.
            isVertical[in]: Indicates if the gradient fill should transition
              vertically in the rectangle.  If this value is false, horizontal
              will be used.                

              All fills will be defined from left to right, or top to bottom.
Return:     true  If the function succeeds
            false If an error occurs and the function fails.
*******************************************************************************/
inline 
bool RectGradient(HDC hDC,
                  const RECT &rc, 
                  COLORREF c1, 
                  COLORREF c2,
                  BOOL isVertical)
{
  return RectGradient(hDC, rc, c1, c2, isVertical, k_opaque, k_opaque);
}

/******************************************************************************
Date:       7/22/2011
Purpose:    Creates a radial gradient.
            The gradient is approximated by breaking up the 
            circle into triangles, and using the Win32 GradientFill call
            to fill all of the individual triangles.
Parameters: hDC[in]:  The device context to write to.
            x[in]: The x coordinate of the center of the gradient.
            y[in]: The y coordinate of the center of the gradient.
            r[in]: The radius of the gradient fill.
            c1[in]: The color to use at the start of the gradient.
            c2[in]: The color to use at the end of the gradient.
            segments[in]: The number of segments to break the circle into.
              The default number of segments is 16.  
              3 is the absolute minimum, which will result in a triangle.
Return:     true  If the function succeeds
            false If an error occurs and the function fails.
*******************************************************************************/
inline 
bool RadialGradient(HDC hdc, 
                    int x, 
                    int y, 
                    int r,
                    COLORREF c1,
                    COLORREF c2,
                    size_t segments
                    )
{
  return RadialGradient(hdc, x, y, r, c1, c2, segments, k_opaque, k_opaque);
}

/******************************************************************************
Date:       7/25/2011
Purpose:    Creates a linear gradient fill directed along an arbitrary angle.
Parameters: hDC[in]:  The device context to write to.
            rect[in]: The rectangle coordinates to fill with the gradient.
            angle[in]: The angle in radians 
            c1[in]: The color to use at the start of the gradient.
            c2[in]: The color to use at the end of the gradient.
Return:     true  If the function succeeds
            false If an error occurs and the function fails.
*******************************************************************************/
inline 
bool AngularGradient(
  HDC hdc,
  const RECT &rc,
  double angle,
  COLORREF c1,
  COLORREF c2
  )
{
  return AngularGradient(hdc, rc, angle, c1, c2, k_opaque, k_opaque);
}

/******************************************************************************
Date:       7/25/2011
Purpose:    Calculates the difference in color between the two colors.
            The difference is calculated individually for each color channel.
Parameters: c1[in]: Start color
            c2[in]: End color
Return:     The difference in each color is returned.
*******************************************************************************/
inline
void GetColorDiff(COLORREF c1, COLORREF c2, int &red, int &green, int &blue)
{
  // Not sure what negative colors will do yet.  
  // Keep a higher level of accuracy until the results are known.
  red    = GetRValue(c2) - GetRValue(c1);
  green  = GetGValue(c2) - GetGValue(c1);
  blue   = GetBValue(c2) - GetBValue(c1);
}

/******************************************************************************
Date:       8/7/2011
Purpose:    Helper function to easily initialize a blend function for alpha blends.
Parameters: globalAlpha[in]: Indicates a global alpha constant, 0-255.
            hasSrcAlpha[in]: True indicates the source image has alpha info,
              this will set the AC_SRC_ALPHA flag.
              Otherwise false will clear this flag.
Return:    An initialized BLENDFUNCTION object is returned for use in GdiAlphaBlend.
*******************************************************************************/
inline 
BLENDFUNCTION GetBlendFn(BYTE globalAlpha, bool hasSrcAlpha)
{
  BLENDFUNCTION bf = {AC_SRC_OVER, 0, globalAlpha, (BYTE)(hasSrcAlpha ? AC_SRC_ALPHA : 0)};
  return bf;
}

/* Functors ******************************************************************/
/* Class **********************************************************************
Purpose:  Functor to multiply individual channels with a specified alpha value.
*******************************************************************************/
struct MultiplyAlpha
{
  void operator()(UINT32 &elt) const
  {
    UINT32 val = elt;
    BYTE alpha = (BYTE)(val >> 24);
    double factor = alpha / 255.0;
    elt = ((alpha) << 24)
      | (BYTE(GetRValue(elt) * factor) ) 
      | (BYTE(GetGValue(elt) * factor) << 8)
      | (BYTE(GetBValue(elt) * factor) << 16);
  }
};

/* Class **********************************************************************
Purpose:  Functor to shift all color channels left.
*******************************************************************************/
struct ShiftLeft
{
  void operator()(UINT32 &elt) const
  {
    elt <<= 8;
  }
};

/* Class **********************************************************************
Purpose:  Functor to shift all color channels right.
*******************************************************************************/
struct ShiftRight
{
  void operator()(UINT32 &elt) const
  {
    elt >>= 8;
  }
};

/* Class **********************************************************************
Purpose:  Functor to invert the bits for the current pixel.
*******************************************************************************/
struct Invert
{
  void operator()(UINT32 &elt) const
  {
    elt = ~elt;
  }
};

/* Class **********************************************************************
Purpose:  Functor to convert a color image into grayscale.
          This grayscale conversion is based on relative color intensity to 
          create a more natural looking image compared to the original.
              Intensity = (Red    * 0.299) 
                        + (Green  * 0.587) 
                        + (Blue   * 0.114)
*******************************************************************************/
struct ToGrayscale
{
  void operator()(UINT32 &elt) const
  {
    // Leave the alpha channel unchanged.
    BYTE intensity = BYTE( (GetRValue(elt) * 0.299)
                         + (GetGValue(elt) * 0.587)
                         + (GetBValue(elt) * 0.114) * 255);
    elt = (elt & 0xFF000000)
        | intensity << 16
        | intensity << 8
        | intensity;
  }
};

/* Class **********************************************************************
Purpose:  Functor to convert a grayscale image to an image with 256 levels of 
          a particular color.
*******************************************************************************/
struct Colorize
{
  Colorize(COLORREF color)
  {
    rVal = GetRValue(color);
    gVal = GetGValue(color);
    bVal = GetBValue(color);
  }  

  void operator()(UINT32 &elt) const
  {
    // Leave the alpha channel unchanged.
    // Get the intensity level for only one of the channels.
    // If a proper grayscale image were passed in, R = G = B.
    float intensity = GetRValue(elt) / 255.0f;
    elt = (elt & 0xFF000000)
        | ((BYTE(intensity * rVal)) << 16) 
        | ((BYTE(intensity * gVal)) << 8)
        |  (BYTE(intensity * bVal));
    //elt = (elt & 0x000000FF)
    //    | ((BYTE(intensity * rVal)) << 16) 
    //    | ((BYTE(intensity * gVal)) << 8)
    //    |  (BYTE(intensity * bVal));
  }

  BYTE rVal;
  BYTE gVal;
  BYTE bVal;
};

/******************************************************************************
Date:       8/5/2011
Purpose:    Pre-multiplies the alpha channel for each pixel in the bitmap.
Parameters: hdc[in]: A DC to the device the bitmap is compatible with.
            hBmp[in]: Handle to the BITMAP to have its alpha channel pre-blended.
              This handle cannot be selected into a DC when it is passed into
              this function.
Return:     true  The function was successful, the pre-blend completed.
            false The function failed, the bitmap is unchanged.
*******************************************************************************/
inline
bool PreBlendAlphaBitmap(HDC hdc, HBITMAP hBmp)
{
  return ManipulateDIBits(hdc, hBmp, MultiplyAlpha());
}

/******************************************************************************
Date:       8/26/2011
Purpose:    Shifts all of the color channels to the left one channel.
            The image passed in must be a 32-bit color bitmap.
Parameters: hdc[in]: DC to be compatible with.
            hBmp[in]: Bitmap to shift the color channels for.
Return:     true  The function was successful, the channels shifted left.
            false The function failed, the bitmap is unchanged.
*******************************************************************************/
inline
bool ShiftColorChannelsLeft(HDC hdc,HBITMAP hBmp)
{
  return ManipulateDIBits(hdc, hBmp, ShiftLeft());
}

/******************************************************************************
Date:       8/26/2011
Purpose:    Shifts all of the color channels to the right one channel.
            The image passed in must be a 32-bit color bitmap.
Parameters: hdc[in]: DC to be compatible with.
            hBmp[in]: Bitmap to shift the color channels for.
*******************************************************************************/
inline
bool ShiftColorChannelsRight(HDC hdc,HBITMAP hBmp)
{
  return ManipulateDIBits(hdc, hBmp, ShiftRight());
}

/******************************************************************************
Date:       8/26/2011
Purpose:    Inverts all of the colors of an image.  Every bit is flipped.
Parameters: hdc[in]: DC to be compatible with.
            hBmp[in]: Bitmap to shift the color channels for.
*******************************************************************************/
inline 
bool InvertBitmap(HDC hdc, HBITMAP hBmp)
{
  // The functor exists to perform the operation this way.
  // return ManipulateDIBits(hdc, hBmp, Invert());

  // However, this ROP3 method in BitBlt is implemented in hardware:
  BITMAP bm;
  ::GetObject(hBmp, sizeof(BITMAP), &bm);

  HDC memDC = ::CreateCompatibleDC(hdc);
  ::SelectObject(memDC, hBmp);
  ::BitBlt(memDC, 0, 0, bm.bmWidth, bm.bmHeight,
           0, 0, 0, DSTINVERT);
  ::DeleteDC(memDC);
  return true;
}

/******************************************************************************
Date:       8/26/2011
Purpose:    Converts the input color image to a grayscale image calculated
            from color intensity.
Parameters: hdc[in]: DC to be compatible with.
            hBmp[in]: Bitmap to convert to grayscale.
*******************************************************************************/
inline
bool ColorToGrayscale(HDC hdc,HBITMAP hBmp)
{
  return ManipulateDIBits(hdc, hBmp, ToGrayscale());
}

/******************************************************************************
Date:       8/27/2011
Purpose:    Converts a grayscale image into an image with color.
            This function is not symmetric with the ColorToGrayscale function.
            It does not add color based on an intensity calculation, rather it
            applies the specified color scaled to the current gray pixel.

            The image will still appear monochromatic, except with a shade of color.
Parameters: hdc[in]: DC to be compatible with.
            hBmp[in]: Bitmap to convert to grayscale.
            color[in]: The color to convert the image to.
*******************************************************************************/
inline
bool GrayscaleToColor(HDC hdc, HBITMAP hBmp, COLORREF color)
{
  Colorize colorize(color);
  return ManipulateDIBits(hdc, hBmp, colorize);
}

} // namespace bitblender

/* BitBlender.cpp *************************************************************
Author:    Paul Watt
Date:      7/21/2011 10:35:29 PM
Purpose:
Copyright 2011 Paul Watt
*******************************************************************************/
#include <math.h>

/* Forward Declarations ******************************************************/
namespace // anonymous
{

/* Forward Declarations ******************************************************/
bool AngularGradient_(HDC,
                      const RECT&,
                      double,
                      COLORREF,
                      COLORREF,
                      BYTE,
                      BYTE
                      );

bool SegmentedRadialGradient_(HDC hdc, 
                              const POINT& ctr, 
                              int radius,
                              double startAngle,
                              COLORREF c1,
                              COLORREF c2,
                              size_t segments,
                              BYTE alpha1,
                              BYTE alpha2
                              );

} // namespace anonymous


namespace bitblender
{

/******************************************************************************
Date:       7/21/2011
Purpose:    Helper function to simplify defining and drawing a gradient fill
            in a rectangular area.  This function will create all of the 
            internal structures for each call to GradientFill.  
            If the same gradient definition will be used repeatedly, !!! will
            provide better performance.
Parameters: hDC[in]:  The device context to write to.
            rect[in]: The rectangle coordinates to fill with the gradient.
            c1[in]: The color to use at the start of the gradient.
            c2[in]: The color to use at the end of the gradient.
            isVertical[in]: Indicates if the gradient fill should transition
              vertically in the rectangle.  If this value is false, horizontal
              will be used.                

              All fills will be defined from left to right, or top to bottom.
            alpha1[in]: Starting alpha level to associate with the gradient.
            alpha2[in]: Ending alpha level to associate with the gradient.

Return:     true  If the function succeeds
            false If an error occurs and the function fails.
*******************************************************************************/
bool RectGradient(
  HDC hDC,
  const RECT &rc, 
  COLORREF c1, 
  COLORREF c2,
  BOOL isVertical,
  BYTE alpha1,
  BYTE alpha2
  )
{
  TRIVERTEX v[2] =
  {
    {rc.left,  rc.top,    RVal16(c1), GVal16(c1), BVal16(c1), ToColor16(alpha1)},  
    {rc.right, rc.bottom, RVal16(c2), GVal16(c2), BVal16(c2), ToColor16(alpha2)}
  };

  GRADIENT_RECT topGradient;
  topGradient.UpperLeft = 0;
  topGradient.LowerRight= 1;

  BOOL result = ::GdiGradientFill(hDC, 
                                  v, 
                                  2, 
                                  &topGradient, 
                                  1, 
                                  isVertical
                                  ? GRADIENT_FILL_RECT_V
                                  : GRADIENT_FILL_RECT_H);

  return FALSE != result;
}

/******************************************************************************
Date:       7/22/2011
Purpose:    Creates a radial gradient.
            The gradient is approximated by breaking up the 
            circle into triangles, and using the Win32 GradientFill call
            to fill all of the individual triangles.
Parameters: hDC[in]:  The device context to write to.
            x[in]: The x coordinate of the center of the gradient.
            y[in]: The y coordinate of the center of the gradient.
            r[in]: The radius of the gradient fill.
            c1[in]: The color to use at the start of the gradient.
            c2[in]: The color to use at the end of the gradient.
            segments[in]: The number of segments to break the circle into.
              The default number of segments is 16.  
              3 is the absolute minimum, which will result in a triangle.
            alpha1[in]: Starting alpha level to associate with the gradient.
            alpha2[in]: Ending alpha level to associate with the gradient.
Return:     true  If the function succeeds
            false If an error occurs and the function fails.
*******************************************************************************/
bool RadialGradient(
  HDC hdc, 
  int x, 
  int y, 
  int r,
  COLORREF c1,
  COLORREF c2,
  size_t segments,
  BYTE alpha1,
  BYTE alpha2
  )
{
  POINT pt = {x,y};
  return SegmentedRadialGradient_(hdc, pt, r, 0.0, c1, c2, segments, alpha1, alpha2);
}

/******************************************************************************
Date:       7/25/2011
Purpose:    Creates a linear gradient fill directed along an arbitrary angle.
Parameters: hDC[in]:  The device context to write to.
            rect[in]: The rectangle coordinates to fill with the gradient.
            angle[in]: The angle in radians 
            c1[in]: The color to use at the start of the gradient.
            c2[in]: The color to use at the end of the gradient.
            alpha1[in]: Starting alpha level to associate with the gradient.
            alpha2[in]: Ending alpha level to associate with the gradient.
Return:     true  If the function succeeds
            false If an error occurs and the function fails.
*******************************************************************************/
bool AngularGradient(
  HDC hdc,
  const RECT &rc,
  double angle,
  COLORREF c1,
  COLORREF c2,
  BYTE alpha1,
  BYTE alpha2
  )
{
  // Immediately test for the axis aligned angles, and call the simpler version.
  if ( 0      == angle
    || k_pi_2 == angle
    || k_pi   == angle
    || k_3pi_2== angle)
  {
    // Determine the direction of the gradient.
    bool isVertical = !(0 == angle || k_pi == angle);
    // The colors will need to be swapped for the 2nd half of the circle.
    if (angle >= k_pi)
    {
      std::swap(c1, c2);
      std::swap(alpha1, alpha2);
    }

    return RectGradient(hdc, rc, c1, c2, isVertical, alpha1, alpha2);
  }

  return AngularGradient_(hdc, rc, angle, c1, c2, alpha1, alpha2);
}

/******************************************************************************
Date:       8/27/2011
Purpose:    Combines two images together. The source image will be used as is, 
            and the red channel from the alpha image will be blended into the 
            alpha channel of the src image.
Parameters: hdc[in]:  DC which the bitmaps are compatible with.
            hImg[in/out]: Image to receive the alpha channel.
            hAlpha[in]: Image to create an alpha channel from.
*******************************************************************************/
bool CombineAlphaChannel(HDC hdc, HBITMAP hImg, HBITMAP hAlpha)
{
  // Get Bitmap info.
  BITMAP bmImg;
  BITMAP bmAlpha;
  ::GetObject(hImg,   sizeof(BITMAP), &bmImg);
  ::GetObject(hAlpha, sizeof(BITMAP), &bmAlpha);

  RECT rc = {0,0,bmAlpha.bmWidth,bmAlpha.bmHeight};

  // Create a working bitmap buffer.
  MemDCBuffer imageDC(hdc, bmAlpha.bmWidth, bmAlpha.bmHeight);

  // Create a memory DC to manipulate the input bitmaps.
  HDC memDC = ::CreateCompatibleDC(hdc);
  ::SelectObject(memDC, hAlpha);
  
  // Remove the Green and Blue channels from the alpha image.
  ::FillRect(imageDC, &rc, AutoBrush(::CreateSolidBrush(RGB(0xFF,0,0))));
  ::BitBlt(imageDC, 0, 0, bmAlpha.bmWidth, bmAlpha.bmHeight,
            memDC, 0, 0, SRCAND);

  // Shift the channel.
  ShiftColorChannelsLeft(hdc, imageDC.BorrowImage());
  // Return the buffer to the working buffer.
  imageDC.ReturnImage();

  // Add the alpha channel to the input image.
  ::SelectObject(memDC, hImg);
  ::BitBlt( memDC, 0, 0, bmImg.bmWidth, bmImg.bmHeight,
            imageDC, 0, 0, SRCPAINT);

  ::DeleteDC(memDC);

  return true;
}

} // namespace bitblender

/* Local Declarations ********************************************************/
namespace // anonymous
{

/******************************************************************************
Date:       8/10/2011
Purpose:    Creates a linear gradient fill directed along an arbitrary angle
            inside of any axis aligned rectangle.
Parameters: hDC[in]:  The device context to write to.
            rect[in]: The rectangle coordinates to fill with the gradient.
            angle[in]: The angle in radians 
            c1[in]: The color to use at the start of the gradient.
            c2[in]: The color to use at the end of the gradient.
            alpha1[in]: Starting alpha level to associate with the gradient.
            alpha2[in]: Ending alpha level to associate with the gradient.
Return:     true  If the function succeeds
            false If an error occurs and the function fails.
*******************************************************************************/
bool AngularGradient_(
  HDC hdc,
  const RECT& rc,
  double angle,
  COLORREF c1,
  COLORREF c2,
  BYTE alpha1,
  BYTE alpha2
  )
{
  using namespace bitblender;

  SIZE sz = bitblender::GetRectSize(rc);
  // The quadrant and rotation offset are used to make corrections for (+/-).
  double quad = (angle / (k_pi / 2));
  int offset = int(ceil(quad) - 1);

  // Determine a ratio of intersection points for 
  // the LL, and UR corners of the provided rectangle.  
  // This will allow the other colors to be derived.
  double cosTheta = sz.cx * cos(angle);
  double sinTheta = sz.cy * sin(angle);

  double len = fabs(cosTheta) + fabs(sinTheta);
  double r1  = fabs(cosTheta / len);
  double r2  = fabs(sinTheta / len);

  // Derive 4 color points from the original 2 colors, and the provided angle.
  // Calculate the total color distance between c1 and c2 for all channels.
  int rDiff = 0;
  int gDiff = 0;
  int bDiff = 0;

  bitblender::GetColorDiff(c1, c2, rDiff, gDiff, bDiff);
  COLOR16 alphaDiff = ToColor16(alpha2 - alpha1);

  // Calculate the other colors by multiplying the color diff with each ratio.
  // There may be two or four colors depending on the type of shape.
  COLORREF cC[2];
  USHORT   alpha[2];
  cC[1] = RGB(GetRValue(c1) + (rDiff * r1),
              GetGValue(c1) + (gDiff * r1),
              GetBValue(c1) + (bDiff * r1));
  cC[0] = RGB(GetRValue(c1) + (rDiff * r2),
              GetGValue(c1) + (gDiff * r2),
              GetBValue(c1) + (bDiff * r2));

  // Perform the same set steps for the alpha channel
  // until each vertex is defined.
  alpha[0] = alpha1 + USHORT(alphaDiff * r1);
  alpha[1] = alpha1 + USHORT(alphaDiff * r2);

  // As the angle's start point changes quadrants, the colors will need to
  // rotate around the vertices to match the starting position.
  if (0 == (offset % 2))
  {
    std::swap(cC[0], cC[1]);
    std::swap(alpha[0], alpha[1]);
  }

  offset = std::abs(offset - 4) % 4;
  COLORREF clr[4] = { c1, cC[0], c2, cC[1]};
  std::rotate(clr, clr + offset, clr + 4);

  USHORT   alphaV[4] = { alpha1, alpha[0], alpha2, alpha[1]};
  std::rotate(alphaV, alphaV + offset, alphaV + 4);

  // Populate the data points.
  TRIVERTEX corners[4] =
  {
    {rc.left,  rc.top,    RVal16(clr[0]), GVal16(clr[0]), BVal16(clr[0]), alphaV[0]},  
    {rc.right, rc.top,    RVal16(clr[1]), GVal16(clr[1]), BVal16(clr[1]), alphaV[1]},
    {rc.right, rc.bottom, RVal16(clr[2]), GVal16(clr[2]), BVal16(clr[2]), alphaV[2]},
    {rc.left,  rc.bottom, RVal16(clr[3]), GVal16(clr[3]), BVal16(clr[3]), alphaV[3]}
  };

  // Create the mesh definitions for the square with only 2 polygons.
  const GRADIENT_TRIANGLE sqGradient[2] =
  {
    {0, 2, 3},
    {0, 1, 2}
  };

  BOOL result = ::GdiGradientFill(hdc, 
                                  corners, 
                                  4, 
                                  (PVOID)sqGradient, 
                                  2, 
                                  GRADIENT_FILL_TRIANGLE);

  return FALSE != result;
}

/******************************************************************************
Date:       7/22/2011
Purpose:    Creates a radial gradient.
            The gradient is approximated by breaking up the 
            circle into triangles, and using the Win32 GradientFill call
            to fill all of the individual triangles.
Parameters: hDC[in]:  The device context to write to.
            ctr[in]: The center point of the gradient.
            radius[in]: The radius length of the gradient fill.
            startAngle[in]: An offset angle to start the gradient segmentation.
              This angle is defined in radians.
            c1[in]: The color to use at the start of the gradient.
            c2[in]: The color to use at the end of the gradient.
            segments[in]: The number of segments to break the circle into.
              The default number of segments is 16.  
              3 is the absolute minimum, which will result in a triangle.
            alpha1[in]: Starting alpha level to associate with the gradient.
            alpha2[in]: Ending alpha level to associate with the gradient.
Return:     true  If the function succeeds
            false If an error occurs and the function fails.
*******************************************************************************/
bool SegmentedRadialGradient_(
  HDC hdc, 
  const POINT& ctr, 
  int radius,
  double startAngle,
  COLORREF c1,
  COLORREF c2,
  size_t segments,
  BYTE alpha1,
  BYTE alpha2
  )
{
  // Less than 3 segments cannot create anything useful with this algorithm.
  if (segments < 3)
  {
    return false;
  }

  // Allocate space for both the vertex and mesh definitions.
  const size_t      k_vertexCount = segments + 2;
  TRIVERTEX         *pVertex = new TRIVERTEX[k_vertexCount];
  GRADIENT_TRIANGLE *pMesh   = new GRADIENT_TRIANGLE[segments];

  // Populate the first point as the center point of the radial gradient.
  // This point will appear in every triangle mesh that is rendered.
  pVertex[0].x      = ctr.x;
  pVertex[0].y      = ctr.y;
  pVertex[0].Red    = bitblender::RVal16(c1); 
  pVertex[0].Green  = bitblender::GVal16(c1); 
  pVertex[0].Blue   = bitblender::BVal16(c1);
  pVertex[0].Alpha  = bitblender::ToColor16(alpha1);

  // Pre-calculate the radial offset around the circle for each segment.
  double segOffset = 1.0 / segments;
  double curOffset = startAngle + segOffset;

  // Populate each vertex and mesh segment.
  for (size_t index = 1; index < k_vertexCount; ++index)
  {
    // Populate the current vertex
    double angle = bitblender::k_2pi * curOffset;
    pVertex[index].x      = ctr.x + static_cast<LONG>(radius * cos(angle));
    pVertex[index].y      = ctr.y + static_cast<LONG>(radius * sin(angle));
    pVertex[index].Red    = bitblender::RVal16(c2); 
    pVertex[index].Green  = bitblender::GVal16(c2); 
    pVertex[index].Blue   = bitblender::BVal16(c2);
    pVertex[index].Alpha  = bitblender::ToColor16(alpha2);

    // Update the next segment;
    curOffset += segOffset;
  }

  // Populate the set of mesh segments.
  for (size_t meshIndex = 0; meshIndex < segments; ++meshIndex)
  {
    pMesh[meshIndex].Vertex1 = 0; // This will be the first vertex for every triangle.
    pMesh[meshIndex].Vertex2 = meshIndex + 1; 
    pMesh[meshIndex].Vertex3 = meshIndex + 2; 
  }

  // Self correction from the loop assignment.  
  // This will correctly set the last vertex to be within a valid range.
  // The last vertex, in the last mesh, will share a vertex with the first mesh.
  // The actual vertex that is shared:
  //    pMesh[0].Vertex2
  pMesh[segments - 1].Vertex3 = 1;

  // All of that setup, and only one call to GradientFill is required.
  BOOL retVal = ::GdiGradientFill(hdc, 
                                  pVertex,
                                  k_vertexCount,
                                  pMesh,
                                  segments,
                                  GRADIENT_FILL_TRIANGLE);

  // Release resources.
  delete[] pMesh;
  pMesh = NULL;

  delete[] pVertex;
  pVertex = NULL;

  return FALSE != retVal;
}

} // namespace anonymous
// END BitBlender.cpp

#endif // BITBLENDER_H_INCLUDED
// END BitBlender.h


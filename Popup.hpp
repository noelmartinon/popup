//  ===========================================================================
//  File    Popup.hpp
//  Desc    The interface of the CPopup class
//  ===========================================================================
#ifndef _CLASS_POPUP_
#define _CLASS_POPUP_

#include <windows.h>
#include "resource.h"
#include "BitBlender.hpp"
#include "Picture.hpp"

#define POPUPCLASSNAME "CPOPUP"

#define WM_SETPARAM         (WM_APP + 1)

#define POPUP_APPEAR_FIXE	        1
#define POPUP_APPEAR_SLIDE	        2
#define POPUP_APPEAR_FIXE_WIZZ	    3
#define POPUP_APPEAR_SLIDE_WIZZ	    4

#define POPUP_ICON_INFORMATION	    "1"
#define POPUP_ICON_WARNING	        "2"
#define POPUP_ICON_ERROR	        "3"
#define POPUP_ICON_OK	            "4"


int* pMapView_Popup;
HANDLE hFileMap_Popup;
bool bCurrentPopupReady=true; // used only in multi Popup() calls in the same process

int nScreenWidth;
int nScreenHeight;

LRESULT DefaultEditTextProc, DefaultEditTitleProc;

typedef struct{
    HWND hwnd;
    char title[256];
    char text[1024];
    char icon[512];
    char background[512];
    BYTE opacity;
    bool showOpaque;        // to always show with no opacity then fadeout if opacity<255
    int appearStyle;
    bool waitPrevious;      // to wait for there is no previous popup (but a new popup with 'waitPrevious'=false can show up immediatly after this one)
    bool lockNext;          // others new popups are locked (not shown) and waiting for this to closed
    int width;
    int height;
    int initialPosX;
    int initialPosY;
    int fixePosX;
    int fixePosY;
    int finalPosX;
    int finalPosY;
    int animDuration;
    int duration;
    int margin;             // margin from another popup in pixels
    bool windowReady;
    bool autoSize;
}PARAM, *PPARAM;

//  ===========================================================================
void SetWindowBlur(HWND hWnd)
{
	const HINSTANCE hModule = LoadLibrary(TEXT("user32.dll"));
	if (hModule)
	{
		struct ACCENTPOLICY
		{
			int nAccentState;
			int nFlags;
			int nColor;
			int nAnimationId;
		};
		struct WINCOMPATTRDATA
		{
			int nAttribute;
			PVOID pData;
			ULONG ulDataSize;
		};
		typedef BOOL(WINAPI*pSetWindowCompositionAttribute)(HWND, WINCOMPATTRDATA*);
		const pSetWindowCompositionAttribute SetWindowCompositionAttribute = (pSetWindowCompositionAttribute)GetProcAddress(hModule, "SetWindowCompositionAttribute");
		if (SetWindowCompositionAttribute)
		{
			ACCENTPOLICY policy = { 3, 0, 0, 0 }; // ACCENT_ENABLE_BLURBEHIND=3...
			WINCOMPATTRDATA data = { 19, &policy, sizeof(ACCENTPOLICY) }; // WCA_ACCENT_POLICY=19
			SetWindowCompositionAttribute(hWnd, &data);
		}
		FreeLibrary(hModule);
	}
}

//  ===========================================================================
//  Retrieve the dimension and position in screen coordinates of a window
//  ===========================================================================
void GetWindowCoordinates(HWND hWnd, int *x=NULL, int *y=NULL, int *h=NULL, int *w=NULL)
{
    RECT rect;
    GetWindowRect(hWnd, &rect);

    if (x)(*x) = rect.left;
    if (y)(*y) = rect.top;
    if (h)(*h) = rect.right-rect.left;
    if (w)(*w) = rect.bottom-rect.top;
}

//  ===========================================================================
//  Check if the mouse cursor is over the window
//  ===========================================================================
bool IsMouseOverWindow(HWND hwnd)
{
    POINT ptCursor;
    GetCursorPos(&ptCursor);

    HWND hwndOver=WindowFromPoint(ptCursor);
    do
    {
     if (hwndOver==hwnd) return true;
     hwndOver=GetParent(hwndOver);
    }
    while (hwndOver);

    return false;
}

//  ===========================================================================
//  Hexadecimal string conversion to int
//  ===========================================================================
int HexToDec(char c)
{
    if (c>='a' && c<='f') return 10+c-'a';
    if (c>='A' && c<='F') return 10+c-'A';
    if (c>='0' && c <= '9') return c-'0';
    return 0;
}
//  ---------------------------------------------------------------------------
int HexToDec(const char *hex)
{
    int dec=0;
    int coeff = 1;
    for (int i=strlen(hex)-1; i>=0; i--)
    {
    dec += coeff*HexToDec(hex[i]);
    coeff*=16;
    }
    return dec;
}

//  ===========================================================================
//  Some functions must be declared as static in CPopup class so we need
//  to have an external class for all functions to retrieve some values to
//  avoid compilation error 'invalid use of member in static member function'
//  or 'Unresolved external referenced from Popup.obj'
//  ===========================================================================
class CPopup_array
{
    public:
        CPopup_array(){
            next = NULL;
            m_hdcBackground = NULL;
            m_mustClose = false;
            m_crText = RGB(255,255,255);
        }

        ~CPopup_array(){
            DeleteDC(m_hdcBackground);
            DeleteObject(m_hBitmap);
        }

        int Count(){          
            int n = 0;
            CPopup_array *current = next;
            while (current) {
                current = current->next;
                n++;
            }
            return n;
        }

        void Add(HWND hwnd, HWND hwndClose, HDC hdcBackground, COLORREF crText){
            CPopup_array *popup_element = Exists(hwnd);
            if (!popup_element){
                popup_element = new CPopup_array;
                CPopup_array *last = this;
                while (last->next) last = last->next;
                last->next = popup_element;
            }

            popup_element->m_hwnd = hwnd;
            popup_element->m_hwndClose = hwndClose;

            popup_element->m_hdcBackground = hdcBackground;
            popup_element->m_crText = crText;
        }

        void Remove(HWND hwnd){
            if (!hwnd) return;
            CPopup_array *previous = this;
            CPopup_array *current = next;
            while (current) {
                if (hwnd == current->m_hwnd) {
                    CPopup_array *after = current->next;
                    delete current;
                    previous->next = after;
                    break;
                }
                previous = current;
                current = current->next;
            }
        }

        void SetClose(HWND hwnd, bool bClose=true){
            if (!hwnd) return;
            CPopup_array *current = next;
            while (current) {
                if (hwnd == current->m_hwnd) {
                    current->m_mustClose = bClose;
                }
                current = current->next;
            }
        }

        bool GetClose(HWND hwnd){
            if (!hwnd) return false;
            CPopup_array *current = next;
            while (current) {
                if (hwnd == current->m_hwnd) {
                    return current->m_mustClose;
                }
                current = current->next;
            }
            return false;
        }

        HDC GetHDC(HWND hwnd){
            if (!hwnd) return NULL;

            CPopup_array *current = next;
            while (current) {
                if (hwnd == current->m_hwnd) {
                    return current->m_hdcBackground;
                }
                current = current->next;
            }
            return NULL;
        }
        
        COLORREF GetTextColor(HWND hwnd){
            if (!hwnd) return NULL;

            CPopup_array *current = next;
            while (current) {
                if (hwnd == current->m_hwnd) {
                    return current->m_crText;
                }
                current = current->next;
            }
            return NULL;
        }

        bool AllExists(){
            bool ret = false;
            CPopup_array *current = next;
            while (current) {
                if (IsWindow(current->m_hwnd)){
                    ret = true;
                    break;
                }
                current = current->next;
            }
            return ret;
        }

        bool AllVisible(HWND hwnd = NULL){
            CPopup_array *current = next;
            while (current) {
                if (hwnd != current->m_hwnd && IsWindow(current->m_hwnd) && !IsWindowVisible(current->m_hwnd)){
                    return false;
                }
                current = current->next;
            }
            return true;
        }
        
        void SetBitmap(HWND hwnd, HBITMAP hBitmap){
            if (!hwnd) return;
            CPopup_array *current = next;
            while (current) {
                if (hwnd == current->m_hwnd) {
                    current->m_hBitmap = hBitmap;
                }
                current = current->next;
            }
        }

        HBITMAP GetBitmap(HWND hwnd){
            if (!hwnd) return NULL;
            CPopup_array *current = next;
            while (current) {
                if (hwnd == current->m_hwnd) {
                    return current->m_hBitmap;
                }
                current = current->next;
            }
            return NULL;
        }

    private:
        CPopup_array *next;
        HWND m_hwnd;
        HWND m_hwndClose;
        HDC m_hdcBackground;
        bool m_mustClose;
        COLORREF m_crText;
        HBITMAP m_hBitmap;

        CPopup_array* Exists(HWND hwnd){
            if (!hwnd) return NULL;
            CPopup_array *current = next;
            while (current) {
                if (hwnd == current->m_hwnd) {
                    return current;
                }
                current = current->next;
            }
            return NULL;
        }
} Popup_array;

//  ===========================================================================
BOOL WINAPI EnumPopup(HWND hwnd, LPARAM lParam)
{
    char ClassName[512];
    CPopup_array* Popup_existing = (CPopup_array*)lParam;

    GetClassName(hwnd, ClassName, sizeof(ClassName));
    if (!strcmp(ClassName, POPUPCLASSNAME))
        Popup_existing->Add(hwnd, NULL, NULL, NULL);

    return TRUE;
}

//  ===========================================================================
BOOL IntersectRect(CONST RECT *lprcSrc1,  CONST RECT *lprcSrc2)
{
    return (lprcSrc1->left < lprcSrc2->right &&
     lprcSrc1->left + lprcSrc1->right > lprcSrc2->left &&
     lprcSrc1->top < lprcSrc2->bottom &&
     lprcSrc1->bottom > lprcSrc2->top);
  
    /*
    // Adapted from source :
    // https://developer.mozilla.org/en-US/docs/Games/Techniques/2D_collision_detection
    long a_width = a.right - a.left;
    long a_height = a.bottom - a.top;
    long b_width = b.right - b.left;
    long b_height = b.bottom - b.top;
  
    return (a.left < b.left + b_width &&
     a.left + a_width > b.left &&
     a.top < b.top + b_height &&
     a_height + a.top > b.top);
     */
}

//  ===========================================================================
BOOL WINAPI EnumPopupIntersect(HWND hwnd, LPARAM lParam)
{
    char ClassName[512];

    PPARAM pparam_Data = (PPARAM)lParam;
    if (hwnd == pparam_Data->hwnd) return true;

    RECT rect;
      rect.left = pparam_Data->fixePosX - pparam_Data->margin;
      rect.top = pparam_Data->fixePosY - pparam_Data->margin;
      rect.right = pparam_Data->fixePosX + pparam_Data->width + pparam_Data->margin;
      rect.bottom = pparam_Data->fixePosY + pparam_Data->height + pparam_Data->margin;

    GetClassName(hwnd, ClassName, sizeof(ClassName));
    if (!strcmp(ClassName, POPUPCLASSNAME))
    {
        if (pparam_Data->waitPrevious && IsWindowVisible(hwnd)) {
           pparam_Data->windowReady=false;
           return false;
        }
    
        RECT rect_popup;
        GetWindowRect(hwnd, &rect_popup);

        // Test if the windows intersect and their x or y coordinates are the same
        // so if an existing popup slides out then the current can start to appear now
        if (IntersectRect(&rect, &rect_popup) &&  // Replace : RECT ResultRect; + if (IntersectRect(&ResultRect, &rect, &rect_popup)
            (rect.left+pparam_Data->margin == rect_popup.left || rect.right-pparam_Data->margin == rect_popup.right))
        {
            pparam_Data->windowReady=false;
            return false;
        }
    }
    return TRUE;
}

//  ===========================================================================
//  Class   CPopup
//  Desc    Use it for displaying popup screen for applications
//          Works on Win2000, WinXP and later versions of Windows
//  ===========================================================================
class CPopup
{
public:
    //  =======================================================================
    //  Func   CPopup
    //  Desc   Default constructor
    //  =======================================================================
    CPopup();

    //  =======================================================================
    //  Func   CPopup
    //  Desc   Constructor
    //  Arg    Path of the Bitmap that will be show on the popup screen
    //  Arg    The color on the bitmap that will be made transparent
    //  =======================================================================
    CPopup(LPCTSTR lpszFileName, COLORREF colTrans);

    //  =======================================================================
    //  Func   CPopup
    //  Desc   Constructor
    //  Arg    Path of the Bitmap that will be show on the popup screen
    //  Arg    - The color on the bitmap that will be made transparent
    //         - The opacity value
    //  =======================================================================
    CPopup(LPCTSTR lpszFileName, COLORREF colTrans, BYTE opacity);

    //  =======================================================================
    //  Func   ~CPopup
    //  Desc   Desctructor
    //  =======================================================================
    virtual ~CPopup();

    HWND GetHandle();
    HWND Create();

    //  =======================================================================
    //  Func   Show
    //  Desc   Launches the non-modal popup screen
    //  Ret    void
    //  =======================================================================
    void Show();

    //  =======================================================================
    //  Func   SetBitmap
    //  Desc   Call this with the path of the bitmap. Not required to be used
    //         when the construcutor with the image path has been used.
    //  Ret    true if succesfull
    //  Arg    Either the file path or the handle to an already loaded bitmap
    //  =======================================================================
    bool SetBitmap(LPCTSTR lpszFileName);
    bool SetBitmap(UINT uBitmap);
    bool SetBitmap(HBITMAP hBitmap);

    //  =======================================================================
    //  Func   SetTransparentColor
    //  Desc   This is used to make one of the color transparent
    //  Ret    1 if succesfull
    //  Arg    The colors RGB value. Not required if the color is specified
    //         using the constructor
    //  =======================================================================
    bool SetTransparentColor(COLORREF col);

    //  =======================================================================
    //  Func   SetOpacity
    //  Desc   This is used to change opacity
    //  Ret    1 if succesfull
    //  Arg    The opacity level value between 0 and 255
    //  =======================================================================
    bool SetOpacity(BYTE opacity);

    //  =======================================================================
    bool SetShowOpaque(bool bShowOpaque);
    void SetSize(int Width=-1, int Height=-1);
    void SetAppearStyle(int AppearStyle);
    void SetIcon(char *s);
    void SetTitle(char *s);
    void SetText(char *s);
    void SetOpacityEndFading(BYTE OpacityEndFading);
    void SetAnimationParam(int AnimDuration=200, int Duration=8000);
    void SetAutoSize(bool bAutoSize);
    void SetColor(const char* hexColor=NULL);
    void SetColor(COLORREF bgColor=RGB(0,0,0));
    void SetWaitPrevious(bool bWaitPrevious);
    void SetLockNext(bool bLockNext);

private:
    HWND m_hwnd; 
    void Init();
    void FreeResources();
    static DWORD WINAPI Animate( LPVOID lpParam );
    static void AnimateShow(HWND hwnd, int FinalPosX, int FinalPosY, int Width, int Height, int AnimDuration, BYTE Opacity);
    static void AnimateHide(HWND hwnd, int FinalPosX, int FinalPosY, int Width, int Height, int AnimDuration, BYTE Opacity);
    static bool ApplyTransparency(HWND hwnd, BYTE m_Opacity);
    static bool Wizz(HWND hwnd);
    static LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK EditTitleProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK EditProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static HWND CreateChildWindow( HWND hwnd, int x, int y, int cx, int cy, WNDPROC wndProcName, CHAR *ClassName, CHAR *WindowName);
    static LRESULT APIENTRY CloseProc( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam );
    HDC *GetDC_Background(HWND hwnd);

    char Title[256];
    char Text[1024];
    char Icon[512];
    COLORREF m_colTrans;
    BYTE m_Opacity;
    bool m_ShowOpaque;
    int m_Width;
    int m_Height;
    bool m_AutoSize;
    //int Icon;
    HBITMAP m_hBitmap;
    LPCTSTR m_lpszClassName;
    int CurrentPosX;
    int CurrentPosY;
    int InitialPosX;
    int InitialPosY;
    int FixePosX;
    int FixePosY;
    int FinalPosX;
    int FinalPosY;
    int m_nAnimDuration;
    int m_nDuration;
    int m_nAppearStyle;
    COLORREF m_bgColor;
    bool m_waitPrevious;
    bool m_lockNext;
};

//  ===========================================================================
HDC *CPopup::GetDC_Background(HWND hwnd)
{
    return NULL;
}

//  ===========================================================================
DWORD WINAPI CPopup::Animate( LPVOID lpParam )
{
    PPARAM pData;
    pData = (PPARAM)lpParam;
    HWND hwnd = pData->hwnd;
    int AppearStyle = pData->appearStyle;
    int FixePosX = pData->fixePosX;
    int FixePosY = pData->fixePosY;
    int InitialPosX = pData->initialPosX;
    int InitialPosY = pData->initialPosY;
    int FinalPosX = pData->finalPosX;
    int FinalPosY = pData->finalPosY;
    int AnimDuration = pData->animDuration;
    int Duration = pData->duration;
    BYTE Opacity = pData->opacity;
    bool bSetOpacityMax = pData->showOpaque;

    int CurrentPosX=InitialPosX;
    int CurrentPosY=InitialPosY;

    int debutPhase;
    float pourcentage;
    float elapse;

    BYTE currentOpacity=Opacity;
    BYTE initialOpacity=Opacity;

    if (bSetOpacityMax) {
        currentOpacity = 255;
        initialOpacity = 255;
    }
    
    AnimateShow(hwnd, FixePosX, FixePosY, pData->width, pData->height, AnimDuration, initialOpacity);
    
    CurrentPosX=FixePosX;
    CurrentPosY=FixePosY;
    
    // Slight shake of the slide window popup when appears with 'slide' style
    if (AppearStyle==POPUP_APPEAR_SLIDE_WIZZ || AppearStyle==POPUP_APPEAR_SLIDE)
    {
       int decalX=InitialPosX-FixePosX;
       if (decalX>0) decalX=1;
       else if (decalX<0) decalX=-1;
       else decalX=0;
      
       int decalY=InitialPosY-FixePosY;
       if (decalY>0) decalY=1;
       else if (decalY<0) decalY=-1;
       else decalY=0;

       for (int i=0; i<=8; i++)
       {
           CurrentPosX+=decalX;
           CurrentPosY+=decalY;
           SetWindowPos(hwnd, 0, CurrentPosX, CurrentPosY, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
           Sleep(1);
       }
       for (int i=8; i>=0; i--)
       {
           CurrentPosX-=decalX;
           CurrentPosY-=decalY;
           SetWindowPos(hwnd, 0, CurrentPosX, CurrentPosY, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
           Sleep(1);
       }
    }

    if (AppearStyle==POPUP_APPEAR_SLIDE_WIZZ || AppearStyle==POPUP_APPEAR_FIXE_WIZZ) Wizz(hwnd);

    //PlaySound("SystemAsterisk", NULL, SND_ALIAS|SND_ASYNC);

    // Reset the global popup marker if it not locks next
    if (!pData->lockNext)
    {
        *pMapView_Popup = 0;
        UnmapViewOfFile(pMapView_Popup);
        CloseHandle(hFileMap_Popup);
        bCurrentPopupReady=true;
    }

    bool currentStatOver=IsMouseOverWindow(hwnd);
    bool prevStatOver=!currentStatOver;

    // Run animation to defined opacity if 'showOpaque' is set
    if (bSetOpacityMax && !currentStatOver)
    {
        debutPhase = GetTickCount();
        while(currentOpacity > Opacity)
        {
          elapse=GetTickCount()-debutPhase;
          pourcentage=elapse/500;
          if (pourcentage>1) pourcentage=1;
          currentOpacity = initialOpacity - abs((255-Opacity)*pourcentage);
          ApplyTransparency(hwnd, currentOpacity);
          Sleep(1);
          if (!IsMouseOverWindow(hwnd)) break;
        }
    }

    //SetWindowBlur(hwnd);

    // Wait at fixes coordinates
    int wait=GetTickCount();
    while (GetTickCount()-wait<=Duration || !Duration)
    {
        currentStatOver=IsMouseOverWindow(hwnd);
        elapse=0;
        debutPhase = GetTickCount();
        initialOpacity = currentOpacity;
        
        if (!prevStatOver && currentStatOver)
            while(currentOpacity<255 && IsMouseOverWindow(hwnd))
            {
                elapse=GetTickCount()-debutPhase;
                pourcentage=elapse/200;
                if (pourcentage>1) pourcentage=1;
                currentOpacity = initialOpacity + abs((255-initialOpacity)*pourcentage);
                ApplyTransparency(hwnd, currentOpacity);
                Sleep(1);
            }
        
        else if (prevStatOver && !currentStatOver)
            while(currentOpacity>Opacity && !IsMouseOverWindow(hwnd))
            {
                elapse=GetTickCount()-debutPhase;
                pourcentage=elapse/500;
                if (pourcentage>1) pourcentage=1;if (pourcentage>1) pourcentage=1;
                //if (pourcentage<0) pourcentage=0;
                currentOpacity = initialOpacity - abs((255-Opacity)*pourcentage);
                ApplyTransparency(hwnd, currentOpacity);
                Sleep(1);
            }
        
        if (currentStatOver)
            wait=GetTickCount();

        prevStatOver = currentStatOver;

        if(Popup_array.GetClose(hwnd)) break; 

        Sleep(10);
    }

    // Hide the popup
    AnimateHide(hwnd, FinalPosX, FinalPosY, pData->width, pData->height, AnimDuration, currentOpacity);

    // Reset the global popup marker if needed
    if (pData->lockNext)
    {
        *pMapView_Popup = 0;
        UnmapViewOfFile(pMapView_Popup);
        CloseHandle(hFileMap_Popup);
        bCurrentPopupReady=true;
    }

    PostMessage(hwnd, WM_QUIT, 0, 0);
    return 0;
}

//  =======================================================================
void CPopup::AnimateShow(HWND hwnd, int FinalPosX=NULL, int FinalPosY=NULL, int Width=NULL, int Height=NULL, int AnimDuration=NULL, BYTE Opacity=NULL)
{
    int InitialPosX, InitialPosY;
    GetWindowCoordinates(hwnd, &InitialPosX, &InitialPosY);
    if (!FinalPosX)
    {
      FinalPosX=InitialPosX;
      FinalPosY=InitialPosY;
      AnimDuration=300;
    }
  
    int CurrentPosX=InitialPosX;
    int CurrentPosY=InitialPosY;
    int debutPhase;
    float pourcentage;
    float elapse;
  
    debutPhase = GetTickCount();
    elapse=0;

    while ( FinalPosX!=CurrentPosX || FinalPosY!=CurrentPosY || elapse<AnimDuration )
    {
        Sleep(1);
        elapse=GetTickCount()-debutPhase;
        pourcentage=elapse/AnimDuration;
        pourcentage*=pourcentage;
        if (pourcentage>1) pourcentage=1;
        
        ApplyTransparency(hwnd, abs(Opacity*pourcentage));
        
        CurrentPosX=InitialPosX+pourcentage*(FinalPosX-InitialPosX);
        if ((CurrentPosX > FinalPosX && InitialPosX < FinalPosX) || (CurrentPosX < FinalPosX && InitialPosX > FinalPosX))
            CurrentPosX=FinalPosX;
        
        CurrentPosY=InitialPosY+pourcentage*(FinalPosY-InitialPosY);
        if ((CurrentPosY > FinalPosY && InitialPosY < FinalPosY) || (CurrentPosY < FinalPosY && InitialPosY > FinalPosY))
            CurrentPosY=FinalPosY;
        
        // Out of screen window's part is hidden when overflow on right and bottom
        int width, height;
        if (CurrentPosX < nScreenWidth && CurrentPosX+Width > nScreenWidth)
            width = nScreenWidth-CurrentPosX;
        else width = Width;

        if (CurrentPosY < nScreenHeight && CurrentPosY+Height > nScreenHeight)
            height = nScreenHeight-CurrentPosY;
        else height = Height;

        // Set position and size
        SetWindowPos(hwnd, 0, CurrentPosX, CurrentPosY, width, height, SWP_NOACTIVATE);
        
        if (elapse > AnimDuration) break;
    }
}

//  =======================================================================
void CPopup::AnimateHide(HWND hwnd, int FinalPosX=NULL, int FinalPosY=NULL, int Width=NULL, int Height=NULL, int AnimDuration=NULL, BYTE Opacity=NULL)
{  
    int InitialPosX, InitialPosY;
    GetWindowCoordinates(hwnd, &InitialPosX, &InitialPosY);
    if (!FinalPosX)
    {
        FinalPosX=InitialPosX;
        FinalPosY=InitialPosY;
        AnimDuration=300;
    }
  
    int CurrentPosX=InitialPosX;
    int CurrentPosY=InitialPosY;
    int debutPhase;
    float pourcentage;
    float elapse;

    debutPhase = GetTickCount();
    elapse=0;

    while ( FinalPosX!=CurrentPosX || FinalPosY!=CurrentPosY  || elapse<AnimDuration)
    {
        Sleep(1);
        
        elapse=GetTickCount()-debutPhase;
        pourcentage=1-elapse/AnimDuration;
        pourcentage*=pourcentage;
        if (pourcentage<0) pourcentage=0;
        
        ApplyTransparency(hwnd, abs(Opacity*pourcentage));
        
        CurrentPosX=InitialPosX+(1-pourcentage)*(FinalPosX-InitialPosX+1);
        if ((CurrentPosX > FinalPosX && InitialPosX < FinalPosX) || (CurrentPosX < FinalPosX && InitialPosX > FinalPosX))
          CurrentPosX=FinalPosX;
        
        CurrentPosY=InitialPosY+(1-pourcentage)*(FinalPosY-InitialPosY+1);
        if ((CurrentPosY > FinalPosY && InitialPosY < FinalPosY) || (CurrentPosY < FinalPosY && InitialPosY > FinalPosY))
          CurrentPosY=FinalPosY;
        
        // Out of screen window's part is hidden when overflow on right and bottom
        int width, height;
        if (CurrentPosX < nScreenWidth && CurrentPosX+Width > nScreenWidth)
            width = nScreenWidth-CurrentPosX;
        else width = Width;

        if (CurrentPosY < nScreenHeight && CurrentPosY+Height > nScreenHeight)
            height = nScreenHeight-CurrentPosY;
        else height = Height;
      
        // Set position and size
        SetWindowPos(hwnd, 0, CurrentPosX, CurrentPosY, width, height, SWP_NOACTIVATE);
  
        if (elapse > AnimDuration) break;
    }
}

//  ===========================================================================
bool CPopup::ApplyTransparency(HWND hwnd, BYTE m_Opacity)
{
    if (m_Opacity<0) return false;
  
    if (!(GetWindowLong(hwnd, GWL_EXSTYLE) & WS_EX_LAYERED))
        SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);
    
    SetLayeredWindowAttributes(hwnd,RGB(0xff,0xff,0xff), m_Opacity, LWA_ALPHA);

    return true;
}

//  ===========================================================================
bool CPopup::Wizz(HWND hwnd)
{
    if (!IsWindowVisible(hwnd)) return false;
  
    RECT rect;
    GetWindowRect( hwnd,&rect );  
  
    for(int i = 0; i < 15; i++)
    {
        SetWindowPos(hwnd, 0, rect.left - 5, rect.top, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
        Sleep(10);
        SetWindowPos(hwnd, 0, rect.left, rect.top - 5, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
        Sleep(10);
        SetWindowPos(hwnd, 0, rect.left + 5, rect.top, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
        Sleep(10);
        SetWindowPos(hwnd, 0, rect.left, rect.top + 5, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
        Sleep(10);
    }

    SetWindowPos(hwnd, 0, rect.left, rect.top, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
    
    return true;
}

//  ===========================================================================
LRESULT APIENTRY CPopup::CloseProc( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam ) {
    static HBITMAP closeButton[2], hbmMask;
    HWND hbtClose;
    static int imageClosed;
    BITMAP bm;
    HDC hdc, hdcMem;
    PAINTSTRUCT ps;
    RECT rect;
    switch (message) {
        case WM_CREATE:
            closeButton[0] = LoadBitmap( GetModuleHandle(NULL), MAKEINTRESOURCE(CLOSE_OFF) );
            closeButton[1] = LoadBitmap( GetModuleHandle(NULL), MAKEINTRESOURCE(CLOSE_ON) );

            hbmMask=CreateBitmapMask(closeButton[ 0 ], RGB (0, 0, 0));
            imageClosed = 1;
            return 0;

        case WM_LBUTTONDOWN:
            imageClosed = 0;
            Popup_array.SetClose(GetParent(hwnd), true);
            return 0;

        case WM_PAINT:
        {
            hdc = BeginPaint( hwnd, &ps );

            //GetClientRect( hwnd, &rect );
            hdcMem = CreateCompatibleDC( hdc );
            SelectObject( hdcMem, closeButton[ imageClosed ] );
            GetObject( closeButton[ imageClosed ], sizeof (BITMAP), (PSTR) &bm );

            SelectObject(hdcMem, hbmMask);
            BitBlt(hdc, 0, 0, bm.bmWidth, bm.bmHeight, hdcMem, 0, 0, SRCAND);

            SelectObject(hdcMem, closeButton[ imageClosed ]);
            BitBlt(hdc, 0, 0, bm.bmWidth, bm.bmHeight, hdcMem, 0, 0, SRCPAINT);

            DeleteDC( hdcMem );

            EndPaint( hwnd, &ps );
            return 0 ;
        }
        case WM_MOUSEHOVER:
            imageClosed = 0;
            InvalidateRect( hwnd, 0, TRUE );
            break;

        case WM_MOUSELEAVE:
            imageClosed = 1;
            InvalidateRect( hwnd, 0, TRUE );
            break;

        case WM_MOUSEMOVE:
            TRACKMOUSEEVENT trmouse;
            trmouse.cbSize = sizeof(TRACKMOUSEEVENT);
            trmouse.dwFlags = TME_LEAVE | TME_HOVER;// | TME_NONCLIENT;
            trmouse.dwHoverTime = 1;
            trmouse.hwndTrack = hwnd;
            if(!TrackMouseEvent(&trmouse))
                return FALSE;
            break;

        case WM_DESTROY:
             DeleteObject(closeButton[0]);
             DeleteObject(closeButton[1]);
             DeleteObject(hbmMask);
             return 0;
    }
    return DefWindowProc( hwnd, message, wParam, lParam );
}

//  ===========================================================================
HWND CPopup::CreateChildWindow( HWND hwnd, int x, int y, int cx, int cy, WNDPROC wndProcName, CHAR *ClassName, CHAR *WindowName=NULL)
{
    HINSTANCE hInstance;
    WNDCLASS wndclass;
    HWND hwnd_ret;

    hInstance = (HINSTANCE) GetWindowLong (hwnd, GWL_HINSTANCE);
    wndclass.style         = CS_HREDRAW | CS_VREDRAW;
    wndclass.cbClsExtra    = 0;
    wndclass.cbWndExtra    = 0;
    wndclass.hInstance     = hInstance;
    wndclass.hIcon         = NULL;
    wndclass.hCursor       = LoadCursor (NULL, IDC_ARROW);
    wndclass.hbrBackground = (HBRUSH) GetStockObject (NULL_BRUSH);
    wndclass.lpszMenuName  = NULL;
    wndclass.lpfnWndProc   = wndProcName;
    wndclass.lpszClassName = ClassName;

    RegisterClass (&wndclass) ;

    hwnd_ret = CreateWindowEx( WS_EX_TRANSPARENT,ClassName, WindowName,
                                WS_CHILD | WS_VISIBLE,
                                x, y, cx, cy,
                                hwnd,0,hInstance,0);
    
    UnregisterClass(ClassName,hInstance);

    return hwnd_ret;
}

//  ===========================================================================
LRESULT CALLBACK CPopup::EditProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        // On mouse over an edit control, cursor is always arrow
        case WM_SETCURSOR:
            SetCursor(LoadCursor(NULL, IDC_ARROW));
            return TRUE;

        // On click on an edit control, the default menu is not displayed
        case WM_RBUTTONDOWN:
        case WM_LBUTTONDOWN:
            return 0;

        // Draw part of background on edit control
        case WM_ERASEBKGND:
            POINT pt;
            RECT rect;
            GetClientRect(hWnd,&rect);// Retrieves the coordinates of the window's client area
            pt.x=rect.left; pt.y=rect.top;
            ClientToScreen(hWnd,&pt);// converts the client coordinates to screen coordinates
            ScreenToClient(GetParent(hWnd),&pt);// converts to parent window coordinates

            BitBlt((HDC)wParam,0,0,rect.right,rect.bottom, Popup_array.GetHDC(GetParent(hWnd)),pt.x,pt.y,SRCCOPY);
            return 1;
    }
    return (CallWindowProc((WNDPROC)DefaultEditTextProc, hWnd, uMsg, wParam, lParam ));
}

//  ===========================================================================
LRESULT CALLBACK CPopup::EditTitleProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        // On mouse over an edit control, cursor is always arrow
        case WM_SETCURSOR:
            SetCursor(LoadCursor(NULL, IDC_ARROW));
            return TRUE;

        // On click on an edit control, the default menu is not displayed
        case WM_RBUTTONDOWN:
        case WM_LBUTTONDOWN:
            return 0;

        // Draw part of background on edit control
        case WM_ERASEBKGND:
            POINT pt;
            RECT rect;
            GetClientRect(hWnd,&rect);// Retrieves the coordinates of the window's client area
            pt.x=rect.left; pt.y=rect.top;
            ClientToScreen(hWnd,&pt);// converts the client coordinates to screen coordinates
            ScreenToClient(GetParent(hWnd),&pt);// converts to parent window coordinates

            BitBlt((HDC)wParam,0,0,rect.right,rect.bottom, Popup_array.GetHDC(GetParent(hWnd)),pt.x,pt.y,SRCCOPY);
            return 1;
    }
    return (CallWindowProc((WNDPROC)DefaultEditTitleProc, hWnd, uMsg, wParam, lParam ));
}

//  ===========================================================================
//  Func    MainWndProc
//  Desc    The windows procedure that is used to forward messages to the
//          CPopup class. CPopup sends the "this" pointer through the
//          CreateWindowEx call and the pointer reaches here in the
//          WM_CREATE message. We store it here and use it for message
//          forwarding.
//  ===========================================================================
LRESULT CALLBACK CPopup::MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static HWND hbtClose;
    HDC hdc;
    PAINTSTRUCT ps;
    RECT rect;

    //HFONT fonttitle=CreateFont(15, 0, 0, 0, FW_BOLD, 0, 0, 0, 0, 0, 0, 0, 0, "MS Shell Dlg");
    //HFONT font=CreateFont(15, 0, 0, 0, FW_NORMAL, 0, 0, 0, 0, 0, 0, 0, 0, "MS Shell Dlg");

    // Determine what font to use for the text.
    LOGFONT lf = {0};
    NONCLIENTMETRICS ncm = { sizeof(NONCLIENTMETRICS) };
    SystemParametersInfo (SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncm, false );
    lf = ncm.lfMessageFont;
    HFONT font = CreateFontIndirect ( &lf );

    lf.lfWeight = FW_BOLD;
    HFONT fonttitle = CreateFontIndirect ( &lf );

    switch (msg)
    {
        case WM_CREATE:
        {
            // Get 'this' pointer from CreateWindowEx()
            CREATESTRUCT *cs = (CREATESTRUCT*)lParam;
            CPopup* popup = (CPopup*)cs->lpCreateParams;

            BITMAP bm; // use to obtain HBITMAP informations
            int text_margin_top=56;
            int text_margin_bottom=10;
            int width, height;
            
            width = popup->m_Width;
            height = popup->m_Height;
                                       
            // Create close button
            // before hwndEditTitle to assure button is over others controls (Z Order)
            HBITMAP hbmTemp = LoadBitmap( GetModuleHandle(NULL), MAKEINTRESOURCE(CLOSE_ON) );
            GetObject(hbmTemp, sizeof(BITMAP), &bm);
            DeleteObject(hbmTemp);
            hbtClose=CreateChildWindow( hwnd, width-bm.bmWidth-5, 5+15,bm.bmWidth, bm.bmHeight, CloseProc, (char*)"CloseProc" );
            int width_Close=bm.bmWidth, height_Close=bm.bmHeight;

            // Create edit controls for title and text (text margin right is 15 pixels)
            // and resize popup if necessary
            HWND hwndEditTitle=CreateWindowEx(WS_EX_TRANSPARENT, "EDIT", popup->Title,
            WS_VISIBLE | WS_CHILD | WS_TABSTOP | ES_LEFT,
            45, 25, width-60, 20,
            hwnd, NULL, GetModuleHandle(NULL), NULL);
            DefaultEditTitleProc = SetWindowLong(hwndEditTitle,GWL_WNDPROC,(LONG) (WNDPROC) EditTitleProc);
            SendMessage(hwndEditTitle,WM_SETFONT,(WPARAM)fonttitle,MAKELPARAM(true,0));

            HWND hwndEdit=CreateWindowEx(0, "EDIT", popup->Text,
            WS_VISIBLE | WS_CHILD | WS_TABSTOP | SS_LEFT | ES_MULTILINE,
            30, text_margin_top, width-45, height-(text_margin_top+text_margin_bottom),
            hwnd, NULL, GetModuleHandle(NULL), NULL);
            DefaultEditTextProc = SetWindowLong(hwndEdit,GWL_WNDPROC,(LONG) (WNDPROC) EditProc);
            SendMessage(hwndEdit,WM_SETFONT,(WPARAM)font,MAKELPARAM(true,0));

            TEXTMETRIC	textMetric;
            GetTextMetrics(GetDC(hwndEdit), &textMetric);
            int nPixelsPerLine = textMetric.tmHeight + textMetric.tmExternalLeading; // character height in current font

            int nLinesCount;
            nLinesCount = SendMessage(hwndEdit, EM_GETLINECOUNT,0,0);

            int newHeight = nLinesCount*nPixelsPerLine + text_margin_top + text_margin_bottom;
            if (newHeight>height && popup->m_AutoSize)
            // adjust popup and window text sizes
            {
                SetWindowPos(hwnd, 0, 0, 0, width, newHeight, SWP_NOMOVE | SWP_NOACTIVATE);
                SetWindowPos(hwndEdit, 0, 0, 0, width-45, newHeight-(text_margin_top+text_margin_bottom), SWP_NOMOVE | SWP_NOACTIVATE);
                popup->m_Height = newHeight;
                height = newHeight;
            }
            else
            // keep popup size and adjust window text size/position to have nice design
            {
                int newPosY=(height-(text_margin_top+text_margin_bottom)-nPixelsPerLine*nLinesCount)/3;
                if (newPosY >0) MoveWindow(hwndEdit, 30, text_margin_top+newPosY, width-45, height-newPosY-(text_margin_top+text_margin_bottom), true);
            }

            // Initialize background bitmap
            GetClientRect( hwnd, &rect );
            HDC hdcBkgnd = CreateCompatibleDC(0);
            HBITMAP hbmBkgnd = NULL;
            if (popup->m_hBitmap) hbmBkgnd = popup->m_hBitmap;
            else hbmBkgnd = CreateBitmap(width, height, 1, 32, NULL); // or rect.right,rect.bottom
            SelectObject( hdcBkgnd, hbmBkgnd );

            // Get icon
            HBITMAP hbmIcon=NULL;
            if (strcmp(popup->Icon, POPUP_ICON_INFORMATION)==0 || stricmp(popup->Icon, "INFORMATION")==0) {
                hbmIcon = ResourceToBitmap(0, (LPCTSTR)IMG_INFORMATION, "PNG"); popup->m_bgColor = RGB(65,105,225);}
            else if (strcmp(popup->Icon, POPUP_ICON_WARNING)==0 || stricmp(popup->Icon, "WARNING")==0) {
                hbmIcon = ResourceToBitmap(0, (LPCTSTR)IMG_WARNING, "PNG"); popup->m_bgColor = RGB(255,165,0);}
            else if (strcmp(popup->Icon, POPUP_ICON_ERROR)==0 || stricmp(popup->Icon, "ERROR")==0) {
                hbmIcon = ResourceToBitmap(0, (LPCTSTR)IMG_ERROR, "PNG"); popup->m_bgColor = RGB(255,69,0);}
            else if (strcmp(popup->Icon, POPUP_ICON_OK)==0 || stricmp(popup->Icon, "OK")==0) {
                hbmIcon = ResourceToBitmap(0, (LPCTSTR)IMG_OK, "PNG"); popup->m_bgColor = RGB(50,205,50);}
            else {
                SIZE size = {32,32};
                HBITMAP hBitmap = FileToBitmap(popup->Icon);
                hbmIcon = ResizeBmp(hBitmap, size);
                DeleteObject(hBitmap);

                if (!hbmIcon)
                    hbmIcon = ResourceToBitmap(0, (LPCTSTR)IMG_INFORMATION, "PNG");
            }

            //
            // Draw main background image
            //
            SelectObject( hdcBkgnd, hbmBkgnd );
            if (!popup->m_hBitmap)
                bitblender::AngularGradient(hdcBkgnd, rect, 45*bitblender::k_degToRad, RGB(0,0,0), popup->m_bgColor);

            int x=0, y=0, w=0, h=0; // Region's bounds
            int margin = 5; // default margin
            HRGN hRgn;

            // Gradient background
            if (!popup->m_hBitmap)
            {
                // Create bitmap to use as a region (transparent color is black)
                HDC hdcRgn = CreateCompatibleDC(0);
                HBITMAP hbmRgn =  CreateBitmap(popup->m_Width, popup->m_Height, 1, 32, NULL);//CreateCompatibleBitmap(hdcRgn, m_Width, m_Height);
                SelectObject(hdcRgn, hbmRgn);
                SelectObject(hdcRgn, GetStockObject(WHITE_BRUSH));
                SelectObject(hdcRgn, GetStockObject(WHITE_PEN));

                RoundRect(hdcRgn,15, 15, popup->m_Width, popup->m_Height, 25,25);
                Ellipse(hdcRgn, 0,0,43,43);

                // Using BitmapToRegion() because result is smoother (anti-aliasing) than CreateRoundRectRgn() and CreateEllipticRgn()
                hRgn = BitmapToRegion(hbmRgn, hdcRgn);

                DeleteObject(hbmRgn);
                DeleteDC( hdcRgn );
            }
            // Bitmap background
            else
            {
                // Use m_colTrans or pixel(0,0) to define transparent background color
                if (popup->m_colTrans) hRgn = BitmapToRegion(hbmBkgnd, hdcBkgnd, popup->m_colTrans, 0x888888);
                else hRgn = BitmapToRegion(hbmBkgnd, hdcBkgnd, GetPixel(hdcBkgnd,0,0), 0x888888);

                // Search for the max display bounds
                bool isObjVisible = false;
                x=0, y=0, w=0, h=0;
                int xx_diagonal=0, yy_diagonal=0;

                // Top left
                for (int xx=0; xx<=width && !isObjVisible; xx++)
                for (int yy=0; yy<=height && !isObjVisible; yy++)
                {
                    if (PtInRegion(hRgn, xx_diagonal, yy)) xx=xx_diagonal;
                    if (PtInRegion(hRgn, xx, yy)) {
                        x=xx; // Here x = x left
                        y=yy; // Here y = y top

                        // Top right
                        for (xx=x; xx<=width; xx++)
                            if (!PtInRegion(hRgn, xx, y)){
                                xx--;
                                break;
                            }

                        w=xx-x; // Here xx = x right

                        // Bottom right
                        for (yy=y; yy<=height; yy++)
                            if (!PtInRegion(hRgn, xx, yy)){
                                yy--; // must substract 1 pixel because last point was out of the region
                                break;
                            }

                        // Bottom left (adjust yy)
                        // Here yy = y bottom before left adjustment
                        while (yy>=y && !PtInRegion(hRgn, x, yy)) yy--;

                        h=yy-y; // Here yy = y bottom

                        isObjVisible = true;
                    }
                    xx_diagonal++;
                }

                // Adjust the elements (position/size of close button, title, text)
                SetWindowPos(hwndEditTitle, 0,
                    x+margin+40,
                    y+margin+15,
                    w-2*margin-40-width_Close/2,
                    20,
                    SWP_NOACTIVATE);
                SetWindowPos(hwndEdit, 0,
                    x+margin+20, 
                    y+margin+text_margin_top-10, 
                    w-2*margin-20-width_Close/2, 
                    h-margin-margin/2-text_margin_top+10, //margin bottom is set to his half value
                    SWP_NOACTIVATE);
                SetWindowPos(hbtClose, 0, 
                    x+w-margin-width_Close, 
                    y+margin+10, 
                    0, 0, SWP_NOSIZE);
            }

            //
            // Draw icon
            //
            // NOTA: Top left corner had black background to hide the display defects (black pixels) due to the transparency applied to the icons
            // but finally with AlphaBlend() function it's not necessary

            /*
            // Using bitmap mask from file: good solution to have nice display
            HDC hdcIcon = CreateCompatibleDC(0);
            HBITMAP hbmIcon_mask = ResourceToBitmap(0, (LPCTSTR)IMG_MASK, "IMAGE");
            SelectObject(hdcIcon, hbmIcon_mask);
            BitBlt(hdcBkgnd, 5, 5, 32, 32, hdcIcon, 0, 0, SRCAND);
            SelectObject(hdcIcon, hbmIcon);
            BitBlt(hdcBkgnd, 5, 5, 32, 32, hdcIcon, 0, 0, SRCPAINT);
            DeleteObject(hbmIcon_mask);
            DeleteDC(hdcIcon);
            */

            /*
            // Using a auto-generated bitmap mask: not so clean than previous solution
            HDC hdcIcon = CreateCompatibleDC(0);
            HBITMAP hbmIcon_mask = CreateBitmapMask(hbmIcon, RGB(0, 0, 0));
            SelectObject(hdcIcon, hbmIcon_mask);
            BitBlt(hdcBkgnd, 5, 5, 32, 32, hdcIcon, 0, 0, SRCAND);
            SelectObject(hdcIcon, hbmIcon);
            BitBlt(hdcBkgnd, 5, 5, 32, 32, hdcIcon, 0, 0, SRCPAINT);
            DeleteObject(hbmIcon_mask);
            DeleteDC(hdcIcon);
            */

            /*
            // Same solution as previous but with direct device context draw (no hdcIcon)
            DrawTransparent(hdcBkgnd, 5, 5, hbmIcon, RGB(0, 0, 0));
            */

            /*
            // Using a drawn by bitmap mask: not so bad
            HDC hdcIcon = CreateCompatibleDC(0);
            HBITMAP hbmIcon_mask =  CreateBitmap(32, 32, 1, 32, NULL);
            SelectObject(hdcIcon, hbmIcon_mask);
            SelectObject(hdcIcon, GetStockObject(WHITE_BRUSH));
            SelectObject(hdcIcon, GetStockObject(WHITE_PEN));
            Rectangle(hdcIcon,0,0,32,32);

            SelectObject(hdcIcon, GetStockObject(BLACK_BRUSH));
            SelectObject(hdcIcon, GetStockObject(BLACK_PEN));
            Ellipse(hdcIcon,0,0,32,32);

            BitBlt(hdcBkgnd, 5, 5, 32, 32, hdcIcon, 0, 0, SRCAND);
            SelectObject(hdcIcon, hbmIcon);
            BitBlt(hdcBkgnd, 5, 5, 32, 32, hdcIcon, 0, 0, SRCPAINT);
            DeleteObject(hbmIcon_mask);
            DeleteDC(hdcIcon);
            */

            // Using AlphaBlend()
            HDC hdcIcon = CreateCompatibleDC(0);
            SelectObject(hdcIcon, hbmIcon);

            BLENDFUNCTION blend = { 0 };
            blend.BlendOp = AC_SRC_OVER;
            blend.BlendFlags = 0;
            blend.SourceConstantAlpha = 255; // Opaque paint
            blend.AlphaFormat = AC_SRC_ALPHA;
            AlphaBlend(hdcBkgnd, (x>0)?x+margin:5, (y>0)?y+margin:5, 32,32, hdcIcon, 0, 0, 32, 32, blend);

            DeleteDC(hdcIcon);

            // Draw title/text separation bar
            int line_posy=(y>0)?y+margin+37:48;
            int line_border=margin + width_Close/2;
            HPEN hPen = CreatePen(PS_SOLID, 1, RGB(128, 128, 128));
            SelectObject(hdcBkgnd, hPen);
            MoveToEx(hdcBkgnd, (x>0)?x+margin+40:45, line_posy, NULL);
            LineTo(hdcBkgnd, (w>0)?x+w-line_border:width-line_border, line_posy);

            // Set text color (black or white) based on background text region luminance
            COLORREF crEdit;
            double luminanceY = GetBitmapLuminance(hbmBkgnd, hdcBkgnd, hRgn);
            if (luminanceY < 0.5) crEdit = RGB(255,255,255); // white
            else crEdit = RGB(0,0,0); // black

            // Store DC
            Popup_array.Add(hwnd, hbtClose, hdcBkgnd, crEdit);

            // HRGN must be apply to window after its last use (so after GetBitmapLuminance)
            SetWindowRgn(hwnd, hRgn, TRUE);

            DeleteObject(hRgn);
            DeleteObject(hbmIcon);
            DeleteObject(hbmBkgnd);
            ShowWindow(hbtClose,SW_SHOW);
            break;
        }
        case WM_PAINT:
        {
            hdc = BeginPaint( hwnd, &ps );
            GetClientRect( hwnd, &rect );

            // Draw background
            BitBlt(hdc,0,0,rect.right,rect.bottom, Popup_array.GetHDC(hwnd),0,0,SRCCOPY);

            EndPaint( hwnd, &ps );
            break;
        }
        case WM_DESTROY:
            DeleteObject(font);
            DeleteObject(fonttitle);
		    break;
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLOREDIT:
    		// Set the background mode to transparent:
    		SetBkMode((HDC)wParam,TRANSPARENT);

    		// Set text color to white
    		SetTextColor((HDC)wParam, Popup_array.GetTextColor(hwnd));

            // return transparency HBRUSH
    		return (LRESULT)GetStockObject(NULL_BRUSH);
        default:
            return DefWindowProc (hwnd, msg, wParam, lParam);
    }
    return 0;
}

//  =======================================================================
void CPopup::Init()
{
    //  =======================================================================
    //  Initialize the variables
    //  =======================================================================
    m_hwnd = NULL;
    m_lpszClassName = POPUPCLASSNAME;
    m_colTrans = 0;
    m_Opacity = 255;
    m_nAppearStyle=POPUP_APPEAR_SLIDE;
    m_nAnimDuration=500;
    m_nDuration=7000;
    m_AutoSize=true;
    m_bgColor = RGB(200,200,200);

    nScreenWidth  = ::GetSystemMetrics(SM_CXFULLSCREEN);
    nScreenHeight = ::GetSystemMetrics(SM_CYFULLSCREEN);
}

//  =======================================================================
CPopup::CPopup()
{
    Init();
    SetSize(230,120);
}

//  =======================================================================
CPopup::CPopup(LPCTSTR lpszFileName, COLORREF colTrans)
{
    Init();
    SetBitmap(lpszFileName);
    SetTransparentColor(colTrans);
}

//  =======================================================================
CPopup::CPopup(LPCTSTR lpszFileName, COLORREF colTrans, BYTE opacity)
{
    Init();
    SetBitmap(lpszFileName);
    SetTransparentColor(colTrans);
    SetOpacity(opacity);
}

//  =======================================================================
CPopup::~CPopup()
{
    FreeResources();
}

//  =======================================================================
HWND CPopup::GetHandle()
{
    return m_hwnd;
}
                         
//  =======================================================================
HWND CPopup::Create()
{
    //  =======================================================================
    //  Register the window with MainWndProc as the window procedure
    //  =======================================================================
    WNDCLASSEX wndclass;
    wndclass.cbSize         = sizeof (wndclass);
    wndclass.style          = CS_BYTEALIGNCLIENT | CS_BYTEALIGNWINDOW;
    wndclass.lpfnWndProc    = MainWndProc;
    wndclass.cbClsExtra     = 0;
    wndclass.cbWndExtra     = DLGWINDOWEXTRA;
    wndclass.hInstance      = ::GetModuleHandle(NULL);
    wndclass.hIcon          = NULL;
    wndclass.hCursor        = ::LoadCursor( NULL, IDC_ARROW );
    wndclass.hbrBackground  = (HBRUSH)::GetStockObject(NULL_BRUSH);
    wndclass.lpszMenuName   = NULL;
    wndclass.lpszClassName  = m_lpszClassName;
    wndclass.hIconSm        = NULL;

    RegisterClassEx (&wndclass);

    //  =======================================================================
    //  Create the window of the application, passing the this pointer (CPopup)
    //  so that MainWndProc can use that for message forwarding
    //  =======================================================================

    m_hwnd = ::CreateWindowEx (WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE, m_lpszClassName, "",
            WS_POPUP| WS_CLIPSIBLINGS | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT, m_Width, m_Height, 0, NULL,NULL, this );

    //UnregisterClass(m_lpszClassName, ::GetModuleHandle(NULL)); // Commented to avoid memory leak

    return m_hwnd;
}

//  =======================================================================
void CPopup::Show()
{
    // Set the global popup marker to avoid conflicts between many popups launched at same time
    // NOTA : Trying to have a global marker with mutex object lead to some deadlocks so we use file-mapping object instead
    hFileMap_Popup = CreateFileMapping(INVALID_HANDLE_VALUE,0,PAGE_READWRITE,0,0x4000,"FILEMAP_POPUP");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        pMapView_Popup = (int*)MapViewOfFile(hFileMap_Popup,FILE_MAP_ALL_ACCESS,0,0,0);
    }
    else {
        pMapView_Popup = (int*)MapViewOfFile(hFileMap_Popup,FILE_MAP_ALL_ACCESS,0,0,0);
        *pMapView_Popup = 0;
    }

    while (*pMapView_Popup>0) {
        Sleep(100);
    }
    *pMapView_Popup = 1;

    // Check screen size and Taskbar placement on each Show() call
    nScreenWidth=::GetSystemMetrics(SM_CXSCREEN);
    nScreenHeight=::GetSystemMetrics(SM_CYSCREEN);

    APPBARDATA appBarData;
    appBarData.cbSize=sizeof(appBarData);
    SHAppBarMessage(ABM_GETTASKBARPOS,&appBarData);

    int nTaskbarWidth=appBarData.rc.right - appBarData.rc.left;
    int nTaskbarHeight=appBarData.rc.bottom - appBarData.rc.top;


    int pixelsBorder = 0; // Used to display the popup beside others

    do {
        int border_margin=5;
        switch(appBarData.uEdge)
        {
            case ABE_RIGHT:
            {
                InitialPosX=nScreenWidth;
                InitialPosY=nScreenHeight;
                FixePosX=nScreenWidth-nTaskbarWidth-m_Width-border_margin;
                FixePosY=nScreenHeight-m_Height-border_margin;
                FixePosY-=pixelsBorder;
                FinalPosX=nScreenWidth;
                FinalPosY=FixePosY;
                break;
            }
            case ABE_LEFT:
            {
                InitialPosX=-m_Width;
                InitialPosY=nScreenHeight+m_Height;
                FixePosX=nTaskbarWidth+border_margin;
                FixePosY=nScreenHeight-m_Height-border_margin;
                FixePosY-=pixelsBorder;
                FinalPosX=-m_Width;
                FinalPosY=FixePosY;
                break;
            }
            case ABE_TOP:
            {
                InitialPosX=nScreenWidth;
                InitialPosY=-m_Height;
                FixePosX=nScreenWidth-m_Width-border_margin;
                FixePosY=nTaskbarHeight+border_margin;
                FixePosY+=pixelsBorder;
                FinalPosX=nScreenWidth+m_Width;
                FinalPosY=FixePosY;
                break;
            }
            case ABE_BOTTOM:
            default:
            {
                InitialPosX=nScreenWidth;
                InitialPosY=nScreenHeight;
                FixePosX=nScreenWidth-m_Width-border_margin;
                FixePosY=nScreenHeight-nTaskbarHeight-m_Height-border_margin;
                FixePosY-=pixelsBorder;
                FinalPosX=nScreenWidth;
                FinalPosY=FixePosY;
                break;
            }
        }
    
        if (FixePosX < 0 || FixePosX+m_Width > nScreenWidth || FixePosY < 0 || FixePosY+m_Height > nScreenHeight) {
            pixelsBorder=0;
            Sleep(100);
            continue;
        }

        // Verify that popup does not intersected with another
        PARAM param_Data;
        param_Data.hwnd=m_hwnd;
        param_Data.windowReady=true;
        param_Data.fixePosX = FixePosX;
        param_Data.fixePosY = FixePosY;
        param_Data.width = m_Width;
        param_Data.height = m_Height;
        param_Data.margin = 5;
        param_Data.waitPrevious = m_waitPrevious;
        EnumWindows(&EnumPopupIntersect, (LPARAM)&param_Data);
        if (param_Data.windowReady) break;  

        pixelsBorder++;
    } while (1);


    if (m_nAppearStyle==POPUP_APPEAR_FIXE_WIZZ || m_nAppearStyle==POPUP_APPEAR_FIXE)
    {
        InitialPosX=FixePosX;
        InitialPosY=FixePosY;
        FinalPosX=FixePosX;
        FinalPosY=FixePosY;
        CurrentPosX=FixePosX;
        CurrentPosY=FixePosY;
    }
    else {
        CurrentPosX=InitialPosX;
        CurrentPosY=InitialPosY;
    }

    MoveWindow(m_hwnd, CurrentPosX, CurrentPosY, 0, 0, true);

    PARAM param_Data;
    param_Data.hwnd=m_hwnd;
    param_Data.fixePosX=FixePosX;
    param_Data.fixePosY=FixePosY;
    param_Data.initialPosX=InitialPosX;
    param_Data.initialPosY=InitialPosY;
    param_Data.finalPosX=FinalPosX;
    param_Data.finalPosY=FinalPosY;
    param_Data.animDuration=m_nAnimDuration;
    param_Data.duration=m_nDuration;
    param_Data.opacity=m_Opacity;
    param_Data.appearStyle=m_nAppearStyle;
    param_Data.width=m_Width;
    param_Data.height=m_Height;
    param_Data.showOpaque=m_ShowOpaque;
    param_Data.lockNext=m_lockNext;

    ShowWindow (m_hwnd, SW_SHOWNOACTIVATE);

    CreateThread(0, NULL, Animate, (LPVOID)&param_Data, NULL, NULL);
    
    MSG messages;
    while (GetMessage (&messages, NULL, 0, 0))
    {
        TranslateMessage(&messages);
        DispatchMessage(&messages);
    }
}

//  =======================================================================
//  Set background file
bool CPopup::SetBitmap(LPCTSTR lpszFileName)
{
    if (!lpszFileName || !lpszFileName[0]) return false;
    HBITMAP hBitmap;
    hBitmap = FileToBitmap(lpszFileName);
    return SetBitmap(hBitmap);
    /*
    //  =======================================================================
    //  load the bitmap
    //  =======================================================================
    HBITMAP    hBitmap;//       = NULL;
    hBitmap = (HBITMAP)::LoadImage(0, lpszFileName, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    if (!hBitmap) hBitmap = LoadBitmap(GetModuleHandle(NULL), lpszFileName);//(HBITMAP)::LoadImage(0, lpszFileName, IMAGE_BITMAP, 0, 0, 0);
    return SetBitmap(hBitmap);
    */
}

//  =======================================================================
//  Set background from ressource
bool CPopup::SetBitmap(UINT uBitmap)
{
    HBITMAP hBitmap;
    hBitmap = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(uBitmap));
    return SetBitmap(hBitmap);
}

//  =======================================================================
//  Set background from hbitmap
bool CPopup::SetBitmap(HBITMAP hBitmap)
{
    if (!hBitmap) return false;
    BITMAP bmp;

    // Free loaded resource
    FreeResources();

    if (hBitmap)
    {
        m_hBitmap = hBitmap;

        // Method to request the properties of the GDI Object.
        if (!::GetObject(hBitmap, sizeof(bmp), &bmp))
        {
            FreeResources();
            return false;
        }
        m_Width = (DWORD)bmp.bmWidth;
        m_Height = (DWORD)bmp.bmHeight;
        CurrentPosX=(nScreenWidth  - m_Width) / 2;
        CurrentPosY=(nScreenHeight - m_Height) / 2;
    }

    return true;
}

//  =======================================================================
void CPopup::FreeResources()
{
    Popup_array.Remove(m_hwnd);
    /*char str[100];
            sprintf(str, "%d", Popup_array.Count());
            MessageBox(NULL, str, "Debug test", MB_ICONINFORMATION | MB_OK);
    */

    if (m_hBitmap)
        ::DeleteObject (m_hBitmap);
    m_hBitmap = NULL;
}

//  =======================================================================
bool CPopup::SetTransparentColor(COLORREF col)
{
    m_colTrans = col;
    return ApplyTransparency(m_hwnd, m_Opacity);
}

//  =======================================================================
bool CPopup::SetOpacity(BYTE opacity)
{
    if (opacity<0) return false;
    m_Opacity = opacity;
    return ApplyTransparency(m_hwnd, opacity);
}

//  =======================================================================
bool CPopup::SetShowOpaque(bool bShowOpaque=true){
    m_ShowOpaque = bShowOpaque;
    return m_ShowOpaque;
}

//  =======================================================================
void CPopup::SetAppearStyle(int AppearStyle)
{
  if (AppearStyle>=1 && AppearStyle<=4) m_nAppearStyle=AppearStyle;
  else m_nAppearStyle=POPUP_APPEAR_SLIDE;
}

//  =======================================================================
void CPopup::SetIcon(char *s)
{
    strcpy(Icon, s);
}

//  =======================================================================
void CPopup::SetTitle(char *s)
{
    strcpy(Title, s);
}

//  =======================================================================
void CPopup::SetText(char *s)
{
    strcpy(Text, s);
}

//  =======================================================================
void CPopup::SetSize(int Width, int Height)
{
    if (m_hwnd) return; // No resize after creation
    if (Width>0) m_Width=Width;
    if (Height>0) m_Height=Height;
}

//  =======================================================================
void CPopup::SetAnimationParam(int AnimDuration, int Duration)
{
    if (AnimDuration>0) m_nAnimDuration=AnimDuration;
    if (Duration>=0) m_nDuration=Duration; // If =0 then popup stay displayed
}

//  =======================================================================
void CPopup::SetAutoSize(bool bAutoSize=true)
{
    m_AutoSize = bAutoSize;
}

//  =======================================================================
void CPopup::SetColor(const char* hexColor)
{
    if (!hexColor || strlen(hexColor)!=6) return;
    int hexValue = HexToDec(hexColor);
    int red = ((hexValue >> 16) & 0xFF);
    int green = ((hexValue >> 8) & 0xFF);
    int blue = ((hexValue) & 0xFF);
    SetColor(RGB(red, green, blue));
}

//  =======================================================================
void CPopup::SetColor(COLORREF bgColor)
{
    m_bgColor = bgColor;    
}

//  =======================================================================
void CPopup::SetWaitPrevious(bool bWaitPrevious=true)
{
    m_waitPrevious = bWaitPrevious;
}

//  =======================================================================
void CPopup::SetLockNext(bool bLockNext=true)
{
    m_lockNext = bLockNext;
}

//  =======================================================================
//  Main popup thread
//  =======================================================================
DWORD WINAPI PopupThread( LPVOID lpParam )
{
    PPARAM pData;
    pData = (PPARAM)lpParam;

    CPopup popup;
    popup.SetOpacity(pData->opacity);
    popup.SetSize(pData->width, pData->height);
    popup.SetAnimationParam(pData->animDuration, pData->duration);
    popup.SetAppearStyle(pData->appearStyle);
    popup.SetTitle(pData->title);
    popup.SetText(pData->text);
    popup.SetIcon(pData->icon);
    popup.SetShowOpaque(pData->showOpaque);
    popup.SetAutoSize(pData->autoSize);
    popup.SetWaitPrevious(pData->waitPrevious);
    popup.SetLockNext(pData->lockNext);
    if (!popup.SetBitmap(pData->background)) // try to set 'background' as image
        popup.SetColor(pData->background);   // else try to set 'background' as color

    pData->hwnd=popup.Create();
    pData->windowReady=true;
    popup.Show();

    return 0;
}

//  =======================================================================
/*
title,           // Title
text,            // Message
icon,            // Icon: POPUP_ICON_INFORMATION("1")(default), POPUP_ICON_WARNING("2"), POPUP_ICON_ERROR("3"), POPUP_ICON_OK("4"), icon file path
opacity,         // Opacity: 0-255 (0=transparent, 255=opaque)
background, // Background image file path
width,           // Popup's width
height,          // Popup's Height
animDuration,    // Animation duration to show/hide popup according to appearStyle
duration,        // Display duration
appearStyle,     // Appear style: POPUP_APPEAR_FIXE(1), POPUP_APPEAR_SLIDE(2)(default), POPUP_APPEAR_FIXE_WIZZ(3), POPUP_APPEAR_SLIDE_WIZZ(4)
showOpaque,      // If true then popup is showing opaque and then fade in to opacity value at the end of the animation
waitPrevious,    // If true then popup is displayed when all previous are closed
lockNext         // If true then the next popup windows will not be shown until that one is displayed
*/
HWND Popup(const char *title, const char *text, const char *icon=NULL, BYTE opacity=-1, const char *background=NULL, int width=-1, int height=-1, int animDuration=-1, int duration=-1, int appearStyle=POPUP_APPEAR_SLIDE, bool showOpaque=true, bool waitPrevious=false, bool lockNext=false)
{
    PARAM param_Data;

    // Wait for previous popup initialization ending
    while(!bCurrentPopupReady) Sleep(1);
    bCurrentPopupReady=false;

    // Convert the strings title and text if necessary (especially with accents)
    if (strstr(title, "OEM_")==title)
        CharToOem(title+4, param_Data.title);
    else strcpy(param_Data.title, title);
  
    if (strstr(text, "OEM_")==text)
       CharToOem(text+4, param_Data.text);
    else strcpy(param_Data.text, text);

    // Find and convert all "\n" characters to CRLF in the 'text' string
    for (int i =0; i<strlen(param_Data.text)-1; i++)
    {
        if (param_Data.text[i]=='\\' && param_Data.text[i+1]=='n')
        {
            param_Data.text[i]=13;
            param_Data.text[i+1]=10;
            i++;
        }
    }
    
    if (!icon || !strlen(icon)) strcpy(param_Data.icon, "1");
    else strcpy(param_Data.icon, icon);

    if (background) strcpy(param_Data.background, background);
    else strcpy(param_Data.background, "");

    // Set other parameters
    param_Data.opacity=opacity;
    param_Data.width=width;
    param_Data.height=height;
    param_Data.animDuration=animDuration;
    param_Data.duration=duration;
    param_Data.appearStyle=appearStyle;
    param_Data.windowReady=false;
    param_Data.showOpaque=showOpaque;
    param_Data.autoSize=true;
    param_Data.waitPrevious=waitPrevious;
    param_Data.lockNext=lockNext;
    
    // Launch popup thread
    DWORD idThread;
    HANDLE hThread=CreateThread(NULL, 0, PopupThread, (LPVOID)&param_Data, NULL, (LPDWORD)&idThread);
    while (!param_Data.windowReady) Sleep(1);

    return param_Data.hwnd;
}

#endif //_CLASS_POPUP_


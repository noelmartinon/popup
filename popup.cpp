#include <windows.h>
#include "popup.hpp"

//#define USE_GDIPLUS

int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,PSTR szCmdLine,int iCmdShow)
{
    LPSTR sTitle = NULL;
    LPSTR sMessage = NULL;
    LPSTR sIcon = NULL;
    LPSTR sBackground = NULL;
    bool bWaitPrevious = false;
    bool bLockNext = false;
    int duration = 8;

    //
    // Check command line arguments
    //
    if (_argc<3 || _argc>7)
    {
        MessageBox(0, ""
            "* Description:\n"
            "Display a customizable popup window\n"
            "\n"
            "* Syntax:\n"
            "Popup.exe -t\"TITLE\" -m\"TEXT\" [-i\"ICON\"] [-d\"SECONDS\"] [-w] [-l] [-b\"IMAGE_OR_COLOR\"]\n"
            "\n"
            "* Arguments:\n"
            "-t  title\n"
            "-m  message text\n"
            "-i  icon: 1=INFORMATION(default), 2=WARNING, 3=ERROR, 4=OK or\n"
            "    'path to a custom icon image' (bmp,jpg,png,gif)\n"
            "-d  duration in seconds (default=8)\n"
            "-w  wait for previous popup to ending (not default)\n"
            "-l  lock next popups (force other popups to wait)(not default)\n"
            "-b  path to a custom background image (bmp,jpg,png,gif) or\n"
            "    HEX formatted color for gradient background (ie:'ff0fa0')\n\n"
            "NOTA: Only the title and the text are required. Other parameters are optional.\n\n"
            "------------------------\n"
            "Copyright (C) 2018 Noël MARTINON\n"
            "License GPL-3.0\n"
            "This program comes with ABSOLUTELY NO WARRANTY; for details see LICENSE.txt\n",
            "Popup - Usage", MB_ICONINFORMATION);
        return 1;
    }

    for (int i = 1; i < _argc; i++) {
        if (strnicmp(_argv[i], "-t", 2)!=0 && // title
            strnicmp(_argv[i], "-m", 2)!=0 && // message
            strnicmp(_argv[i], "-i", 2)!=0 && // icon : 1=INFORMATION(default), 2=WARNING, 3=ERROR, 4=OK or 'path to a custom icon image'
            strnicmp(_argv[i], "-w", 2)!=0 && // wait for previous popup to ending
            strnicmp(_argv[i], "-l", 2)!=0 && // lock next popup (force other popups to wait if they are not in 'wait' mode)
            strnicmp(_argv[i], "-b", 2)!=0 && // path to a custom background image or HEX color
            strnicmp(_argv[i], "-d", 2)!=0 )  // duration
            {
                MessageBox(0, _argv[i], "Popup - Incorrect arguments", MB_ICONERROR);
                return 1;
            }
        
        if (strnicmp(_argv[i], "-t", 2)==0) sTitle = _argv[i]+2;
        else if (strnicmp(_argv[i], "-m", 2)==0) sMessage = _argv[i]+2;
        else if (strnicmp(_argv[i], "-i", 2)==0) sIcon = _argv[i]+2;
        else if (strnicmp(_argv[i], "-w", 2)==0) bWaitPrevious = true;
        else if (strnicmp(_argv[i], "-l", 2)==0) bLockNext = true;
        else if (strnicmp(_argv[i], "-b", 2)==0) sBackground = _argv[i]+2;
        else if (strnicmp(_argv[i], "-d", 2)==0) duration = atoi(_argv[i]+2);
    }
    
    // Parameters adjustment :
    if (duration<=1) duration=1;

    //
    // Launch popup and wait to terminate
    //
    HWND hwnd_pop = Popup(  sTitle,                 // Title
                            sMessage,               // Message
                            sIcon,                  // Icon
                            200,                    // opacity (0-255)
                            sBackground,            // Background image
                            310,                    // Window's width
                            130,                    // Window's Height
                            200,                    // Animation duration (slide, fade)
                            duration*1000,          // Display duration
                            POPUP_APPEAR_SLIDE,     // Appear style
                            true,                   // Show Opaque (then go to opacity value)
                            bWaitPrevious,          // Wait for previous popup to close
                            bLockNext);             // Lock next popups until that one is displayed

    while (IsWindow(hwnd_pop)) Sleep(1);

    return 0;
}


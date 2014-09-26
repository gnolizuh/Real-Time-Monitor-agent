#include "stdafx.h"
#include <mutex>
#include "ToolTip.h"

BOOL g_TrackingMouse = FALSE;
std::mutex g_hwnd_lock;
HWND g_hwndTrackingTT;
TOOLINFO g_toolItem;

HWND CreateTrackingToolTip(int toolID, HWND hDlg, WCHAR *pText) 
{ 
    HWND hwndTT = CreateWindowEx(WS_EX_TOPMOST, 
                                 TOOLTIPS_CLASS, 
                                 NULL, 
                                 WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP, 
                                 CW_USEDEFAULT, 
                                 CW_USEDEFAULT, 
                                 CW_USEDEFAULT, 
                                 CW_USEDEFAULT, 
                                 hDlg, 
                                 NULL, 
                                 AfxGetInstanceHandle(), 
                                 NULL); 
 
    if (!hwndTT) 
    { 
        return NULL; 
    } 
 
    g_toolItem.cbSize = sizeof(TOOLINFO); 
    g_toolItem.uFlags = TTF_IDISHWND | TTF_TRACK | TTF_ABSOLUTE; 
    g_toolItem.hwnd = hDlg; 
    g_toolItem.hinst = AfxGetInstanceHandle(); 
    g_toolItem.lpszText = pText; 
    g_toolItem.uId = (UINT_PTR)hDlg; 
 
    GetClientRect(hDlg, &g_toolItem.rect); 
 
    SendMessage(hwndTT, TTM_ADDTOOL, 0, (LPARAM)(LPTOOLINFO)&g_toolItem); 
 
    return hwndTT; 
}
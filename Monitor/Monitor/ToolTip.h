#ifndef __AVS_PROXY_CLIENT_TOOL_TIP__
#define __AVS_PROXY_CLIENT_TOOL_TIP__

#include "Com.h"

HWND CreateTrackingToolTip(int toolID, HWND hDlg, WCHAR *pText) ;

extern BOOL g_TrackingMouse;

extern HWND g_hwndTrackingTT;

extern TOOLINFO g_toolItem;

#endif
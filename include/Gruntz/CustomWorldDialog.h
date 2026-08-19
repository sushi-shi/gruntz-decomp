#ifndef GRUNTZ_GRUNTZ_CUSTOMWORLDDIALOG_H
#define GRUNTZ_GRUNTZ_CUSTOMWORLDDIALOG_H

#include <rva.h>

#include <Mfc.h>

#include <Enums.h>

GZ_ENUM_CONST_BEGIN(CustomWorldCtrlId)
    CTRL_CUSTOM_WORLD_LIST = 0x3fc,
    CTRL_CUSTOM_WORLD_INFO = 0x42a
GZ_ENUM_CONST_END(CustomWorldCtrlId)

class CDDrawSurfaceMgr;
extern class CDDrawSurfaceMgr* g_customWorldSurfaceMgr;
extern char g_mapNameBuf[0x200];

i32 LoadCustomWorldInfo(HWND hDlg);
i32 FillLevelInfoDialog(HWND hDlg);
i32 LoadCustomWorldSelection(HWND hWnd);
i32 FileExists(const char* path);
i32 FillCustomLevelList(HWND hWnd);

BOOL CALLBACK CustomWorldInfoDlgProc(HWND, UINT, WPARAM, LPARAM);

#endif // GRUNTZ_GRUNTZ_CUSTOMWORLDDIALOG_H

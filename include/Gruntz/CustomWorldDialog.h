#ifndef GRUNTZ_GRUNTZ_CUSTOMWORLDDIALOG_H
#define GRUNTZ_GRUNTZ_CUSTOMWORLDDIALOG_H

#include <Mfc.h>
#include <rva.h>

class CDDrawSurfaceMgr;
extern class CDDrawSurfaceMgr* g_dat62c268;
extern char g_mapNameBuf[0x200];

i32 LoadCustomWorldInfo(HWND hDlg);
i32 FillLevelInfoDialog(HWND hDlg);
i32 LoadCustomWorldSelection(HWND hWnd);
i32 FileExists(char* path);
namespace m4 {
    i32 FillCustomLevelList(HWND hWnd);
}

INT_PTR CALLBACK CustomWorldInfoDlgProc(HWND, UINT, WPARAM, LPARAM);

#endif // GRUNTZ_GRUNTZ_CUSTOMWORLDDIALOG_H

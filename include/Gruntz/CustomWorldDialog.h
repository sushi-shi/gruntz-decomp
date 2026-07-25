// CustomWorldDialog.h - the CustomWorldDialog TU's exported globals/data.
#ifndef GRUNTZ_GRUNTZ_CUSTOMWORLDDIALOG_H
#define GRUNTZ_GRUNTZ_CUSTOMWORLDDIALOG_H


#include <Mfc.h> // afx.h FIRST: Win32 types named below
#include <rva.h>

class CDDrawSurfaceMgr;
extern class CDDrawSurfaceMgr* g_dat62c268;
extern char g_mapNameBuf[0x200];

// File-scope prototypes moved from the .cpp (external linkage
// belongs in the owner header).
i32 LoadCustomWorldInfo(HWND hDlg);      // 0x3b7c0
i32 FillLevelInfoDialog(HWND hDlg);      // 0x3b1a0
i32 LoadCustomWorldSelection(HWND hWnd); // 0x3b310
i32 FileExists(char* path); // 0x1189c0 (heapdiag; "PathFileExists 0x4282" was a thunk to it)
namespace m4 {
    i32 FillCustomLevelList(HWND hWnd); // 0x3af90
} // namespace m4



// Dialog proc, declared in the owner header (file-scope prototypes have
// external linkage).
INT_PTR CALLBACK CustomWorldInfoDlgProc(HWND, UINT, WPARAM, LPARAM);

#endif // GRUNTZ_GRUNTZ_CUSTOMWORLDDIALOG_H

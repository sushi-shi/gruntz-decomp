#ifndef GRUNTZ_CUSTOMWORLDINFODLG_H
#define GRUNTZ_CUSTOMWORLDINFODLG_H

#include <rva.h>

extern char g_dotDot[];

extern "C" INT_PTR CALLBACK CustomWorldDlgProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK CustomWorldInfoDlgProc(HWND, UINT, WPARAM, LPARAM);

extern "C" i32 CustomGate(const char* name);

extern "C" i32 func_2176(HWND hDlg);

#endif // GRUNTZ_CUSTOMWORLDINFODLG_H

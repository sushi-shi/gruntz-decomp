#ifndef GRUNTZ_GRUNTZ_ADVANCEDOPTIONS_H
#define GRUNTZ_GRUNTZ_ADVANCEDOPTIONS_H

#include <Mfc.h>

#include <Ints.h>
#include <Utils/RegMgr.h>

BOOL CALLBACK AdvancedOptionsDialogProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

void SaveOption(HWND hWnd, CRegMgr* reg, char* szValueName, DWORD controlId);
void SetDefaults(HWND hWnd);
void LoadOptions(HWND hWnd, CRegMgr* reg);
void SaveOptions(HWND hWnd, CRegMgr* reg);

#endif // GRUNTZ_GRUNTZ_ADVANCEDOPTIONS_H

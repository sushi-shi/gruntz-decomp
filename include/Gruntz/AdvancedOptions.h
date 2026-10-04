#ifndef GRUNTZ_GRUNTZ_ADVANCEDOPTIONS_H
#define GRUNTZ_GRUNTZ_ADVANCEDOPTIONS_H

#include <Ints.h>
#include <Io/Settings.h>

BOOL CALLBACK AdvancedOptionsDialogProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

void SaveOption(HWND hWnd, Settings* reg, char* szValueName, DWORD controlId);
void SetDefaults(HWND hWnd);
void LoadOptions(HWND hWnd, Settings* reg);
void SaveOptions(HWND hWnd, Settings* reg);

#endif

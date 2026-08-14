#ifndef GRUNTZ_GRUNTZ_ADVANCEDOPTIONS_H
#define GRUNTZ_GRUNTZ_ADVANCEDOPTIONS_H

#include <Mfc.h>

#include <Ints.h>
#include <Utils/RegistryHelper.h>

INT_PTR CALLBACK AdvancedOptionsDialogProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

void SaveOption(
    HWND hWnd,
    Utils::RegistryHelper* pRegistryHelper,
    char* szValueName,
    DWORD controlId
);
void SetDefaults(HWND hWnd);
void LoadOptions(HWND hWnd, Utils::RegistryHelper* pRegistryHelper);
void SaveOptions(HWND hWnd, Utils::RegistryHelper* pRegistryHelper);

#endif // GRUNTZ_GRUNTZ_ADVANCEDOPTIONS_H

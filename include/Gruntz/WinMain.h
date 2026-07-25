// WinMain.h - the WinMain TU's external declarations.
#ifndef GRUNTZ_WINMAIN_H
#define GRUNTZ_WINMAIN_H

#include <Mfc.h> // afx.h FIRST (umbrella for any Win32 types below)
#include <Ints.h>
#include <rva.h>

i32 FindProcessByName(const char* name, i32 wantCount, void** pHandleOut);
i32 StartUpPrompt(HWND__* parent);
void ActiveWait(u32 milliseconds);


// Dialog proc, declared in the owner header (file-scope prototypes have
// external linkage).
i32 CALLBACK AdvancedOptionsDialogProc(HWND, UINT, WPARAM, LPARAM);

#endif // GRUNTZ_WINMAIN_H

#ifndef GRUNTZ_CUSTOMWORLDINFODLG_H
#define GRUNTZ_CUSTOMWORLDINFODLG_H

#include <rva.h>

extern char g_dotDot[]; // 0x0020cf90 ".." (def in CustomWorldDialog.cpp)

// --- the TU's extern surface (moved out of the .cpp; addresses/thunk
// VAs are reloc-masked at use) ---
extern "C" INT_PTR CALLBACK CustomWorldDlgProc(HWND, UINT, WPARAM, LPARAM);
extern "C" INT_PTR CALLBACK CustomWorldInfoDlgProcThunk(HWND, UINT, WPARAM, LPARAM);
extern "C" i32 CustomGate(const char* name); // 0x0018d290

extern "C" i32 func_2176(HWND hDlg); // thunk 0x2176 (per-dialog refresh helper)

#endif // GRUNTZ_CUSTOMWORLDINFODLG_H

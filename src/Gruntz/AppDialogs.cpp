// AppDialogs.cpp - Win32 dialog-box callbacks for the Gruntz app's option
// dialogs (C:\Proj\Gruntz). These are free __stdcall DialogProc callbacks
// (INT_PTR CALLBACK(HWND, UINT, WPARAM, LPARAM)); the engine passes them to the
// USER32 dialog manager, which dispatches WM_* messages here.
//
// WarpDialogProc backs the developer "warp" cheat dialog: on WM_INITDIALOG it
// seeds two edit controls (0x40e / 0x40f) with the current warp X/Y from the
// game registry; on WM_COMMAND it ends the dialog (IDCANCEL=2) or, on IDOK=1,
// reads the two edit fields back, stores them, and - if checkbox 0x410 is set -
// persists them to the registry as the warp target.
//
// Field names are placeholders (m_<hexoffset>); only the OFFSETS, control IDs,
// and code bytes are load-bearing (campaign doctrine).
#include <rva.h>
#include <Gruntz/GameRegistry.h>
#include <stdio.h> // engine sprintf (reloc-masked)

// The USER32 dialog API (EndDialog / GetDlgItemInt / SetDlgItemInt /
// IsDlgButtonChecked), the HWND/UINT/WPARAM/LPARAM/INT_PTR types, and the
// WM_INITDIALOG/WM_COMMAND ids come from the real <windows.h>, pulled the
// afx-first way via <Mfc.h> (superset of Win32.h) so <Utils/RegistryHelper.h>'s
// MFC winreg types coexist without the C1189 windows.h-before-afx wall.
#include <Mfc.h>
#include <Globals.h>
#include <Utils/RegistryHelper.h> // canonical Utils::RegistryHelper

// The game-registry singleton (CGameRegistry.h). This proc reaches three owned
// slots through AUTHENTIC per-site downcasts (their concrete class lives in other
// clusters, so CGameRegistry keeps the base/void* type and the consumer casts):
//   ((CGameRegLevel*)g_gameReg->m_curState)->m_1c   current level number
//                                            (CState* base -> concrete level view)
//   ((Utils::RegistryHelper*)g_gameReg->m_settings)->SetValueDword(...)
//                                            (void* m_settings -> RegistryHelper)
//   ((CGameRegWarp*)g_gameReg->m_world->m_24->m_5c)->m_84 / ->m_88  seed X/Y
//                                            (viewport addr stored i32 -> +0x84/+0x88)
struct CGameRegLevel { // downcast view of CState* m_curState (+0x1c level number)
    char m_pad00[0x1c];
    i32 m_1c;
};
struct CGameRegWarp { // reinterpret view of the m_5c viewport (+0x84/+0x88 seed X/Y)
    char m_pad00[0x84];
    i32 m_84;
    i32 m_88;
};
DATA(0x0024556c)
extern CGameRegistry* g_gameReg;

// File-scope sinks the IDOK path stores the two edit-field values into before
// the (optional) registry write (reloc-masked DIR32 stores).

// ---------------------------------------------------------------------------
// WarpDialogProc - the warp-cheat dialog callback.
RVA(0x0008e4e0, 0x172)
INT_PTR CALLBACK WarpDialogProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    char szValue[64];

    switch (msg) {
        case WM_INITDIALOG: {
            CGameRegWarp* warp = (CGameRegWarp*)g_gameReg->m_world->m_24->m_5c;
            i32 seedX = warp->m_84;
            i32 seedY = warp->m_88;
            SetDlgItemInt(hDlg, 0x40e, seedX, 0);
            SetDlgItemInt(hDlg, 0x40f, seedY, 0);
            return 1;
        }

        case WM_COMMAND:
            if (wParam == 2) {
                EndDialog(hDlg, 0);
                return 1;
            }
            if (wParam == 1) {
                i32 valX = GetDlgItemInt(hDlg, 0x40e, 0, 0);
                i32 valY = GetDlgItemInt(hDlg, 0x40f, 0, 0);
                g_warpX = valX;
                g_warpY = valY;
                if (IsDlgButtonChecked(hDlg, 0x410)) {
                    sprintf(
                        szValue,
                        "Level %i Warp X",
                        ((CGameRegLevel*)g_gameReg->m_curState)->m_1c
                    );
                    ((Utils::RegistryHelper*)g_gameReg->m_settings)->SetValueDword(szValue, valX);
                    sprintf(
                        szValue,
                        "Level %i Warp Y",
                        ((CGameRegLevel*)g_gameReg->m_curState)->m_1c
                    );
                    ((Utils::RegistryHelper*)g_gameReg->m_settings)->SetValueDword(szValue, valY);
                    ((Utils::RegistryHelper*)g_gameReg->m_settings)
                        ->SetValueDword(
                            "Last Warp Level",
                            ((CGameRegLevel*)g_gameReg->m_curState)->m_1c
                        );
                }
                EndDialog(hDlg, 1);
                return 1;
            }
            break;
    }
    return 0;
}

// ---------------------------------------------------------------------------
// LevelNumberDialogProc (0x0008e7c0 / 0x0008e8c0) - two byte-identical developer
// "go to level" dialog callbacks. On WM_INITDIALOG the current level number
// (g_gameReg->m_curState->m_1c) seeds edit control 0x40c; on WM_COMMAND IDCANCEL
// (2) ends the dialog with 0, IDOK (1) ends it with the entered level number
// (GetDlgItemInt of 0x40c). The two procs back two separate dialog resources with
// the same layout, so the compiler emits them identically.
RVA(0x0008e7c0, 0x86)
INT_PTR CALLBACK LevelNumberDialogProc8e7c0(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_INITDIALOG:
            SetDlgItemInt(hDlg, 0x40c, ((CGameRegLevel*)g_gameReg->m_curState)->m_1c, 0);
            return 1;
        case WM_COMMAND:
            if (wParam == 2) {
                EndDialog(hDlg, 0);
                return 1;
            }
            if (wParam == 1) {
                EndDialog(hDlg, GetDlgItemInt(hDlg, 0x40c, 0, 0));
                return 1;
            }
            break;
    }
    return 0;
}

RVA(0x0008e8c0, 0x86)
INT_PTR CALLBACK LevelNumberDialogProc8e8c0(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_INITDIALOG:
            SetDlgItemInt(hDlg, 0x40c, ((CGameRegLevel*)g_gameReg->m_curState)->m_1c, 0);
            return 1;
        case WM_COMMAND:
            if (wParam == 2) {
                EndDialog(hDlg, 0);
                return 1;
            }
            if (wParam == 1) {
                EndDialog(hDlg, GetDlgItemInt(hDlg, 0x40c, 0, 0));
                return 1;
            }
            break;
    }
    return 0;
}

SIZE_UNKNOWN(CGameRegLevel);
SIZE_UNKNOWN(CGameRegWarp);

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
#include <Gruntz/CGameRegistry.h>
#include <stdio.h> // engine sprintf (reloc-masked)

// The USER32 dialog API (EndDialog / GetDlgItemInt / SetDlgItemInt /
// IsDlgButtonChecked), the HWND/UINT/WPARAM/LPARAM/INT_PTR types, and the
// WM_INITDIALOG/WM_COMMAND ids all come from the real <windows.h> (via Win32.h;
// pure-Win32 TU, no MFC).
#include <Win32.h>
#include <Globals.h>

// The engine's registry/config helper (Utils::RegistryHelper) - only the one
// method this proc calls is declared, to keep the symbol set tiny.
namespace Utils {
    class RegistryHelper {
    public:
        i32 SetValueDword(char* szValueName, u32 value);
    };
} // namespace Utils

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

SIZE_UNKNOWN(CGameRegLevel);
SIZE_UNKNOWN(CGameRegWarp);

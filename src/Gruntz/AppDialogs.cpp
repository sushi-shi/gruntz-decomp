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
#include <stdio.h> // engine sprintf (reloc-masked)

// The USER32 dialog API (EndDialog / GetDlgItemInt / SetDlgItemInt /
// IsDlgButtonChecked), the HWND/UINT/WPARAM/LPARAM/INT_PTR types, and the
// WM_INITDIALOG/WM_COMMAND ids all come from the real <windows.h> (via Win32.h;
// pure-Win32 TU, no MFC).
#include <Win32.h>

// The engine's registry/config helper (Utils::RegistryHelper) - only the one
// method this proc calls is declared, to keep the symbol set tiny.
namespace Utils {
    class RegistryHelper {
    public:
        int SetValueDword(char* szValueName, unsigned long value);
    };
} // namespace Utils

// The game-registry singleton. Only the offsets this proc walks are modeled:
//   g_gameReg->m_2c->m_1c   the current level number
//   g_gameReg->m_38         the Utils::RegistryHelper the values persist through
//   g_gameReg->m_30->m_24->m_5c->m_84 / ->m_88   the seed X/Y for WM_INITDIALOG
struct CGameRegLevel {
    char m_pad00[0x1c];
    int m_1c;
};
struct CGameRegWarp {
    char m_pad00[0x84];
    int m_84;
    int m_88;
};
struct CGameRegSub24 {
    char m_pad00[0x5c];
    CGameRegWarp* m_5c;
};
struct CGameRegSub30 {
    char m_pad00[0x24];
    CGameRegSub24* m_24;
};
struct CGameReg {
    char m_pad00[0x2c];
    CGameRegLevel* m_2c; // +0x2c  level info
    CGameRegSub30* m_30; // +0x30  warp seed chain
    char m_pad34[0x38 - 0x34];
    Utils::RegistryHelper* m_38; // +0x38  registry helper
};
DATA(0x24556c)
extern CGameReg* g_gameReg;

// File-scope sinks the IDOK path stores the two edit-field values into before
// the (optional) registry write (reloc-masked DIR32 stores).
DATA(0x212610)
extern int g_warpX; // 0x612610
DATA(0x212614)
extern int g_warpY; // 0x612614

// ---------------------------------------------------------------------------
// WarpDialogProc - the warp-cheat dialog callback.
RVA(0x8e4e0, 0x172)
INT_PTR __stdcall WarpDialogProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    char szValue[64];

    switch (msg) {
        case WM_INITDIALOG: {
            CGameRegWarp* warp = g_gameReg->m_30->m_24->m_5c;
            int seedX = warp->m_84;
            int seedY = warp->m_88;
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
                int valX = GetDlgItemInt(hDlg, 0x40e, 0, 0);
                int valY = GetDlgItemInt(hDlg, 0x40f, 0, 0);
                g_warpX = valX;
                g_warpY = valY;
                if (IsDlgButtonChecked(hDlg, 0x410)) {
                    sprintf(szValue, "Level %i Warp X", g_gameReg->m_2c->m_1c);
                    g_gameReg->m_38->SetValueDword(szValue, valX);
                    sprintf(szValue, "Level %i Warp Y", g_gameReg->m_2c->m_1c);
                    g_gameReg->m_38->SetValueDword(szValue, valY);
                    g_gameReg->m_38->SetValueDword("Last Warp Level", g_gameReg->m_2c->m_1c);
                }
                EndDialog(hDlg, 1);
                return 1;
            }
            break;
    }
    return 0;
}

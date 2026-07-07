#include <Mfc.h> // afx.h FIRST (before any windows.h): GameRegistry.h now pulls MFC (unified CObject)
#include <rva.h>
#include <Gruntz/GameRegistry.h>
// VideoConfig.cpp - the video-resolution combo-box config pair on the Gruntz
// options/setup dialog (C:\Proj\Gruntz). Both functions translate between the
// global selected-resolution mode (g_videoResolutionMode: 1=640x480,
// 2=800x600, 3=1024x768) and a UI combo-box + a "current resolution" static
// text control whose caption is built as "Video Resolution " + the resolution
// suffix string.
//
//   LoadVideoResolutionConfig - push the radio/combo
//       state into the dialog via CWnd::FromHandle + CSliderCtrl::SetRange, then
//       refresh the "Video Resolution (WxH)" caption.
//   SaveVideoResolutionConfig - read the combo's
//       current selection back into the mode global, then refresh the caption.
//
// Only offsets / control IDs / code bytes are load-bearing; names are placeholders.
//
// BYTE-EXACT body modulo one MSVC5 deferred-callee-save (`push ebp`) scheduling
// coin-flip (the target defers the ebp save past the four early-return guards;
// our cl saves it eagerly) - see config/units.toml. Kept wip, not strict-exact.

// The USER32 dialog API (GetDlgItem / SendMessageA / SetWindowTextA) + the
// HWND/UINT/WPARAM/LPARAM/LRESULT/LPCSTR types come from the real <windows.h>
// (via Win32.h; pure-Win32 TU, no MFC). strcat comes from <string.h> - under
// /O2 /Oi MSVC5 expands it inline to repnz scasb + rep movs (the suffix-append
// idiom in the disassembly).
#include <Win32.h>
#include <Gruntz/Wnd.h>
#include <Gruntz/Enums.h>
#include <string.h>

// Control-ID literal (kept local, not from <windows.h>).
#define IDC_RESCAPTION 0x52d // the "current resolution" static text ctrl

// ---------------------------------------------------------------------------
// The global selected-resolution discriminator (an int). Save
// stores SendMessage(...,0x400,...)'s return (the combo's current selection)
// here; Load reads it to pick the resolution suffix string. The reloc that
// names it is masked in objdiff; only the address-load bytes are load-bearing.
// ---------------------------------------------------------------------------
DATA(0x0020ccc4)
extern i32 g_videoResolutionMode;

// ---------------------------------------------------------------------------
// MFC controls reached by call-rel32 (external/no-body so the call displacements
// reloc-mask). The combo is wrapped through MFC's CWnd::FromHandle (a static
// __stdcall permanent/temporary-map lookup that returns a CWnd*), then driven as a
// CSliderCtrl: SetRange (__thiscall) seeds the
// (min,max,redraw) range, and the engine 0x405/0x400 messages are exchanged with
// the wrapped HWND held at CWnd+0x1c (m_hWnd).
// ---------------------------------------------------------------------------
// CWnd is the shared minimal MFC view (see <Gruntz/Wnd.h>): FromHandle wraps the
// HWND, m_hWnd at +0x1c.
VTBL(CSliderCtrl, 0x001ecb24);
class CSliderCtrl : public CWnd {
public:
    virtual ~CSliderCtrl() OVERRIDE;         // slot 1
    void SetRange(i32 nMin, i32 nMax, i32 bRedraw);
};

// The CGruntzMgr settings singleton; only the current backbuffer width/height
// (+0x94/+0x98) are touched here. Modeled minimally (defined inline per-TU in the
// retail source, cf. CMenuState/CPlay).
DATA(0x0024556c)
extern CGameRegistry* g_mgrSettings;

// 0x363a0: GetResolutionCode - map the live backbuffer (width,height) to the
// resolution combo index (1024x768 -> 3, 800x600 -> 2, else 1).
RVA(0x000363a0, 0x41)
i32 GetResolutionCode() {
    i32 w = g_mgrSettings->m_savedModeW;
    i32 h = g_mgrSettings->m_savedModeH;
    if (w == 0x400 && h == 0x300) {
        return RES_1024x768;
    }
    if (w == 0x320 && h == 0x258) {
        return RES_800x600;
    }
    return RES_640x480;
}

// ---------------------------------------------------------------------------
// LoadVideoResolutionConfig
// hDlg            - the owning dialog.
// nIDCombo        - control ID of the resolution combo (resolved via GetDlgItem).
// nSel            - the selection index to push into the combo / option control.
// Resolves the engine option-control wrapper, seeds its range (1,3,1), forwards
// nSel to the wrapped child (msg 0x405), then rebuilds the "Video Resolution
// (WxH)" caption on the IDC_RESCAPTION static text from the global mode.
RVA(0x00036f30, 0x114)
void LoadVideoResolutionConfig(HWND hDlg, i32 nIDCombo, i32 nSel) {
    if (!hDlg) {
        return;
    }

    HWND hCombo = GetDlgItem(hDlg, nIDCombo);
    if (!hCombo) {
        return;
    }

    CSliderCtrl* pCtrl = (CSliderCtrl*)CWnd::FromHandle((HWND__*)hCombo);
    if (!pCtrl) {
        return;
    }

    pCtrl->SetRange(1, 3, 1);
    SendMessageA(pCtrl->m_hWnd, 0x405, 1, nSel);

    HWND hCaption = GetDlgItem(hDlg, IDC_RESCAPTION);
    if (!hCaption) {
        return;
    }

    char szCaption[64] = "Video Resolution ";
    switch (g_videoResolutionMode) {
        case RES_640x480:
            strcat(szCaption, "(640x480)");
            break;
        case RES_800x600:
            strcat(szCaption, "(800x600)");
            break;
        case RES_1024x768:
            strcat(szCaption, "(1024x768)");
            break;
        default:
            return;
    }
    SetWindowTextA(hCaption, szCaption);
}

// ---------------------------------------------------------------------------
// SaveVideoResolutionConfig
// hDlg     - the owning dialog (for IDC_RESCAPTION).
// hCombo   - the resolution combo HWND.
// Reads the combo's current selection (engine msg 0x400 -> the wrapped child),
// stores it into the global mode, then rebuilds the caption (same tail as Load).
RVA(0x000370a0, 0xf1)
void SaveVideoResolutionConfig(HWND hDlg, HWND hCombo) {
    CWnd* pCtrl = CWnd::FromHandle((HWND__*)hCombo);
    if (!pCtrl) {
        return;
    }

    g_videoResolutionMode = SendMessageA(pCtrl->m_hWnd, 0x400, 0, 0);

    HWND hCaption = GetDlgItem(hDlg, IDC_RESCAPTION);
    if (!hCaption) {
        return;
    }

    char szCaption[64] = "Video Resolution ";
    switch (g_videoResolutionMode) {
        case RES_640x480:
            strcat(szCaption, "(640x480)");
            break;
        case RES_800x600:
            strcat(szCaption, "(800x600)");
            break;
        case RES_1024x768:
            strcat(szCaption, "(1024x768)");
            break;
        default:
            return;
    }
    SetWindowTextA(hCaption, szCaption);
}

// 0x378c0: SaveVideoCheckboxes(hDlg) - latch the two video option checkboxes
// (IDC 0x46f smooth-scroll, 0x4d5 show-fps) into the settings singleton. No-op
// when the singleton is not yet live.
// @early-stop
// 99.5%: the same deferred-callee-save (push) scheduling coin-flip as this TU's
// resolution-config pair; logic + offsets byte-exact.
RVA(0x000378c0, 0x40)
void SaveVideoCheckboxes(HWND hDlg) {
    if (g_mgrSettings == 0) {
        return;
    }
    g_mgrSettings->m_isHighDetail = IsDlgButtonChecked(hDlg, 0x46f);
    g_mgrSettings->m_isEffectsEnabled = IsDlgButtonChecked(hDlg, 0x4d5);
}
SIZE_UNKNOWN(CSliderCtrl);

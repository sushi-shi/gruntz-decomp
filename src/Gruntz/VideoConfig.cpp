// VideoConfig.cpp - the video-resolution combo-box config pair on the Gruntz
// options/setup dialog (C:\Proj\Gruntz). Both functions translate between the
// global selected-resolution mode (g_videoResolutionMode @0x60ccc4: 1=640x480,
// 2=800x600, 3=1024x768) and a UI combo-box + a "current resolution" static
// text control whose caption is built as "Video Resolution " + the resolution
// suffix string.
//
//   LoadVideoResolutionConfig @0x36f30 (276 B, __cdecl) - push the radio/combo
//       state into the dialog via CWnd::FromHandle + CSliderCtrl::SetRange, then
//       refresh the "Video Resolution (WxH)" caption.
//   SaveVideoResolutionConfig @0x370a0 (241 B, __cdecl) - read the combo's
//       current selection back into the mode global, then refresh the caption.
//
// Only offsets / control IDs / code bytes are load-bearing; names are placeholders.
//
// BYTE-EXACT body modulo one MSVC5 deferred-callee-save (`push ebp`) scheduling
// coin-flip (the target defers the ebp save past the four early-return guards;
// our cl saves it eagerly) - see config/units.toml. Kept wip, not strict-exact.

// ---------------------------------------------------------------------------
// Minimal Win32 surface (USER32 dialog API). We deliberately do NOT pull in
// <windows.h> - keep the visible symbol SET small (the compiler hashes it;
// entropy follows header churn - see docs/matching-patterns.md). This
// reproduces the FF15 [IAT] direct-call form for the imports.
// ---------------------------------------------------------------------------
typedef void *         HWND;
typedef int            BOOL;
typedef unsigned int   UINT;
typedef unsigned int   WPARAM;
typedef long           LPARAM;
typedef long           LRESULT;
typedef const char *   LPCSTR;

extern "C" {
__declspec(dllimport) HWND    __stdcall GetDlgItem(HWND hDlg, int nIDDlgItem);
__declspec(dllimport) LRESULT __stdcall SendMessageA(HWND hWnd, UINT Msg,
                                                     WPARAM wParam, LPARAM lParam);
__declspec(dllimport) BOOL    __stdcall SetWindowTextA(HWND hWnd, LPCSTR lpString);

// The CRT strcat - under /O2 /Oi MSVC5 expands it inline to repnz scasb (find
// the destination's nul) + rep movs (copy the source over it), exactly the
// suffix-append idiom in the disassembly.
char *__cdecl strcat(char *dest, const char *src);
}

// Control-ID literal (kept local, not from <windows.h>).
#define IDC_RESCAPTION 0x52d        // the "current resolution" static text ctrl

// ---------------------------------------------------------------------------
// The global selected-resolution discriminator (an int at VA 0x60ccc4). Save
// stores SendMessage(...,0x400,...)'s return (the combo's current selection)
// here; Load reads it to pick the resolution suffix string. The reloc that
// names it is masked in objdiff; only the address-load bytes are load-bearing.
// ---------------------------------------------------------------------------
// @data: 0x20ccc4
extern int g_videoResolutionMode;

// ---------------------------------------------------------------------------
// MFC controls reached by call-rel32 (external/no-body so the call displacements
// reloc-mask). The combo is wrapped through MFC's CWnd::FromHandle (a static
// __stdcall permanent/temporary-map lookup that returns a CWnd*; the delinked
// target names it ?FromHandle@CWnd@@SGPAV1@PAUHWND__@@@Z), then driven as a
// CSliderCtrl: SetRange (?SetRange@CSliderCtrl@@QAEXHHH@Z, __thiscall) seeds the
// (min,max,redraw) range, and the engine 0x405/0x400 messages are exchanged with
// the wrapped HWND held at CWnd+0x1c (m_hWnd).
// ---------------------------------------------------------------------------
struct HWND__;          // the strong HWND tag MFC's signature mangles in
class CWnd {
public:
    static CWnd *__stdcall FromHandle(HWND__ *hWnd);   // @0x1bb23a
    char m_pad00[0x1c];
    HWND m_hWnd;        // +0x1c  the wrapped window handle
};
class CSliderCtrl : public CWnd {
public:
    void SetRange(int nMin, int nMax, int bRedraw);    // @0x11e0f9 (__thiscall)
};

// ---------------------------------------------------------------------------
// LoadVideoResolutionConfig  @0x36f30
// hDlg            - the owning dialog.
// nIDCombo        - control ID of the resolution combo (resolved via GetDlgItem).
// nSel            - the selection index to push into the combo / option control.
// Resolves the engine option-control wrapper, seeds its range (1,3,1), forwards
// nSel to the wrapped child (msg 0x405), then rebuilds the "Video Resolution
// (WxH)" caption on the IDC_RESCAPTION static text from the global mode.
//
// @address: 0x36f30
// @size:    0x114
// ---------------------------------------------------------------------------
void LoadVideoResolutionConfig(HWND hDlg, int nIDCombo, int nSel)
{
    if (!hDlg)
        return;

    HWND hCombo = GetDlgItem(hDlg, nIDCombo);
    if (!hCombo)
        return;

    CSliderCtrl *pCtrl = (CSliderCtrl *)CWnd::FromHandle((HWND__ *)hCombo);
    if (!pCtrl)
        return;

    pCtrl->SetRange(1, 3, 1);
    SendMessageA(pCtrl->m_hWnd, 0x405, 1, nSel);

    HWND hCaption = GetDlgItem(hDlg, IDC_RESCAPTION);
    if (!hCaption)
        return;

    char szCaption[64] = "Video Resolution ";
    switch (g_videoResolutionMode) {
    case 1:  strcat(szCaption, "(640x480)");  break;
    case 2:  strcat(szCaption, "(800x600)");  break;
    case 3:  strcat(szCaption, "(1024x768)"); break;
    default: return;
    }
    SetWindowTextA(hCaption, szCaption);
}

// ---------------------------------------------------------------------------
// SaveVideoResolutionConfig  @0x370a0
// hDlg     - the owning dialog (for IDC_RESCAPTION).
// hCombo   - the resolution combo HWND.
// Reads the combo's current selection (engine msg 0x400 -> the wrapped child),
// stores it into the global mode, then rebuilds the caption (same tail as Load).
//
// @address: 0x370a0
// @size:    0xf1
// ---------------------------------------------------------------------------
void SaveVideoResolutionConfig(HWND hDlg, HWND hCombo)
{
    CWnd *pCtrl = CWnd::FromHandle((HWND__ *)hCombo);
    if (!pCtrl)
        return;

    g_videoResolutionMode = SendMessageA(pCtrl->m_hWnd, 0x400, 0, 0);

    HWND hCaption = GetDlgItem(hDlg, IDC_RESCAPTION);
    if (!hCaption)
        return;

    char szCaption[64] = "Video Resolution ";
    switch (g_videoResolutionMode) {
    case 1:  strcat(szCaption, "(640x480)");  break;
    case 2:  strcat(szCaption, "(800x600)");  break;
    case 3:  strcat(szCaption, "(1024x768)"); break;
    default: return;
    }
    SetWindowTextA(hCaption, szCaption);
}

// Dialogs.cpp - the MFC CDialog-subclass constructors for the battle/multiplayer
// setup dialogs (CBattlezDlg / CBattlezDlgCustom / CBattlezDlgColors /
// CMultiStartDlg), plus the WarpDialogProc dialog procedure. Each ctor chains the
// NAFXCW CDialog(UINT, CWnd*) base ctor (reloc-masked), stores its own derived
// vftable, default-constructs its embedded MFC members, and zero/inits the scalar
// members it touches.
//
// WarpDialogProc @0x08e4e0 is a free __stdcall dialog proc for the warp/teleport
// dialog. Handles WM_INITDIALOG (sets X/Y edit controls from the current view) and
// WM_COMMAND (reads X/Y, optionally persists them to the registry via the game's
// config manager if the checkbox is checked, then saves the Last Warp Level).
//
// Built /GX: three of the four ctors construct embedded MFC C++ objects (CString /
// CObList) and so carry an fs:0 EH frame (push -1 / push handler / mov fs:0,esp) to
// unwind a half-built object if a member ctor throws. (CBattlezDlgColors has NO
// embedded C++ member, so its body carries no EH frame even under /GX.)
//
// Field names are placeholders (m_<hexoffset>); only the OFFSETS + the code bytes
// are load-bearing (campaign doctrine).
// ---------------------------------------------------------------------------
#include "Dialogs.h"
#include <stdio.h>   // engine sprintf @0x11f890 (reloc-masked)

// The global CGameRegistry (@0x64556c) CMultiStartDlg's ctor snapshots: it copies
// g_gameReg->m_2c into the file-scope sink g_64bd5c (both reloc-masked DIR32).
// Named externs so the DIR32 loads reloc-match the engine; @data names the
// delinked target DATA symbol (RVA = VA - 0x400000).
// @data: 0x24556c
extern int *g_gameReg;   // the CGameRegistry pointer stored at VA 0x64556c
// @data: 0x24bd5c
extern int g_64bd5c;     // the file-scope int sink at VA 0x64bd5c

// ---------------------------------------------------------------------------
// Minimal Win32 surface for the Warp dialog. Only the exact imports WarpDialogProc
// reaches through the IAT (FF15 [IAT]) are declared.
// ---------------------------------------------------------------------------
typedef int INT_PTR;
typedef unsigned int UINT;
typedef unsigned long WPARAM;
typedef long LPARAM;
#ifdef _WIN64
typedef unsigned long long HWND_;
#else
typedef unsigned long HWND_;
#endif
#define HWND HWND_

extern "C" {
__declspec(dllimport) INT_PTR __stdcall EndDialog(HWND, INT_PTR);
__declspec(dllimport) UINT __stdcall GetDlgItemInt(HWND, int, int *, int);
__declspec(dllimport) int __stdcall IsDlgButtonChecked(HWND, int);
__declspec(dllimport) int __stdcall SetDlgItemInt(HWND, int, UINT, int);
}

#define WM_INITDIALOG 0x0110
#define WM_COMMAND    0x0111

// The three format/literal strings the warp-dialog uses to read/write the registry.
// @data: 0x210bdc  (RVA of "Level %i Warp X")
// @data: 0x210be8  (RVA of "Level %i Warp Y")
// @data: 0x210bd4  (RVA of "Last Warp Level")

// File-scope globals the Warp dialog caches the X/Y edit selections in (VA 0x612610
// / 0x612614). Static decls so the DIR32 data relocs are TU-local (reloc-masked).
// @data: 0x212610
static int g_warpX;      // VA 0x612610  (the cached X coordinate)
// @data: 0x212614
static int g_warpY;      // VA 0x612614  (the cached Y coordinate)

// The game-registry config helper (thiscall, ret 8 = 2 stack args + ecx(this)).
// This is the function at 0x139460, called as reg->m_38->SetValue(key, value).
// Modeled as an external no-body with the right mangled shape.
struct RegConfig {
    void SetValue(const char *key, int value);  // @0x139460 (thiscall, 2 args)
};

// ---------------------------------------------------------------------------
// WarpDialogProc  @ RVA 0x08e4e0  (INT_PTR CALLBACK __stdcall ret 0x10)
//
// The warp/teleport dialog procedure. Handles:
//   WM_INITDIALOG  -- sets the X/Y edit controls from the current view offset.
//   WM_COMMAND     -- IDOK (1): reads X/Y, caches to g_warpX/Y, and if the
//                     "save" checkbox (0x410) is checked, persists (X,Y) to
//                     the registry under "Level {id} Warp X/Y" and saves the
//                     "Last Warp Level". IDCANCEL (2): closes. Both return 1.
//   default        -- returns 0 (unhandled).
//
// @address: 0x08e4e0
// @size:    0x172
// ---------------------------------------------------------------------------
INT_PTR __stdcall WarpDialogProc(HWND hDlg, UINT message,
                                 WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_COMMAND:
        switch (wParam) {
        case 2:     // IDCANCEL
            EndDialog(hDlg, 0);
            return 1;

        case 1: {   // IDOK
            int x = GetDlgItemInt(hDlg, 0x40e, 0, 0);
            int y = GetDlgItemInt(hDlg, 0x40f, 0, 0);
            int checked = IsDlgButtonChecked(hDlg, 0x410);

            g_warpX = x;
            g_warpY = y;

            if (checked != 0) {
                char buf[0x80];

                // reg->m_2c (pointer at +0x2c) -> field at +0x1c = current level id
                int *pLevel = (int *)g_gameReg[0x2c / 4];
                int levelId = pLevel[0x1c / 4];

                sprintf(buf, "Level %i Warp X", levelId);
                ((RegConfig *)(g_gameReg[0x38 / 4]))->SetValue(buf, x);

                sprintf(buf, "Level %i Warp Y", levelId);
                ((RegConfig *)(g_gameReg[0x38 / 4]))->SetValue(buf, y);

                ((RegConfig *)(g_gameReg[0x38 / 4]))->SetValue("Last Warp Level", levelId);
            }

            EndDialog(hDlg, 1);
            return 1;
        }
        }
        break;

    case WM_INITDIALOG: {
        // Read X/Y from the current view/camera state:
        // g_gameReg->m_30->m_24->m_5c->{m_84=.., m_88=..}
        int *viewData = (int *)g_gameReg[0x30 / 4];     // reg->m_30
        viewData = (int *)viewData[0x24 / 4];           // reg->m_30->m_24
        viewData = (int *)viewData[0x5c / 4];           // reg->m_30->m_24->m_5c
        int warpX = viewData[0x84 / 4];
        int warpY = viewData[0x88 / 4];

        SetDlgItemInt(hDlg, 0x40e, warpX, 0);
        SetDlgItemInt(hDlg, 0x40f, warpY, 0);
        return 1;
    }
    }

    return 0;
}

// ---------------------------------------------------------------------------
// CBattlezDlg::CBattlezDlg @0x14b30  (vftable @0x5e8bac)
// ---------------------------------------------------------------------------
// @address: 0x14b30
// @size:    0x64
CBattlezDlg::CBattlezDlg(int a0, CWnd *pParent)
    : CDialog(0xc0, pParent)
{
    m_5c = a0;
    m_68 = 0;
}

// ---------------------------------------------------------------------------
// CBattlezDlgCustom::CBattlezDlgCustom @0x18030  (vftable @0x5e8ee4)
// ---------------------------------------------------------------------------
// @address: 0x18030
// @size:    0x56
CBattlezDlgCustom::CBattlezDlgCustom(CWnd *pParent)
    : CDialog(0xc3, pParent)
{
}

// ---------------------------------------------------------------------------
// CBattlezDlgColors::CBattlezDlgColors @0x17930  (vftable @0x5e8d94)
// ---------------------------------------------------------------------------
// @address: 0x17930
// @size:    0x3a
CBattlezDlgColors::CBattlezDlgColors(int a0, int a1, int a2, CWnd *pParent)
    : CDialog(0xc2, pParent)
{
    m_5c = a0;
    m_60 = a1;
    m_64 = 0;
    m_68 = a2;
}

// ---------------------------------------------------------------------------
// CMultiStartDlg::CMultiStartDlg @0xc1750  (vftable @0x5ea8ec)
// ---------------------------------------------------------------------------
// @address: 0xc1750
// @size:    0x88
CMultiStartDlg::CMultiStartDlg(int a0, CWnd *pParent)
    : CDialog(0xc5, pParent), m_74(0xa)
{
    m_5c = a0;
    m_6c = 0;
    m_60 = 0;
    g_64bd5c = g_gameReg[0x2c / 4];
}

// ---------------------------------------------------------------------------
// CCheckpointDlg::CCheckpointDlg @0x234a0  (vftable @0x5e9504)
// ---------------------------------------------------------------------------
// @address: 0x234a0
// @size:    0x1e
CCheckpointDlg::CCheckpointDlg(CWnd *pParent)
    : CDialog(0xcd, pParent)
{
}

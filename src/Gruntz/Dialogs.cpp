// Dialogs.cpp - the MFC CDialog-subclass constructors for the battle/multiplayer
// setup dialogs (CBattlezDlg / CBattlezDlgCustom / CBattlezDlgColors /
// CMultiStartDlg). Each ctor chains the NAFXCW CDialog(UINT, CWnd*) base ctor
// (reloc-masked), stores its own derived vftable, default-constructs its embedded
// MFC members, and zero/inits the scalar members it touches.
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

// The global CGameRegistry (@0x64556c) CMultiStartDlg's ctor snapshots: it copies
// g_gameReg->m_2c into the file-scope sink g_64bd5c (both reloc-masked DIR32).
// Named externs so the DIR32 loads reloc-match the engine; @data names the
// delinked target DATA symbol (RVA = VA - 0x400000).
// @data: 0x24556c
extern int *g_gameReg;   // the CGameRegistry pointer stored at VA 0x64556c
// @data: 0x24bd5c
extern int g_64bd5c;     // the file-scope int sink at VA 0x64bd5c

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

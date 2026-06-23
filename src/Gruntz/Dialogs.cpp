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
#include <Gruntz/Dialogs.h>
#include <rva.h>

// The global CGameRegistry CMultiStartDlg's ctor snapshots: it copies
// g_gameReg->m_2c into the file-scope sink g_64bd5c (both reloc-masked DIR32).
// Named externs so the DIR32 loads reloc-match the engine; @data names the
// delinked target DATA symbol (RVA = VA - 0x400000).
DATA(0x0024556c)
extern int* g_gameReg; // the CGameRegistry pointer (reloc-masked DATA symbol)
DATA(0x0024bd5c)
extern int g_64bd5c; // the file-scope int sink (reloc-masked DATA symbol)

// ---------------------------------------------------------------------------
RVA(0x00014b30, 0x64)
CBattlezDlg::CBattlezDlg(int a0, CWnd* pParent) : CDialog(0xc0, pParent) {
    m_5c = a0;
    m_68 = 0;
}

// ---------------------------------------------------------------------------
RVA(0x00018030, 0x56)
CBattlezDlgCustom::CBattlezDlgCustom(CWnd* pParent) : CDialog(0xc3, pParent) {}

// ---------------------------------------------------------------------------
RVA(0x00017930, 0x3a)
CBattlezDlgColors::CBattlezDlgColors(int a0, int a1, int a2, CWnd* pParent)
    : CDialog(0xc2, pParent) {
    m_5c = a0;
    m_60 = a1;
    m_64 = 0;
    m_68 = a2;
}

// ---------------------------------------------------------------------------
RVA(0x000c1750, 0x88)
CMultiStartDlg::CMultiStartDlg(int a0, CWnd* pParent) : CDialog(0xc5, pParent), m_74(0xa) {
    m_5c = a0;
    m_6c = 0;
    m_60 = 0;
    g_64bd5c = g_gameReg[0x2c / 4];
}

// -------------------------------------------------------------------------
// Engine-label backlog stub (relocated from src/Stub/ - own this class here).
// -------------------------------------------------------------------------
// @confidence: med
// @source: string-xref
// @stub
RVA(0x000c20a0, 0x45a)
void CMultiStartDlg::InitPlayerSlots() {}

// ---------------------------------------------------------------------------
RVA(0x000234a0, 0x1e)
CCheckpointDlg::CCheckpointDlg(CWnd* pParent) : CDialog(0xcd, pParent) {}

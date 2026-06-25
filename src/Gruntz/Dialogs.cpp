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
extern i32* g_gameReg; // the CGameRegistry pointer (reloc-masked DATA symbol)
DATA(0x0024bd5c)
extern i32 g_64bd5c; // the file-scope int sink (reloc-masked DATA symbol)

// ---------------------------------------------------------------------------
RVA(0x00014b30, 0x64)
CBattlezDlg::CBattlezDlg(i32 a0, CWnd* pParent) : CDialog(0xc0, pParent) {
    m_5c = a0;
    m_68 = 0;
}

// ~CBattlezDlg @0x14c90 - destroy the CString member m_6c, then chain the NAFXCW
// ~CDialog base dtor (both reloc-masked). The /GX EH frame unwinds the half-torn
// object across the member dtor.
// @early-stop
// vptr-restamp-presence wall (docs/patterns/eh-dtor-vptr-restamp-presence.md): the
// /GX frame + member ~CString + base ~CDialog chain are byte-exact, but our
// polymorphic model emits one extra `mov [esi],&??_7CBattlezDlg` re-stamp at entry
// that retail elided (its vtable already equals the base through the dtor). Not
// source-steerable; ~94.4%.
RVA(0x00014c90, 0x47)
CBattlezDlg::~CBattlezDlg() {}

// ---------------------------------------------------------------------------
RVA(0x00018030, 0x56)
CBattlezDlgCustom::CBattlezDlgCustom(CWnd* pParent) : CDialog(0xc3, pParent) {}

// ~CBattlezDlgCustom @0x17140 - destroy the CString member m_5c, then chain the
// NAFXCW ~CDialog base dtor. /GX EH frame for the member unwind.
// @early-stop
// vptr-restamp-presence wall (docs/patterns/eh-dtor-vptr-restamp-presence.md): same
// as ~CBattlezDlg - one extra most-derived vptr re-stamp our polymorphic model emits
// that retail elided; chain otherwise byte-exact. ~94.4%.
RVA(0x00017140, 0x47)
CBattlezDlgCustom::~CBattlezDlgCustom() {}

// ---------------------------------------------------------------------------
RVA(0x00017930, 0x3a)
CBattlezDlgColors::CBattlezDlgColors(i32 a0, i32 a1, i32 a2, CWnd* pParent)
    : CDialog(0xc2, pParent) {
    m_5c = a0;
    m_60 = a1;
    m_64 = 0;
    m_68 = a2;
}

// ---------------------------------------------------------------------------
RVA(0x000c1750, 0x88)
CMultiStartDlg::CMultiStartDlg(i32 a0, CWnd* pParent) : CDialog(0xc5, pParent), m_74(0xa) {
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

// ---------------------------------------------------------------------------
// CBattlezDlg control accessors: switch(index) over a 4-entry control-ID table,
// each case tail-calling this->GetDlgItem(constID) (which returns the child
// CWnd*). Default (index>3) returns null. Four families, identical shape over
// different ID tables. The inline .rdata jump table reloc-masks.
//
// The dispatch + all four case bodies are byte-exact vs retail (verified by
// llvm-objdump base-vs-target); the ~70% plateau is the inline jump-table DATA
// region + its base reloc scored as mismatched (docs/patterns/jumptable-data-
// overlap.md). The result-var spelling forces the retail edx-index/xor-eax-eax
// dispatch (docs/patterns/switch-pointer-default-result-var.md).
// ---------------------------------------------------------------------------
// @early-stop
// jump-table-data scoring artifact (code byte-exact) - docs/patterns/jumptable-data-overlap.md
RVA(0x00015ac0, 0x46)
CWnd* CBattlezDlg::GetCtrlA(i32 index) {
    CWnd* result = 0;
    switch (index) {
        case 0:
            result = GetDlgItem(0x500);
            break;
        case 1:
            result = GetDlgItem(0x50e);
            break;
        case 2:
            result = GetDlgItem(0x50f);
            break;
        case 3:
            result = GetDlgItem(0x510);
            break;
    }
    return result;
}

// @early-stop
// jump-table-data scoring artifact (code byte-exact) - docs/patterns/jumptable-data-overlap.md
RVA(0x00015b40, 0x46)
CWnd* CBattlezDlg::GetCtrlB(i32 index) {
    CWnd* result = 0;
    switch (index) {
        case 0:
            result = GetDlgItem(0x50a);
            break;
        case 1:
            result = GetDlgItem(0x50b);
            break;
        case 2:
            result = GetDlgItem(0x50c);
            break;
        case 3:
            result = GetDlgItem(0x50d);
            break;
    }
    return result;
}

// @early-stop
// jump-table-data scoring artifact (code byte-exact) - docs/patterns/jumptable-data-overlap.md
RVA(0x00015bc0, 0x46)
CWnd* CBattlezDlg::GetCtrlC(i32 index) {
    CWnd* result = 0;
    switch (index) {
        case 0:
            result = GetDlgItem(0x51e);
            break;
        case 1:
            result = GetDlgItem(0x520);
            break;
        case 2:
            result = GetDlgItem(0x521);
            break;
        case 3:
            result = GetDlgItem(0x522);
            break;
    }
    return result;
}

// @early-stop
// jump-table-data scoring artifact (code byte-exact) - docs/patterns/jumptable-data-overlap.md
RVA(0x00015c40, 0x46)
CWnd* CBattlezDlg::GetCtrlD(i32 index) {
    CWnd* result = 0;
    switch (index) {
        case 0:
            result = GetDlgItem(0x501);
            break;
        case 1:
            result = GetDlgItem(0x503);
            break;
        case 2:
            result = GetDlgItem(0x505);
            break;
        case 3:
            result = GetDlgItem(0x507);
            break;
    }
    return result;
}

// SetCtrlBText - resolve control `index` via GetCtrlB (through the thunk) and
// push `text` into it via CWnd::SetWindowTextA (both NAFXCW, reloc-masked).
RVA(0x00015db0, 0x19)
void CBattlezDlg::SetCtrlBText(i32 index, const char* text) {
    CWnd* w = GetCtrlB(index);
    w->SetWindowTextA(text);
}

// -------------------------------------------------------------------------
// Engine-label backlog stubs (relocated from src/Stub/ - own this class here).
// -------------------------------------------------------------------------
// @confidence: low
// @source: winapi:InvalidateRect
// @stub
RVA(0x00016cd0, 0x98)
i32 CBattlezDlg::winapi_016cd0_InvalidateRect() {
    return 0;
}

// @confidence: low
// @source: winapi:InvalidateRect
// @stub
RVA(0x00016dc0, 0x97)
i32 CBattlezDlg::winapi_016dc0_InvalidateRect() {
    return 0;
}

// @confidence: low
// @source: winapi:InvalidateRect
// @stub
RVA(0x00016e90, 0x98)
i32 CBattlezDlg::winapi_016e90_InvalidateRect() {
    return 0;
}

// @confidence: low
// @source: winapi:InvalidateRect
// @stub
RVA(0x00016f60, 0x98)
i32 CBattlezDlg::winapi_016f60_InvalidateRect() {
    return 0;
}

// @confidence: low
// @source: winapi:GetWindow;SendMessageA
// @stub
RVA(0x000171b0, 0xca)
i32 CBattlezDlg::winapi_0171b0_GetWindow_SendMessageA() {
    return 0;
}

// ---------------------------------------------------------------------------
// SetSlotValue - store `val` into the 0x158 field of slot `index` in the slot
// array based at m_5c (0x238 bytes/slot). Returns TRUE.
RVA(0x00017460, 0x22)
i32 CBattlezDlg::SetSlotValue(i32 index, i32 val) {
    *(i32*)((char*)((CBattlezSlot*)m_5c + index) + 0x158) = val;
    return 1;
}

// MultiStartDlg.cpp - the CMultiStartDlg (multiplayer-start CDialog, resource 0xc5)
// core: its ctor (the vtable emission anchor), the slot-list build/update model,
// the per-index control accessors, and the GetCtrlE combo helpers. One contiguous
// retail .text block (0xc1750..0xc296b). Split out of the Dialogs.cpp aggregate
// (matcher-1 de-fragmentation); the rest of the class lives in its thematic sibling
// units (MultiStartDlg{World,Roster,Color,Net}.cpp + the NetMgr facet units).
//
// Built /GX (eh): the ctor default-constructs embedded MFC members (CString m_70 /
// CObList m_74) and BuildSlotList's new-expression carries the fs:0 EH frame.
// Field names are placeholders (m_<hexoffset>); only offsets + code bytes are
// load-bearing (campaign doctrine).
// ---------------------------------------------------------------------------
#include <Gruntz/Dialogs.h>
#include <Net/InterfaceObject.h>
#include <Gruntz/GameRegistry.h> // the real CGameRegistry (g_gameReg; m_curState @+0x2c)
#include <Gruntz/Multi.h>        // the real CMulti (the 0x64bd5c multiplayer game-state singleton)
#include <Net/LatencyList.h>     // CLatencyList (m_slotList; Dispatch/FillCombo/SelectItem)
#include <rva.h>
#include <Globals.h>

// The global CGameRegistry the ctor snapshots: it copies g_gameReg->m_curState into
// the file-scope multiplayer game-state sink g_64bd5c (both reloc-masked DIR32).
DATA(0x0024556c)
extern CGameRegistry* g_gameReg; // the CGameRegistry pointer (reloc-masked DATA symbol)
// 0x64bd5c holds the multiplayer game-state singleton (a CMulti, xref-proven); the
// ctor snapshots it from g_gameReg->m_curState (+0x2c). (DATA also bound in
// MultiStartDlgWorld.cpp / ReconBatch2.)
DATA(0x0024bd5c)
extern CMulti* g_64bd5c;

// A player-slot record in the m_host slot array (0x238 stride); only the +0x16c
// occupancy field is read here.
struct CMultiSlot {
    char m_pad00[0x16c];
    i32 m_16c; // +0x16c
    char m_pad170[0x238 - 0x170];
};

// GetCtrlE (0xc2640): the fifth per-index combo getter (control IDs 0x500/0x50e/
// 0x50f/0x510 on the shared multiplayer-setup dialog). A free __stdcall helper;
// declared-only here (owned as a data mislabel in globals), so the call reloc-masks.
extern CWnd* __stdcall GetCtrlE(i32 index);

// ---------------------------------------------------------------------------
RVA(0x000c1750, 0x88)
CMultiStartDlg::CMultiStartDlg(i32 a0, CWnd* pParent) : CDialog(0xc5, pParent), m_74(0xa) {
    m_host = a0;
    m_6c = 0;
    m_slotList = 0;
    g_64bd5c = (CMulti*)g_gameReg->m_curState;
}

// CMultiStartDlg::BuildSlotList (0xc1e60): allocate the player-slot list, derive
// the player count from the registry snapshot (a forced count, else a cascade of
// slot-occupancy probes), seed the list with the count + the dialog's selection.
// /GX EH frame for the new-expression's ctor unwind.
// @early-stop
// regalloc/scheduling wall (docs/patterns/zero-register-pinning.md): the new-expr,
// the count cascade, the inlined GetSafe1c null-check, and the three list calls are
// all logic-faithful; the residual is the callee-saved register assignment for the
// count/pi/selection values (ebp-vs-edi choice) cascading into push scheduling. ~89%.
RVA(0x000c1e60, 0x115)
void CMultiStartDlg::BuildSlotList() {
    m_slotList = new CLatencyList(0xa);
    CMulti* reg = g_64bd5c;
    i32 count = 5;
    CMultiPlayerInfo* pi = reg->m_netGate->m_70;
    if (reg->m_588) {
        count = 2;
    } else if (pi) {
        if (((InterfaceObject*)pi)->IsInterface1()) {
            count = 1;
        }
        if (((InterfaceObject*)pi)->IsInterface2()) {
            count = 2;
        }
        if (((InterfaceObject*)pi)->IsInterface3()) {
            count = 3;
        }
        if (((InterfaceObject*)pi)->IsInterface4()) {
            count = 4;
        }
    }
    m_slotList->Dispatch(count);
    i32 v = GetSafe1c();
    m_slotList->FillCombo(v, 0x527);
    m_slotList->SelectItem(v, 0x527, 0, 0);
    reg->m_600 = 1;
}

// CMultiStartDlg::UpdateSlot (0xc1fd0): enable the team control by current-slot
// occupancy, then push the dialog selection (with the registry color pair, unless
// already committed) into the slot list. Returns 1 (0 when the control is absent).
// @early-stop
// regalloc coin-flip wall (docs/patterns/zero-register-pinning.md): GetDlgItem gate,
// the 0x238-stride slot probe, the inlined GetSafe1c, and the committed/color
// Method3396 branch are byte-faithful; the inlined GetSafe1c result lands in ecx
// (retail keeps it in eax), cascading into the g_64bd5c register + the final push
// schedule. A pure allocator choice, no source lever. ~92%.
RVA(0x000c1fd0, 0x99)
i32 CMultiStartDlg::UpdateSlot() {
    CWnd* w = GetDlgItem(0x527);
    if (w == 0) {
        return 0;
    }
    CMulti* reg = g_64bd5c;
    i32 enable;
    if (reg->m_isHost) {
        i32 idx = GetSlotIndex();
        enable = (((CMultiSlot*)m_host)[idx].m_16c == 0);
    } else {
        enable = 0;
    }
    w->EnableWindow(enable);
    i32 v = GetSafe1c();
    CMulti* reg2 = g_64bd5c;
    if (reg2->m_600) {
        m_slotList->SelectItem(v, 0x527, 0, 0);
    } else {
        m_slotList->SelectItem(v, 0x527, reg2->m_5a4, reg2->m_drainReload);
    }
    return 1;
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
// CMultiStartDlg per-slot control accessors: switch(index) over a 4-entry
// control-ID table, each case returning this->GetDlgItem(constID). SAME shape as
// CBattlezDlg::GetCtrlA..D; the inline .rdata jump table reloc-masks.
// @early-stop
// jump-table-data scoring artifact (code byte-exact) - docs/patterns/jumptable-data-overlap.md
RVA(0x000c26c0, 0x46)
CWnd* CMultiStartDlg::GetCtrlA(i32 index) {
    CWnd* result = 0;
    switch (index) {
        case 0:
            result = GetDlgItem(0x51f);
            break;
        case 1:
            result = GetDlgItem(0x523);
            break;
        case 2:
            result = GetDlgItem(0x524);
            break;
        case 3:
            result = GetDlgItem(0x525);
            break;
    }
    return result;
}

// @early-stop
// jump-table-data scoring artifact (code byte-exact) - docs/patterns/jumptable-data-overlap.md
RVA(0x000c2740, 0x46)
CWnd* CMultiStartDlg::GetCtrlB(i32 index) {
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
RVA(0x000c27c0, 0x46)
CWnd* CMultiStartDlg::GetCtrlC(i32 index) {
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
RVA(0x000c2840, 0x46)
CWnd* CMultiStartDlg::GetCtrlD(i32 index) {
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

// SetComboSelE (0xc28c0, __stdcall): set the GetCtrlE combo's current selection
// (CB_SETCURSEL 0x14e), if the control exists.
RVA(0x000c28c0, 0x27)
void __stdcall SetComboSelE(i32 index, i32 sel) {
    CWnd* c = GetCtrlE(index);
    if (c != 0) {
        SendMessageA(c->m_hWnd, 0x14e, sel, 0);
    }
}

// GetComboSelE (0xc2900, __stdcall): the GetCtrlE combo's current selection
// (CB_GETCURSEL 0x147), or -1 when the control is missing.
RVA(0x000c2900, 0x2a)
i32 __stdcall GetComboSelE(i32 index) {
    CWnd* c = GetCtrlE(index);
    if (c == 0) {
        return -1;
    }
    return SendMessageA(c->m_hWnd, 0x147, 0, 0);
}

// GetComboSelC (0xc2940): the GetCtrlC combo's cur-sel + 1 (CB_GETCURSEL 0x147), or
// -1 when the control is missing. The GetCtrlC sibling of CBattlezDlg::Query015d30
// (which lacks the null guard).
RVA(0x000c2940, 0x2b)
i32 CMultiStartDlg::GetComboSelC(i32 id) {
    CWnd* c = GetCtrlC(id);
    if (c == 0) {
        return -1;
    }
    return SendMessageA(c->m_hWnd, 0x147, 0, 0) + 1;
}

SIZE_UNKNOWN(CMultiSlot);

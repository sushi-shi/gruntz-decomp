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
// CMultiStartDlg multiplayer-setup helpers (BuildSlotList / UpdateSlot model).
// ---------------------------------------------------------------------------
// The game-registry snapshot g_64bd5c holds (a per-team player-info sub-object at
// +0x524->+0x70 the slot probes run on, a team/mode latch at +0x528/+0x588/+0x600,
// and the +0x5a4/+0x5a8 color pair). Reloc-masked field offsets.
struct CMultiPlayerInfo { // reg->m_524->m_70
    i32 Q1794b0();        // 0x1794b0  slot-1 occupancy probe
    i32 Q1794e0();        // 0x1794e0  slot-2
    i32 Q179510();        // 0x179510  slot-3
    i32 Q179540();        // 0x179540  slot-4
};
struct CMultiRegSub {
    char m_pad00[0x70];
    CMultiPlayerInfo* m_70; // +0x70
};
struct CMultiReg {
    char m_pad000[0x524];
    CMultiRegSub* m_524; // +0x524
    void* m_528;         // +0x528  mode/team latch
    char m_pad52c[0x588 - 0x52c];
    void* m_588;         // +0x588  forced-count latch
    char m_pad58c[0x5a4 - 0x58c];
    i32 m_5a4; // +0x5a4
    i32 m_5a8; // +0x5a8
    char m_pad5ac[0x600 - 0x5ac];
    i32 m_600; // +0x600  committed flag
};

// A player-slot record in the m_5c slot array (0x238 stride); only the +0x16c
// occupancy field is read.
struct CMultiSlot {
    char m_pad00[0x16c];
    i32 m_16c; // +0x16c
    char m_pad170[0x238 - 0x170];
};

// The player-slot list (m_60): a CObList member (its ctor sets the vptr) plus one
// trailing dword; allocated 0x20 bytes. The three add/refresh methods reloc-mask.
struct CMultiSlotList {
    CObList m_list; // +0x00  (CObList, 0x1c bytes)
    i32 m_1c;       // +0x1c
    CMultiSlotList(i32 nBlockSize) : m_list(nBlockSize) { m_1c = 0; }
    void Method1546(i32 a);                      // 0x37910
    void Method2a45(i32 a, i32 b);               // 0x37ff0
    void Method3396(i32 a, i32 b, i32 c, i32 d); // 0x38150
};

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
    m_60 = (i32) new CMultiSlotList(0xa);
    CMultiReg* reg = (CMultiReg*)g_64bd5c;
    i32 count = 5;
    CMultiPlayerInfo* pi = reg->m_524->m_70;
    if (reg->m_588) {
        count = 2;
    } else if (pi) {
        if (pi->Q1794b0()) {
            count = 1;
        }
        if (pi->Q1794e0()) {
            count = 2;
        }
        if (pi->Q179510()) {
            count = 3;
        }
        if (pi->Q179540()) {
            count = 4;
        }
    }
    ((CMultiSlotList*)m_60)->Method1546(count);
    i32 v = GetSafe1c();
    ((CMultiSlotList*)m_60)->Method2a45(v, 0x527);
    ((CMultiSlotList*)m_60)->Method3396(v, 0x527, 0, 0);
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
    CMultiReg* reg = (CMultiReg*)g_64bd5c;
    i32 enable;
    if (reg->m_528) {
        i32 idx = GetSlotIndex();
        enable = (((CMultiSlot*)m_5c)[idx].m_16c == 0);
    } else {
        enable = 0;
    }
    w->EnableWindow(enable);
    i32 v = GetSafe1c();
    CMultiReg* reg2 = (CMultiReg*)g_64bd5c;
    if (reg2->m_600) {
        ((CMultiSlotList*)m_60)->Method3396(v, 0x527, 0, 0);
    } else {
        ((CMultiSlotList*)m_60)->Method3396(v, 0x527, reg2->m_5a4, reg2->m_5a8);
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

// ApplyOption0..3 (0x15de0/15e60/15ee0/15f60): set the active option N, refresh
// the dialog, then enable IDOK (GetDlgItem(1)) when any of slots 1..3 is occupied
// (the short-circuit `||` reuses the failed-probe's zero in eax on the false path).
// The Sub015fe0/Sub0173e0/Query015d00 calls reloc-mask (own CBattlezDlg methods
// homed as RVA stubs in src/Stub/ApiCallers.cpp).
RVA(0x00015de0, 0x5f)
void CBattlezDlg::ApplyOption0() {
    Sub015fe0(0);
    Sub0173e0();
    if (Query015d00(1) || Query015d00(2) || Query015d00(3)) {
        GetDlgItem(1)->EnableWindow(1);
    } else {
        GetDlgItem(1)->EnableWindow(0);
    }
}

RVA(0x00015e60, 0x5f)
void CBattlezDlg::ApplyOption1() {
    Sub015fe0(1);
    Sub0173e0();
    if (Query015d00(1) || Query015d00(2) || Query015d00(3)) {
        GetDlgItem(1)->EnableWindow(1);
    } else {
        GetDlgItem(1)->EnableWindow(0);
    }
}

RVA(0x00015ee0, 0x5f)
void CBattlezDlg::ApplyOption2() {
    Sub015fe0(2);
    Sub0173e0();
    if (Query015d00(1) || Query015d00(2) || Query015d00(3)) {
        GetDlgItem(1)->EnableWindow(1);
    } else {
        GetDlgItem(1)->EnableWindow(0);
    }
}

RVA(0x00015f60, 0x5f)
void CBattlezDlg::ApplyOption3() {
    Sub015fe0(3);
    Sub0173e0();
    if (Query015d00(1) || Query015d00(2) || Query015d00(3)) {
        GetDlgItem(1)->EnableWindow(1);
    } else {
        GetDlgItem(1)->EnableWindow(0);
    }
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

// ~CMultiStartDlg @0x0b8960 - destroy the CObList member m_74 then the CString
// member m_70, then chain the NAFXCW ~CDialog base dtor (all reloc-masked). /GX
// frame unwinds the half-torn object across the member dtors.
// @early-stop
// vptr-restamp-presence wall (docs/patterns/eh-dtor-vptr-restamp-presence.md): same
// as ~CBattlezDlg - our polymorphic model emits one extra most-derived vptr re-stamp
// at entry that retail elided; the member/base teardown chain is otherwise byte-exact.
RVA(0x000b8960, 0x59)
CMultiStartDlg::~CMultiStartDlg() {}

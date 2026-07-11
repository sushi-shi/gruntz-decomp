// MultiStartDlg.cpp - the CMultiStartDlg (multiplayer-start CDialog, resource
// 0xc5) TU, interval 0xc16b0-0xc296b (+ the 0xc3e30 stray). ONE original TU per
// docs/exe-map/interval-dossiers.md #4a: our multistartdlg + multistartdlgworld
// units were slices of this single file - the "world" unit's fns are
// CMultiStartDlg's own methods (SetupWorldCombo) + its free helpers
// (BuildNamedGruntTable, _WndProc_c1a10), and the two units' init frags form one
// fragment neighborhood before the code. The adjacent WOVEN roster/color/net
// interval (0x0c2980+) is a SEPARATE dialog obj - do not merge across 0x0c296b.
//
// Contents: the ctor (the vtable emission anchor), the world-combo setup, the
// color-item updater (dossier seam 0xc1aa0), the slot-list build/update model,
// the per-index control accessors, and the GetCtrlE combo helpers.
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
#include <Bute/SymTab.h>
#include <Gruntz/NetDlgHost.h> // CMultiStartDlg::m_host (its +0x34 m_registry is a CSymParser)
#include <Bute/SymParser.h>    // CSymParser::ResolvePath (0x13c030), the world name registry
#include <string.h>            // inline strcmp (empty-text WM_SETTEXT gate / name resync)
#include <rva.h>
#include <Globals.h>

// The global CGameRegistry the ctor snapshots: it copies g_gameReg->m_curState into
// the file-scope multiplayer game-state sink g_64bd5c (both reloc-masked DIR32).
DATA(0x0024556c)
extern CGameRegistry* g_gameReg; // the CGameRegistry pointer (reloc-masked DATA symbol)
// 0x64bd5c holds the multiplayer game-state singleton (a CMulti, xref-proven); the
// ctor snapshots it from g_gameReg->m_curState (+0x2c). (DATA also bound in ReconBatch2.)
DATA(0x0024bd5c)
extern CMulti* g_64bd5c;
// The shared empty-string literal (0x6293f4; homed in NetMgrReportError.cpp).
extern "C" char g_emptyString[];

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

// The subclass window-proc installed on the combo's edit child (0x4c1a10). Only
// its address is taken (push offset -> DIR32 reloc-masks).
extern "C" i32 CALLBACK WndProc_c1a10(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// The GAME_MULTI registry path -> a name registry. m_host is the canonical CNetDlgHost;
// its +0x34 m_registry is the real Bute CSymParser (ResolvePath 0x13c030 - proven), which
// returns a CSymTab symbol table. The table is iterated FirstSym/NextSym2 (first entry)
// then NextSym3 (advance) - CSymTab methods; kept as this TU's iteration view MpSymTable
// (each returned payload's +0x00 is the entry name). Folding MpSymTable onto CSymTab is
// proven but DEFERRED (its payload/item identity - CSymRec vs child scope - is unsettled).
struct MpSymItem {
    char* m_name; // +0x00  entry name (LPCSTR)
};

// The m4 color-dialog facet (dossier seam 0xc1aa0, from the former multicolordlg
// unit). @identity-TODO: the dialog object is a CDialog-derived class with its
// own fields from +0x5c (the same span CMultiStartDlg uses); whether MultiColorDlg
// IS CMultiStartDlg (the 0x4ff combo + the 0x238-stride slot table both match) or
// a sibling dialog is unsettled - kept the m4 view this package (no class renames
// per the wave1-D brief).
namespace m4 {
    // Game Win32 pointer table (0x6c44xx) -> reloc-masked indirect calls.
    extern HWND(WINAPI* g_pGetWindow)(HWND, UINT);                       // 0x006c44d8
    extern LRESULT(WINAPI* g_pSendMessageA)(HWND, UINT, WPARAM, LPARAM); // 0x006c44a4

    // The multiplayer lobby game-state singleton at 0x64bd5c is a CMulti (xref-
    // proven): m_isHost (+0x528) gates the active branch, m_5b0 (+0x5b0) is the
    // current selection, Name42ff/Name31d4 return the current-slot / default name
    // (CString by value).
    struct MultiColorDlg : public CDialog {
        char* m_5c; // +0x5c color-slot table base
        char m_pad60[0x6c - 0x60];
        i32 m_6c;            // +0x6c cached selection
        i32 SlotIndex2d4c(); // 0x00002d4c
        i32 UpdateColorItems();
    };
} // namespace m4

// ---------------------------------------------------------------------------
// BuildNamedGruntTable (0xc16b0) - seeds the 4 default named-grunt CString globals
// (the contiguous array at 0x64bdb0, read back as the multiplayer channel labels)
// with their default names via CString::operator=(LPCSTR) (FUN_001b9d4c, reloc-
// masked __thiscall). Its caller is the (unmodeled) CMultiStartDlg init routine at
// 0xc1690, one function before the ctor (0xc1750).
// ---------------------------------------------------------------------------
struct EngStrAssign {
    char* m_pszData;
    void operator=(const char* s); // CString::operator=, FUN_001b9d4c
};
// 4 contiguous CString globals at 0x64bdb0 (defined in the engine's data).
DATA(0x0064bdb0)
extern EngStrAssign g_gruntNames[4];

RVA(0x000c16b0, 0x3d)
void BuildNamedGruntTable() {
    g_gruntNames[0] = "Beefy";
    g_gruntNames[1] = "Zed";
    g_gruntNames[2] = "Serra";
    g_gruntNames[3] = "Jebediah";
}

// ---------------------------------------------------------------------------
RVA(0x000c1750, 0x88)
CMultiStartDlg::CMultiStartDlg(i32 a0, CWnd* pParent) : CDialog(0xc5, pParent), m_74(0xa) {
    m_host = a0;
    m_6c = 0;
    m_slotList = 0;
    g_64bd5c = (CMulti*)g_gameReg->m_curState;
}

// ---------------------------------------------------------------------------
// SetupWorldCombo (0xc1840, /GX): fill the 0x4ff "world" combo from the GAME_MULTI
// registry path (each entry name uppercased through a scratch CString), make the
// combo's edit child read-only, select the first item, and subclass the edit
// child's window-proc (saving the old proc in g_64bdc0).
RVA(0x000c1840, 0x16e)
i32 CMultiStartDlg::SetupWorldCombo() {
    CWnd* combo = GetDlgItem(0x4ff);
    if (combo == 0) {
        return 0;
    }
    CSymTab* st = (CSymTab*)((CNetDlgHost*)m_host)->m_registry->ResolvePath("GAME_MULTI");
    if (st == 0) {
        return 0;
    }
    MpSymItem* item = (MpSymItem*)st->NextSym2(st->FirstSym());
    while (item != 0) {
        CString name(item->m_name);
        name.MakeUpper();
        SendMessageA(combo->m_hWnd, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR)name);
        item = (MpSymItem*)st->NextSym3(item);
    }
    CWnd* combo2 = GetDlgItem(0x4ff);
    CWnd* child = CWnd::FromHandle(GetWindow(combo2->m_hWnd, GW_CHILD));
    if (child == 0) {
        return 0;
    }
    SendMessageA(child->m_hWnd, EM_SETREADONLY, 1, 0);
    SendMessageA(combo->m_hWnd, CB_SETCURSEL, 0, 0);
    HWND__* h = child->m_hWnd;
    g_64bdc0 = GetWindowLongA(h, GWL_WNDPROC);
    SetWindowLongA(h, GWL_WNDPROC, (i32)WndProc_c1a10);
    Sub_c3e30();
    return 1;
}

// WndProc_c1a10 (0xc1a10) - the subclass window-proc installed on the world combo's
// read-only edit child: swallow an empty WM_SETTEXT (keeps the shown selection text)
// and chain everything else to the saved original proc (g_64bdc0).
RVA(0x000c1a10, 0x70)
extern "C" i32 CALLBACK WndProc_c1a10(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_SETTEXT) {
        if (strcmp(g_emptyString, (const char*)lParam) == 0) {
            return 0;
        }
    }
    return CallWindowProcA((WNDPROC)g_64bdc0, hWnd, msg, wParam, lParam);
}

// ---------------------------------------------------------------------------
// m4::MultiColorDlg::UpdateColorItems (0xc1aa0, dossier seam -> this TU): refresh
// three dialog items (0x4ff combo, 0x42b, 0x4e9) plus the combo's child window.
// In an active session the items are enabled per an empty-slot table probe; out
// of session the combo's cursel is cleared and the child's text is (re)synced to
// the lobby's current name, then all three are disabled. thiscall member, /GX
// (destructible CString temporaries).
// @early-stop
// regalloc + EH-state wall. Complete correct reconstruction: the session-active
// branch (four GetItem/child fetches, the null-guard chain, the empty-slot table
// probe SlotIndex*71*8+0x16c and the enable/disable) and the out-of-session
// branch (CB_SETCURSEL -1, the m_5b0 gate, the two by-value CString name fetches,
// the inline strcmp resync, and the disable trio) all align by shape (llvm-objdump
// -dr). Residual is MSVC5 permuting the four item pointers across edi/ebp/ebx and
// the dead arg/temp stack slots between the two branches, shifting [esp+N] operands
// - plus the demangled-vs-mangled MFC/CString reloc names - not steerable.
RVA(0x000c1aa0, 0x2f8)
i32 m4::MultiColorDlg::UpdateColorItems() {
    if (g_64bd5c->m_isHost != 0) {
        CWnd* it4ff = GetDlgItem(0x4ff);
        CWnd* itChild = CWnd::FromHandle(g_pGetWindow(GetDlgItem(0x4ff)->m_hWnd, 5));
        CWnd* it42b = GetDlgItem(0x42b);
        CWnd* it4e9 = GetDlgItem(0x4e9);
        if (!itChild) {
            return 0;
        }
        if (!it4ff) {
            return 0;
        }
        if (!it42b) {
            return 0;
        }
        if (!it4e9) {
            return 0;
        }
        i32 idx = SlotIndex2d4c();
        i32 en = (*(i32*)(m_5c + idx * 568 + 0x16c) == 0);
        it4ff->EnableWindow(en);
        it42b->EnableWindow(en);
        it4e9->EnableWindow(0);
        return 1;
    }
    CWnd* it4ff = GetDlgItem(0x4ff);
    CWnd* itChild = CWnd::FromHandle(g_pGetWindow(GetDlgItem(0x4ff)->m_hWnd, 5));
    CWnd* it42b = GetDlgItem(0x42b);
    CWnd* it4e9 = GetDlgItem(0x4e9);
    if (!itChild) {
        return 0;
    }
    if (!it4ff) {
        return 0;
    }
    if (!it42b) {
        return 0;
    }
    if (!it4e9) {
        return 0;
    }
    g_pSendMessageA(it4ff->m_hWnd, 0x14e, (WPARAM)-1, 0);
    m_6c = g_64bd5c->m_5b0;
    if (g_64bd5c->m_5b0 != 0) {
        CString name = g_64bd5c->Name42ff();
        itChild->SetWindowTextA(name);
    } else {
        CString cur;
        itChild->GetWindowTextA(cur);
        if (strcmp(cur, g_64bd5c->Name31d4()) != 0) {
            itChild->SetWindowTextA(g_64bd5c->Name31d4());
        }
    }
    it4ff->EnableWindow(0);
    it42b->EnableWindow(0);
    it4e9->EnableWindow(0);
    return 1;
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
SIZE_UNKNOWN(EngStrAssign);
SIZE_UNKNOWN(MpSymItem);
SIZE_UNKNOWN(MpSymTable);

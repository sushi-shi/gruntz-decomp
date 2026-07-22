#include <Gruntz/GruntzMgr.h> // m_host's real type (the ex CNetDlgHost/CMultiSlot views)
#include <Gruntz/Dialogs.h>
#include <Gruntz/GameRegMfcPtr.h> // g_gameReg at its REAL type (CGruntzMgr)
#include <Net/NetMgr.h>          // the real CNetMgr (m_netGate selection latches)
#include <Gruntz/GruntzMgr.h>
#include <Net/InterfaceObject.h>
#include <Gruntz/GameRegistry.h> // the real CGameRegistry (g_gameReg; m_curState @+0x2c)
#include <Gruntz/Multi.h>        // the real CMulti (the 0x64bd5c multiplayer game-state singleton)
#include <Net/LatencyList.h>     // CLatencyList (m_slotList; Dispatch/FillCombo/SelectItem)
#include <Bute/SymTab.h>
#include <Bute/SymParser.h>       // CSymParser::ResolvePath (0x13c030), the world name registry
#include <Utils/RegistryHelper.h> // g_gameReg->m_settings (the config registry: GetValue*/SetValue*)
#include <string.h>               // inline strcmp (empty-text WM_SETTEXT gate / name resync)
#include <stdio.h>                // sprintf/fopen/fclose (DoDataExchange custom-level probe)
#include <rva.h>
#include <Gruntz/MultiStartDlg.h> // own exported globals (ex Globals.h)

enum {
    NUM_PLAYER_SLOTS = 4
};



#include <Gruntz/MpSymItem.h>

VTBL(CMultiStartDlg, 0x001ea8ec); // vtable_names -> code (RTTI game class)
DATA(0x0024bdb0)
CString g_gruntNames[4];
DATA(0x0024bdc0)
i32 g_savedMultiWndProc = 0; // 0x24bdc0

RVA(0x000c16b0, 0x3d)
void BuildNamedGruntTable() {
    g_gruntNames[0].CString::CString("Beefy");
    g_gruntNames[1].CString::CString("Zed");
    g_gruntNames[2].CString::CString("Serra");
    g_gruntNames[3].CString::CString("Jebediah");
}

RVA(0x000c1750, 0x88)
CMultiStartDlg::CMultiStartDlg(CGruntzMgr* a0, CWnd* pParent) : CDialog(0xc5, pParent), m_74(0xa) {
    m_host = a0;
    m_6c = 0;
    m_slotList = 0;
    g_multiState = static_cast<CMulti*>(g_gameReg->m_curState);
}

RVA(0x000c1840, 0x16e)
i32 CMultiStartDlg::SetupWorldCombo() {
    CWnd* combo = GetDlgItem(0x4ff);
    if (combo == 0) {
        return 0;
    }
    CSymTab* st = static_cast<CSymTab*>(m_host->m_symParser->ResolvePath("GAME_MULTI"));
    if (st == 0) {
        return 0;
    }
    MpSymItem* item = static_cast<MpSymItem*>(st->NextSym2(st->FirstSym()));
    while (item != 0) {
        CString name(item->m_name);
        name.MakeUpper();
        ::SendMessageA(combo->m_hWnd, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(static_cast<LPCTSTR>(name)));
        item = static_cast<MpSymItem*>(st->NextSym3(item));
    }
    CWnd* combo2 = GetDlgItem(0x4ff);
    CWnd* child = CWnd::FromHandle(::GetWindow(combo2->m_hWnd, GW_CHILD));
    if (child == 0) {
        return 0;
    }
    ::SendMessageA(child->m_hWnd, EM_SETREADONLY, 1, 0);
    ::SendMessageA(combo->m_hWnd, CB_SETCURSEL, 0, 0);
    HWND__* h = child->m_hWnd;
    g_savedMultiWndProc = GetWindowLongA(h, GWL_WNDPROC);
    SetWindowLongA(h, GWL_WNDPROC, reinterpret_cast<i32>(WndProc_c1a10));
    CommitWorldHost();
    return 1;
}

RVA(0x000c1a10, 0x70)
i32 CALLBACK WndProc_c1a10(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_SETTEXT) {
        if (strcmp(g_emptyString, reinterpret_cast<const char*>(lParam)) == 0) {
            return 0;
        }
    }
    return CallWindowProcA(reinterpret_cast<WNDPROC>(g_savedMultiWndProc), hWnd, msg, wParam, lParam);
}

// ---------------------------------------------------------------------------
// CMultiStartDlg::UpdateColorItems (0xc1aa0, dossier seam -> this TU): refresh
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
i32 CMultiStartDlg::UpdateColorItems() {
    if (g_multiState->m_isHost != 0) {
        CWnd* it4ff = GetDlgItem(0x4ff);
        CWnd* itChild = CWnd::FromHandle(::GetWindow(GetDlgItem(0x4ff)->m_hWnd, 5));
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
        i32 idx = GetSlotIndex();
        i32 en = (m_host->m_options[idx].m_readyFlag == 0);
        it4ff->EnableWindow(en);
        it42b->EnableWindow(en);
        it4e9->EnableWindow(0);
        return 1;
    }
    CWnd* it4ff = GetDlgItem(0x4ff);
    CWnd* itChild = CWnd::FromHandle(::GetWindow(GetDlgItem(0x4ff)->m_hWnd, 5));
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
    ::SendMessageA(it4ff->m_hWnd, 0x14e, static_cast<WPARAM>(-1), 0);
    m_6c = g_multiState->m_5b0;
    if (g_multiState->m_5b0 != 0) {
        CString name = g_multiState->Name42ff();
        itChild->SetWindowTextA(name);
    } else {
        CString cur;
        itChild->GetWindowTextA(cur);
        if (strcmp(cur, g_multiState->Name31d4()) != 0) {
            itChild->SetWindowTextA(g_multiState->Name31d4());
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
i32 CMultiStartDlg::BuildSlotList() {
    m_slotList = new CLatencyList(0xa);
    CMulti* reg = g_multiState;
    i32 count = 5;
    InterfaceObject* pi = reg->m_netGate->m_groupSel;
    if (reg->m_588) {
        count = 2;
    } else if (pi) {
        if (pi->IsInterface1()) {
            count = 1;
        }
        if (pi->IsInterface2()) {
            count = 2;
        }
        if (pi->IsInterface3()) {
            count = 3;
        }
        if (pi->IsInterface4()) {
            count = 4;
        }
    }
    m_slotList->Dispatch(count);
    i32 v = GetSafe1c();
    m_slotList->FillCombo(v, 0x527);
    m_slotList->SelectItem(v, 0x527, 0, 0);
    reg->m_600 = 1;
    return 1;
}

// CMultiStartDlg::UpdateSlot (0xc1fd0): enable the team control by current-slot
// occupancy, then push the dialog selection (with the registry color pair, unless
// already committed) into the slot list. Returns 1 (0 when the control is absent).
// @early-stop
// regalloc coin-flip wall (docs/patterns/zero-register-pinning.md): GetDlgItem gate,
// the 0x238-stride slot probe, the inlined GetSafe1c, and the committed/color
// Method3396 branch are byte-faithful; the inlined GetSafe1c result lands in ecx
// (retail keeps it in eax), cascading into the g_multiState register + the final push
// schedule. A pure allocator choice, no source lever. ~92%.
RVA(0x000c1fd0, 0x99)
i32 CMultiStartDlg::UpdateSlot() {
    CWnd* w = GetDlgItem(0x527);
    if (w == 0) {
        return 0;
    }
    CMulti* reg = g_multiState;
    i32 enable;
    if (reg->m_isHost) {
        i32 idx = GetSlotIndex();
        enable = (m_host->m_options[idx].m_readyFlag == 0);
    } else {
        enable = 0;
    }
    w->EnableWindow(enable);
    i32 v = GetSafe1c();
    CMulti* reg2 = g_multiState;
    if (reg2->m_600) {
        m_slotList->SelectItem(v, 0x527, 0, 0);
    } else {
        m_slotList->SelectItem(v, 0x527, reg2->m_5a4, reg2->m_drainReload);
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CMultiStartDlg::DoDataExchange (0xc20a0, slot 35, /GX): the multiplayer-start
// dialog's DDX - the retail vtable's slot-35 entry (was mislabeled the non-virtual
// "InitPlayerSlots" backlog stub; xref-proven via ??_7CMultiStartDlg@@6B@+0x8c).
//   LOAD  (m_bSaveAndValidate == 0): populate every control from the CMulti lobby
//     game-state - the 0x512 game-name edit, the four kind combos (None/Computer
//     easy|normal|difficult/Human), the four 9-char name edits, the 100-char chat
//     input, and (host-side, when a saved custom map exists on disk) the world
//     combo's read-only edit child + the CMulti custom-world name pair - then cache
//     the log-edit HWND and re-drive the connect state.
//   SAVE  (m_bSaveAndValidate != 0): read the world name (host: persist Last/Custom
//     MultiMap) and each slot's name-edit text back into the m_host slot array.
// SendMessageA is hoisted through the engine's cached USER32 fn-ptr global (pSend);
// GetWindow is the uncached g_pGetWindow global. /GX EH frame for the CString temps.
// @early-stop
// ~95.6%: complete + correct (LOAD/SAVE both align by shape, arg-eval order matched
// by pre-loading each control's HWND before the SendMessage constant args). Residual is
// the zero-register-pinning wall (docs/patterns/zero-register-pinning.md): retail pins
// the persistent 0 constant to ebx, our cl to esi - cascading through every null-compare
// / ehstate store - plus the eax/ecx/edx colouring of the per-control HWND reads and the
// save loop's induction-variable choice (typed slots[i] base-hoist vs retail's byte off).
// Not source-steerable; g_pGetWindow/g_pSendMessageA reloc-mask UNBOUND (the whole
// 0x2c44xx USER32 fn-ptr table is unbound infra, same as g_pPostMessageA).
RVA(0x000c20a0, 0x45a)
void CMultiStartDlg::DoDataExchange(CDataExchange* pDX) {
    Utils::RegistryHelper* reg = static_cast<Utils::RegistryHelper*>(g_gameReg->m_settings);
    if (pDX->m_bSaveAndValidate == 0) {
        GetDlgItem(0x512)->SetWindowTextA(g_multiState->GetString59c());
        NetLobby::g_curDlg = reinterpret_cast<HWND__*>(GetSafe1c());
        if (!SetupWorldCombo()) {
            return;
        }
        if (!BuildSlotList()) {
            return;
        }
        WapSendMessageA pSend = g_pSendMessageA;
        i32 i;
        for (i = 0; i < NUM_PLAYER_SLOTS; i++) {
            HWND kc;
            kc = KindCombo1929(i)->m_hWnd;
            pSend(kc, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>("None"));
            kc = KindCombo1929(i)->m_hWnd;
            pSend(kc, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>("Computer (easy)"));
            kc = KindCombo1929(i)->m_hWnd;
            pSend(kc, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>("Computer (normal)"));
            kc = KindCombo1929(i)->m_hWnd;
            pSend(kc, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>("Computer (difficult)"));
            kc = KindCombo1929(i)->m_hWnd;
            pSend(kc, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>("Human"));
        }
        for (i = 0; i < NUM_PLAYER_SLOTS; i++) {
            CWnd* e = NameEdit298c(i);
            if (e != 0) {
                pSend(e->m_hWnd, EM_LIMITTEXT, 9, 0);
            }
        }
        HWND chatEdit = GetDlgItem(0x42d)->m_hWnd;
        pSend(chatEdit, EM_LIMITTEXT, 100, 0);
        i32 customFlag = reg->GetValueDword("CustomMultiMap", 2);
        if (g_multiState->m_isHost != 0 && customFlag != 2) {
            char mapName[0x100];
            u32 size = 0x100;
            reg->GetValueString("LastMultiMap", mapName, &size, g_emptyString);
            m_6c = customFlag;
            if (customFlag != 0) {
                char path[0x100];
                sprintf(path, "custom\\%s", mapName);
                FILE* f = fopen(path, "rb");
                if (f != 0) {
                    HWND worldCombo = GetDlgItem(0x4ff)->m_hWnd;
                    CWnd* child = CWnd::FromHandle(g_pGetWindow(worldCombo, GW_CHILD));
                    if (child == 0) {
                        return;
                    }
                    child->SetWindowTextA(mapName);
                    g_multiState->m_5b0 = 1;
                    g_multiState->m_5b8 = mapName;
                    g_multiState->m_5b4 = g_emptyString;
                    fclose(f);
                }
            } else {
                CWnd* child = CWnd::FromHandle(g_pGetWindow(GetDlgItem(0x4ff)->m_hWnd, GW_CHILD));
                if (child == 0) {
                    return;
                }
                child->SetWindowTextA(mapName);
                g_multiState->m_5b0 = 0;
                g_multiState->m_5b8 = g_emptyString;
                g_multiState->m_5b4 = mapName;
            }
        }
        {
            CWnd* w = GetDlgItem(0x511);
            g_sharedFlag = (w == 0) ? 0 : reinterpret_cast<i32>(w->m_hWnd);
        }
        g_multiState->m_netGate->m_sessionSel = 0;
        g_multiState->PollSession();
        if (!UpdateColorItems()) {
            return;
        }
        if (!Sync38d2()) {
            return;
        }
        if (!Sync16db(1)) {
            return;
        }
    } else {
        HWND worldCombo = GetDlgItem(0x4ff)->m_hWnd;
        CWnd* child = CWnd::FromHandle(g_pGetWindow(worldCombo, GW_CHILD));
        if (child == 0) {
            return;
        }
        child->GetWindowTextA(m_70);
        if (g_multiState->m_isHost != 0) {
            reg->SetValueString("LastMultiMap", m_70);
            reg->SetValueDword("CustomMultiMap", m_6c);
        }
        GruntzPlayer* slots = m_host->m_options;
        for (i32 i = 0; i < NUM_PLAYER_SLOTS; i++) {
            CWnd* e = NameEdit298c(i);
            if (e != 0) {
                CString temp;
                e->GetWindowTextA(temp);
                slots[i].m_name = temp;
            }
        }
        NetLobby::g_curDlg = 0;
    }
    FlashCtrlD();
}

// ---------------------------------------------------------------------------
// GetCtrlE (0xc2640, free __stdcall): the fifth per-index combo getter over control
// IDs 0x500/0x50e/0x50f/0x510. Reclaimed from the globals unit's bogus g_typeDesc2
// char-array DATA mislabel - 0xc2640 is this function, not a data global. Unlike the
// member CMultiStartDlg::GetCtrlA..D, this is a FREE __stdcall function whose body
// threads the caller's dialog `this` (ecx) straight into GetDlgItem (see the helper
// decl above). Callers SetComboSelE/GetComboSelE now bind to it.
// @early-stop
// ~69%: the switch shape, hoisted `xor eax,eax`, per-case `push id; call; ret 4` and
// the 4-entry jump table all match retail; residual is (1) the index register - retail
// keeps it in edx because ecx is live holding the threaded `this` (the body IS a member
// threading ecx into GetDlgItem), but the free __stdcall reconstruction (forced by the
// ?GetCtrlE@@YG.. symbol the callers reference) leaves ecx free so MSVC picks ecx for
// the index; no C++ spelling reserves ecx without a `this` param. (2) jump-table-data
// scoring artifact - docs/patterns/jumptable-data-overlap.md.
RVA(0x000c2640, 0x46)
CWnd* __stdcall GetCtrlE(i32 index) {
    CWnd* result = 0;
    switch (index) {
        case 0:
            result = GetDlgItemThreaded(0x500);
            break;
        case 1:
            result = GetDlgItemThreaded(0x50e);
            break;
        case 2:
            result = GetDlgItemThreaded(0x50f);
            break;
        case 3:
            result = GetDlgItemThreaded(0x510);
            break;
    }
    return result;
}

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

RVA(0x000c28c0, 0x27)
void __stdcall SetComboSelE(i32 index, i32 sel) {
    CWnd* c = GetCtrlE(index);
    if (c != 0) {
        ::SendMessageA(c->m_hWnd, 0x14e, sel, 0);
    }
}

RVA(0x000c2900, 0x2a)
i32 __stdcall GetComboSelE(i32 index) {
    CWnd* c = GetCtrlE(index);
    if (c == 0) {
        return -1;
    }
    return ::SendMessageA(c->m_hWnd, 0x147, 0, 0);
}

RVA(0x000c2940, 0x2b)
i32 CMultiStartDlg::GetComboSelC(i32 id) {
    CWnd* c = GetCtrlC(id);
    if (c == 0) {
        return -1;
    }
    return ::SendMessageA(c->m_hWnd, 0x147, 0, 0) + 1;
}


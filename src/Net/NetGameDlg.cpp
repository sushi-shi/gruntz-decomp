// NetGameDlg.cpp - the multiplayer net-game configuration methods of the ONE
// multiplayer-start dialog. PROVEN same class as CMultiStartDlg (matcher-5): the
// per-channel reconcile (0xc2ab0) dispatches this class's own per-slot accessors
// 0x1929/0x298c/GetCtrlD(0xc2840 via thunk 0x30da)/0x1753/0x1159 on `this` and reads
// [this+0x5c] (== CMultiStartDlg::m_host, the 0x238-stride slot array), and the
// control re-enable (0xc4120) / custom-level verify (0xc4c00) are ordinary CDialog
// subclass methods on `this`. The former placeholder `struct CNetGameDlg` is folded
// into <Gruntz/Dialogs.h>'s CMultiStartDlg; these bodies stay in this unit so the
// delinker packing is undisturbed. Offsets + code bytes are load-bearing; field names
// are placeholders.
#include <Gruntz/Dialogs.h> // CMultiStartDlg + its CWnd/CString/CDialog models
#include <Gruntz/Multi.h>   // the real CMulti (the 0x64bd5c multiplayer game-state singleton)
#include <rva.h>

// The per-channel player-slot record (lives at +0x150 inside a 0x238-byte channel
// entry, reached off CMultiStartDlg::m_host). Same memory as the roster's RosterSlot
// (MultiStartDlgRoster.cpp); the two interpretations are kept local to their units
// per the established "roster interpretation stays local" decision.
struct ChannelSlot {
    i32 m_playerId;  // +0x00 player id
    CString m_label; // +0x04 label
    i32 m_slotIndex; // +0x08 slot index
    char m_pad0c[0x10 - 0x0c];
    i32 m_selectionIndex; // +0x10
    i32 m_14;             // +0x14
    char m_pad18[0x1c - 0x18];
    i32 m_ready;  // +0x1c ready flag
    i32 m_active; // +0x20 active flag
};
SIZE_UNKNOWN(ChannelSlot);

// The multiplayer game-state singleton at 0x64bd5c is a CMulti (xref-proven). The
// DATA symbol is owned by ReconBatch2.cpp; this extern reloc-masks against it.
extern CMulti* g_64bd5c; // 0x64bd5c

// The game-settings singleton (CGruntzMgr) used to resolve the level + show modals -
// the settings facet of the 0x64556c dual-view (a REQUIRED CGameRegistry/CGruntzMgr
// split; not unified here). Only the two touched methods are modeled.
struct CGameSettings {
    void* BuildRezPath(i32 a, void* name, i32 c, i32 d, CString cap); // 0x93d40
    void ShowModal(const char* msg);                                  // 0x8ef10
};
extern "C" CGameSettings* g_mgrSettings; // _g_mgrSettings (0x64556c)
SIZE_UNKNOWN(CGameSettings);

// SendMessageA reached through the game's own import pointer.
extern "C" long(WINAPI*
                    g_pSendMessageA)(void* hwnd, unsigned msg, unsigned wp, long lp); // 0x6c44a4
DATA(0x0024bdb0)
extern CString g_64bdb0[]; // 0x64bdb0 per-channel label table

void ChannelSlots_Set(i32 slot, i32 v); // 0xdb2b0
i32 ChannelSlots_FindFree();            // 0xdb280
CString GetConfigNameA();               // 0xb6090
CString GetConfigNameB();               // 0xb60d0

// @early-stop
// /GX EH-frame representation wall (~84%): the code stream is byte-faithful (all
// GetDlgItem/EnableWindow calls + the g_optCfg load pair), but the delinker emits
// the scope-table push addend (0x8 vs 0x0) and the fs:0 handler-registration relocs
// differently than the MSVC base obj, so the EH prologue/epilogue can't pair.
// re-enable the four player-config controls, then (when no custom level
// name is set) build and discard an empty caption string.
RVA(0x000c4120, 0xc2)
i32 CMultiStartDlg::EnableControls() {
    GetDlgItem(2)->EnableWindow(1);
    GetDlgItem(0x4c6)->EnableWindow(1);
    GetDlgItem(0x42d)->EnableWindow(1);
    GetDlgItem(0x511)->EnableWindow(1);
    CString s1;
    if (g_64bd5c->m_5b0 == 0) {
        CString s2;
    }
    return 1;
}

// @early-stop
// regalloc / const-materialize wall (~69%): the control flow is byte-faithful, but
// retail pins `this` in esi and `ch` in edi (this cl swaps them), and materializes
// 0 into the dead `ch` register to drive the `cmp` against the slot flags where
// this cl re-tests; the swap + 0-in-reg-vs-`test` cascade the register renames.
// reconcile one channel's player slot after a join/leave: poll the owner
// window, drop or assign the channel's logical slot, and toggle the two controls.
// (0xc2ab0; also reached as the roster's per-row reconcile via ILT thunk 0x3ffd.)
RVA(0x000c2ab0, 0x161)
void CMultiStartDlg::SyncChannelSlot(i32 ch) {
    CWnd* owner = KindCombo1929(ch); // 0x1929  the list whose selection drives the slot
    CWnd* c1 = NameEdit298c(ch);     // 0x298c
    CWnd* c2 = GetCtrlD(ch);         // 0x30da -> 0xc2840
    ColourBtn1753(ch);               // 0x1753 (side effect only)
    ReadyCheck1159(ch);              // 0x1159 (side effect only)
    ChannelSlot* s = (ChannelSlot*)((char*)m_host + ch * 0x238 + 0x150);
    long(WINAPI * pSend)(void*, unsigned, unsigned, long) = g_pSendMessageA;
    if (pSend(owner->m_hWnd, 0x147, 0, 0) == 0) {
        if (s->m_14 != 0) {
            if (s->m_active != 0) {
                g_64bd5c->DropPlayer(s->m_playerId);
            }
        } else if (s->m_active != 0) {
            ChannelSlots_Set(s->m_slotIndex, 1);
        }
        s->m_active = 0;
        s->m_ready = 0;
        c1->EnableWindow(0);
        c2->EnableWindow(0);
    } else {
        if (pSend(owner->m_hWnd, 0x147, 0, 0) != 4) {
            if (s->m_14 != 0) {
                if (s->m_active != 0) {
                    g_64bd5c->DropPlayer(s->m_playerId);
                }
                i32 free = ChannelSlots_FindFree();
                s->m_slotIndex = free;
                ChannelSlots_Set(free, 0);
            } else if (s->m_active == 0) {
                i32 free = ChannelSlots_FindFree();
                s->m_slotIndex = free;
                ChannelSlots_Set(free, 0);
            }
            s->m_ready = 1;
            s->m_14 = 0;
            s->m_selectionIndex = (i32)pSend(owner->m_hWnd, 0x147, 0, 0) - 1;
            s->m_active = 1;
            s->m_label = g_64bdb0[ch];
        }
        c1->EnableWindow(1);
        c2->EnableWindow(1);
    }
}

// @early-stop
// /GX CString cleanup-state-machine wall (~52%): the branch logic + the merged
// BuildRezPath / by-value caption copy are reconstructed, but two retail early
// guards test the relocatable addresses of CTileExclusiveTriggerSwitchLogic /
// ReleaseResources (a pointer-to-member null check this cl can't re-spell), which
// shifts the layout, and the a/b CString destruct-state numbering is EH residue.
// (CLEANUP p2: folding the CNetSession lens into the real CMulti re-mangled the
// Poll/SendStatFlag callees CMulti::-side; the code loads are byte-identical -
// (void*)m_5b0 is the same DWORD mov as the old void* field - but the re-mangled
// reloc symbol set nudged the EH-scope fuzzy score 54.6% -> 51.9%. Accepted.)
// before the match starts, confirm every player has the same custom
// level; otherwise re-enable the dialog and pop the appropriate error modal.
RVA(0x000c4c00, 0x190)
void CMultiStartDlg::VerifyCustomLevel() {
    CMulti* mgr = g_64bd5c;
    if (mgr->m_isHost == 0) {
        return;
    }
    mgr->SendStatFlag(0x3fc, 1);
    void* token;
    if (g_64bd5c->m_5b0 != 0) {
        CString b = GetConfigNameB();
        token = g_mgrSettings->BuildRezPath(0, (void*)g_64bd5c->m_5b0, 0, 0, b);
    } else {
        CString a = GetConfigNameA();
        token = g_mgrSettings->BuildRezPath(0, (void*)g_64bd5c->m_5b0, 0, 0, a);
    }
    g_64bd5c->m_53c = 0;
    if (g_64bd5c->Poll((i32)token) == 0) {
        g_64bd5c->m_530 = 0;
        EnableWindow(0);
        g_mgrSettings->ShowModal("Unable to verify custom level with other players");
        EnableWindow(1);
    } else if (g_64bd5c->m_53c == 0) {
        g_64bd5c->m_530 = 1;
        OnOK();
    } else {
        g_64bd5c->m_530 = 0;
        EnableWindow(0);
        g_mgrSettings->ShowModal("Not all players have the (same) custom level.");
        EnableWindow(1);
    }
}

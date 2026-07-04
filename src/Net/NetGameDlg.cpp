// NetGameDlg.cpp - the multiplayer net-game configuration dialog (the class whose
// singleton is the int handle g_64bd5c).  These methods drive per-channel player
// slot bookkeeping (0xc2ab0), re-enable the dialog controls (0xc4120) and verify
// that every player shares the same custom level before the match starts (0xc4c00).
// Offsets + code bytes are load-bearing; field/class names are placeholders.
#include <Mfc.h>
#include <Gruntz/Multi.h> // the real CMulti (the 0x64bd5c multiplayer game-state singleton)
#include <Ints.h>
#include <rva.h>

// A child control returned by CWnd::GetDlgItem; only EnableWindow is reached here.
struct CCtrl {
    void EnableWindow(i32 enable); // 0x1be6a7
};

// The per-channel slot record (lives at +0x150 inside a 0x238-byte channel entry).
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

// A full channel entry (stride 0x238); the player slot is embedded at +0x150.
struct Channel {
    char m_pad000[0x150];
    ChannelSlot m_slot; // +0x150
    char m_pad_tail[0x238 - (0x150 + sizeof(ChannelSlot))];
};

// The dialog object.  m_channels is the per-channel array.
struct CNetGameDlg {
    char m_pad000[0x5c];
    Channel* m_channels; // +0x5c channel array

    // declared-only siblings (reloc-masked)
    CCtrl* GetDlgItem(i32 id);     // 0x1be27d (CWnd::GetDlgItem)
    void EnableWindow(i32 enable); // 0x1be6a7 (CWnd::EnableWindow on this)
    void OnOK();                   // 0x1bacc3 (CDialog::OnOK)
    CCtrl* M_c2640(i32 ch);        // 0xc2640 (returns the channel HWND owner)
    void M_c26c0(i32 ch);          // 0xc26c0
    CCtrl* M_c2740(i32 ch);        // 0xc2740
    void M_c27c0(i32 ch);          // 0xc27c0
    CCtrl* M_c2840(i32 ch);        // 0xc2840

    // my targets
    i32 EnableControls();     // 0xc4120
    void UpdateSlot(i32 ch);  // 0xc2ab0
    void VerifyCustomLevel(); // 0xc4c00
};

// The HWND owner returned by M_c2640.
struct CChanWnd {
    char m_pad00[0x1c];
    void* m_hwnd; // +0x1c HWND
};

// The multiplayer game-state singleton at 0x64bd5c is a CMulti (xref-proven). The
// former per-TU CNetSession lens (a same-memory alias of this pointer) is gone: its
// fields are genuine CMulti members (m_isHost/m_530/m_53c/m_5b0) and its methods are
// genuine CMulti methods (DropPlayer/Poll/SendStatFlag) - see <Gruntz/Multi.h>. The
// DATA symbol is owned by ReconBatch2.cpp; this extern reloc-masks against it.
extern CMulti* g_64bd5c; // 0x64bd5c

// The game-settings singleton (CGruntzMgr) used to resolve the level + show modals.
struct CGameSettings {
    void* BuildRezPath(i32 a, void* name, i32 c, i32 d, CString cap); // 0x93d40
    void ShowModal(const char* msg);                                  // 0x8ef10
};
extern "C" CGameSettings* g_mgrSettings; // _g_mgrSettings (0x64556c)

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
i32 CNetGameDlg::EnableControls() {
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
RVA(0x000c2ab0, 0x161)
void CNetGameDlg::UpdateSlot(i32 ch) {
    CChanWnd* owner = (CChanWnd*)M_c2640(ch);
    CCtrl* c1 = M_c2740(ch);
    CCtrl* c2 = M_c2840(ch);
    M_c27c0(ch);
    M_c26c0(ch);
    ChannelSlot* s = &m_channels[ch].m_slot;
    long(WINAPI * pSend)(void*, unsigned, unsigned, long) = g_pSendMessageA;
    if (pSend(owner->m_hwnd, 0x147, 0, 0) == 0) {
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
        if (pSend(owner->m_hwnd, 0x147, 0, 0) != 4) {
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
            s->m_selectionIndex = (i32)pSend(owner->m_hwnd, 0x147, 0, 0) - 1;
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
void CNetGameDlg::VerifyCustomLevel() {
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

SIZE_UNKNOWN(CChanWnd);
SIZE_UNKNOWN(CCtrl);
SIZE_UNKNOWN(CGameSettings);
SIZE_UNKNOWN(CNetGameDlg);
SIZE_UNKNOWN(Channel);
SIZE_UNKNOWN(ChannelSlot);

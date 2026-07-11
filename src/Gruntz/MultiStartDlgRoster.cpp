// MultiStartDlgRoster.cpp - the multiplayer roster/color/net dialog TU: the WOVEN
// original obj at retail .text [0x0c2980 .. 0x0c5f15] (TU_MIGRATION interval
// 0x0c2980, weave 0.43; own 8-frag init run @0xc5360 per interval-dossiers #4a -
// a SEPARATE obj from MultiStartDlg.cpp's 0xc16b0 interval). wave2-F merge of the
// five former units multistartdlgroster + netmgrmisc(interval fns) + netgamedlg +
// multistartdlgcolor + multistartdlgnet, + the Sub_c3e30 stray from
// MultiStartDlg.cpp (TU_MIGRATION MOVE row). Everything here runs on the ONE
// multiplayer-start dialog (CMultiStartDlg) or its roster records; definitions in
// strict ascending retail-RVA order.
//
// Homed in its own unit (not Dialogs.cpp) so it can't perturb that TU's parked dtors;
// it reuses the shared Dialogs.h dialog models, the canonical CGameRegistry spine
// (GameRegistry.h) and the canonical CMulti game-state (Multi.h). Field names are
// placeholders (m_<hexoffset>); only offsets + code bytes are load-bearing.
#include <Gruntz/Dialogs.h>
#include <Gruntz/Multi.h>         // the real CMulti (the 0x64bd5c multiplayer game-state singleton)
#include <Gruntz/NetDlgHost.h>    // CNetDlgHost (m_host +0x5c facet; FindOptionsSlot @0x92e80)
#include <Gruntz/GameRegistry.h>  // the canonical g_gameReg spine (CGameRegistry, VA 0x64556c)
#include <Net/NetSessHost.h>      // CNetSessHost::SelectColor (0xc4b60), the +0x5c facet
#include <Net/NetMgr.h>           // CNetMgr::BroadcastChatLine (0xbb190), the chat-broadcast facet
#include <Wap32/ZDArrayDerived.h> // CZDArrayDerived (the g_netBe90 singleton's Construct)
#include <rva.h>
#include <string.h> // strcat (inline CRT, reloc-masked)

// A roster slot's FormatName_3e54 @0x3e54 IS GruntzPlayer::GetName; cast at the call.
#include <Gruntz/GruntzPlayer.h> // canonical GruntzPlayer (GetName)

// --- shared globals (canonical homes elsewhere; reloc-masked references here) ---
// The game-manager singleton (VA 0x64556c); the SelHost/roster handlers cache each
// player-slot's combo value into its m_focusSlots[] record. Reference-only (undefined
// external, reloc-masks) as in LobbyDialogs.cpp.
extern CGameRegistry* g_gameReg;
// The multiplayer game-state (a CMulti, xref-proven); the roster reads m_isHost /
// m_hostIndex off it. DATA reloc-masks against ReconBatch2's home.
DATA(0x0024bd5c)
extern CMulti* g_64bd5c;
// The shared empty-string literal (0x6293f4; homed in NetMgrReportError.cpp).
extern "C" char g_emptyString[];
// USER32 entry points reached through the game's own IAT-style function pointers
// (ff 15 [ptr]); UpdatePlayers drives its listboxes/redraws through them.
DATA(0x002c44a4)
extern LRESULT(WINAPI* g_pSendMessageA)(HWND, UINT, WPARAM, LPARAM);
DATA(0x002c44f0)
extern BOOL(WINAPI* g_pInvalidateRect)(HWND, const RECT*, BOOL);
DATA(0x002c4520)
extern HWND(WINAPI* g_pGetFocus)();
// More USER32 entry points via the game's own IAT-style pointers.
DATA(0x002c44d8)
extern HWND(WINAPI* g_pGetWindow)(HWND, UINT);
// "Using CmdDelay of %d and ResendDelay of %d\n" (the EchoLatencySettings format).
DATA(0x0024243c)
extern char s_UsingCmdDelay[];
// Singletons the forwarders dispatch onto.
DATA(0x0024be90)
extern CZDArrayDerived g_netBe90; // VA 0x64be90

// The per-player roster record IS the canonical CFocusSlot (GameRegistry.h,
// CGameRegistry::m_focusSlots[] +0x150, stride 0x238) - the former local RosterSlot
// facet is dissolved: its roster fields (m_10/m_18/m_1c/m_228/m_16c + the mode-reused
// m_14/m_20 roles + FormatName_3e54) now live on the canonical (name-preserving
// union; no offset conflicted, so no conflation to split).

// Dialog-item resolvers (push id; __stdcall) that rely on the caller's live ecx=this:
// each is really a thiscall control accessor spelled __stdcall so the byte stream omits
// the `mov ecx` the caller already satisfied (same RVAs as the CMultiStartDlg thiscall
// accessors below; reloc-masks either way).
CWnd* __stdcall ResolveItem_1753(i32 slot); // 0x01753
CWnd* __stdcall ResolveItem_1159(i32 idx);  // 0x01159
CWnd* __stdcall ResolveItem_c27c0(i32 id);  // 0xc27c0
// Roster free helpers (__stdcall, reloc-masked).
void __stdcall Func1d70(i32 flag);            // 0x01d70
void __stdcall Refresh185c(CFocusSlot* slot); // 0x0185c

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

// The game-settings singleton (CGruntzMgr) used to resolve the level + show modals -
// the settings facet of the 0x64556c dual-view (a REQUIRED CGameRegistry/CGruntzMgr
// split; not unified here). Only the two touched methods are modeled.
struct CGameSettings {
    void* BuildRezPath(i32 a, void* name, i32 c, i32 d, CString cap); // 0x93d40
    void ShowModal(const char* msg);                                  // 0x8ef10
};
extern "C" CGameSettings* g_mgrSettings; // _g_mgrSettings (0x64556c)
SIZE_UNKNOWN(CGameSettings);
DATA(0x0024bdb0)
extern CString g_64bdb0[]; // 0x64bdb0 per-channel label table

void ChannelSlots_Set(i32 slot, i32 v); // 0xdb2b0
i32 ChannelSlots_FindFree();            // 0xdb280
CString GetConfigNameA();               // 0xb6090
CString GetConfigNameB();               // 0xb60d0

// __stdcall(hDlg, id, *lo, *hi): split control `id`'s selected listbox item data into
// two words; returns 1 when a valid item is selected. Generic listbox helper.
RVA(0x00038220, 0x73)
i32 __stdcall GetSelItemData(HWND hDlg, i32 id, i32* outLo, i32* outHi) {
    HWND list = GetDlgItem(hDlg, id);
    if (!list) {
        return 0;
    }
    i32 sel = SendMessageA(list, 0x147, 0, 0);
    if (sel == -1) {
        return 0;
    }
    i32 data = SendMessageA(list, 0x150, sel, 0);
    if (data == -1) {
        return 0;
    }
    *outLo = data & 0xffff;
    *outHi = (u32)data >> 0x10;
    return 1;
}

// __stdcall(id, wParam): if item `id` resolves, set its list selection to wParam-1
// (LB_SETCURSEL). Free helper preserving the caller's ecx=this (see resolvers above).
RVA(0x000c2980, 0x28)
void __stdcall SetListCurSel(i32 id, i32 wParam) {
    CWnd* it = ResolveItem_1753(id);
    if (it) {
        SendMessageA(it->m_hWnd, 0x14e, wParam - 1, 0);
    }
}

// ---------------------------------------------------------------------------
// One connect step: reconcile slot 1 (0xc2ab0) then the connect drive (0xc40b0).
// ---------------------------------------------------------------------------
RVA(0x000c2a20, 0x13)
void CMultiStartDlg::ConnectStep() {
    SyncChannelSlot(1);
    Drive();
}

// ---------------------------------------------------------------------------
// Channel 2 / 3 handlers: reconcile the channel's player slot (0xc2ab0) then re-drive
// the connect state (0xc40b0). Twins of ConnectStep (channel 1). Re-homed here from
// src/Stub/Cluster0c.cpp (CCluster0c::Run) and src/Stub/ReconBatch2.cpp (Host_c2a80::Run) -
// both PROVEN CMultiStartDlg: they self-call SyncChannelSlot + Drive on `this`.
// ---------------------------------------------------------------------------
RVA(0x000c2a50, 0x13)
void CMultiStartDlg::Method_c2a50() {
    SyncChannelSlot(2);
    Drive();
}

RVA(0x000c2a80, 0x13)
void CMultiStartDlg::Method_c2a80() {
    SyncChannelSlot(3);
    Drive();
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
    LRESULT(WINAPI * pSend)(HWND, UINT, WPARAM, LPARAM) = g_pSendMessageA;
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

// __thiscall(str): resolve control 0x511 (the chat/message log edit) and append `str`
// to it - CRLF-prefixed when the control already has text, then scroll the caret into
// view. Same shape as LobbyDialogs' AppendEditLine.
RVA(0x000c2ce0, 0xf3)
void CMultiStartDlg::AppendChatLine(char* str) {
    CWnd* item = GetDlgItem(0x511);
    HWND edit;
    if (!item) {
        edit = 0;
    } else {
        edit = item->m_hWnd;
    }
    if (!edit || !str || !str[0]) {
        return;
    }
    i32 len = GetWindowTextLengthA(edit);
    if (len == 0) {
        SendMessageA(edit, 0xb1, len, -1);
    } else {
        SendMessageA(edit, 0xb1, len, len);
    }
    char buf[0x80];
    buf[0] = 0;
    if (len > 0) {
        strcat(buf, "\r\n");
    }
    strcat(buf, str);
    SendMessageA(edit, 0xc2, 0, (LPARAM)buf);
    SendMessageA(edit, 0xb6, 0, 0x270f);
}

// ---------------------------------------------------------------------------
// Per-slot colour handlers (0xc3830/0xc3950/0xc3a70/0xc3b90). Slot N owns swatch
// control 0x501+2*N. The pick is allowed when the host set the slot's colour gate
// (m_164==0) or when it is unlocked (m_16c==0) and owned by us (m_168==m_hostIndex).
// All four are byte-exact code (~99.84%); residual is two reloc/regalloc artifacts:
// the /GX scope-table push addend (delinker names it Unwind@005dda10+8 vs our own
// $L scope table at +0) and a single eax-vs-ecx coin-flip on the InvalidateRect hwnd
// load (`mov ecx,[eax+0x1c]` retail vs `mov eax,...`). Neither is source-steerable.
// @early-stop
// reloc scope-table addend + InvalidateRect-hwnd eax/ecx regalloc coin-flip (~99.84%).
RVA(0x000c3830, 0xd1)
void CMultiStartDlg::OnColorSlot0() {
    CMulti* mp = g_64bd5c;
    if ((mp->m_isHost == 0 || ((CFocusSlot*)m_host)[0].m_164 != 0)
        && (((CFocusSlot*)m_host)[0].m_16c != 0
            || ((CFocusSlot*)m_host)[0].m_168 != mp->m_hostIndex)) {
        return;
    }
    CBattlezDlgColors dlg(m_host, 0, 1, 0);
    if (dlg.DoModal() == 1) {
        if (((CNetSessHost*)this)->SelectColor(0, dlg.m_pickedColor)) {
            Drive();
            HWND h = GetDlgItem(0x501)->m_hWnd;
            g_pInvalidateRect(h, 0, 1);
        }
    }
}

RVA(0x000c3950, 0xd1)
void CMultiStartDlg::OnColorSlot1() {
    CMulti* mp = g_64bd5c;
    if ((mp->m_isHost == 0 || ((CFocusSlot*)m_host)[1].m_164 != 0)
        && (((CFocusSlot*)m_host)[1].m_16c != 0
            || ((CFocusSlot*)m_host)[1].m_168 != mp->m_hostIndex)) {
        return;
    }
    CBattlezDlgColors dlg(m_host, 1, 1, 0);
    if (dlg.DoModal() == 1) {
        if (((CNetSessHost*)this)->SelectColor(1, dlg.m_pickedColor)) {
            Drive();
            HWND h = GetDlgItem(0x503)->m_hWnd;
            g_pInvalidateRect(h, 0, 1);
        }
    }
}

RVA(0x000c3a70, 0xd1)
void CMultiStartDlg::OnColorSlot2() {
    CMulti* mp = g_64bd5c;
    if ((mp->m_isHost == 0 || ((CFocusSlot*)m_host)[2].m_164 != 0)
        && (((CFocusSlot*)m_host)[2].m_16c != 0
            || ((CFocusSlot*)m_host)[2].m_168 != mp->m_hostIndex)) {
        return;
    }
    CBattlezDlgColors dlg(m_host, 2, 1, 0);
    if (dlg.DoModal() == 1) {
        if (((CNetSessHost*)this)->SelectColor(2, dlg.m_pickedColor)) {
            Drive();
            HWND h = GetDlgItem(0x505)->m_hWnd;
            g_pInvalidateRect(h, 0, 1);
        }
    }
}

RVA(0x000c3b90, 0xd1)
void CMultiStartDlg::OnColorSlot3() {
    CMulti* mp = g_64bd5c;
    if ((mp->m_isHost == 0 || ((CFocusSlot*)m_host)[3].m_164 != 0)
        && (((CFocusSlot*)m_host)[3].m_16c != 0
            || ((CFocusSlot*)m_host)[3].m_168 != mp->m_hostIndex)) {
        return;
    }
    CBattlezDlgColors dlg(m_host, 3, 1, 0);
    if (dlg.DoModal() == 1) {
        if (((CNetSessHost*)this)->SelectColor(3, dlg.m_pickedColor)) {
            Drive();
            HWND h = GetDlgItem(0x507)->m_hWnd;
            g_pInvalidateRect(h, 0, 1);
        }
    }
}

// inlines OnCustomWorld's teardown (member ~CString m_customName + base ~CDialog as
// separate calls) exactly as retail did - the out-of-line ??1 call misses that shape.
inline CBattlezDlgCustom::~CBattlezDlgCustom() {}

// ---------------------------------------------------------------------------
// OnCustomWorld (0xc3cb0): double-click the world combo (0x4ff). Host-only: run the
// modal CBattlezDlgCustom name dialog, and on IDOK with a non-empty name uppercase it
// into the combo's edit child and commit it as the game's custom world/host name.
// ---------------------------------------------------------------------------
// @early-stop
// Same wall family as the sibling CBattlezDlg::ShowCustomDlg (Dialogs.cpp, ~92.9%):
// the inlined ~CBattlezDlgCustom teardown, /GX EH trylevel numbering (retail 0/1/2/-1
// vs 0/1/-1), the child!=0 branch polarity, and an esi-save shrink-wrap our newer
// codegen does that MSVC5 didn't - none source-steerable. Body byte-faithful. ~86.5%.
RVA(0x000c3cb0, 0x128)
void CMultiStartDlg::OnCustomWorld() {
    if (g_64bd5c->m_isHost == 0) {
        return;
    }
    CBattlezDlgCustom dlg(0);
    if (dlg.DoModal() == 1 && dlg.m_customName.GetLength() != 0) {
        CWnd* child = CWnd::FromHandle(g_pGetWindow(GetDlgItem(0x4ff)->m_hWnd, GW_CHILD));
        if (child != 0) {
            dlg.m_customName.MakeUpper();
            child->SetWindowTextA((LPCTSTR)dlg.m_customName);
            m_6c = 1;
            g_64bd5c->m_5b0 = 1;
            g_64bd5c->m_5b8 = (LPCTSTR)dlg.m_customName;
            g_64bd5c->m_5b4 = g_emptyString;
            g_64bd5c->Commit3ada(0);
        }
    }
}

// ---------------------------------------------------------------------------
// CMultiStartDlg::Sub_c3e30 (0xc3e30) - the caller SetupWorldCombo runs it as a
// self-call. When this is the host, read the current selection of the 0x4ff world
// combo, and if its text is non-empty commit it as the game's world/host name into
// the CMulti game-state (m_5b4 = name, m_5b8 = "", m_5b0 = 0, Commit3ada). The /GX
// EH frame unwinds the local scratch CString. GetLBText (CComboBox::GetLBText
// 0x1ce7db) / operator= / Commit3ada / SendMessageA all reloc-mask.
// (wave2-F: re-homed here from MultiStartDlg.cpp - RVA 0xc3e30 lies in THIS
// roster interval, per the TU_MIGRATION MOVE row deferred by wave1-D.)
RVA(0x000c3e30, 0xfe)
void CMultiStartDlg::Sub_c3e30() {
    if (g_64bd5c->m_isHost != 0) {
        CWnd* item = GetDlgItem(0x4ff);
        if (item != 0) {
            i32 r = SendMessageA(item->m_hWnd, 0x147, 0, 0);
            if (r != -1) {
                CString name;
                item->GetLBText1ce7db(r, name);
                if (name.GetLength() != 0) {
                    m_6c = 0;
                }
                g_64bd5c->m_5b0 = 0;
                g_64bd5c->m_5b8 = g_emptyString;
                g_64bd5c->m_5b4 = (LPCTSTR)name;
                g_64bd5c->Commit3ada(0);
            }
        }
    }
}

// OnChatSend (0xc3f70): compose "<localName> says: <typed text>" and, when the input
// (control 0x42d) is non-empty, append it to the chat log and broadcast it to every
// peer, then clear the input. The /GX EH frame unwinds the two scratch CStrings.
RVA(0x000c3f70, 0xfb)
void CMultiStartDlg::OnChatSend() {
    CWnd* input = GetDlgItem(0x42d);
    if (input == 0) {
        return;
    }
    CString a, b;
    GetCtrlB(GetSlotIndex())->GetWindowTextA(a); // a = the local player's name
    a += " says: ";
    input->GetWindowTextA(b); // b = the typed message
    if (b.GetLength() != 0) {
        a += b;
        AppendChatLine((char*)(const char*)a);
        input->SetWindowTextA(g_emptyString);
        ((CNetMgr*)g_64bd5c)->BroadcastChatLine((char*)(const char*)a, 0, 0, 0);
    }
}

// ---------------------------------------------------------------------------
// Drive the connect state off the file-scope CMulti: if this is the host, broadcast
// the channel table + refresh players; else transform the local id and submit it.
// ---------------------------------------------------------------------------
RVA(0x000c40b0, 0x42)
void CMultiStartDlg::Drive() {
    CMulti* netMgr = g_64bd5c;
    if (netMgr->m_isHost != 0) {
        netMgr->BroadcastChannelTable(0);
        UpdatePlayers(1); // 0xc4230 (reloc-masked; return discarded)
    } else {
        i32 transformedPlayerId = (i32)((CNetDlgHost*)m_host)->FindOptionsSlot(netMgr->m_hostIndex);
        g_64bd5c->BroadcastOneChannel(transformedPlayerId);
    }
}

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

// __thiscall(force): refresh every player row from the roster + selection owner.
// @early-stop
// EH-representation wall: /GX frame (CString `name` temp) - the per-branch EH
// state-index stamps ([esp+EHstate] = N) and the aggregate-TU regalloc/spill recolor
// diverge from retail; code shape + all DIR32 data refs match. ~62%.
RVA(0x000c4230, 0x38e)
i32 CMultiStartDlg::UpdatePlayers(i32 force) {
    CWnd::FromHandle(g_pGetFocus());
    i32 f1c = 1;
    i32 f18 = 0;
    i32 idx = 0;
    i32 t = this->LocalSlot2d4c();
    i32 localColour = g_64bd5c->m_isHost ? ((CFocusSlot*)m_host)[t].m_16c : 1;
    i32 off = 0;
    do {
        CFocusSlot* slot = (CFocusSlot*)((char*)g_gameReg + off + 0x150);
        if (slot) {
            if (slot->m_18 != g_64bd5c->m_hostIndex && slot->m_14 && slot->m_20) {
                f18 = 1;
            }
            i32 enName;
            if (g_64bd5c->m_isHost && slot->m_14 == 0) {
                enName = 1;
            } else {
                enName = slot->m_18 == g_64bd5c->m_hostIndex ? 1 : 0;
            }
            this->NameEdit298c(idx)->EnableWindow(enName);
            this->KindCombo1929(idx)->EnableWindow(
                g_64bd5c->m_isHost && localColour == 0 && slot->m_18 != g_64bd5c->m_hostIndex ? 1
                                                                                              : 0
            );
            CWnd* ready = this->ReadyCheck1159(idx);
            ready->EnableWindow(slot->m_18 == g_64bd5c->m_hostIndex ? 1 : 0);
            if (slot->m_1c) {
                if (slot->m_20) {
                    g_pSendMessageA(ready->m_hWnd, 0xf1, 1, 0);
                } else {
                    g_pSendMessageA(ready->m_hWnd, 0xf1, 0, 0);
                }
            } else if (slot->m_20) {
                g_pSendMessageA(ready->m_hWnd, 0xf1, 0, 0);
                f1c = 0;
            } else {
                g_pSendMessageA(ready->m_hWnd, 0xf1, 0, 0);
            }
            this->ColourBtn1753(idx)->EnableWindow(
                g_64bd5c->m_isHost && slot->m_20 && localColour == 0 ? 1 : 0
            );
            this->SyncColour3a5d(idx, slot->m_20 ? slot->m_228 : 0);
            if (force == 0) {
                if (this->LocalSlot2d4c() == idx) {
                    goto next;
                }
                if (g_64bd5c->m_isHost && slot->m_14 == 0) {
                    goto next;
                }
            }
            if (slot->m_20) {
                {
                    CString name = ((GruntzPlayer*)slot)->GetName();
                    LPCTSTR pch = (LPCTSTR)name;
                    force = 0;
                    this->NameEdit298c(idx)->SetWindowTextA(pch);
                }
                if (slot->m_14) {
                    g_pSendMessageA(this->KindCombo1929(idx)->m_hWnd, 0x14e, 4, 0);
                } else {
                    g_pSendMessageA(this->KindCombo1929(idx)->m_hWnd, 0x14e, slot->m_10 + 1, 0);
                }
            } else {
                this->NameEdit298c(idx)->SetWindowTextA(g_emptyString);
                g_pSendMessageA(this->KindCombo1929(idx)->m_hWnd, 0x14e, 0, 0);
            }
            this->SyncChannelSlot(idx); // 0x3ffd thunk -> 0xc2ab0 reconcile (== SyncChannelSlot)
        }
    next:
        off += 0x238;
        idx++;
    } while (off < 0x8e0);
    if (g_64bd5c->m_isHost) {
        CWnd* ok = this->GetDlgItem(1);
        if (ok == 0) {
            return 0;
        }
        ok->EnableWindow(f18 & f1c);
    }
    g_pInvalidateRect(this->GetDlgItem(0x501)->m_hWnd, 0, 1);
    g_pInvalidateRect(this->GetDlgItem(0x503)->m_hWnd, 0, 1);
    g_pInvalidateRect(this->GetDlgItem(0x505)->m_hWnd, 0, 1);
    g_pInvalidateRect(this->GetDlgItem(0x507)->m_hWnd, 0, 1);
    return 1;
}

// GetSlotIndex (0xc4b30): resolve the local player's options slot through the host
// facet (m_host->FindOptionsSlot(g_64bd5c->m_hostIndex)); return the stored slot value
// or -1 when absent. __thiscall. Re-homed from src/Stub/ReconBatch2.cpp (was the
// OptOwner_c4b30::Resolve view; m_5c is CMultiStartDlg::m_host, xref-proven).
RVA(0x000c4b30, 0x1f)
i32 CMultiStartDlg::GetSlotIndex() {
    i32* slot = (i32*)((CNetDlgHost*)m_host)->FindOptionsSlot(g_64bd5c->m_hostIndex);
    if (slot == 0) {
        return -1;
    }
    return *slot;
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

// __thiscall(): cache list N's current selection (+1) into the Nth player-slot's combo
// value, then re-drive the connect state. Four handlers, one per player slot; slot 2
// resolves its list through GetCtrlC (ResolveItem_c27c0), the rest through 0x1753.
RVA(0x000c4ee0, 0x33)
void CMultiStartDlg::OnSlotSelect0() {
    HWND h = ResolveItem_1753(0)->m_hWnd;
    g_gameReg->m_focusSlots[0].m_228 = SendMessageA(h, 0x147, 0, 0) + 1;
    Drive();
}

RVA(0x000c4f30, 0x33)
void CMultiStartDlg::OnSlotSelect1() {
    HWND h = ResolveItem_1753(1)->m_hWnd;
    g_gameReg->m_focusSlots[1].m_228 = SendMessageA(h, 0x147, 0, 0) + 1;
    Drive();
}

RVA(0x000c4f80, 0x33)
void CMultiStartDlg::OnSlotSelect2() {
    HWND h = ResolveItem_c27c0(2)->m_hWnd;
    g_gameReg->m_focusSlots[2].m_228 = SendMessageA(h, 0x147, 0, 0) + 1;
    Drive();
}

RVA(0x000c4fd0, 0x33)
void CMultiStartDlg::OnSlotSelect3() {
    HWND h = ResolveItem_1753(3)->m_hWnd;
    g_gameReg->m_focusSlots[3].m_228 = SendMessageA(h, 0x147, 0, 0) + 1;
    Drive();
}

// CommitLatencyOption (0xc5020): host-only. Split the battle-latency combo's (control
// 0x527) selection into its lo/hi words; if either is set, commit them into the CMulti
// session config (m_5a4 / m_drainReload) and re-save, else flag "none selected" (m_600).
// @early-stop
// dead-member-read wall (~92%): retail emits a DEAD `mov ecx,[this+0x60]` (m_slotList)
// right after the GetSafe1c hwnd load - it occupies ecx, forcing both GetSelItemData
// out-arg `lea`s into edx (retail `lea (esp),edx; push; lea 8(esp),edx` vs our `lea
// (esp),ecx; lea 4(esp),edx; push`). MSVC5 emitted that dead read; /O2 reconstruction
// DCEs any discarded `m_slotList;` access, so the read + its register/offset cascade are
// the only residual. Logic + every other byte faithful.
RVA(0x000c5020, 0x95)
void CMultiStartDlg::CommitLatencyOption() {
    if (g_64bd5c->m_isHost == 0) {
        return;
    }
    i32 lo, hi;
    i32 h = GetSafe1c();
    GetSelItemData((HWND)h, 0x527, &lo, &hi);
    if (lo != 0 || hi != 0) {
        g_64bd5c->m_5a4 = lo;
        g_64bd5c->m_drainReload = hi;
        g_64bd5c->m_600 = 0;
        g_64bd5c->Commit3ada(0);
    } else {
        g_64bd5c->m_600 = 1;
    }
}

// __thiscall(idx): toggle slot idx's ready flag from its checkbox, then either re-sync
// the whole roster (host) or just refresh that one slot.
// @early-stop
// regalloc coin-flip wall (~97.7%, docs/patterns/zero-register-pinning.md family): the
// full logic is byte-faithful, but the final slot lea overwrites the INDEX register
// (ecx) here while retail overwrites the BASE register (edx) - so retail carries the
// slot pointer in edx, this cl in ecx, cascading through the g_64bd5c/m_isHost load
// pair (ecx/eax vs eax/edx) and the Refresh185c arg push (push edx vs push ecx). A pure
// allocator choice, no source lever.
RVA(0x000c50f0, 0x9b)
void CMultiStartDlg::ToggleReady(i32 idx) {
    CWnd* it = ResolveItem_1159(idx);
    if (!it) {
        return;
    }
    i32 sel = SendMessageA(it->m_hWnd, 0xf0, 0, 0);
    CFocusSlot* slot = (CFocusSlot*)((char*)g_gameReg + idx * 0x238 + 0x150);
    if (!slot) {
        return;
    }
    if (sel) {
        slot->m_1c = 1;
    } else {
        slot->m_1c = 0;
    }
    if (g_64bd5c->m_isHost) {
        Func1d70(0);
        Sync16db(1);
        Sync227a();
        Sync2c0c();
        Sync38d2();
    } else {
        Refresh185c(slot);
    }
}

// ---------------------------------------------------------------------------
// 0x0c5240 (spatially re-homed from src/Stub/Cluster0c.cpp). Teardown of the
// unidentified per-session net object: release+free the +0x60 CNetThing child,
// then run the final Destroy_1bbb7c. @orphan (class identity unrecovered; its Init
// sibling homes to src/Net/NetCmdSlot.cpp).
extern "C" void RezFree(void*); // 0x1b9b82
struct CNetThing {              // TU-local view of the header-less CNetThing (netthingdtor unit)
    ~CNetThing();               // dtor @0xc5280 (Release IS ~CNetThing)
};
struct CCluster0c {
    char pad00[0x60];
    CNetThing* m_60;       // +0x60 owned child
    void Destroy_1bbb7c(); // 0x1bbb7c
    void Cleanup();        // 0xc5240
};
RVA(0x000c5240, 0x2c)
void CCluster0c::Cleanup() {
    CNetThing* p = m_60;
    if (p) {
        p->~CNetThing();
        RezFree(p);
        m_60 = 0;
    }
    Destroy_1bbb7c();
}
SIZE_UNKNOWN(CCluster0c);
SIZE_UNKNOWN(CNetThing);

// EchoLatencySettings (0xc52f0): print the current session CmdDelay (m_5a4) and
// ResendDelay (m_drainReload) to the chat log via wsprintfA into a stack buffer.
RVA(0x000c52f0, 0x43)
void CMultiStartDlg::EchoLatencySettings() {
    char buf[128];
    wsprintfA(buf, s_UsingCmdDelay, g_64bd5c->m_5a4, g_64bd5c->m_drainReload);
    AppendChatLine(buf);
}

// ---------------------------------------------------------------------------
// Configure the singleton with two fixed ids.
// ---------------------------------------------------------------------------
RVA(0x000c5f00, 0x15)
void NetConfigureBe90() {
    ((CZDArrayDerived*)&g_netBe90)->Construct(0x7d0, 0x7da);
}

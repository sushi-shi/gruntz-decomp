// MultiStartDlgRoster.cpp - the multiplayer roster/color/net dialog TU: the WOVEN
// original obj at retail .text [0x0c2980 .. 0x0c5f15] (own 8-frag init run @0xc5360;
// a SEPARATE obj from MultiStartDlg.cpp's 0xc16b0 interval). The five former units
// multistartdlgroster + netmgrmisc(interval fns) + netgamedlg + multistartdlgcolor +
// multistartdlgnet, + the Sub_c3e30 stray from MultiStartDlg.cpp interleave into one
// TU. Everything here runs on the ONE
// multiplayer-start dialog (CMultiStartDlg) or its roster records; definitions in
// strict ascending retail-RVA order.
//
// Homed in its own unit (not Dialogs.cpp) so it can't perturb that TU's parked dtors;
// it reuses the shared Dialogs.h dialog models, the canonical CGameRegistry spine
// (GameRegistry.h) and the canonical CMulti game-state (Multi.h). Field names are
// placeholders (m_<hexoffset>); only offsets + code bytes are load-bearing.
#include <Gruntz/Dialogs.h>
#include <Gruntz/GameRegPtr.h>
#include <Gruntz/Random.h> // g_randSeed/g_randSeeded (FlashCtrlD's swatch colour)
#include <EmptyString.h>       // g_emptyString
#include <Gruntz/Multi.h>      // the real CMulti (the 0x64bd5c multiplayer game-state singleton)
#include <Gruntz/NetDlgHost.h> // CNetDlgHost (m_host +0x5c facet)
#include <Gruntz/GruntzMgr.h> // CGruntzMgr::FindOptionsSlot (0x92e80, the m_host FindOptionsSlot callee)
#include <Gruntz/GameRegistry.h> // the canonical g_gameReg spine (CGameRegistry, VA 0x64556c)
#include <Net/NetSessHost.h>     // CNetSessHost::SelectColor (0xc4b60), the +0x5c facet
#include <Net/LatencyList.h>     // CLatencyList : CKeyedList (m_slotList; its dtor is 0xc5280)
#include <Net/NetMgr.h>          // CNetMgr::BroadcastChatLine (0xbb190), the chat-broadcast facet
#include <rva.h>
#include <string.h> // strcat (inline CRT, reloc-masked)

// A roster slot's FormatName_3e54 @0x3e54 IS GruntzPlayer::GetName; cast at the call.
#include <Gruntz/GruntzPlayer.h> // canonical GruntzPlayer (GetName)
// @interleaver GruntzPlayer::GetName (0x0001f450) emitted-in bootystateactivate - blocked:
// inline-header COMDAT (not body-movable). GetName is defined INLINE in
// <Gruntz/GruntzPlayer.h> (RVA there), so MSVC emits its out-of-line COMDAT in EVERY TU
// that references it; symbol_names.csv currently attributes 0x1f450 to THIS unit
// (multistartdlgroster calls it at line ~870). Retail's real home is bootystateactivate
// (its .text sits between bootystateactivate FrameSlot28 @0x1e660 pool and QueryGruntSlots
// @0x1ecf0). Fixing this is an inline-header emission-migration (steer which TU emits the
// COMDAT), NOT a .cpp body relocation - deferred + flagged.

// --- shared globals (canonical homes elsewhere; reloc-masked references here) ---
// The game-manager singleton (VA 0x64556c); the SelHost/roster handlers cache each
// player-slot's combo value into its m_focusSlots[] record. Reference-only (undefined
// external, reloc-masks) as in LobbyDialogs.cpp.
// The multiplayer game-state (a CMulti, xref-proven); the roster reads m_isHost /
// m_hostIndex off it. Declared in <Gruntz/Multi.h>.
// The shared empty-string literal (0x6293f4; homed in NetMgrReportError.cpp).
// USER32 entry points reached through the game's own IAT-style function pointers
// (ff 15 [ptr]); UpdatePlayers drives its listboxes/redraws through them.
// More USER32 entry points via the game's own IAT-style pointers.
// The EchoLatencySettings printf format (.data), DEFINED here (owner TU). The retail
// bytes are exactly this 44-byte string, so the initializer is byte-faithful.
DATA(0x0021243c)
char s_UsingCmdDelay[] = "Using CmdDelay of %d and ResendDelay of %d.";

// The per-player roster record IS the canonical CFocusSlot (GameRegistry.h,
// CGameRegistry::m_focusSlots[] +0x150, stride 0x238).

// Dialog-item resolver (push idx; __stdcall) that relies on the caller's live ecx=this:
// a thiscall ready-checkbox accessor (0x1159) spelled __stdcall so the byte stream omits
// the `mov ecx` the caller already satisfied. (SetListCurSel / OnSlotSelect0..3 now call
// the real CMultiStartDlg::GetCtrlC @0xc27c0 directly.)
CWnd* __stdcall ResolveItem_1159(i32 idx); // 0x01159
// Roster free helpers (__stdcall, reloc-masked).
void __stdcall Func1d70(i32 flag);            // 0x01d70
void __stdcall Refresh185c(CFocusSlot* slot); // 0x0185c

// The per-channel player-slot record (a proven +0x150-shifted view of CFocusSlot) lives
// in <Gruntz/ChannelSlot.h>; the full CFocusSlot fold is deferred there (the +0x154
// CString member into CGameRegistry's by-value CFocusSlot array needs ctor reconciliation).
#include <Gruntz/ChannelSlot.h>

extern CString g_gruntNames[];       // 0x64bdb0 per-channel label table

void ChannelSlots_Set(i32 slot, i32 v); // 0xdb2b0
i32 ChannelSlots_FindFree();            // 0xdb280
CString GetConfigNameA();               // 0xb6090
CString GetConfigNameB();               // 0xb60d0

// __stdcall(hDlg, id, *lo, *hi): split control `id`'s selected listbox item data into
// two words; returns 1 when a valid item is selected. Generic listbox helper.
// @interleaver GetSelItemData emitted-in <boundary: slotcombofill?>
// (REHOME D10 not-homeable: BOUNDARY COMDAT - retail neighbours are slotcombofill
// CLatencyList::SelectItem @0x38150 (before) + statemgrbz StateMgrBZ::Init @0x382c0
// (after), NOT one host both sides; free __stdcall listbox helper, no class anchor.
// Home hint videoconfig is proximity-only (block 0x363a0.., not adjacent); the
// preceding slotcombofill (a dialog combo helper) is the thematic candidate but its
// obj boundary isn't pinned. Kept in place + flagged.)
RVA(0x00038220, 0x73)
i32 __stdcall GetSelItemData(HWND hDlg, i32 id, i32* outLo, i32* outHi) {
    HWND list = GetDlgItem(hDlg, id);
    if (!list) {
        return 0;
    }
    i32 sel = ::SendMessageA(list, 0x147, 0, 0);
    if (sel == -1) {
        return 0;
    }
    i32 data = ::SendMessageA(list, 0x150, sel, 0);
    if (data == -1) {
        return 0;
    }
    *outLo = data & 0xffff;
    *outHi = static_cast<u32>(data) >> 0x10;
    return 1;
}

// __thiscall(id, wParam): if list control `id` resolves (GetCtrlC @0xc27c0), set its
// LB_SETCURSEL to wParam-1. ecx=this passes through to GetCtrlC (no `mov ecx` emitted).
RVA(0x000c2980, 0x28)
void CMultiStartDlg::SetListCurSel(i32 id, i32 wParam) {
    CWnd* it = GetCtrlC(id);
    if (it) {
        ::SendMessageA(it->m_hWnd, 0x14e, wParam - 1, 0);
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
    LRESULT(WINAPI * pSend)(HWND, UINT, WPARAM, LPARAM) = ::SendMessageA;
    if (pSend(owner->m_hWnd, 0x147, 0, 0) == 0) {
        if (s->m_14 != 0) {
            if (s->m_active != 0) {
                g_multiState->DropPlayer(s->m_playerId);
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
                    g_multiState->DropPlayer(s->m_playerId);
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
            s->m_selectionIndex = static_cast<i32>(pSend(owner->m_hWnd, 0x147, 0, 0)) - 1;
            s->m_active = 1;
            s->m_label = g_gruntNames[ch];
        }
        c1->EnableWindow(1);
        c2->EnableWindow(1);
    }
}

// ---------------------------------------------------------------------------
// CMultiStartDlg::OnInitDialog (0x0c2cb0): the WM_INITDIALOG handler - vtable slot 49
// (== ??_7CMultiStartDlg@@6B@+0xc4, reached via ILT thunk 0x16fe; declared in Dialogs.h).
// The former AreaTimerDlg @identity-TODO is DISSOLVED: the vtable DATA-ref proved this is
// CMultiStartDlg's own OnInitDialog override, not a separate dialog. It chains the base
// CDialog::OnInitDialog (0x1bac5e, reloc-masked) then arms a 50 ms repaint timer; m_hWnd
// is CWnd::m_hWnd (+0x1c), inherited via CDialog.
// ---------------------------------------------------------------------------
RVA(0x000c2cb0, 0x1f)
i32 CMultiStartDlg::OnInitDialog() {
    CDialog::OnInitDialog(); // 0x1bac5e ?OnInitDialog@CDialog@@UAEHXZ (base call, exempt)
    ::SetTimer(m_hWnd, 1, 0x32, 0);
    return 1;
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
    i32 len = ::GetWindowTextLengthA(edit);
    if (len == 0) {
        ::SendMessageA(edit, 0xb1, len, -1);
    } else {
        ::SendMessageA(edit, 0xb1, len, len);
    }
    char buf[0x80];
    buf[0] = 0;
    if (len > 0) {
        strcat(buf, "\r\n");
    }
    strcat(buf, str);
    ::SendMessageA(edit, 0xc2, 0, reinterpret_cast<LPARAM>(buf));
    ::SendMessageA(edit, 0xb6, 0, 0x270f);
}

// Advance the shared LCG one step (lazily seeded); returns 15-bit value.
// Retail inlines this three times per colour, so force it inline.
static __inline i32 GameRand() {
    i32 seed;
    if (!(g_randSeeded & 1)) {
        g_randSeeded |= 1;
        seed = static_cast<i32>(::timeGetTime());
    } else {
        seed = g_randSeed;
    }
    g_randSeed = seed * 214013 + 2531011;
    return (g_randSeed >> 0x10) & 0x7fff;
}

// @early-stop
// EH frame-size wall (~95%). Complete correct reconstruction (the twin of
// CBattlezDlg::FlashCtrlD @0x160f0 in Dialogs.cpp, minus the rect-deflate,
// returning 1): walks the 4-entry GetCtrlD swatch family, maps each child's
// client rect into the host dialog's client coords, builds a random-gray
// (enabled) or fixed 0x808080 (disabled) solid brush and FillRects it. Residual
// is MSVC5's 0x70 vs 0x20 frame reservation shifting the dc-handle / EH-state
// stack slots - not steerable from source. (Formerly split out into a
// name-grouped FlashRect.cpp; 0xc2e20 sits inside THIS TU's 0xc2980..0xc5333
// block, so it is reunited here.)
RVA(0x000c2e20, 0x21d)
i32 CMultiStartDlg::FlashCtrlD() {
    CPaintDC dc(this);
    BOOL(WINAPI * cts)(HWND, LPPOINT) = ::ClientToScreen;
    BOOL(WINAPI * stc)(HWND, LPPOINT) = ::ScreenToClient;
    for (i32 i = 0; i < 4; i++) {
        CWnd* it = GetCtrlD(i);
        if (it == 0) {
            continue;
        }
        RECT rc;
        ::GetClientRect(it->m_hWnd, &rc);
        cts(it->m_hWnd, (LPPOINT)&rc);
        cts(it->m_hWnd, (LPPOINT)&rc + 1);
        stc(m_hWnd, (LPPOINT)&rc);
        stc(m_hWnd, (LPPOINT)&rc + 1);
        CBrush scratch;
        i32 color;
        if (it->IsWindowEnabled()) {
            GameRand();
            GameRand();
            i32 v = (GameRand() % 0xff) & 0xff;
            color = (v << 8 | v) << 8 | v;
        } else {
            color = 0x808080;
        }
        scratch.Attach(::CreateSolidBrush(color));
        ::FillRect(dc.m_hDC, &rc, scratch);
    }
    return 1;
}

// ---------------------------------------------------------------------------
// Owner-draw swatch fill for CMultiStartDlg::OnDrawItem (twin of the CBattlezDlg swatch
// draw in Dialogs.cpp). The former CSwatchBrush/CSwatchDrawBase/CWndOnDrawR fake views are
// DISSOLVED onto the real MFC classes: the stack brush is ::CBrush (CGdiObject base -
// ??_7CObject/CGdiObject/CBrush COMDATs first-instantiated in Dialogs.cpp), the stack DC
// is ::CDC, and the base owner-draw default is CWnd::OnDrawItem (0x1bbde7). Passing the
// CBrush to FillRect runs its inline operator HBRUSH(). The /GX EH
// frame unwinds the CDC/CBrush locals.
// The game's FillRect fn-ptr (the .idata IAT slot, reloc-masked indirect call). The
// canonical DATA binding is in Dialogs.cpp (g_pFillRectDlg @0x006c44e0); reference-
// only here so the roster call reloc-masks without a duplicate DATA binding.

// CMultiStartDlg::OnDrawItem (0xc3100): owner-draw the four team-color swatch static
// controls (0x501/0x503/0x505/0x507) of the multiplayer roster - the exact twin of
// CBattlezDlg::OnDrawItem, over this dialog's GetCtrlD (0xc2840) + the m_host slot
// array's per-slot color index (+0x158, stride 0x238). Disabled child -> light gray;
// then chains the base CWnd owner-draw default. /GX EH frame unwinds the CDC/CBrush.
//
// @early-stop
// jump-table-data split artifact (~71.5%; docs/patterns/jumptable-data-overlap.md) -
// the exact same wall as the CBattlezDlg twin (0x165a0): prologue, /GX EH frame (0x18),
// dispatch, the four inlined palette switches, and the whole CDC/CBrush/FillRect/Detach
// + base tail are byte-exact. The residual is the delinker splitting the 5 INLINE .text
// jump tables (MSVC5 emits them at 0x491..0x5c0) into separate switchdataD_* data
// symbols (Ghidra boundary 0x491), so objdiff scores our 0x5c0 base against a 0x491
// target. Not source-steerable (this shape momentarily scored 98.7% when the delinker
// carved the full 0x5c0).
RVA(0x000c3100, 0x5c0)
void CMultiStartDlg::OnDrawItem(i32 nIDCtl, DRAWITEMSTRUCT* lpdis) {
    COLORREF color;
    i32 bDraw = 0;
    switch (nIDCtl) {
        case 0x501:
            if (GetCtrlD(0)->IsWindowEnabled()) {
                switch ((reinterpret_cast<CBattlezSlot*>(m_host))[0].m_158) {
                    case 0:
                        color = 0x0080ff;
                        break;
                    case 1:
                        color = 0x00ff00;
                        break;
                    case 2:
                        color = 0xff0000;
                        break;
                    case 3:
                        color = 0x0000ff;
                        break;
                    case 4:
                        color = 0x800080;
                        break;
                    case 5:
                        color = 0x00ffff;
                        break;
                    case 6:
                        color = 0x8000ff;
                        break;
                    case 7:
                        color = 0;
                        break;
                    case 8:
                        color = 0x800000;
                        break;
                    case 9:
                        color = 0x008000;
                        break;
                    case 10:
                        color = 0x808000;
                        break;
                    case 11:
                        color = 0x000080;
                        break;
                    case 12:
                        color = 0xff00ff;
                        break;
                    case 13:
                        color = 0x008080;
                        break;
                    case 14:
                        color = 0x808080;
                        break;
                    case 15:
                        color = 0xffff00;
                        break;
                    case 16:
                        color = 0xffffff;
                        break;
                    default:
                        color = 0;
                        break;
                }
            } else {
                color = 0xc8c8c8;
            }
            bDraw = 1;
            break;
        case 0x503:
            if (GetCtrlD(1)->IsWindowEnabled()) {
                switch ((reinterpret_cast<CBattlezSlot*>(m_host))[1].m_158) {
                    case 0:
                        color = 0x0080ff;
                        break;
                    case 1:
                        color = 0x00ff00;
                        break;
                    case 2:
                        color = 0xff0000;
                        break;
                    case 3:
                        color = 0x0000ff;
                        break;
                    case 4:
                        color = 0x800080;
                        break;
                    case 5:
                        color = 0x00ffff;
                        break;
                    case 6:
                        color = 0x8000ff;
                        break;
                    case 7:
                        color = 0;
                        break;
                    case 8:
                        color = 0x800000;
                        break;
                    case 9:
                        color = 0x008000;
                        break;
                    case 10:
                        color = 0x808000;
                        break;
                    case 11:
                        color = 0x000080;
                        break;
                    case 12:
                        color = 0xff00ff;
                        break;
                    case 13:
                        color = 0x008080;
                        break;
                    case 14:
                        color = 0x808080;
                        break;
                    case 15:
                        color = 0xffff00;
                        break;
                    case 16:
                        color = 0xffffff;
                        break;
                    default:
                        color = 0;
                        break;
                }
            } else {
                color = 0xc8c8c8;
            }
            bDraw = 1;
            break;
        case 0x505:
            if (GetCtrlD(2)->IsWindowEnabled()) {
                switch ((reinterpret_cast<CBattlezSlot*>(m_host))[2].m_158) {
                    case 0:
                        color = 0x0080ff;
                        break;
                    case 1:
                        color = 0x00ff00;
                        break;
                    case 2:
                        color = 0xff0000;
                        break;
                    case 3:
                        color = 0x0000ff;
                        break;
                    case 4:
                        color = 0x800080;
                        break;
                    case 5:
                        color = 0x00ffff;
                        break;
                    case 6:
                        color = 0x8000ff;
                        break;
                    case 7:
                        color = 0;
                        break;
                    case 8:
                        color = 0x800000;
                        break;
                    case 9:
                        color = 0x008000;
                        break;
                    case 10:
                        color = 0x808000;
                        break;
                    case 11:
                        color = 0x000080;
                        break;
                    case 12:
                        color = 0xff00ff;
                        break;
                    case 13:
                        color = 0x008080;
                        break;
                    case 14:
                        color = 0x808080;
                        break;
                    case 15:
                        color = 0xffff00;
                        break;
                    case 16:
                        color = 0xffffff;
                        break;
                    default:
                        color = 0;
                        break;
                }
            } else {
                color = 0xc8c8c8;
            }
            bDraw = 1;
            break;
        case 0x507:
            if (GetCtrlD(3)->IsWindowEnabled()) {
                switch ((reinterpret_cast<CBattlezSlot*>(m_host))[3].m_158) {
                    case 0:
                        color = 0x0080ff;
                        break;
                    case 1:
                        color = 0x00ff00;
                        break;
                    case 2:
                        color = 0xff0000;
                        break;
                    case 3:
                        color = 0x0000ff;
                        break;
                    case 4:
                        color = 0x800080;
                        break;
                    case 5:
                        color = 0x00ffff;
                        break;
                    case 6:
                        color = 0x8000ff;
                        break;
                    case 7:
                        color = 0;
                        break;
                    case 8:
                        color = 0x800000;
                        break;
                    case 9:
                        color = 0x008000;
                        break;
                    case 10:
                        color = 0x808000;
                        break;
                    case 11:
                        color = 0x000080;
                        break;
                    case 12:
                        color = 0xff00ff;
                        break;
                    case 13:
                        color = 0x008080;
                        break;
                    case 14:
                        color = 0x808080;
                        break;
                    case 15:
                        color = 0xffff00;
                        break;
                    case 16:
                        color = 0xffffff;
                        break;
                    default:
                        color = 0;
                        break;
                }
            } else {
                color = 0xc8c8c8;
            }
            bDraw = 1;
            break;
    }
    if (bDraw) {
        ::CDC dc;
        dc.Attach(lpdis->hDC);
        ::CBrush brush(color);
        ::FillRect(dc.m_hDC, &lpdis->rcItem, brush);
        dc.Detach();
    }
    CWnd::OnDrawItem(nIDCtl, lpdis);
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
    CMulti* mp = g_multiState;
    if ((mp->m_isHost == 0 || (reinterpret_cast<CFocusSlot*>(m_host))[0].m_164 != 0)
        && ((reinterpret_cast<CFocusSlot*>(m_host))[0].m_16c != 0
            || (reinterpret_cast<CFocusSlot*>(m_host))[0].m_168 != mp->m_hostIndex)) {
        return;
    }
    CBattlezDlgColors dlg(m_host, 0, 1, 0);
    if (dlg.DoModal() == 1) {
        if (((CNetSessHost*)this)->SelectColor(0, dlg.m_pickedColor)) {
            Drive();
            HWND h = GetDlgItem(0x501)->m_hWnd;
            ::InvalidateRect(h, 0, 1);
        }
    }
}

RVA(0x000c3950, 0xd1)
void CMultiStartDlg::OnColorSlot1() {
    CMulti* mp = g_multiState;
    if ((mp->m_isHost == 0 || (reinterpret_cast<CFocusSlot*>(m_host))[1].m_164 != 0)
        && ((reinterpret_cast<CFocusSlot*>(m_host))[1].m_16c != 0
            || (reinterpret_cast<CFocusSlot*>(m_host))[1].m_168 != mp->m_hostIndex)) {
        return;
    }
    CBattlezDlgColors dlg(m_host, 1, 1, 0);
    if (dlg.DoModal() == 1) {
        if (((CNetSessHost*)this)->SelectColor(1, dlg.m_pickedColor)) {
            Drive();
            HWND h = GetDlgItem(0x503)->m_hWnd;
            ::InvalidateRect(h, 0, 1);
        }
    }
}

RVA(0x000c3a70, 0xd1)
void CMultiStartDlg::OnColorSlot2() {
    CMulti* mp = g_multiState;
    if ((mp->m_isHost == 0 || (reinterpret_cast<CFocusSlot*>(m_host))[2].m_164 != 0)
        && ((reinterpret_cast<CFocusSlot*>(m_host))[2].m_16c != 0
            || (reinterpret_cast<CFocusSlot*>(m_host))[2].m_168 != mp->m_hostIndex)) {
        return;
    }
    CBattlezDlgColors dlg(m_host, 2, 1, 0);
    if (dlg.DoModal() == 1) {
        if (((CNetSessHost*)this)->SelectColor(2, dlg.m_pickedColor)) {
            Drive();
            HWND h = GetDlgItem(0x505)->m_hWnd;
            ::InvalidateRect(h, 0, 1);
        }
    }
}

RVA(0x000c3b90, 0xd1)
void CMultiStartDlg::OnColorSlot3() {
    CMulti* mp = g_multiState;
    if ((mp->m_isHost == 0 || (reinterpret_cast<CFocusSlot*>(m_host))[3].m_164 != 0)
        && ((reinterpret_cast<CFocusSlot*>(m_host))[3].m_16c != 0
            || (reinterpret_cast<CFocusSlot*>(m_host))[3].m_168 != mp->m_hostIndex)) {
        return;
    }
    CBattlezDlgColors dlg(m_host, 3, 1, 0);
    if (dlg.DoModal() == 1) {
        if (((CNetSessHost*)this)->SelectColor(3, dlg.m_pickedColor)) {
            Drive();
            HWND h = GetDlgItem(0x507)->m_hWnd;
            ::InvalidateRect(h, 0, 1);
        }
    }
}

// (the local `inline CBattlezDlgCustom::~CBattlezDlgCustom() {}` that used to sit here is
// GONE. It was written to force /Ob1 to inline OnCustomWorld's teardown - which it did, but
// at the cost of a `mov [..],??_7CBattlezDlgCustom` vptr re-stamp retail does not have. The
// dtor is COMPILER-GENERATED in the real source (see <Gruntz/Dialogs.h>): cl inlines an
// implicit dtor at a stack local's scope exit AND omits the stamp, which is exactly the
// shape retail emits. Nothing to define here.)

// ---------------------------------------------------------------------------
// OnCustomWorld (0xc3cb0): double-click the world combo (0x4ff). Host-only: run the
// modal CBattlezDlgCustom name dialog, and on IDOK with a non-empty name uppercase it
// into the combo's edit child and commit it as the game's custom world/host name.
// ---------------------------------------------------------------------------
// @early-stop
// Same wall family as the sibling CBattlezDlg::ShowCustomDlg (Dialogs.cpp): /GX EH trylevel
// numbering (retail 0/1/2/-1 vs 0/1/-1), the child!=0 branch polarity, and an esi-save
// shrink-wrap our newer codegen does that MSVC5 didn't - none source-steerable.
// ~CBattlezDlgCustom is compiler-generated (what the binary says; the dtor COMDAT
// is 100%). Body byte-faithful.
RVA(0x000c3cb0, 0x128)
void CMultiStartDlg::OnCustomWorld() {
    if (g_multiState->m_isHost == 0) {
        return;
    }
    CBattlezDlgCustom dlg(0);
    if (dlg.DoModal() == 1 && dlg.m_customName.GetLength() != 0) {
        CWnd* child = CWnd::FromHandle(::GetWindow(GetDlgItem(0x4ff)->m_hWnd, GW_CHILD));
        if (child != 0) {
            dlg.m_customName.MakeUpper();
            child->SetWindowTextA((LPCTSTR)dlg.m_customName);
            m_6c = 1;
            g_multiState->m_5b0 = 1;
            g_multiState->m_5b8 = (LPCTSTR)dlg.m_customName;
            g_multiState->m_5b4 = g_emptyString;
            g_multiState->SaveConfig(0);
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
// (RVA 0xc3e30 lies in THIS roster interval.)
RVA(0x000c3e30, 0xfe)
void CMultiStartDlg::Sub_c3e30() {
    if (g_multiState->m_isHost != 0) {
        CWnd* item = GetDlgItem(0x4ff);
        if (item != 0) {
            i32 r = ::SendMessageA(item->m_hWnd, 0x147, 0, 0);
            if (r != -1) {
                CString name;
                ((CComboBox*)item)->GetLBText(r, name); // CComboBox::GetLBText @0x1ce7db
                if (name.GetLength() != 0) {
                    m_6c = 0;
                }
                g_multiState->m_5b0 = 0;
                g_multiState->m_5b8 = g_emptyString;
                g_multiState->m_5b4 = (LPCTSTR)name;
                g_multiState->SaveConfig(0);
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
        AppendChatLine(const_cast<char*>(static_cast<const char*>(a)));
        input->SetWindowTextA(g_emptyString);
        g_multiState->BroadcastChatLine(
            const_cast<char*>(static_cast<const char*>(a)),
            0,
            0,
            0
        ); // CMulti::BroadcastChatLine @0xbb190
    }
}

// ---------------------------------------------------------------------------
// Drive the connect state off the file-scope CMulti: if this is the host, broadcast
// the channel table + refresh players; else transform the local id and submit it.
// ---------------------------------------------------------------------------
RVA(0x000c40b0, 0x42)
void CMultiStartDlg::Drive() {
    CMulti* netMgr = g_multiState;
    if (netMgr->m_isHost != 0) {
        netMgr->BroadcastChannelTable(0);
        UpdatePlayers(1); // 0xc4230 (reloc-masked; return discarded)
    } else {
        i32 transformedPlayerId = reinterpret_cast<i32>((reinterpret_cast<CGruntzMgr*>(m_host))->FindOptionsSlot(netMgr->m_hostIndex));
        g_multiState->BroadcastOneChannel(transformedPlayerId);
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
    if (g_multiState->m_5b0 == 0) {
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
    CWnd::FromHandle(::GetFocus());
    i32 f1c = 1;
    i32 f18 = 0;
    i32 idx = 0;
    i32 t = this->LocalSlot2d4c();
    i32 localColour = g_multiState->m_isHost ? (reinterpret_cast<CFocusSlot*>(m_host))[t].m_16c : 1;
    i32 off = 0;
    do {
        CFocusSlot* slot = (CFocusSlot*)((char*)g_gameReg + off + 0x150);
        if (slot) {
            if (slot->m_18 != g_multiState->m_hostIndex && slot->m_14 && slot->m_20) {
                f18 = 1;
            }
            i32 enName;
            if (g_multiState->m_isHost && slot->m_14 == 0) {
                enName = 1;
            } else {
                enName = slot->m_18 == g_multiState->m_hostIndex ? 1 : 0;
            }
            this->NameEdit298c(idx)->EnableWindow(enName);
            this->KindCombo1929(idx)->EnableWindow(
                g_multiState->m_isHost && localColour == 0
                        && slot->m_18 != g_multiState->m_hostIndex
                    ? 1
                    : 0
            );
            CWnd* ready = this->ReadyCheck1159(idx);
            ready->EnableWindow(slot->m_18 == g_multiState->m_hostIndex ? 1 : 0);
            if (slot->m_1c) {
                if (slot->m_20) {
                    ::SendMessageA(ready->m_hWnd, 0xf1, 1, 0);
                } else {
                    ::SendMessageA(ready->m_hWnd, 0xf1, 0, 0);
                }
            } else if (slot->m_20) {
                ::SendMessageA(ready->m_hWnd, 0xf1, 0, 0);
                f1c = 0;
            } else {
                ::SendMessageA(ready->m_hWnd, 0xf1, 0, 0);
            }
            this->ColourBtn1753(idx)->EnableWindow(
                g_multiState->m_isHost && slot->m_20 && localColour == 0 ? 1 : 0
            );
            this->SyncColour3a5d(idx, slot->m_20 ? slot->m_228 : 0);
            if (force == 0) {
                if (this->LocalSlot2d4c() == idx) {
                    goto next;
                }
                if (g_multiState->m_isHost && slot->m_14 == 0) {
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
                    ::SendMessageA(this->KindCombo1929(idx)->m_hWnd, 0x14e, 4, 0);
                } else {
                    ::SendMessageA(this->KindCombo1929(idx)->m_hWnd, 0x14e, slot->m_10 + 1, 0);
                }
            } else {
                this->NameEdit298c(idx)->SetWindowTextA(g_emptyString);
                ::SendMessageA(this->KindCombo1929(idx)->m_hWnd, 0x14e, 0, 0);
            }
            this->SyncChannelSlot(idx); // 0x3ffd thunk -> 0xc2ab0 reconcile (== SyncChannelSlot)
        }
    next:
        off += 0x238;
        idx++;
    } while (off < 0x8e0);
    if (g_multiState->m_isHost) {
        CWnd* ok = this->GetDlgItem(1);
        if (ok == 0) {
            return 0;
        }
        ok->EnableWindow(f18 & f1c);
    }
    ::InvalidateRect(this->GetDlgItem(0x501)->m_hWnd, 0, 1);
    ::InvalidateRect(this->GetDlgItem(0x503)->m_hWnd, 0, 1);
    ::InvalidateRect(this->GetDlgItem(0x505)->m_hWnd, 0, 1);
    ::InvalidateRect(this->GetDlgItem(0x507)->m_hWnd, 0, 1);
    return 1;
}

// ===========================================================================
// CMultiStartDlg::Watchdog  (0x0c46b0). Guarded by a re-entrancy flag, it refreshes the
// per-slot roster display, advances two blink counters, then walks the net-session
// status flags and, on any terminal condition, kills the poll timer, pops a status
// message, and tears down. Field/class names are placeholders; g_multiState/g_gameReg
// reuse this TU's canonical decls.
// ===========================================================================
// The watchdog reads three fields (m_14 present / m_20 active / m_22c display value)
// per player off the canonical CGameRegistry::m_focusSlots[] (GameRegistry.h, +0x150,
// stride 0x238).

// The cached timeGetTime fn-ptr (DATA symbol; 0-arg, bound by m5_PaletteLerp).
// Watchdog re-entrancy guard + two blink counters (.data).
extern "C" i32 g_watchBusy;      // 0x64bdc4
extern "C" i32 g_watchBlinkA;    // 0x64bdc8
extern "C" i32 g_watchBlinkB;    // 0x64bdcc

// @early-stop
// ~94% regalloc-coloring wall (all control flow + calls + the DIR32 globals pair):
// retail re-materializes the zero constant into ebx after the roster loop and reuses
// it for the state-chain `push 0` + the m_58c store, whereas MSVC5 colors the m_hWnd
// KillTimer temps into ecx/edx; plus a 2-instr timeGetTime `this`-load schedule. Not
// source-steerable. docs/patterns/zero-register-pinning.md.
RVA(0x000c46b0, 0x371)
void CMultiStartDlg::Watchdog() {
    if (g_watchBusy != 0) {
        return;
    }
    g_watchBusy = 1;
    void* h = g_multiState->m_netGate->m_player;
    if (h == 0) {
        return;
    }
    g_multiState->m_netGate->M178a80(h, 0);
    g_multiState->ResolveLocalPlayer();
    if (g_watchBlinkA == 0) {
        u32 t = ::timeGetTime();
        g_multiState->SendNetStat(0x41f, static_cast<i32>(t), 0);
    }
    if (g_multiState->m_isHost == 0) {
        if (g_watchBlinkA == 0) {
            g_multiState->ReportAckLatency();
        }
        EnableWindow(0);
        i32 r = g_multiState->VerifyCustomLevel(h, g_multiState->m_5bc);
        EnableWindow(1);
        if (r != 0) {
            M1bab37(1);
            g_watchBusy = 0;
            return;
        }
    } else {
        g_multiState->PollSession();
        if (g_multiState->m_600 != 0) {
            g_multiState->AutoTuneCmdDelay();
        }
    }
    i32 a = g_watchBlinkA + 1;
    g_watchBlinkA = a;
    if (a > 3) {
        g_watchBlinkA = 0;
    }
    if (g_watchBlinkB == 0) {
        for (i32 i = 0; i < 4; i++) {
            CFocusSlot* slot = &g_gameReg->m_focusSlots[i];
            CWnd* item1;
            CWnd* item2;
            switch (i) {
                case 0:
                    item1 = GetDlgItem(0x531);
                    item2 = GetDlgItem(0x534);
                    break;
                case 1:
                    item1 = GetDlgItem(0x532);
                    item2 = GetDlgItem(0x536);
                    break;
                case 2:
                    item1 = GetDlgItem(0x533);
                    item2 = GetDlgItem(0x537);
                    break;
                case 3:
                    item1 = GetDlgItem(0x535);
                    item2 = GetDlgItem(0x538);
                    break;
            }
            if (slot->m_20 != 0 && slot->m_14 != 0) {
                char buf[0x20];
                wsprintfA(buf, "%d", slot->m_22c);
                item1->SetWindowTextA(buf);
                item2->SetWindowTextA("R");
            } else {
                item1->SetWindowTextA("");
                item2->SetWindowTextA("");
            }
        }
    }
    i32 b = g_watchBlinkB + 1;
    g_watchBlinkB = b;
    if (b > 0x31) {
        g_watchBlinkB = 0;
    }
    if (g_multiState->m_sessionTerminated != 0) {
        ::KillTimer(m_hWnd, 1);
        g_multiState->ReportVersionMsg("terminated", 0);
        g_watchBusy = 0;
        return;
    }
    if (g_multiState->m_568 != 0) {
        g_multiState->m_568 = 0;
        g_multiState->ReportVersionMsg("selected", 0);
        g_watchBusy = 0;
        return;
    }
    char* msg;
    if (g_multiState->m_538 != 0) {
        ::KillTimer(m_hWnd, 1);
        msg = "removed";
    } else if (g_multiState->m_5ac != 0) {
        ::KillTimer(m_hWnd, 1);
        msg = "closed";
    } else if (g_multiState->m_56c != 0) {
        ::KillTimer(m_hWnd, 1);
        msg = "full";
    } else if (g_multiState->m_570 != 0) {
        ::KillTimer(m_hWnd, 1);
        msg = "version";
    } else {
        if (g_playerLeftFlag != 0) {
            Sync16db(1);
            Sync227a();
            UpdateColorItems();
            Sync38d2();
            g_playerLeftFlag = 0;
        }
        if (g_multiState->m_58c != 0) {
            Sync227a();
            UpdateColorItems();
            Sync38d2();
            g_multiState->m_58c = 0;
        }
        g_watchBusy = 0;
        return;
    }
    g_multiState->ReportVersionMsg(msg, 0);
    M1bab37(0);
    g_watchBusy = 0;
}

// GetSlotIndex (0xc4b30): resolve the local player's options slot through the host
// facet (m_host->FindOptionsSlot(g_multiState->m_hostIndex)); return the stored slot value
// or -1 when absent. __thiscall. Re-homed from src/Stub/ReconBatch2.cpp (was the
// OptOwner_c4b30::Resolve view; m_5c is CMultiStartDlg::m_host, xref-proven).
RVA(0x000c4b30, 0x1f)
i32 CMultiStartDlg::GetSlotIndex() {
    i32* slot = (i32*)(reinterpret_cast<CGruntzMgr*>(m_host))->FindOptionsSlot(g_multiState->m_hostIndex);
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
// before the match starts, confirm every player has the same custom
// level; otherwise re-enable the dialog and pop the appropriate error modal.
RVA(0x000c4c00, 0x190)
void CMultiStartDlg::VerifyCustomLevel() {
    CMulti* mgr = g_multiState;
    if (mgr->m_isHost == 0) {
        return;
    }
    mgr->SendStatFlag(0x3fc, 1);
    i32 token;
    if (g_multiState->m_5b0 != 0) {
        CString b = GetConfigNameB();
        token = ((CGruntzMgr*)g_gameReg)->BuildLevelRezPath(0, g_multiState->m_5b0, 0, 0, b);
    } else {
        CString a = GetConfigNameA();
        token = ((CGruntzMgr*)g_gameReg)->BuildLevelRezPath(0, g_multiState->m_5b0, 0, 0, a);
    }
    g_multiState->m_levelVerifyResult = 0;
    if (g_multiState->Poll(token) == 0) {
        g_multiState->m_530 = 0;
        EnableWindow(0);
        ((CGruntzMgr*)(void*)g_gameReg)
            ->EnterModalUI("Unable to verify custom level with other players");
        EnableWindow(1);
    } else if (g_multiState->m_levelVerifyResult == 0) {
        g_multiState->m_530 = 1;
        CDialog::OnOK(); // 0x1bacc3 direct base call (?OnOK@CDialog@@MAEXXZ, reloc-masked)
    } else {
        g_multiState->m_530 = 0;
        EnableWindow(0);
        ((CGruntzMgr*)(void*)g_gameReg)
            ->EnterModalUI("Not all players have the (same) custom level.");
        EnableWindow(1);
    }
}

// __thiscall(): cache list N's current selection (+1) into the Nth player-slot's combo
// value, then re-drive the connect state. Four handlers, one per player slot; slot 2
// each resolves its list through the real CMultiStartDlg::GetCtrlC accessor (0xc27c0).
RVA(0x000c4ee0, 0x33)
void CMultiStartDlg::OnSlotSelect0() {
    HWND h = GetCtrlC(0)->m_hWnd;
    g_gameReg->m_focusSlots[0].m_228 = ::SendMessageA(h, 0x147, 0, 0) + 1;
    Drive();
}

RVA(0x000c4f30, 0x33)
void CMultiStartDlg::OnSlotSelect1() {
    HWND h = GetCtrlC(1)->m_hWnd;
    g_gameReg->m_focusSlots[1].m_228 = ::SendMessageA(h, 0x147, 0, 0) + 1;
    Drive();
}

RVA(0x000c4f80, 0x33)
void CMultiStartDlg::OnSlotSelect2() {
    HWND h = GetCtrlC(2)->m_hWnd;
    g_gameReg->m_focusSlots[2].m_228 = ::SendMessageA(h, 0x147, 0, 0) + 1;
    Drive();
}

RVA(0x000c4fd0, 0x33)
void CMultiStartDlg::OnSlotSelect3() {
    HWND h = GetCtrlC(3)->m_hWnd;
    g_gameReg->m_focusSlots[3].m_228 = ::SendMessageA(h, 0x147, 0, 0) + 1;
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
    if (g_multiState->m_isHost == 0) {
        return;
    }
    i32 lo, hi;
    i32 h = GetSafe1c();
    GetSelItemData((HWND)h, 0x527, &lo, &hi);
    if (lo != 0 || hi != 0) {
        g_multiState->m_5a4 = lo;
        g_multiState->m_drainReload = hi;
        g_multiState->m_600 = 0;
        g_multiState->SaveConfig(0);
    } else {
        g_multiState->m_600 = 1;
    }
}

// __thiscall(idx): toggle slot idx's ready flag from its checkbox, then either re-sync
// the whole roster (host) or just refresh that one slot.
// @early-stop
// regalloc coin-flip wall (~97.7%, docs/patterns/zero-register-pinning.md family): the
// full logic is byte-faithful, but the final slot lea overwrites the INDEX register
// (ecx) here while retail overwrites the BASE register (edx) - so retail carries the
// slot pointer in edx, this cl in ecx, cascading through the g_multiState/m_isHost load
// pair (ecx/eax vs eax/edx) and the Refresh185c arg push (push edx vs push ecx). A pure
// allocator choice, no source lever.
RVA(0x000c50f0, 0x9b)
void CMultiStartDlg::ToggleReady(i32 idx) {
    CWnd* it = ResolveItem_1159(idx);
    if (!it) {
        return;
    }
    i32 sel = ::SendMessageA(it->m_hWnd, 0xf0, 0, 0);
    CFocusSlot* slot = (CFocusSlot*)((char*)g_gameReg + idx * 0x238 + 0x150);
    if (!slot) {
        return;
    }
    if (sel) {
        slot->m_1c = 1;
    } else {
        slot->m_1c = 0;
    }
    if (g_multiState->m_isHost) {
        Func1d70(0);
        Sync16db(1);
        Sync227a();
        UpdateColorItems();
        Sync38d2();
    } else {
        Refresh185c(slot);
    }
}

// ---------------------------------------------------------------------------
// CMultiStartDlg::DestroyWindow (0x0c5240): vtable slot 24 (== ??_7CMultiStartDlg@@6B@
// +0x60, reached via ILT thunk 0x218a; declared in Dialogs.h as the own CWnd override).
// The former CCluster0c @orphan is DISSOLVED: the vtable DATA-ref proved this is
// CMultiStartDlg's own DestroyWindow. It frees the +0x60 connection-latency slot list
// (m_slotList, a CLatencyList : CKeyedList) via the container's CKeyedList teardown at
// 0xc5280 (the CNetThing == CKeyedList unification is now DONE - the dtor lives on
// CKeyedList, so no cast is needed), then chains CWnd::DestroyWindow.
// ---------------------------------------------------------------------------
RVA(0x000c5240, 0x2c)
i32 CMultiStartDlg::DestroyWindow() {
    CLatencyList* p = m_slotList; // +0x60 connection-latency slot list
    if (p) {
        p->CKeyedList::~CKeyedList(); // 0xc5280 (qualified, non-virtual - direct near call)
        ::operator delete(p);         // 0x1b9b82 == ??3@YAXPAX@Z (reloc-masked/exempt)
        m_slotList = 0;
    }
    return CWnd::DestroyWindow(); // 0x1bbb7c ?DestroyWindow@CWnd@@UAEHXZ (base, eax passthrough)
}

// EchoLatencySettings (0xc52f0): print the current session CmdDelay (m_5a4) and
// ResendDelay (m_drainReload) to the chat log via wsprintfA into a stack buffer.
RVA(0x000c52f0, 0x43)
void CMultiStartDlg::EchoLatencySettings() {
    char buf[128];
    wsprintfA(buf, s_UsingCmdDelay, g_multiState->m_5a4, g_multiState->m_drainReload);
    AppendChatLine(buf);
}

// (The 0xc5f00 "NetConfigureBe90" that was parked here is really
//  CObjectDropper::InitActReg - the dropped-object TU's registry construct; it
//  lives in src/Gruntz/DroppedObject.cpp (wave2-H, private-globals-oracle-proven).
//  This TU's retail extent ends at 0xc5333 + its 8-frag init run @0xc5360.)

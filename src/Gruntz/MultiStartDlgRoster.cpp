#include <Gruntz/Dialogs.h>
#include <Gruntz/GameRegMfcPtr.h> // g_gameReg at its REAL type (CGruntzMgr)
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/Random.h> // g_randSeed/g_randSeeded (FlashCtrlD's swatch colour)
#include <EmptyString.h>   // g_emptyString
#include <Gruntz/Multi.h>  // the real CMulti (the 0x64bd5c multiplayer game-state singleton)
#include <Gruntz/GruntzMgr.h> // CGruntzMgr::FindOptionsSlot (0x92e80, the m_host FindOptionsSlot callee)
#include <Gruntz/GameRegistry.h> // the canonical g_gameReg spine (CGameRegistry, VA 0x64556c)
#include <Net/LatencyList.h>     // CLatencyList : CKeyedList (m_slotList; its dtor is 0xc5280)
#include <Net/NetMgr.h>          // CNetMgr::BroadcastChatLine (0xbb190), the chat-broadcast facet
#include <rva.h>
#include <string.h> // strcat (inline CRT, reloc-masked)

#include <Gruntz/GruntzPlayer.h> // canonical GruntzPlayer (GetName)

DATA(0x0021243c)
char s_UsingCmdDelay[] = "Using CmdDelay of %d and ResendDelay of %d.";

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

RVA(0x000c2980, 0x28)
void CMultiStartDlg::SetListCurSel(i32 id, i32 wParam) {
    CWnd* it = GetCtrlC(id);
    if (it) {
        ::SendMessageA(it->m_hWnd, 0x14e, wParam - 1, 0);
    }
}

RVA(0x000c29f0, 0x13)
void CMultiStartDlg::ReconcileChannel0() {
    SyncChannelSlot(0);
    Drive();
}

RVA(0x000c2a20, 0x13)
void CMultiStartDlg::ConnectStep() {
    SyncChannelSlot(1);
    Drive();
}

RVA(0x000c2a50, 0x13)
void CMultiStartDlg::ReconcileChannel2() {
    SyncChannelSlot(2);
    Drive();
}

RVA(0x000c2a80, 0x13)
void CMultiStartDlg::ReconcileChannel3() {
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
    CWnd* owner = GetCtrlE(ch); // 0x1929  the list whose selection drives the slot
    CWnd* c1 = GetCtrlB(ch);
    CWnd* c2 = GetCtrlD(ch); // 0x30da -> 0xc2840
    GetCtrlC(ch);
    GetCtrlA(ch);
    GruntzPlayer* s = &m_host->m_options[ch];
    LRESULT(WINAPI * pSend)(HWND, UINT, WPARAM, LPARAM) = ::SendMessageA;
    if (pSend(owner->m_hWnd, 0x147, 0, 0) == 0) {
        if (s->m_014 != 0) {
            if (s->m_liveGate != 0) {
                g_multiState->DropChannelPlayer(s->m_playerIndex);
            }
        } else if (s->m_liveGate != 0) {
            ChannelSlots_Set(s->m_008, 1);
        }
        s->m_liveGate = 0;
        s->m_readyFlag = 0;
        c1->EnableWindow(0);
        c2->EnableWindow(0);
    } else {
        if (pSend(owner->m_hWnd, 0x147, 0, 0) != 4) {
            if (s->m_014 != 0) {
                if (s->m_liveGate != 0) {
                    g_multiState->DropChannelPlayer(s->m_playerIndex);
                }
                i32 free = ChannelSlots_FindFree();
                s->m_008 = free;
                ChannelSlots_Set(free, 0);
            } else if (s->m_liveGate == 0) {
                i32 free = ChannelSlots_FindFree();
                s->m_008 = free;
                ChannelSlots_Set(free, 0);
            }
            s->m_readyFlag = 1;
            s->m_014 = 0;
            s->m_configId = static_cast<i32>(pSend(owner->m_hWnd, 0x147, 0, 0)) - 1;
            s->m_liveGate = 1;
            s->m_name = g_gruntNames[ch];
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
        cts(it->m_hWnd, reinterpret_cast<LPPOINT>(&rc));
        cts(it->m_hWnd, reinterpret_cast<LPPOINT>(&rc) + 1);
        stc(m_hWnd, reinterpret_cast<LPPOINT>(&rc));
        stc(m_hWnd, reinterpret_cast<LPPOINT>(&rc) + 1);
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
                switch (m_host->m_options[0].m_008) {
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
                switch (m_host->m_options[1].m_008) {
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
                switch (m_host->m_options[2].m_008) {
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
                switch (m_host->m_options[3].m_008) {
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
    if ((mp->m_isHost == 0 || m_host->m_options[0].m_014 != 0)
        && (m_host->m_options[0].m_readyFlag != 0
            || m_host->m_options[0].m_slotKey != mp->m_hostIndex)) {
        return;
    }
    CBattlezDlgColors dlg(m_host, 0, 1, 0);
    if (dlg.DoModal() == 1) {
        if (SelectColor(0, dlg.m_pickedColor)) {
            Drive();
            HWND h = GetDlgItem(0x501)->m_hWnd;
            ::InvalidateRect(h, 0, 1);
        }
    }
}

RVA(0x000c3950, 0xd1)
void CMultiStartDlg::OnColorSlot1() {
    CMulti* mp = g_multiState;
    if ((mp->m_isHost == 0 || m_host->m_options[1].m_014 != 0)
        && (m_host->m_options[1].m_readyFlag != 0
            || m_host->m_options[1].m_slotKey != mp->m_hostIndex)) {
        return;
    }
    CBattlezDlgColors dlg(m_host, 1, 1, 0);
    if (dlg.DoModal() == 1) {
        if (SelectColor(1, dlg.m_pickedColor)) {
            Drive();
            HWND h = GetDlgItem(0x503)->m_hWnd;
            ::InvalidateRect(h, 0, 1);
        }
    }
}

RVA(0x000c3a70, 0xd1)
void CMultiStartDlg::OnColorSlot2() {
    CMulti* mp = g_multiState;
    if ((mp->m_isHost == 0 || m_host->m_options[2].m_014 != 0)
        && (m_host->m_options[2].m_readyFlag != 0
            || m_host->m_options[2].m_slotKey != mp->m_hostIndex)) {
        return;
    }
    CBattlezDlgColors dlg(m_host, 2, 1, 0);
    if (dlg.DoModal() == 1) {
        if (SelectColor(2, dlg.m_pickedColor)) {
            Drive();
            HWND h = GetDlgItem(0x505)->m_hWnd;
            ::InvalidateRect(h, 0, 1);
        }
    }
}

RVA(0x000c3b90, 0xd1)
void CMultiStartDlg::OnColorSlot3() {
    CMulti* mp = g_multiState;
    if ((mp->m_isHost == 0 || m_host->m_options[3].m_014 != 0)
        && (m_host->m_options[3].m_readyFlag != 0
            || m_host->m_options[3].m_slotKey != mp->m_hostIndex)) {
        return;
    }
    CBattlezDlgColors dlg(m_host, 3, 1, 0);
    if (dlg.DoModal() == 1) {
        if (SelectColor(3, dlg.m_pickedColor)) {
            Drive();
            HWND h = GetDlgItem(0x507)->m_hWnd;
            ::InvalidateRect(h, 0, 1);
        }
    }
}

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
            child->SetWindowTextA(static_cast<LPCTSTR>(dlg.m_customName));
            m_6c = 1;
            g_multiState->m_5b0 = 1;
            g_multiState->m_5b8 = static_cast<LPCTSTR>(dlg.m_customName);
            g_multiState->m_5b4 = g_emptyString;
            g_multiState->SaveConfig(0);
        }
    }
}

RVA(0x000c3e30, 0xfe)
void CMultiStartDlg::CommitWorldHost() {
    if (g_multiState->m_isHost != 0) {
        CWnd* item = GetDlgItem(0x4ff);
        if (item != 0) {
            i32 r = ::SendMessageA(item->m_hWnd, 0x147, 0, 0);
            if (r != -1) {
                CString name;
                (static_cast<CComboBox*>(item))
                    ->GetLBText(r, name); // CComboBox::GetLBText @0x1ce7db
                if (name.GetLength() != 0) {
                    m_6c = 0;
                }
                g_multiState->m_5b0 = 0;
                g_multiState->m_5b8 = g_emptyString;
                g_multiState->m_5b4 = static_cast<LPCTSTR>(name);
                g_multiState->SaveConfig(0);
            }
        }
    }
}

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

RVA(0x000c40b0, 0x42)
void CMultiStartDlg::Drive() {
    CMulti* netMgr = g_multiState;
    if (netMgr->m_isHost != 0) {
        netMgr->BroadcastChannelTable(0);
        UpdatePlayers(1); // 0xc4230 (reloc-masked; return discarded)
    } else {
        g_multiState->BroadcastOneChannel(m_host->FindOptionsSlot(netMgr->m_hostIndex));
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
    i32 t = this->GetSlotIndex();
    i32 localColour = g_multiState->m_isHost ? m_host->m_options[t].m_readyFlag : 1;
    i32 off = 0;
    do {
        GruntzPlayer* slot = &g_gameReg->m_options[idx];
        if (slot) {
            if (slot->m_slotKey != g_multiState->m_hostIndex && slot->m_014 && slot->m_liveGate) {
                f18 = 1;
            }
            i32 enName;
            if (g_multiState->m_isHost && slot->m_014 == 0) {
                enName = 1;
            } else {
                enName = slot->m_slotKey == g_multiState->m_hostIndex ? 1 : 0;
            }
            GetCtrlB(idx)->EnableWindow(enName);
            GetCtrlE(idx)->EnableWindow(
                g_multiState->m_isHost && localColour == 0
                        && slot->m_slotKey != g_multiState->m_hostIndex
                    ? 1
                    : 0
            );
            CWnd* ready = GetCtrlA(idx);
            ready->EnableWindow(slot->m_slotKey == g_multiState->m_hostIndex ? 1 : 0);
            if (slot->m_readyFlag) {
                if (slot->m_liveGate) {
                    ::SendMessageA(ready->m_hWnd, 0xf1, 1, 0);
                } else {
                    ::SendMessageA(ready->m_hWnd, 0xf1, 0, 0);
                }
            } else if (slot->m_liveGate) {
                ::SendMessageA(ready->m_hWnd, 0xf1, 0, 0);
                f1c = 0;
            } else {
                ::SendMessageA(ready->m_hWnd, 0xf1, 0, 0);
            }
            GetCtrlC(idx)->EnableWindow(
                g_multiState->m_isHost && slot->m_liveGate && localColour == 0 ? 1 : 0
            );
            SetListCurSel(idx, slot->m_liveGate ? slot->m_comboSel : 0);
            if (force == 0) {
                if (this->GetSlotIndex() == idx) {
                    goto next;
                }
                if (g_multiState->m_isHost && slot->m_014 == 0) {
                    goto next;
                }
            }
            if (slot->m_liveGate) {
                {
                    CString name = slot->GetName();
                    LPCTSTR pch = static_cast<LPCTSTR>(name);
                    force = 0;
                    GetCtrlB(idx)->SetWindowTextA(pch);
                }
                if (slot->m_014) {
                    ::SendMessageA(GetCtrlE(idx)->m_hWnd, 0x14e, 4, 0);
                } else {
                    ::SendMessageA(GetCtrlE(idx)->m_hWnd, 0x14e, slot->m_configId + 1, 0);
                }
            } else {
                GetCtrlB(idx)->SetWindowTextA(g_emptyString);
                ::SendMessageA(GetCtrlE(idx)->m_hWnd, 0x14e, 0, 0);
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
    void* h = g_multiState->m_netGate->m_playerSel;
    if (h == 0) {
        return;
    }
    g_multiState->m_netGate->EnumGroupsRange(h, 0);
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
            EndDialog(1);
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
            GruntzPlayer* slot = &g_gameReg->m_options[i];
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
            if (slot->m_liveGate != 0 && slot->m_014 != 0) {
                char buf[0x20];
                wsprintfA(buf, "%d", slot->m_latency);
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
            UpdatePlayers(1);
            EnableControls();
            UpdateColorItems();
            UpdateSlot();
            g_playerLeftFlag = 0;
        }
        if (g_multiState->m_58c != 0) {
            EnableControls();
            UpdateColorItems();
            UpdateSlot();
            g_multiState->m_58c = 0;
        }
        g_watchBusy = 0;
        return;
    }
    g_multiState->ReportVersionMsg(msg, 0);
    EndDialog(0);
    g_watchBusy = 0;
}

RVA(0x000c4b30, 0x1f)
i32 CMultiStartDlg::GetSlotIndex() {
    i32* slot = reinterpret_cast<i32*>(m_host->FindOptionsSlot(g_multiState->m_hostIndex));
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
void CMultiStartDlg::OnOK() {
    CMulti* mgr = g_multiState;
    if (mgr->m_isHost == 0) {
        return;
    }
    mgr->SendStatFlag(0x3fc, 1);
    i32 token;
    if (g_multiState->m_5b0 != 0) {
        CString b = mgr->GetConfigNameB();
        token = (g_gameReg)->BuildLevelRezPath(0, g_multiState->m_5b0, 0, 0, b);
    } else {
        CString a = mgr->GetConfigNameA();
        token = (g_gameReg)->BuildLevelRezPath(0, g_multiState->m_5b0, 0, 0, a);
    }
    g_multiState->m_levelVerifyResult = 0;
    if (g_multiState->Poll(token) == 0) {
        g_multiState->m_530 = 0;
        EnableWindow(0);
        (static_cast<CGruntzMgr*>(static_cast<void*>(g_gameReg)))
            ->EnterModalUI("Unable to verify custom level with other players");
        EnableWindow(1);
    } else if (g_multiState->m_levelVerifyResult == 0) {
        g_multiState->m_530 = 1;
        CDialog::OnOK(); // 0x1bacc3 direct base call (?OnOK@CDialog@@MAEXXZ, reloc-masked)
    } else {
        g_multiState->m_530 = 0;
        EnableWindow(0);
        (static_cast<CGruntzMgr*>(static_cast<void*>(g_gameReg)))
            ->EnterModalUI("Not all players have the (same) custom level.");
        EnableWindow(1);
    }
}

RVA(0x000c4ee0, 0x33)
void CMultiStartDlg::OnSlotSelect0() {
    HWND h = GetCtrlC(0)->m_hWnd;
    g_gameReg->m_options[0].m_comboSel = ::SendMessageA(h, 0x147, 0, 0) + 1;
    Drive();
}

RVA(0x000c4f30, 0x33)
void CMultiStartDlg::OnSlotSelect1() {
    HWND h = GetCtrlC(1)->m_hWnd;
    g_gameReg->m_options[1].m_comboSel = ::SendMessageA(h, 0x147, 0, 0) + 1;
    Drive();
}

RVA(0x000c4f80, 0x33)
void CMultiStartDlg::OnSlotSelect2() {
    HWND h = GetCtrlC(2)->m_hWnd;
    g_gameReg->m_options[2].m_comboSel = ::SendMessageA(h, 0x147, 0, 0) + 1;
    Drive();
}

RVA(0x000c4fd0, 0x33)
void CMultiStartDlg::OnSlotSelect3() {
    HWND h = GetCtrlC(3)->m_hWnd;
    g_gameReg->m_options[3].m_comboSel = ::SendMessageA(h, 0x147, 0, 0) + 1;
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
    GetSelItemData(reinterpret_cast<HWND>(h), 0x527, &lo, &hi);
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
RVA(0x000c50f0, 0x9b)
void CMultiStartDlg::ToggleReady(i32 idx) {
    CWnd* it = GetCtrlA(idx);
    if (!it) {
        return;
    }
    i32 sel = ::SendMessageA(it->m_hWnd, 0xf0, 0, 0);
    GruntzPlayer* slot = &g_gameReg->m_options[idx];
    if (!slot) {
        return;
    }
    if (sel) {
        slot->m_readyFlag = 1;
    } else {
        slot->m_readyFlag = 0;
    }
    if (g_multiState->m_isHost) {
        g_multiState->BroadcastChannelTable(0);
        UpdatePlayers(1);
        EnableControls();
        UpdateColorItems();
        UpdateSlot();
    } else {
        g_multiState->BroadcastOneChannel(slot);
    }
}

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

RVA(0x000c52f0, 0x43)
void CMultiStartDlg::EchoLatencySettings() {
    char buf[128];
    wsprintfA(buf, s_UsingCmdDelay, g_multiState->m_5a4, g_multiState->m_drainReload);
    AppendChatLine(buf);
}

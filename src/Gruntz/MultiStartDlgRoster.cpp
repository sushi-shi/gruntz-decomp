#include <rva.h>

#include <EmptyString.h>
#include <Enums.h>
#include <Gruntz/ColorTint.h>
#include <Gruntz/Dialogs.h>
#include <Gruntz/GameRand.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/Multi.h>
#include <Gruntz/MultiStartDlg.h>
#include <Gruntz/Random.h>
#include <MsgParam.h>
#include <Net/LatencyList.h>
#include <Net/NetMgr.h>

#include <string.h>

DATA(0x0024bdb0)
CString g_gruntNames[4] = {"Beefy", "Zed", "Serra", "Jebediah"};

DATA(0x0024bdc0)
i32 g_savedMultiWndProc = 0;

DATA(0x0024bd5c)
CMulti* g_multiState;

DATA(0x0024bdc4)
i32 g_watchBusy;
DATA(0x0024bdc8)
i32 g_watchBlinkA;
DATA(0x0024bdcc)
i32 g_watchBlinkB;

#include <Gruntz/GruntzPlayer.h>
#include <Gruntz/GruntzCmdMgr.h>
#include <Net/KeyedList.h>
#include <Ints.h>
#include <Net/NetLobbyCtrlId.h>

DATA(0x0021243c)
char s_UsingCmdDelay[] = "Using CmdDelay of %d and ResendDelay of %d.";

RVA(0x00038220, 0x73)
i32 __stdcall GetSelItemData(HWND hDlg, i32 id, i32* outLo, i32* outHi) {
    HWND list = GetDlgItem(hDlg, id);
    if (!list) {
        return 0;
    }
    i32 sel = SendMessageA(list, CB_GETCURSEL, 0, 0);
    if (sel == -1) {
        return 0;
    }
    i32 data = SendMessageA(list, CB_GETITEMDATA, sel, 0);
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
        ::SendMessageA(it->m_hWnd, CB_SETCURSEL, wParam - 1, 0);
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
RVA(0x000c2ab0, 0x161)
void CMultiStartDlg::SyncChannelSlot(i32 ch) {
    CWnd* owner = GetCtrlE(ch);
    CWnd* c1 = GetCtrlB(ch);
    CWnd* c2 = GetCtrlD(ch);
    GetCtrlC(ch);
    GetCtrlA(ch);
    GruntzPlayer* s = &m_host->m_options[ch];
    LRESULT(WINAPI * pSend)(HWND, UINT, WPARAM, LPARAM) = ::SendMessageA;
    if (pSend(owner->m_hWnd, CB_GETCURSEL, 0, 0) == 0) {
        if (s->m_humanControlled != 0) {
            if (s->m_liveGate != 0) {
                g_multiState->DropChannelPlayer(s->m_playerIndex);
            }
        } else if (s->m_liveGate != 0) {
            ChannelSlots_Set(IDX(s->m_colorIndex), 1);
        }
        s->m_liveGate = 0;
        s->m_readyFlag = 0;
        c1->EnableWindow(0);
        c2->EnableWindow(0);
    } else {
        if (static_cast<MultiplayerPlayerKind>(pSend(owner->m_hWnd, CB_GETCURSEL, 0, 0))
            != MULTI_PLAYER_HUMAN) {
            if (s->m_humanControlled != 0) {
                if (s->m_liveGate != 0) {
                    g_multiState->DropChannelPlayer(s->m_playerIndex);
                }
                i32 free = ChannelSlots_FindFree();
                s->m_colorIndex = static_cast<ColorTint>(free);
                ChannelSlots_Set(free, 0);
            } else if (s->m_liveGate == 0) {
                i32 free = ChannelSlots_FindFree();
                s->m_colorIndex = static_cast<ColorTint>(free);
                ChannelSlots_Set(free, 0);
            }
            s->m_readyFlag = 1;
            s->m_humanControlled = 0;
            s->m_configId = static_cast<i32>(pSend(owner->m_hWnd, CB_GETCURSEL, 0, 0)) - 1;
            s->m_liveGate = 1;
            s->m_name = g_gruntNames[ch];
        }
        c1->EnableWindow(1);
        c2->EnableWindow(1);
    }
}

RVA(0x000c2cb0, 0x1f)
i32 CMultiStartDlg::OnInitDialog() {
    CDialog::OnInitDialog();
    ::SetTimer(m_hWnd, 1, 0x32, 0);
    return 1;
}

RVA(0x000c2ce0, 0xf3)
void CMultiStartDlg::AppendChatLine(char* str) {
    CWnd* item = GetDlgItem(0x511);
    HWND edit;
    if (!item) {
        edit = NULL;
    } else {
        edit = item->m_hWnd;
    }
    if (!edit || !str || !str[0]) {
        return;
    }
    i32 len = ::GetWindowTextLengthA(edit);
    if (len == 0) {
        ::SendMessageA(edit, EM_SETSEL, len, -1);
    } else {
        ::SendMessageA(edit, EM_SETSEL, len, len);
    }
    char buf[0x80];
    buf[0] = 0;
    if (len > 0) {
        strcat(buf, "\r\n");
    }
    strcat(buf, str);
    MsgParam text;
    text.m_str = buf;
    ::SendMessageA(edit, EM_REPLACESEL, 0, text.m_lparam);
    ::SendMessageA(edit, EM_LINESCROLL, 0, 0x270f);
}

// @early-stop
RVA(0x000c2e20, 0x21d)
i32 CMultiStartDlg::FlashCtrlD() {
    CPaintDC dc(this);
    BOOL(WINAPI * cts)(HWND, LPPOINT) = ::ClientToScreen;
    BOOL(WINAPI * stc)(HWND, LPPOINT) = ::ScreenToClient;
    for (i32 i = 0; i < 4; i++) {
        CWnd* it = GetCtrlD(i);
        if (it == NULL) {
            continue;
        }

        CRect rc;
        ::GetClientRect(it->m_hWnd, &rc);
        cts(it->m_hWnd, &rc.TopLeft());
        cts(it->m_hWnd, &rc.BottomRight());
        stc(m_hWnd, &rc.TopLeft());
        stc(m_hWnd, &rc.BottomRight());
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
        scratch.Attach(CreateSolidBrush(color));
        FillRect(dc.m_hDC, &rc, scratch);
    }
    return 1;
}

RVA(0x000c3100, 0x5c0)
void CMultiStartDlg::OnDrawItem(i32 nIDCtl, DRAWITEMSTRUCT* lpdis) {
    COLORREF color;
    i32 bDraw = 0;
    switch (nIDCtl) {
        case CTRL_PLAYER_COLOR0:
            if (GetCtrlD(0)->IsWindowEnabled()) {
                switch (m_host->m_options[0].m_colorIndex) {
                    case TINT_DKBLUE:
                        color = 0x800000;
                        break;
                    case TINT_DKGREEN:
                        color = 0x008000;
                        break;
                    case TINT_TURQ:
                        color = 0x808000;
                        break;
                    case TINT_DKRED:
                        color = 0x000080;
                        break;
                    case TINT_PURPLE:
                        color = 0x800080;
                        break;
                    case TINT_DKYELLOW:
                        color = 0x008080;
                        break;
                    case TINT_GREY:
                        color = 0x808080;
                        break;
                    case TINT_BLUE:
                        color = 0xff0000;
                        break;
                    case TINT_GREEN:
                        color = 0x00ff00;
                        break;
                    case TINT_CYAN:
                        color = 0xffff00;
                        break;
                    case TINT_RED:
                        color = 0x0000ff;
                        break;
                    case TINT_PINK:
                        color = 0xff00ff;
                        break;
                    case TINT_YELLOW:
                        color = 0x00ffff;
                        break;
                    case TINT_WHITE:
                        color = 0xffffff;
                        break;
                    case TINT_ORANGE:
                        color = 0x0080ff;
                        break;
                    case TINT_HOTPINK:
                        color = 0x8000ff;
                        break;
                    case TINT_BLACK:
                    default:
                        color = 0;
                        break;
                }
            } else {
                color = 0xc8c8c8;
            }
            bDraw = 1;
            break;
        case CTRL_PLAYER_COLOR1:
            if (GetCtrlD(1)->IsWindowEnabled()) {
                switch (m_host->m_options[1].m_colorIndex) {
                    case TINT_DKBLUE:
                        color = 0x800000;
                        break;
                    case TINT_DKGREEN:
                        color = 0x008000;
                        break;
                    case TINT_TURQ:
                        color = 0x808000;
                        break;
                    case TINT_DKRED:
                        color = 0x000080;
                        break;
                    case TINT_PURPLE:
                        color = 0x800080;
                        break;
                    case TINT_DKYELLOW:
                        color = 0x008080;
                        break;
                    case TINT_GREY:
                        color = 0x808080;
                        break;
                    case TINT_BLUE:
                        color = 0xff0000;
                        break;
                    case TINT_GREEN:
                        color = 0x00ff00;
                        break;
                    case TINT_CYAN:
                        color = 0xffff00;
                        break;
                    case TINT_RED:
                        color = 0x0000ff;
                        break;
                    case TINT_PINK:
                        color = 0xff00ff;
                        break;
                    case TINT_YELLOW:
                        color = 0x00ffff;
                        break;
                    case TINT_WHITE:
                        color = 0xffffff;
                        break;
                    case TINT_ORANGE:
                        color = 0x0080ff;
                        break;
                    case TINT_HOTPINK:
                        color = 0x8000ff;
                        break;
                    case TINT_BLACK:
                    default:
                        color = 0;
                        break;
                }
            } else {
                color = 0xc8c8c8;
            }
            bDraw = 1;
            break;
        case CTRL_PLAYER_COLOR2:
            if (GetCtrlD(2)->IsWindowEnabled()) {
                switch (m_host->m_options[2].m_colorIndex) {
                    case TINT_DKBLUE:
                        color = 0x800000;
                        break;
                    case TINT_DKGREEN:
                        color = 0x008000;
                        break;
                    case TINT_TURQ:
                        color = 0x808000;
                        break;
                    case TINT_DKRED:
                        color = 0x000080;
                        break;
                    case TINT_PURPLE:
                        color = 0x800080;
                        break;
                    case TINT_DKYELLOW:
                        color = 0x008080;
                        break;
                    case TINT_GREY:
                        color = 0x808080;
                        break;
                    case TINT_BLUE:
                        color = 0xff0000;
                        break;
                    case TINT_GREEN:
                        color = 0x00ff00;
                        break;
                    case TINT_CYAN:
                        color = 0xffff00;
                        break;
                    case TINT_RED:
                        color = 0x0000ff;
                        break;
                    case TINT_PINK:
                        color = 0xff00ff;
                        break;
                    case TINT_YELLOW:
                        color = 0x00ffff;
                        break;
                    case TINT_WHITE:
                        color = 0xffffff;
                        break;
                    case TINT_ORANGE:
                        color = 0x0080ff;
                        break;
                    case TINT_HOTPINK:
                        color = 0x8000ff;
                        break;
                    case TINT_BLACK:
                    default:
                        color = 0;
                        break;
                }
            } else {
                color = 0xc8c8c8;
            }
            bDraw = 1;
            break;
        case CTRL_PLAYER_COLOR3:
            if (GetCtrlD(3)->IsWindowEnabled()) {
                switch (m_host->m_options[3].m_colorIndex) {
                    case TINT_DKBLUE:
                        color = 0x800000;
                        break;
                    case TINT_DKGREEN:
                        color = 0x008000;
                        break;
                    case TINT_TURQ:
                        color = 0x808000;
                        break;
                    case TINT_DKRED:
                        color = 0x000080;
                        break;
                    case TINT_PURPLE:
                        color = 0x800080;
                        break;
                    case TINT_DKYELLOW:
                        color = 0x008080;
                        break;
                    case TINT_GREY:
                        color = 0x808080;
                        break;
                    case TINT_BLUE:
                        color = 0xff0000;
                        break;
                    case TINT_GREEN:
                        color = 0x00ff00;
                        break;
                    case TINT_CYAN:
                        color = 0xffff00;
                        break;
                    case TINT_RED:
                        color = 0x0000ff;
                        break;
                    case TINT_PINK:
                        color = 0xff00ff;
                        break;
                    case TINT_YELLOW:
                        color = 0x00ffff;
                        break;
                    case TINT_WHITE:
                        color = 0xffffff;
                        break;
                    case TINT_ORANGE:
                        color = 0x0080ff;
                        break;
                    case TINT_HOTPINK:
                        color = 0x8000ff;
                        break;
                    case TINT_BLACK:
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
        CDC dc;
        dc.Attach(lpdis->hDC);
        CBrush brush(color);
        FillRect(dc.m_hDC, &lpdis->rcItem, brush);
        dc.Detach();
    }
    CWnd::OnDrawItem(nIDCtl, lpdis);
}

RVA(0x000c3830, 0xd1)
void CMultiStartDlg::OnColorSlot0() {
    CMulti* mp = g_multiState;
    if ((mp->m_isHost == 0 || m_host->m_options[0].m_humanControlled != 0)
        && (m_host->m_options[0].m_readyFlag != 0
            || m_host->m_options[0].m_slotKey != mp->m_hostIndex)) {
        return;
    }
    CBattlezDlgColors dlg(m_host, 0, 1, 0);
    if (dlg.DoModal() == 1) {
        if (SelectColor(0, static_cast<ColorTint>(dlg.m_pickedColor))) {
            Drive();
            GetDlgItem(CTRL_PLAYER_COLOR0)->InvalidateRect(0, 1);
        }
    }
}

RVA(0x000c3950, 0xd1)
void CMultiStartDlg::OnColorSlot1() {
    CMulti* mp = g_multiState;
    if ((mp->m_isHost == 0 || m_host->m_options[1].m_humanControlled != 0)
        && (m_host->m_options[1].m_readyFlag != 0
            || m_host->m_options[1].m_slotKey != mp->m_hostIndex)) {
        return;
    }
    CBattlezDlgColors dlg(m_host, 1, 1, 0);
    if (dlg.DoModal() == 1) {
        if (SelectColor(1, static_cast<ColorTint>(dlg.m_pickedColor))) {
            Drive();
            GetDlgItem(CTRL_PLAYER_COLOR1)->InvalidateRect(0, 1);
        }
    }
}

RVA(0x000c3a70, 0xd1)
void CMultiStartDlg::OnColorSlot2() {
    CMulti* mp = g_multiState;
    if ((mp->m_isHost == 0 || m_host->m_options[2].m_humanControlled != 0)
        && (m_host->m_options[2].m_readyFlag != 0
            || m_host->m_options[2].m_slotKey != mp->m_hostIndex)) {
        return;
    }
    CBattlezDlgColors dlg(m_host, 2, 1, 0);
    if (dlg.DoModal() == 1) {
        if (SelectColor(2, static_cast<ColorTint>(dlg.m_pickedColor))) {
            Drive();
            GetDlgItem(CTRL_PLAYER_COLOR2)->InvalidateRect(0, 1);
        }
    }
}

RVA(0x000c3b90, 0xd1)
void CMultiStartDlg::OnColorSlot3() {
    CMulti* mp = g_multiState;
    if ((mp->m_isHost == 0 || m_host->m_options[3].m_humanControlled != 0)
        && (m_host->m_options[3].m_readyFlag != 0
            || m_host->m_options[3].m_slotKey != mp->m_hostIndex)) {
        return;
    }
    CBattlezDlgColors dlg(m_host, 3, 1, 0);
    if (dlg.DoModal() == 1) {
        if (SelectColor(3, static_cast<ColorTint>(dlg.m_pickedColor))) {
            Drive();
            GetDlgItem(CTRL_PLAYER_COLOR3)->InvalidateRect(0, 1);
        }
    }
}

RVA(0x000c3cb0, 0x128)
void CMultiStartDlg::OnCustomWorld() {
    if (g_multiState->m_isHost == 0) {
        return;
    }
    CBattlezDlgCustom dlg(0);
    if (dlg.DoModal() == 1 && dlg.m_customName.GetLength() != 0) {

        CWnd* item = GetDlgItem(0x4ff);
        CWnd* child = CWnd::FromHandle(::GetWindow(item->m_hWnd, GW_CHILD));

        if (child == NULL) {
            return;
        }
        dlg.m_customName.MakeUpper();
        child->SetWindowTextA(static_cast<LPCTSTR>(dlg.m_customName));
        m_customWorldFlag = 1;
        g_multiState->m_customLevel = 1;
        g_multiState->m_customLevelName = static_cast<LPCTSTR>(dlg.m_customName);
        g_multiState->m_builtInLevelName = g_emptyString;
        g_multiState->SaveConfig(0);
    }
}

RVA(0x000c3e30, 0xfe)
void CMultiStartDlg::CommitWorldHost() {
    if (g_multiState->m_isHost != 0) {
        CWnd* item = GetDlgItem(0x4ff);
        if (item != NULL) {
            i32 r = ::SendMessageA(item->m_hWnd, CB_GETCURSEL, 0, 0);
            if (r != -1) {
                CString name;
                (static_cast<CComboBox*>(item))->GetLBText(r, name);
                if (name.GetLength() != 0) {
                    m_customWorldFlag = 0;
                }
                g_multiState->m_customLevel = 0;
                g_multiState->m_customLevelName = g_emptyString;
                g_multiState->m_builtInLevelName = static_cast<LPCTSTR>(name);
                g_multiState->SaveConfig(0);
            }
        }
    }
}

RVA(0x000c3f70, 0xfb)
void CMultiStartDlg::OnChatSend() {
    CWnd* input = GetDlgItem(0x42d);
    if (input == NULL) {
        return;
    }
    CString a, b;
    GetCtrlB(GetSlotIndex())->GetWindowTextA(a);
    a += " says: ";
    input->GetWindowTextA(b);
    if (b.GetLength() != 0) {
        a += b;
        AppendChatLine(const_cast<char*>(static_cast<const char*>(a)));
        input->SetWindowTextA(g_emptyString);
        g_multiState->BroadcastChatLine(const_cast<char*>(static_cast<const char*>(a)), 0, 0, 0);
    }
}

RVA(0x000c40b0, 0x42)
void CMultiStartDlg::Drive() {
    CMulti* netMgr = g_multiState;
    if (netMgr->m_isHost != 0) {
        netMgr->BroadcastChannelTable(0);
        UpdatePlayers(1);
    } else {
        g_multiState->BroadcastOneChannel(m_host->FindOptionsSlot(netMgr->m_hostIndex));
    }
}

// @early-stop
RVA(0x000c4120, 0xc2)
i32 CMultiStartDlg::EnableControls() {
    GetDlgItem(IDCANCEL)->EnableWindow(1);
    GetDlgItem(IDX(IDC_NETCHAT_SEND))->EnableWindow(1);
    GetDlgItem(0x42d)->EnableWindow(1);
    GetDlgItem(0x511)->EnableWindow(1);
    CString s1;
    if (g_multiState->m_customLevel == 0) {
        CString s2;
    }
    return 1;
}

// @early-stop
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
            if (slot->m_slotKey != g_multiState->m_hostIndex && slot->m_humanControlled
                && slot->m_liveGate) {
                f18 = 1;
            }
            i32 enName;
            if (g_multiState->m_isHost && slot->m_humanControlled == 0) {
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
                    ::SendMessageA(ready->m_hWnd, BM_SETCHECK, 1, 0);
                } else {
                    ::SendMessageA(ready->m_hWnd, BM_SETCHECK, 0, 0);
                }
            } else if (slot->m_liveGate) {
                ::SendMessageA(ready->m_hWnd, BM_SETCHECK, 0, 0);
                f1c = 0;
            } else {
                ::SendMessageA(ready->m_hWnd, BM_SETCHECK, 0, 0);
            }
            GetCtrlC(idx)->EnableWindow(
                g_multiState->m_isHost && slot->m_liveGate && localColour == 0 ? 1 : 0
            );
            SetListCurSel(idx, slot->m_liveGate ? slot->m_comboSel : 0);
            if (force == 0) {
                if (this->GetSlotIndex() == idx) {
                    goto next;
                }
                if (g_multiState->m_isHost && slot->m_humanControlled == 0) {
                    goto next;
                }
            }
            if (slot->m_liveGate) {
                {
                    force = 0;
                    GetCtrlB(idx)->SetWindowTextA(slot->GetName());
                }
                if (slot->m_humanControlled) {
                    ::SendMessageA(GetCtrlE(idx)->m_hWnd, CB_SETCURSEL, 4, 0);
                } else {
                    ::SendMessageA(GetCtrlE(idx)->m_hWnd, CB_SETCURSEL, slot->m_configId + 1, 0);
                }
            } else {
                GetCtrlB(idx)->SetWindowTextA(g_emptyString);
                ::SendMessageA(GetCtrlE(idx)->m_hWnd, CB_SETCURSEL, 0, 0);
            }
            this->SyncChannelSlot(idx);
        }
    next:
        off += 0x238;
        idx++;
    } while (off < 0x8e0);
    if (g_multiState->m_isHost) {
        CWnd* ok = this->GetDlgItem(1);
        if (ok == NULL) {
            return 0;
        }
        ok->EnableWindow(f18 & f1c);
    }
    ::InvalidateRect(this->GetDlgItem(CTRL_PLAYER_COLOR0)->m_hWnd, 0, 1);
    ::InvalidateRect(this->GetDlgItem(CTRL_PLAYER_COLOR1)->m_hWnd, 0, 1);
    ::InvalidateRect(this->GetDlgItem(CTRL_PLAYER_COLOR2)->m_hWnd, 0, 1);
    ::InvalidateRect(this->GetDlgItem(CTRL_PLAYER_COLOR3)->m_hWnd, 0, 1);
    return 1;
}

// @early-stop
RVA(0x000c46b0, 0x384)
void CMultiStartDlg::Watchdog() {
    if (g_watchBusy != 0) {
        return;
    }
    g_watchBusy = 1;
    void* h = g_multiState->m_netGate->m_playerSel;
    if (h == NULL) {
        return;
    }
    g_multiState->m_netGate->EnumGroupsRange(h, 0);
    g_multiState->ResolveLocalPlayer();
    if (g_watchBlinkA == 0) {
        u32 t = timeGetTime();
        g_multiState->SendNetStat(NETMSG_STAT_REQUEST, static_cast<i32>(t), 0);
    }
    if (g_multiState->m_isHost == 0) {
        if (g_watchBlinkA == 0) {
            g_multiState->ReportAckLatency();
        }
        EnableWindow(0);
        i32 r = g_multiState->VerifyCustomLevel(h, g_multiState->m_localPlayer);
        EnableWindow(1);
        if (r != 0) {
            EndDialog(1);
            g_watchBusy = 0;
            return;
        }
    } else {
        g_multiState->PollSession();
        if (g_multiState->m_autoCommandDelay != 0) {
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
            switch (static_cast<PlayerSlot>(i)) {
                case PLAYER_SLOT_0:
                    item1 = GetDlgItem(CTRL_PLAYER_LATENCY0);
                    item2 = GetDlgItem(CTRL_PLAYER_READY0);
                    break;
                case PLAYER_SLOT_1:
                    item1 = GetDlgItem(CTRL_PLAYER_LATENCY1);
                    item2 = GetDlgItem(CTRL_PLAYER_READY1);
                    break;
                case PLAYER_SLOT_2:
                    item1 = GetDlgItem(CTRL_PLAYER_LATENCY2);
                    item2 = GetDlgItem(CTRL_PLAYER_READY2);
                    break;
                case PLAYER_SLOT_3:
                    item1 = GetDlgItem(CTRL_PLAYER_LATENCY3);
                    item2 = GetDlgItem(CTRL_PLAYER_READY3);
                    break;
            }
            if (slot->m_liveGate != 0 && slot->m_humanControlled != 0) {
                char buf[0x20];
                wsprintfA(buf, "%d", slot->m_latency.m_avg);
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
    if (g_multiState->m_colorSelectionRejected != 0) {
        g_multiState->m_colorSelectionRejected = 0;
        g_multiState->ReportVersionMsg("selected", 0);
        g_watchBusy = 0;
        return;
    }
    char* msg;
    if (g_multiState->m_removedByHost != 0) {
        ::KillTimer(m_hWnd, 1);
        msg = "removed";
    } else if (g_multiState->m_gameClosed != 0) {
        ::KillTimer(m_hWnd, 1);
        msg = "closed";
    } else if (g_multiState->m_gameFull != 0) {
        ::KillTimer(m_hWnd, 1);
        msg = "full";
    } else if (g_multiState->m_versionMismatch != 0) {
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
        if (g_multiState->m_connectAccepted != 0) {
            EnableControls();
            UpdateColorItems();
            UpdateSlot();
            g_multiState->m_connectAccepted = 0;
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
    GruntzPlayer* slot = m_host->FindOptionsSlot(g_multiState->m_hostIndex);
    if (slot == NULL) {
        return -1;
    }
    return slot->m_playerIndex;
}

// @early-stop
RVA(0x000c4b60, 0x77)
i32 CMultiStartDlg::SelectColor(i32 colorIndex, ColorTint playerColor) {
    GruntzPlayer* colorSlot = &m_host->m_options[colorIndex];
    if (g_multiState->m_isHost != 0) {
        i32 r = ChannelSlots_Get(IDX(playerColor));
        if (r == 0) {
            g_multiState->ReportVersionMsg("Someone has already selected that color.", r);
            return 0;
        }
        ChannelSlots_Set(IDX(colorSlot->m_colorIndex), 1);
        ChannelSlots_Set(IDX(playerColor), 0);
    }
    colorSlot->m_colorIndex = playerColor;
    return 1;
}

// @early-stop
RVA(0x000c4c00, 0x190)
void CMultiStartDlg::OnOK() {
    CMulti* mgr = g_multiState;
    if (mgr->m_isHost == 0) {
        return;
    }
    if (&CMulti::GetCommandDelay == NULL) {
        return;
    }
    if (&CMulti::GetResendDelay == NULL) {
        return;
    }
    mgr->SendStatFlag(NETMSG_VERIFY_CUSTOM_LEVEL, 1);
    CString levelName =
        g_multiState->m_customLevel != 0 ? mgr->GetConfigNameB() : mgr->GetConfigNameA();
    i32 token = (g_gameReg)->BuildLevelRezPath(0, g_multiState->m_customLevel, 0, 0, levelName);
    g_multiState->m_levelVerifyResult = 0;
    if (g_multiState->Poll(token) == 0) {
        g_multiState->m_customLevelVerificationPending = 0;
        EnableWindow(0);
        g_gameReg->EnterModalUI(
            "Unable to verify custom level with other players. The game will not start."
        );
        EnableWindow(1);
    } else if (g_multiState->m_levelVerifyResult != 0) {
        g_multiState->m_customLevelVerificationPending = 1;
        CDialog::OnOK();
    } else {
        g_multiState->m_customLevelVerificationPending = 0;
        EnableWindow(0);
        g_gameReg->EnterModalUI("Not all players have the (same) custom level.");
        EnableWindow(1);
    }
}

RVA(0x000c4e00, 0x7)
i32 CMulti::GetCommandDelay() {
    return m_commandDelay;
}

RVA(0x000c4e20, 0x7)
i32 CMulti::GetResendDelay() {
    return m_drainReload;
}

RVA(0x000c4ee0, 0x33)
void CMultiStartDlg::OnSlotSelect0() {
    HWND h = GetCtrlC(0)->m_hWnd;
    g_gameReg->m_options[0].m_comboSel = ::SendMessageA(h, CB_GETCURSEL, 0, 0) + 1;
    Drive();
}

RVA(0x000c4f30, 0x33)
void CMultiStartDlg::OnSlotSelect1() {
    HWND h = GetCtrlC(1)->m_hWnd;
    g_gameReg->m_options[1].m_comboSel = ::SendMessageA(h, CB_GETCURSEL, 0, 0) + 1;
    Drive();
}

RVA(0x000c4f80, 0x33)
void CMultiStartDlg::OnSlotSelect2() {
    HWND h = GetCtrlC(2)->m_hWnd;
    g_gameReg->m_options[2].m_comboSel = ::SendMessageA(h, CB_GETCURSEL, 0, 0) + 1;
    Drive();
}

RVA(0x000c4fd0, 0x33)
void CMultiStartDlg::OnSlotSelect3() {
    HWND h = GetCtrlC(3)->m_hWnd;
    g_gameReg->m_options[3].m_comboSel = ::SendMessageA(h, CB_GETCURSEL, 0, 0) + 1;
    Drive();
}

// @early-stop
RVA(0x000c5020, 0x95)
void CMultiStartDlg::CommitLatencyOption() {
    if (g_multiState->m_isHost == 0) {
        return;
    }
    i32 lo, hi;
    HWND h = GetSafe1c();
    GetSelItemData(h, 0x527, &lo, &hi);
    if (lo != 0 || hi != 0) {
        g_multiState->m_commandDelay = lo;
        g_multiState->m_drainReload = hi;
        g_multiState->m_autoCommandDelay = 0;
        g_multiState->SaveConfig(0);
    } else {
        g_multiState->m_autoCommandDelay = 1;
    }
}

RVA(0x000c50f0, 0x9b)
void CMultiStartDlg::ToggleReady(i32 idx) {
    CWnd* it = GetCtrlA(idx);
    if (!it) {
        return;
    }
    i32 sel = ::SendMessageA(it->m_hWnd, BM_GETCHECK, 0, 0);
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
    CLatencyList* p = m_slotList;
    if (p) {
        p->CKeyedList::~CKeyedList();
        ::operator delete(p);
        m_slotList = NULL;
    }
    return CWnd::DestroyWindow();
}

RVA(0x000c5280, 0x49)
CKeyedList::~CKeyedList() {
    Clear();
}
RVA(0x000c52f0, 0x43)
void CMultiStartDlg::EchoLatencySettings() {
    char buf[128];
    wsprintfA(buf, s_UsingCmdDelay, g_multiState->m_commandDelay, g_multiState->m_drainReload);
    AppendChatLine(buf);
}

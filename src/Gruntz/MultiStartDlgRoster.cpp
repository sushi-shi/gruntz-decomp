// MultiStartDlgRoster.cpp - the CMultiStartDlg multiplayer roster/refresh method
// cluster (the 0xc2000-0xc5000 band), re-homed from the ApiCaller stubs. Every one
// of these ran on the ONE multiplayer-start dialog (CMultiStartDlg): the former
// per-function ApiCaller host views (SelHost_0c4ee0 / SelHost_0c4f80 /
// RosterHost_0c50f0 / BattlezDlg_c4230 / EditAppendHost_0c2ce0) were placeholder
// duplicates of this class, proven by their shared control accessors (GetCtrlA..D
// @0xc26c0/40/c0/40) and self-call into Drive (@0xc40b0). The net-game-config facets
// CNetGameDlg (NetGameDlg.cpp) / CNetConnCoord (NetMgrMisc.cpp) were the SAME dialog
// too and are now FOLDED into CMultiStartDlg (matcher-5): their methods dispatch this
// class's per-slot accessors (0x1929/0x298c/GetCtrlD/0x1753/0x1159) on `this` and
// self-call Drive/UpdatePlayers - bodies stay in their own units (delinker packing).
//
// Homed in its own unit (not Dialogs.cpp) so it can't perturb that TU's parked dtors;
// it reuses the shared Dialogs.h dialog models, the canonical CGameRegistry spine
// (GameRegistry.h) and the canonical CMulti game-state (Multi.h). Field names are
// placeholders (m_<hexoffset>); only offsets + code bytes are load-bearing.
#include <Gruntz/Dialogs.h>
#include <Gruntz/Multi.h>        // the real CMulti (the 0x64bd5c multiplayer game-state singleton)
#include <Gruntz/GameRegistry.h> // the canonical g_gameReg spine (CGameRegistry, VA 0x64556c)
#include <rva.h>
#include <string.h> // strcat (inline CRT, reloc-masked)

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

// __stdcall(id, wParam): if item `id` resolves, set its list selection to wParam-1
// (LB_SETCURSEL). Free helper preserving the caller's ecx=this (see resolvers above).
RVA(0x000c2980, 0x28)
void __stdcall SetListCurSel(i32 id, i32 wParam) {
    CWnd* it = ResolveItem_1753(id);
    if (it) {
        SendMessageA(it->m_hWnd, 0x14e, wParam - 1, 0);
    }
}

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
                    CString name = slot->FormatName_3e54();
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

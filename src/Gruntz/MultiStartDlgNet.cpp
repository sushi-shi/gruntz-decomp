// MultiStartDlgNet.cpp - two more CMultiStartDlg multiplayer-setup handlers homed from
// the GapFunctions stubs: the chat-send button (0xc3f70) and the battle-latency combo
// commit (0xc5020). Own /GX eh unit so they cannot perturb the matched sibling TUs; the
// shared Dialogs.h dialog models + CMulti game-state are reused. Every callee/global is
// external (reloc-masked); field names are placeholders, only offsets + code bytes are
// load-bearing.
#include <Gruntz/Dialogs.h>
#include <Gruntz/Multi.h> // the real CMulti (the 0x64bd5c multiplayer game-state singleton)
#include <Net/NetMgr.h>   // CNetMgr::BroadcastChatLine (0xbb190), the chat-broadcast facet
#include <rva.h>

// The multiplayer game-state singleton (a CMulti, xref-proven). DATA reloc-masks
// against ReconBatch2's home.
DATA(0x0024bd5c)
extern CMulti* g_64bd5c;
// "Using CmdDelay of %d and ResendDelay of %d\n" (the EchoLatencySettings format).
DATA(0x0024243c)
extern char s_UsingCmdDelay[];
// The shared empty-string literal (0x6293f4; homed in NetMgrReportError.cpp).
extern "C" char g_emptyString[];
// The generic listbox-selection splitter (0x38220, __stdcall; body in
// MultiStartDlgRoster.cpp): reads control `id`'s selected item data into lo/hi words.
i32 __stdcall GetSelItemData(HWND hDlg, i32 id, i32* outLo, i32* outHi); // 0x38220

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

// EchoLatencySettings (0xc52f0): print the current session CmdDelay (m_5a4) and
// ResendDelay (m_drainReload) to the chat log via wsprintfA into a stack buffer.
RVA(0x000c52f0, 0x43)
void CMultiStartDlg::EchoLatencySettings() {
    char buf[128];
    wsprintfA(buf, s_UsingCmdDelay, g_64bd5c->m_5a4, g_64bd5c->m_drainReload);
    AppendChatLine(buf);
}

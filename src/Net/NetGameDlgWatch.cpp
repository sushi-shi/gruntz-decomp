// NetGameDlgWatch.cpp - CNetDlgWatch::Watchdog (0x0c46b0): the multiplayer lobby
// dialog's per-timer session watchdog. Guarded by a re-entrancy flag (g_64bdc4),
// it refreshes the per-slot roster display, advances two blink counters
// (g_64bdc8 / g_64bdcc), then walks the net-session status flags and, on any
// terminal condition, kills the poll timer, pops a status message, and tears down.
//
// Self-contained views: the dialog (`this`), the net-session singleton (*0x64bd5c)
// and the game-registry slot array (*0x64556c) are modeled with ONLY the offsets
// this method touches. Engine callees + Win32 imports are external (reloc-masked);
// field/class names are placeholders (campaign doctrine).
#include <Mfc.h>          // wsprintfA / KillTimer (imports reloc-mask through their IAT ptrs)
#include <Gruntz/Multi.h> // the real CMulti (the 0x64bd5c multiplayer game-state singleton)
#include <Ints.h>
#include <rva.h>

// The game-registry singleton pointer (the scoring symbol CPlay reaches too).
extern "C" void* g_mgrSettings; // 0x64556c

// A child dialog control (GetDlgItem result); SetWindowTextA is a NAFXCW __thiscall.
struct WatchCtrl {
    void SetWindowTextA(const char* text); // 0x1be520 (reloc-masked)
};

// The multiplayer game-state singleton at 0x64bd5c is a CMulti. The former per-TU
// WatchSess / WatchSess524 lens types (same-memory aliases of this pointer and its
// +0x524 report gate) are gone: the status flags are genuine CMulti members
// (m_isHost/m_sessionTerminated/m_538/m_568/m_56c/m_570/m_58c/m_5ac/m_5bc/m_600) and the terminal
// helpers are genuine CMulti / CMultiReportGate methods - see <Gruntz/Multi.h>.
extern CMulti* g_64bd5c; // 0x64bd5c

// The game-registry slot array (*0x64556c + 0x150, stride 0x238/slot).
struct WatchRegSlot {
    char m_pad00[0x14];
    i32 m_14; // +0x14 present flag
    char m_pad18[0x20 - 0x18];
    i32 m_20; // +0x20 active flag
    char m_pad24[0x22c - 0x24];
    i32 m_22c; // +0x22c display value
    char m_tail[0x238 - 0x230];
};
struct WatchReg {
    char m_pad000[0x150];
    WatchRegSlot m_slots[1]; // +0x150
};

// The cached timeGetTime fn-ptr (DATA symbol; 0-arg, bound by m5_PaletteLerp).
extern u32(WINAPI* g_pTimeGetTime)(); // 0x6c4650

// Watchdog re-entrancy guard + two blink counters (.data).
extern "C" i32 g_watchBusy;      // 0x64bdc4
extern "C" i32 g_watchBlinkA;    // 0x64bdc8
extern "C" i32 g_watchBlinkB;    // 0x64bdcc
extern "C" i32 g_playerLeftFlag; // 0x648ce4

// The lobby dialog whose window handle drives KillTimer + roster refresh.
struct CNetDlgWatch {
    char m_pad00[0x1c];
    HWND m_hWnd; // +0x1c

    WatchCtrl* GetDlgItem(i32 id); // 0x1be27d (CWnd::GetDlgItem)
    void EnableWindow(i32 enable); // 0x1be6a7 (CWnd::EnableWindow on this)
    void M1bab37(i32);             // 0x1bab37
    void M16db(i32);               // 0x16db
    void M227a();                  // 0x227a
    void M2c0c();                  // 0x2c0c
    void M38d2();                  // 0x38d2

    void Watchdog(); // 0x0c46b0
};

// ===========================================================================
// CNetDlgWatch::Watchdog  (0x0c46b0)
// ===========================================================================
// @early-stop
// ~94% regalloc-coloring wall (all control flow + calls + the DIR32 globals pair):
// retail re-materializes the zero constant into ebx after the roster loop
// (`xor ebx,ebx`) and reuses it for the state-chain `push 0` (M1af0 2nd arg) +
// the m_58c store, whereas MSVC5 here emits push-immediates / colors the m_hWnd
// KillTimer temps into ecx/edx instead of retail's eax/ecx; plus a 2-instr
// timeGetTime `this`-load schedule. Not source-steerable. docs/patterns/zero-register-pinning.md.
RVA(0x000c46b0, 0x371)
void CNetDlgWatch::Watchdog() {
    if (g_watchBusy != 0) {
        return;
    }
    g_watchBusy = 1;
    void* h = g_64bd5c->m_netGate->m_player;
    if (h == 0) {
        return;
    }
    g_64bd5c->m_netGate->M178a80(h, 0);
    g_64bd5c->ResolveLocalPlayer();
    if (g_watchBlinkA == 0) {
        u32 t = g_pTimeGetTime();
        g_64bd5c->SendNetStat(0x41f, (i32)t, 0);
    }
    if (g_64bd5c->m_isHost == 0) {
        if (g_watchBlinkA == 0) {
            g_64bd5c->ReportAckLatency();
        }
        EnableWindow(0);
        i32 r = g_64bd5c->VerifyCustomLevel(h, g_64bd5c->m_5bc);
        EnableWindow(1);
        if (r != 0) {
            M1bab37(1);
            g_watchBusy = 0;
            return;
        }
    } else {
        g_64bd5c->PollSession();
        if (g_64bd5c->m_600 != 0) {
            g_64bd5c->AutoTuneCmdDelay();
        }
    }
    i32 a = g_watchBlinkA + 1;
    g_watchBlinkA = a;
    if (a > 3) {
        g_watchBlinkA = 0;
    }
    if (g_watchBlinkB == 0) {
        for (i32 i = 0; i < 4; i++) {
            WatchRegSlot* slot = &((WatchReg*)g_mgrSettings)->m_slots[i];
            WatchCtrl* item1;
            WatchCtrl* item2;
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
    if (g_64bd5c->m_sessionTerminated != 0) {
        KillTimer(m_hWnd, 1);
        g_64bd5c->ReportVersionMsg("terminated", 0);
        g_watchBusy = 0;
        return;
    }
    if (g_64bd5c->m_568 != 0) {
        g_64bd5c->m_568 = 0;
        g_64bd5c->ReportVersionMsg("selected", 0);
        g_watchBusy = 0;
        return;
    }
    char* msg;
    if (g_64bd5c->m_538 != 0) {
        KillTimer(m_hWnd, 1);
        msg = "removed";
    } else if (g_64bd5c->m_5ac != 0) {
        KillTimer(m_hWnd, 1);
        msg = "closed";
    } else if (g_64bd5c->m_56c != 0) {
        KillTimer(m_hWnd, 1);
        msg = "full";
    } else if (g_64bd5c->m_570 != 0) {
        KillTimer(m_hWnd, 1);
        msg = "version";
    } else {
        if (g_playerLeftFlag != 0) {
            M16db(1);
            M227a();
            M2c0c();
            M38d2();
            g_playerLeftFlag = 0;
        }
        if (g_64bd5c->m_58c != 0) {
            M227a();
            M2c0c();
            M38d2();
            g_64bd5c->m_58c = 0;
        }
        g_watchBusy = 0;
        return;
    }
    g_64bd5c->ReportVersionMsg(msg, 0);
    M1bab37(0);
    g_watchBusy = 0;
}

SIZE_UNKNOWN(CNetDlgWatch);
SIZE_UNKNOWN(WatchCtrl);
SIZE_UNKNOWN(WatchReg);
SIZE_UNKNOWN(WatchRegSlot);

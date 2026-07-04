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
#include <Mfc.h> // wsprintfA / KillTimer (imports reloc-mask through their IAT ptrs)
#include <Ints.h>
#include <rva.h>

// The game-registry singleton pointer (the scoring symbol CPlay reaches too).
extern "C" void* g_64556c; // 0x64556c

// A child dialog control (GetDlgItem result); SetWindowTextA is a NAFXCW __thiscall.
struct WatchCtrl {
    void SetWindowTextA(const char* text); // 0x1be520 (reloc-masked)
};

// The net-session singleton reached through *0x64bd5c.  Its status flags gate the
// watchdog's terminal branches; the m_524 sub-object holds the poll worker.
struct WatchSess524 {
    char m_pad00[0x74];
    void* m_74;                 // +0x74  poll worker handle
    void M178a80(void* h, i32); // 0x178a80 (thiscall on this sub-object)
};
struct WatchSess {
    char m_pad000[0x524];
    WatchSess524* m_524; // +0x524 poll worker owner
    i32 m_528;           // +0x528 active flag
    i32 m_52c;           // +0x52c terminated flag
    char m_pad530[0x538 - 0x530];
    i32 m_538; // +0x538 removed flag
    char m_pad53c[0x568 - 0x53c];
    i32 m_568; // +0x568 selection-taken flag
    i32 m_56c; // +0x56c full flag
    i32 m_570; // +0x570 version-mismatch flag
    char m_pad574[0x58c - 0x574];
    i32 m_58c; // +0x58c stat-reset gate
    char m_pad590[0x5ac - 0x590];
    i32 m_5ac; // +0x5ac closed flag
    char m_pad5b0[0x5bc - 0x5b0];
    i32 m_5bc; // +0x5bc poll token
    char m_pad5c0[0x600 - 0x5c0];
    i32 m_600; // +0x600 abort gate

    // reloc-masked __thiscall leaves (thunks / engine methods):
    void M2338();                     // 0x2338
    void M2955(i32 a, i32 b, i32 c);  // 0x2955
    void M360c();                     // 0x360c
    i32 M1cee(void* h, i32 token);    // 0x1cee -> worker handle
    void M2c39();                     // 0x2c39
    void M2365();                     // 0x2365
    void M1af0(const char* msg, i32); // 0x1af0 show status message
};

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

// The net-session singleton pointer (canonical DATA symbol owned by ReconBatch2.cpp).
struct OptCfg_c4b30;
extern OptCfg_c4b30* g_optCfg_64bd5c; // 0x64bd5c
inline WatchSess* Sess() {
    return (WatchSess*)g_optCfg_64bd5c;
}

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
    void* h = Sess()->m_524->m_74;
    if (h == 0) {
        return;
    }
    Sess()->m_524->M178a80(h, 0);
    Sess()->M2338();
    if (g_watchBlinkA == 0) {
        u32 t = g_pTimeGetTime();
        Sess()->M2955(0x41f, (i32)t, 0);
    }
    if (Sess()->m_528 == 0) {
        if (g_watchBlinkA == 0) {
            Sess()->M360c();
        }
        EnableWindow(0);
        i32 r = Sess()->M1cee(h, Sess()->m_5bc);
        EnableWindow(1);
        if (r != 0) {
            M1bab37(1);
            g_watchBusy = 0;
            return;
        }
    } else {
        Sess()->M2c39();
        if (Sess()->m_600 != 0) {
            Sess()->M2365();
        }
    }
    i32 a = g_watchBlinkA + 1;
    g_watchBlinkA = a;
    if (a > 3) {
        g_watchBlinkA = 0;
    }
    if (g_watchBlinkB == 0) {
        for (i32 i = 0; i < 4; i++) {
            WatchRegSlot* slot = &((WatchReg*)g_64556c)->m_slots[i];
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
    if (Sess()->m_52c != 0) {
        KillTimer(m_hWnd, 1);
        Sess()->M1af0("terminated", 0);
        g_watchBusy = 0;
        return;
    }
    if (Sess()->m_568 != 0) {
        Sess()->m_568 = 0;
        Sess()->M1af0("selected", 0);
        g_watchBusy = 0;
        return;
    }
    const char* msg;
    if (Sess()->m_538 != 0) {
        KillTimer(m_hWnd, 1);
        msg = "removed";
    } else if (Sess()->m_5ac != 0) {
        KillTimer(m_hWnd, 1);
        msg = "closed";
    } else if (Sess()->m_56c != 0) {
        KillTimer(m_hWnd, 1);
        msg = "full";
    } else if (Sess()->m_570 != 0) {
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
        if (Sess()->m_58c != 0) {
            M227a();
            M2c0c();
            M38d2();
            Sess()->m_58c = 0;
        }
        g_watchBusy = 0;
        return;
    }
    Sess()->M1af0(msg, 0);
    M1bab37(0);
    g_watchBusy = 0;
}

SIZE_UNKNOWN(CNetDlgWatch);
SIZE_UNKNOWN(WatchCtrl);
SIZE_UNKNOWN(WatchReg);
SIZE_UNKNOWN(WatchRegSlot);
SIZE_UNKNOWN(WatchSess);
SIZE_UNKNOWN(WatchSess524);

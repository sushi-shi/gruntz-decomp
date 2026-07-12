// TimeSplit.cpp - millisecond -> H:M:S splitter (0x119210).
//
// Unsigned division-by-constant: hours = n/3600000, minutes = (n%3600000)/60000,
// seconds = (n%60000)/1000, each written through an out-param. MSVC lowers the three
// constant divides to magic-multiply and strength-reduces the `% C` remainders to
// lea/shift multiplies of the quotient. Owner unidentified (free __cdecl helper).
//
// Also here (RVA-adjacent free __cdecl helper, same retail .obj): the WM_SYSCOMMAND
// screen-saver / monitor-power blocker (0x1192d0).
#include <Win32.h> // IsIconic / HWND
#include <rva.h>

RVA(0x00119210, 0x66)
void SplitMillisToHMS(unsigned n, unsigned* hh, unsigned* mm, unsigned* ss) {
    unsigned q1 = n / 3600000;
    *hh = q1;
    n -= q1 * 3600000;
    unsigned q2 = n / 60000;
    *mm = q2;
    n -= q2 * 60000;
    *ss = n / 1000;
}

// BlockScreenSaver (0x1192d0, __cdecl): return 1 to swallow a SC_SCREENSAVE/
// SC_MONITORPOWER WM_SYSCOMMAND while the window is not iconic. (Re-homed from
// ApiMiscHelpers.) A DlgProc pre-handler - its only callers are the Net/LobbyDialogs
// DlgProcs, which pass all four dialog params (lParam unused here), so the retail
// signature is 4-arg; keeping it 4-arg makes those calls bind to this RVA.
RVA(0x001192d0, 0x39)
i32 BlockScreenSaver(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == 0x112) {
        i32 sc = wParam & 0xfff0;
        if (sc == 0xf140 || sc == 0xf170) {
            if (!IsIconic(hWnd)) {
                return 1;
            }
        }
    }
    return 0;
}

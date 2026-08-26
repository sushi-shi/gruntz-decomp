#include <rva.h>

#include <Win32.h>

#include <Utils/MillisPer.h>

RVA(0x00119420, 0x66)
void SplitMillisToHMS(unsigned n, unsigned* hh, unsigned* mm, unsigned* ss) {
    unsigned q1 = n / MILLIS_PER_HOUR;
    *hh = q1;
    n -= q1 * MILLIS_PER_HOUR;
    unsigned q2 = n / MILLIS_PER_MINUTE;
    *mm = q2;
    n -= q2 * MILLIS_PER_MINUTE;
    *ss = n / MILLIS_PER_SECOND;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x001194b0, 0x1d)
void TerminateString(char* text, i32 limit) {
    i32 i = 0;
    while (i < limit && *text != 0) {
        text++;
        i++;
    }
    *text = 0;
}

RVA(0x001194e0, 0x39)
i32 BlockScreenSaver(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_SYSCOMMAND) {
        i32 sc = wParam & 0xfff0;
        if (sc == SC_SCREENSAVE || sc == SC_MONITORPOWER) {
            if (!IsIconic(hWnd)) {
                return 1;
            }
        }
    }
    return 0;
}

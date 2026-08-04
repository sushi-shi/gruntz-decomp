#include <rva.h>

#include <Win32.h>

#include <Utils/MillisPer.h>

RVA(0x00119210, 0x66)
void SplitMillisToHMS(unsigned n, unsigned* hh, unsigned* mm, unsigned* ss) {
    unsigned q1 = n / MILLIS_PER_HOUR;
    *hh = q1;
    n -= q1 * MILLIS_PER_HOUR;
    unsigned q2 = n / MILLIS_PER_MINUTE;
    *mm = q2;
    n -= q2 * MILLIS_PER_MINUTE;
    *ss = n / MILLIS_PER_SECOND;
}

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

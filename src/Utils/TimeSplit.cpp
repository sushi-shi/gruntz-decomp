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

// CScanRectInit.h - the shared rect-init helper (0x34a4) that fills a RECT and
// returns it. Modeled NO-body so the call reloc-masks. Placeholder name; only the
// signature + emitted code bytes are load-bearing.
#ifndef GRUNTZ_GRUNTZ_CSCANRECTINIT_H
#define GRUNTZ_GRUNTZ_CSCANRECTINIT_H

#include <rva.h>

#include <Win32.h> // RECT

struct CScanRectInit { // 0x34a4 - init a rect + return it
    RECT* Set34a4(i32 l, i32 t, i32 r, i32 b);
};

#endif // GRUNTZ_GRUNTZ_CSCANRECTINIT_H

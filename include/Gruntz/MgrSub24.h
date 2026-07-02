// MgrSub24.h - the shared settings sub-object at MgrSub30+0x24 (the board-state
// record); +0x5c is the board base pointer. Reached only as a pointer, so the
// exact +0x5c type is cosmetic (placeholder). Only the offsets are load-bearing.
#ifndef GRUNTZ_GRUNTZ_MGRSUB24_H
#define GRUNTZ_GRUNTZ_MGRSUB24_H

#include <rva.h>

struct MgrSub24 {
    char pad[0x5c];
    char* m_5c; // +0x5c board base
};

#endif // GRUNTZ_GRUNTZ_MGRSUB24_H

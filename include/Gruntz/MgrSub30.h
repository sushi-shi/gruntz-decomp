// MgrSub30.h - the shared settings sub-object at MgrSettings+0x30 whose +0x24 holds
// the MgrSub24 record (forward-declared; each TU that needs its fields defines it
// locally). Placeholder name; only the offsets are load-bearing.
#ifndef GRUNTZ_GRUNTZ_MGRSUB30_H
#define GRUNTZ_GRUNTZ_MGRSUB30_H

#include <rva.h>

struct MgrSub24;

struct MgrSub30 {
    char pad[0x24];
    MgrSub24* m_24;
};

#endif // GRUNTZ_GRUNTZ_MGRSUB30_H

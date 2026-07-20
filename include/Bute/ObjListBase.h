#ifndef GRUNTZ_BUTE_OBJLISTBASE_H
#define GRUNTZ_BUTE_OBJLISTBASE_H

#include <Ints.h>
#include <rva.h>

SIZE(CObjListBase, 0x4);
VTBL(CObjListBase, 0x001ef760);
struct CObjListBase {
    virtual void V0() = 0; // slot 0 (__purecall)
};

#endif // GRUNTZ_BUTE_OBJLISTBASE_H

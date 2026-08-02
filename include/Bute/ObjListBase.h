#ifndef GRUNTZ_BUTE_OBJLISTBASE_H
#define GRUNTZ_BUTE_OBJLISTBASE_H

#include <rva.h>

#include <Ints.h>

VTBL(CObjListBase, 0x001ef760);
struct CObjListBase {
    virtual void UnusedListHook() = 0;

    ~CObjListBase() {}
};
SIZE(0x4);

#endif // GRUNTZ_BUTE_OBJLISTBASE_H

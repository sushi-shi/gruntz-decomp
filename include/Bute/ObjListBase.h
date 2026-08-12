#ifndef GRUNTZ_BUTE_OBJLISTBASE_H
#define GRUNTZ_BUTE_OBJLISTBASE_H

#include <rva.h>

#include <Ints.h>

struct CObjListBase {
    virtual void UnusedListHook() = 0;

    ~CObjListBase() {}
};

#endif // GRUNTZ_BUTE_OBJLISTBASE_H

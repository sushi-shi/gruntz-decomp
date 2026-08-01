#ifndef GRUNTZ_WWD_WWDFACTORYOBJECT_H
#define GRUNTZ_WWD_WWDFACTORYOBJECT_H

#include <Ints.h>
#include <rva.h>

#include <DDrawMgr/AnimWorkerObj.h>

struct CDDrawRect {
    i32 left;
    i32 top;
    i32 right;
    i32 bottom;
};
SIZE_UNKNOWN();
i32 __stdcall RectsOverlap(CDDrawRect* a, CDDrawRect* b);

namespace Rng {
    i32 Next2();
}

#endif // GRUNTZ_WWD_WWDFACTORYOBJECT_H

#ifndef GRUNTZ_WWD_WWDFACTORYOBJECT_H
#define GRUNTZ_WWD_WWDFACTORYOBJECT_H

#include <Ints.h>
#include <rva.h>

#include <DDrawMgr/AnimWorkerObj.h>

struct CDDrawRect {
    i32 left;   // +0x00
    i32 top;    // +0x04
    i32 right;  // +0x08
    i32 bottom; // +0x0c
};
SIZE_UNKNOWN(CDDrawRect);
i32 __stdcall RectsOverlap(CDDrawRect* a, CDDrawRect* b); // 0x15bfb0 (I obj)

#endif // GRUNTZ_WWD_WWDFACTORYOBJECT_H

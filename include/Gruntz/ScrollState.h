// ScrollState.h - the map auto-scroll state block at 0x64cfb0 (owner TU
// src/Gruntz/MgrAutoScroll.cpp). Declared here so the scroll consumers
// (CmdScrollApply, MapLogic's scroll-state serializer) reference these from
// this owner header instead of per-TU externs. DATA homes live in MgrAutoScroll.cpp.
#ifndef GRUNTZ_GRUNTZ_SCROLLSTATE_H
#define GRUNTZ_GRUNTZ_SCROLLSTATE_H

#include <Ints.h>

extern i64 g_scrollAccum; // 0x64cfb0 (64-bit scroll accumulator; serialized block head)
extern u32 g_scrollClock; // 0x64cfc0

#endif // GRUNTZ_GRUNTZ_SCROLLSTATE_H

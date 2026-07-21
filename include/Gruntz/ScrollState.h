#ifndef GRUNTZ_GRUNTZ_SCROLLSTATE_H
#define GRUNTZ_GRUNTZ_SCROLLSTATE_H

#include <Ints.h>

extern i64 g_scrollAccum; // 0x64cfb0 (64-bit scroll accumulator; serialized block head)
extern i64 g_scrollLimit; // 0x64cfb8 (+0x08)  UpdateMgrScroll's threshold
extern u32 g_scrollClock; // 0x64cfc0 (+0x10)
extern u32 g_scrollTimer; // 0x64cfc4 (+0x14)  Cmd_ResetScroll / UpdateMgrScroll
// +0x18 / +0x1c (0x64cfc8 / 0x64cfcc): serializer-only dwords - each has exactly 2
// absolute .text refs, BOTH inside MapSerializeCurve (read 0xec2b5/0xec311, write
// 0xec2c3/0xec31f); they round-trip through the save file and nothing else reads them.
// Declared with POSITIONAL names (no semantic claim) so the serializer needs no
// offset-casts and the .data relocs bind exactly.
extern i32 g_scrollSave18; // 0x64cfc8 (+0x18)
extern i32 g_scrollSave1c; // 0x64cfcc (+0x1c)
extern i32 g_lastScrollX;  // 0x64cfd0 (+0x20)  last-frame scroll position
extern i32 g_lastScrollY;  // 0x64cfd4 (+0x24)

#endif // GRUNTZ_GRUNTZ_SCROLLSTATE_H

// FixedPtrArray32.h - a fixed-capacity (32-slot) pointer array, embedded by
// value in its owner. Layout: a flag/tag at +0x00, a count at +0x04, and the
// 32-entry pointer table at +0x08 (0x80 bytes). Add appends until full; the
// FillFrom helper resets the object and bulk-appends a source list, skipping
// null entries. Non-polymorphic (no vtable); names are placeholders, offsets +
// code bytes are load-bearing.
#ifndef GRUNTZ_CFIXEDPTRARRAY32_H
#define GRUNTZ_CFIXEDPTRARRAY32_H

#include <Ints.h>
#include <rva.h>

SIZE_UNKNOWN(CFixedPtrArray32);
class CFixedPtrArray32 {
public:
    void Clear();
    i32 FillFrom(void** src, i32 n, i32 unused);
    i32 Add(void* item);

    i32 m_00;          // +0x00 flag/tag (reset to 0 by FillFrom, untouched by ctor)
    i32 m_count;       // +0x04
    void* m_items[32]; // +0x08
};

#endif // GRUNTZ_CFIXEDPTRARRAY32_H

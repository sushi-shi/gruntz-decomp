#ifndef GRUNTZ_CFIXEDPTRARRAY32_H
#define GRUNTZ_CFIXEDPTRARRAY32_H

#include <Ints.h>
#include <rva.h>

class CFixedPtrArray32 {
public:
    void Clear();
    i32 FillFrom(void** src, i32 n, i32 unused);
    i32 Add(void* item);

    i32 m_00;          // +0x00 flag/tag (reset to 0 by FillFrom, untouched by ctor)
    i32 m_count;       // +0x04
    void* m_items[32]; // +0x08
};
SIZE_UNKNOWN();

#endif // GRUNTZ_CFIXEDPTRARRAY32_H

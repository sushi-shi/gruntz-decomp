#ifndef GRUNTZ_CFIXEDPTRARRAY32_H
#define GRUNTZ_CFIXEDPTRARRAY32_H

#include <rva.h>

#include <Ints.h>

class CInputDevBase;

class CFixedPtrArray32 {
public:
    void Clear();
    i32 FillFrom(CInputDevBase** src, i32 n, i32 unused);
    i32 Add(CInputDevBase* item);

    i32 m_reserved00;
    i32 m_count;
    CInputDevBase* m_items[32];
};

extern "C" CFixedPtrArray32* g_actorList;

#endif // GRUNTZ_CFIXEDPTRARRAY32_H

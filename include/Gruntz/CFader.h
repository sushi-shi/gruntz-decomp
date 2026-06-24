// CFader.h - the CFaderMgr element base class (tracer placeholder
// MallocCtor_17fdb0, keyed off a CFader subtype ctor at 0x17fdb0). CFader is the
// polymorphic base of the six screen-fader subtypes the CFaderMgr factory
// (CFaderMgr::Add) allocates; its vftable is at RVA 0x1f07a8. Each fader owns a
// growable color/shade-table cache (a CShadeTableCache subobject at +0x04) plus a
// handful of timing fields the manager primes via SetTimers/Set2c, and a busy-wait
// helper (Wait) that spins on GetTickCount.
//
// Layout (offsets/sizes load-bearing; field NAMES are placeholders):
//   +0x00  vptr (the fader vftable, 0x5f07a8 on the base)
//   +0x04  m_cache (CShadeTableCache, 0x18 bytes; spans +0x04..+0x1b)
//   +0x1c  m_table (CShadeTable*, created on demand; freed via m_cache.FindRemove)
//   +0x20  m_20    (base field; left uninitialized by the base ctor)
//   +0x24  m_24    (timer A, set by SetTimers)
//   +0x28  m_28    (timer B, set by SetTimers)
//   +0x2c  m_2c    (set by Set2c)
//   +0x30  m_flag  (=1 in the base ctor; gates the m_table FindRemove on teardown)
//
// The base ctor builds the cache subobject (0x14de30), stamps the vftable, and
// zeroes m_table + sets m_flag; the base dtor (/GX EH frame) restores the vftable,
// FindRemoves m_table from the cache when m_flag is set, then destructs the cache
// subobject (0x14de50). The CShadeTableCache ctor/dtor/FindRemove and operator
// new/delete are external/reloc-masked.
#ifndef GRUNTZ_GRUNTZ_CFADER_H
#define GRUNTZ_GRUNTZ_CFADER_H

#include <Ints.h>
#include <rva.h>

#include <DDrawMgr/ShadeTableCache.h> // CShadeTableCache / CShadeTable (the +0x04 cache)

// The fader vftable + the grand-base dtor vtable, stamped by the ctor/dtor as
// reloc-masked DIR32 data. (g_remusBaseDtorVtbl is also declared in CFaderMgr.h;
// each TU declares its own extern - one definition per address, reloc-masked.)
DATA(0x001f07a8)
extern void* g_faderVtbl; // 0x5f07a8 - the CFader base vftable

class CFader {
public:
    CFader();                     // 0x17e450
    ~CFader();                    // 0x17e4a0  (/GX EH frame)
    void Wait(i32 delay);         // 0x17e510 - busy-wait until GetTickCount >= now+delay
    void SetTimers(i32 a, i32 b); // 0x17e760 - store m_24/m_28
    void Set2c(i32 v);            // 0x17e780 - store m_2c

    void* m_vptr;             // +0x00
    CShadeTableCache m_cache; // +0x04 (0x18 bytes)
    CShadeTable* m_table;     // +0x1c
    i32 m_20;                 // +0x20
    i32 m_24;                 // +0x24
    i32 m_28;                 // +0x28
    i32 m_2c;                 // +0x2c
    i32 m_flag;               // +0x30
};

SIZE(CFader, 0x34);

#endif // GRUNTZ_GRUNTZ_CFADER_H

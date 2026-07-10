// Fader.h - the CFaderMgr element base class (tracer placeholder
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
//   +0x1c  m_table   (CShadeTable*, created on demand; freed via m_cache.FindRemove)
//   +0x20  m_20      (base field; left uninitialized by the base ctor)
//   +0x24  m_timerA  (timer A, set by SetTimers)
//   +0x28  m_timerB  (timer B, set by SetTimers)
//   +0x2c  m_set2cArg (set by Set2c)
//   +0x30  m_flag    (=1 in the base ctor; gates the m_table FindRemove on teardown)
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

// CFader is a real polymorphic class (vftable @0x5f07a8, 5 slots): the virtual
// dtor (slot 0), two pure virtuals (slots 1/2, == &__purecall), and two engine
// virtuals at 0x17e790/0x17e7a0 (slots 3/4, defined in sibling TUs). Declaring
// them makes MSVC emit ??_7CFader and auto-stamp the implicit vptr in the ctor/
// dtor; the slot relocs + the stamp reloc-mask against the (differently-named)
// target symbols. No manual g_faderVtbl stamp needed.

class CFader {
public:
    CFader();                   // 0x17e450
    virtual ~CFader();          // 0x17e4a0  (/GX EH frame; vtable slot 0)
    virtual void v1(i32 f) = 0; // slot 1 (__purecall in base; render-frame in subtypes)
    virtual i32 v2() = 0;       // slot 2 (__purecall in base; frame-count in subtypes)
    virtual void v3();          // slot 3 (0x17e790, sibling TU) - begin
    virtual void v4();          // slot 4 (0x17e7a0, sibling TU) - end

    void Wait(i32 delay);         // 0x17e510 - busy-wait until GetTickCount >= now+delay
    void SetTimers(i32 a, i32 b); // 0x17e760
    void Set2c(i32 v);            // 0x17e780
    // 0x17e540 - the stepped counterpart of RunFade: prime frame 0, busy-wait the
    // lead-in, then render every `step`-th frame from 1..v2() back-to-back (no timing;
    // poke the fade sink + v1(frame) each step), then finalize v1(count)/v4() and record
    // the achieved frame rate in m_34. Used for the non-timed / max-speed transition.
    void RunFadeStepped(i32 step, i32 lead, i32 notify); // 0x17e540

    // 0x17e620 - drive the whole timed fade: prime frame 0, busy-wait the lead-in,
    // then map elapsed/duration onto the [0..v2()] frame index, poking the m_set2cArg
    // fade sink + v1(frame) per newly-reached frame; records the achieved frame rate
    // in m_34 and finalizes via v4(). (Was the standalone FaderRun view - dissolved.)
    void RunFade(u32 dur, i32 lead, i32 notify); // 0x17e620

    // implicit vptr        // +0x00
    CShadeTableCache m_cache; // +0x04 (0x18 bytes)
    CShadeTable* m_table;     // +0x1c
    i32 m_20;                 // +0x20  base field (left uninitialized by the base ctor)
    i32 m_timerA;             // +0x24  timer A (set by SetTimers)
    i32 m_timerB;             // +0x28  timer B (set by SetTimers)
    i32 m_set2cArg;           // +0x2c  value set by Set2c (RunFade reads it as an IFadeSink**)
    i32 m_flag;               // +0x30  teardown gate (=1 in the base ctor)
    i32 m_34;                 // +0x34  RunFade's measured frame rate
};

SIZE(CFader, 0x38);
// Own class-private base vtable (5 slots: dtor + 2 pure virtuals + 2 engine
// virtuals). cl emits ??_7CFader; catalogs/pairs the 0x1f07a8 datum (was
// vtbl-placeholders vtbl-cluster-66). Slots 1/2 are __purecall (pure); a
// per-slot FUN_<rva> is impossible for overridden virtuals - subtypes must share
// the base slot name (v1/v2), so those slot names are C++-mandated, not anonymous.
VTBL(CFader, 0x001f07a8);

#endif // GRUNTZ_GRUNTZ_CFADER_H

#ifndef GRUNTZ_GRUNTZ_CFADER_H
#define GRUNTZ_GRUNTZ_CFADER_H

#include <Ints.h>
#include <rva.h>

#include <DDrawMgr/ShadeTableCache.h> // CShadeTableCache / CShadeTable (the +0x04 cache)

class CFader {
public:
    CFader();                   // 0x17e450
    virtual ~CFader();          // 0x17e4a0  (/GX EH frame; vtable slot 0)
    // Slot roles PROVEN by the two run-drivers below: `count = GetFrameCount(); BeginFade();
    // RenderFrame(0); ... RenderFrame(frame); ... RenderFrame(count); EndFade();`.
    virtual void RenderFrame(i32 f) = 0; // slot 1 (__purecall in base; renders one fade frame)
    virtual i32 GetFrameCount() = 0;     // slot 2 (__purecall in base; total fade frame count)
    virtual void BeginFade();            // slot 3 (0x17e790, sibling TU; base = empty default)
    virtual void EndFade();              // slot 4 (0x17e7a0, sibling TU; base = empty default)

    void Wait(i32 delay);         // 0x17e510 - busy-wait until GetTickCount >= now+delay
    void SetTimers(i32 a, i32 b); // 0x17e760
    void Set2c(i32 v);            // 0x17e780
    // 0x17e540 - the stepped counterpart of RunFade: prime frame 0, busy-wait the
    // lead-in, then render every `step`-th frame from 1..GetFrameCount() back-to-back (no timing;
    // poke the fade sink + RenderFrame(frame) each step), then finalize RenderFrame(count)/EndFade() and record
    // the achieved frame rate in m_34. Used for the non-timed / max-speed transition.
    void RunFadeStepped(i32 step, i32 lead, i32 notify); // 0x17e540

    // 0x17e620 - drive the whole timed fade: prime frame 0, busy-wait the lead-in,
    // then map elapsed/duration onto the [0..GetFrameCount()] frame index, poking the m_set2cArg
    // fade sink + RenderFrame(frame) per newly-reached frame; records the achieved frame rate
    // in m_34 and finalizes via EndFade(). (Was the standalone FaderRun view - dissolved.)
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
VTBL(CFader, 0x001f07a8);

#endif // GRUNTZ_GRUNTZ_CFADER_H

// CFaderRun.cpp - the CFader "run timed fade" driver (0x17e620), a CFader-subtype
// method the ApiCaller stub misfiled as winapi_17e620_GetTickCount. It drives the
// whole fade: primes frame 0, busy-waits the lead-in (Wait), then spins on
// GetTickCount mapping elapsed/duration onto the [0..count] frame index (v2 = frame
// count, v1 = render frame N). Each newly-reached frame optionally pokes a COM-style
// sink (m_2c) then renders; at the end it records the achieved frame rate in m_34
// and finalizes via v4. Field NAMES are placeholders; offsets + bytes load-bearing.
//
// `this` is modelled as a standalone CFader-subtype shape (vtable + fields to +0x38)
// rather than deriving CFader, because the recovered CFader.h fixes v1/v2 as void()
// while this driver needs v1(int)/v2()->int; the vtable SLOTS + call bytes are what
// match (reloc-masked), not the names. NON-EH (base /O2) frame.
#include <Ints.h>
#include <Win32.h> // GetTickCount
#include <rva.h>

// The busy-wait spinner (0x17e510): spins until GetTickCount() >= now + delay.
// Reloc-masked rel32 callee, __thiscall (ignores its this).

// The COM-style fade sink reached through this->m_2c (a pointer-to-interface-
// pointer): *m_2c is the interface, whose first field is a C-vtable; slot 0x58 is
// poked (arg 1, 0) once per newly-reached frame. External / reloc-masked.
struct IFadeSink;
struct IFadeSinkVtbl {
    char _00[0x58];
    void(__stdcall* m_58)(IFadeSink*, i32, i32); // +0x58
};
struct IFadeSink {
    IFadeSinkVtbl* vtbl; // +0x00  C-vtable
};

class FaderRun {
public:
    virtual ~FaderRun();    // slot 0
    virtual void v1(i32 f); // slot 1  render frame f
    virtual i32 v2();       // slot 2  frame count
    virtual void v3();      // slot 3  begin
    virtual void v4();      // slot 4  end
    void Wait(i32 delay);   // 0x17e510

    void RunFade(u32 dur, i32 lead, i32 notify); // 0x17e620

    char _04[0x2c - 0x04]; // +0x04
    IFadeSink** m_2c;      // +0x2c  fade sink (pointer-to-interface-pointer)
    char _30[0x34 - 0x30]; // +0x30
    i32 m_34;              // +0x34  measured frame rate
};

// @early-stop
// Complete + correct (~86%). Wall = the x87 loop-invariant conversion block: retail
// batches (float)startTick + (float)dur + (float)count as fild QWORD x2 + fild DWORD
// then a single `fxch st(2)` + 3 fstp, and pins fStart@0x2c / fDur@0x10 / fCount@0x14;
// MSVC5 here schedules the three (unsigned/int)->float conversions separately and
// assigns different stack slots, which cascades the fsub/fdiv/fmul memory operands
// (same FP-schedule/local-slot class as the blit reassociation walls). No source
// ordering of the three float decls pins the fxch batching. Loop body, guards, sink
// COM-call and the m_34 frame-rate store (0.001f const) are byte-exact.
RVA(0x0017e620, 0x13b)
void FaderRun::RunFade(u32 dur, i32 lead, i32 notify) {
    i32 prev = 0;
    i32 frame = 0;
    i32 count = v2();
    if (count < 1) {
        return;
    }
    v3();
    v1(0);
    Wait(lead);
    i32 loops = 0;
    DWORD startTick = GetTickCount();
    float fStart = (float)startTick;
    float fDur = (float)dur;
    float fCount = (float)count;
    if (count >= 0) {
        do {
            frame = (i32)(((float)GetTickCount() - fStart) / fDur * fCount);
            if (prev != frame && frame <= count && frame > 0) {
                if (notify && m_2c) {
                    IFadeSink* o = *m_2c;
                    o->vtbl->m_58(o, 1, 0);
                }
                v1(frame);
                loops++;
            }
            prev = frame;
        } while (frame <= count);
    }
    if (frame != count) {
        v1(count);
        loops++;
    }
    float fLoops = (float)loops;
    DWORD elapsed = GetTickCount() - startTick;
    m_34 = (i32)(fLoops / ((float)elapsed * 0.001f));
    v4();
}

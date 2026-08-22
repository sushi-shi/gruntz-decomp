# The emitted member-store run is NOT the source order - try declaration order

## Symptom

`walls diagnose` says REGALLOC/SCHEDULING with **identical** byte length,
instruction count, call count, branch count and relocation count, and the whole
divergence is one straight-line run of `mov [this+off],<reg>` stores whose
OFFSETS appear in a different order on the two sides, with the parameter loads
that feed them permuted the same way.

    BASE (ours)                     TARGET (retail)
    mov edx,DWORD PTR [esp+0x10]  | mov edx,DWORD PTR [esp+0x1c]
    mov DWORD PTR [esi+0x8],ecx   | mov DWORD PTR [esi+0x8],ecx
    mov ecx,DWORD PTR [esp+0x1c]  | mov ecx,DWORD PTR [esp+0x10]
    mov DWORD PTR [esi+0xc],eax   | mov DWORD PTR [esi],edx
    ...

## Mechanism

C2 schedules a straight-line store run for its own reasons - it stores a value
that is already register-resident before one that still needs a load, and it
picks which parameter load starts the merge block. So retail's EMITTED order is
an output of the scheduler, not a picture of the devs' source.

A reconstruction that transcribes the emitted order therefore feeds C2 an input
it never had, and C2 reschedules THAT, producing a different permutation. The
fixed point is usually the order a person would actually write: the class's
**declaration order**.

## Evidence

`CActionOptionsMenuBar::Init` 0x00009220 (`actionoptionsmenubar`) sat at 95.28
with 53 instructions, 0x8f bytes, 1 call, 6 branches, 2 relocations on both
sides - every count equal, first divergence at +0x54, in the six-store run.

The source carried retail's emitted order, transcribed:

    m_screenX = x;                      // +0x08
    m_gridX = gx;                       // +0x00
    m_screenY = yy;                     // +0x0c
    m_buttonState[1] = secondaryState;  // +0x18
    m_gridY = gy;                       // +0x04
    m_buttonState[0] = primaryState;    // +0x14

C2 emitted 8, c, 0, 18, 14, 4 from it - the source order with two adjacent pairs
transposed. Rewriting the run in the header's declaration order
(`m_gridX, m_gridY, m_screenX, m_screenY, m_buttonState[2]`):

    m_gridX = gx;
    m_gridY = gy;
    m_screenX = x;
    m_screenY = yy;
    m_buttonState[0] = primaryState;
    m_buttonState[1] = secondaryState;

emits 8, 0, c, 18, 4, 14 - retail's order, **100.00% EXACT**, no other change.

## Use

On an all-counts-equal store-run residue, try the declaration order of the
members BEFORE reaching for a scheduling wall. It costs one build, it is the
humane spelling, and it removes a transcription that was fitted to the output.

Related: `param-store-last-forces-callee-saved.md` (same "emitted order does not
name the source order" mechanism, seen through the callee-saved push count) and
`imm-store-floats-to-end-of-store-run.md` (the same scheduler, immediate stores).

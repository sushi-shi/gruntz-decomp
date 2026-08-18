# A two-member commutative op canonicalizes to ASCENDING member offset - source order is inert

**Tags:** `cpp:expr` `cpp:member` | `asm:mov` `asm:add` `asm:imul` | `topic:wall`
**Confidence:** 9/10 (two functions, three refuted levers, one build each)

## Symptom

A tiny accessor sits one byte short of exact and `walls diagnose` calls it
REGALLOC/SCHEDULING with the divergence at the displacement byte of the FIRST
instruction. Both sides have the same instruction count, the same opcodes and
the same operands - only the ORDER of the two member loads differs:

```asm
; ours                                  ; retail
mov  eax,[ecx+0x4]                      mov  eax,[ecx+0x8]
imul eax,[ecx+0x8]                      imul eax,[ecx+0x4]
add  eax,0x10                           add  eax,0x10
ret                                     ret
```

## The rule

cl 5.0 evaluates `a OP b` for a commutative OP over two members of the same
object in **ascending member offset order**, and materializes the lower-offset
operand in EAX. The source spelling does not survive to C2: `m_height *
m_width` and `m_width * m_height` emit the identical object.

So retail's DESCENDING order was not produced by writing the operands the other
way round, and this row is not a spelling question.

## Refuted, one build each

| lever | result |
|---|---|
| swap the operands (`m_width * m_height`) | byte-identical, 99.5000 unchanged |
| an inline `GetArea()` member used at both sites | byte-identical here, and **-11 fresh regressions** in other TUs through the shared header |
| a named local for the first operand (`i32 h = m_height; return h * m_width + 0x10;`) | byte-identical |

The layout was independently confirmed CORRECT before any of this, which is
what makes the refutations meaningful: retail's own exact `CImageSet1::ScanRight`
(`m_width - 1`) reads `[ecx+0x4]` and `ScanDown` (`m_height - 1`) reads
`[ecx+0x8]`, pinning both offsets.

## Where it bites

`CImageSet3::GetStride` 0x00161590 (`imul`) and `CFaderSine::GetFrameCount`
0x00180400 (`add`) are the same shape and the same residue - both parked. On a
9-11 byte body one reordered pair IS the whole gap, so expect these rows to sit
in the high 99s forever; read the instruction count, not the percentage
(a 0.5% gap here is one byte).

## What is still open

Something in retail's TU gave that expression a different tree. The three
cheapest source shapes do not, so the next candidate is C1 state rather than a
spelling - which puts it with the tuple-count question in
[`wall-reasons-allocation`](../relevations/wall-reasons-allocation.md) (retail's
block holding one more register-requesting tuple than ours). Do not spend
another sweep on the expression itself.

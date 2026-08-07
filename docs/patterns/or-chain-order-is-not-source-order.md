# The emitted `or ax,[lut]` chain order is NOT the source order — A/B it, don't transcribe it

tags: cpp:expr cpp:loop | asm:or asm:mov | topic:codegen-idiom
symptoms: `or ax,WORD PTR [reg+idx*2]` chains, blend/LUT triples, three
`mov <reg>,[this+0xNN]` table pointers, a whole family of near-clone loops whose
emitted term order differs per site
confidence: 9/10

## Symptom

A pixel blend reads three lookup tables and ORs the results:

```asm
mov ax,WORD PTR [esi+eax*2]        ; r  = lutBank2[...]
or  ax,WORD PTR [ebp+edi*2+0x0]    ; r |= lutBank1[...]
or  ax,WORD PTR [ecx+edx*2]        ; r |= lutBank0[...]
```

The obvious move is to transcribe that emitted order into the source. **Do not.**
MSVC5 reassociates a chain of pure loads OR'd into a `u16`, so the emitted order is
a scheduling artifact of that site's register pressure, not the order the dev wrote.

## Evidence

`CDDrawShadeBlit` has 16 of these sites across four functions. Reading the chains off
both objects (accumulator-first `mov`, then each `or`, resolving which `[this+0x30/34/38]`
table pointer each index register came from) gives:

| fn | source order | BASE emits | TARGET emits |
|---|---|---|---|
| `ConvertRow` 0x14c9f0 | 210 048 102 102 | 408 048 408 048 | 840 048 408 408 |
| `ConvertRowFlip` 0x14cfc0 | 120 021 120 120 | 048 048 408 048 | 480 084 480 480 |

Both functions had been transcribed as "source order = target emitted order" — and in
both, the SAME source compiles to a different emitted order. So the emitted order
cannot be read back as the source order in either direction.

Retail's own emitted orders are not even self-consistent across clones:
`BlitShadedForward` 0x14a200 emits `804 840 408 840` for its four blend sites while
`BlitShadedMirrored` 0x14b770 emits `048` at all four — same four arms, different
schedules.

## What IS readable

The **operand roles and the term set** are solid, and that is where the real bug hides.
Count the table references per side:

```
$ grep -c 'ebx+0x30' T_BlitShadedForward.txt   # 10 (retail)
$ grep -c 'ebx + 0x30' B_BlitShadedForward.txt #  9 (ours)
```

Ours was short exactly one `m_lutBank0`. The source had named `m_lutBank1` twice with an
identical index expression and dropped `m_lutBank0` entirely — cl CSE'd the duplicate
away, so the defect showed up only as the missing tenth reference, never as a chain-order
diff. (The same arm also had the scratch and source pixel operands swapped.)

## Rule

1. **Audit the term SET first** — per-table reference counts, base vs target. A count
   mismatch is a real, byte-proven source bug (a dropped or duplicated term).
2. Then treat the chain ORDER as a **search axis, not evidence**. All six permutations
   per site, scored in one sweep; each function's sites are independent, so one build
   yields one measurement per function. Measured spread on `ConvertRow`: 75.5 → 77.5
   between the worst and best uniform order.

related:
[commutative-operand-order-is-canonical.md](commutative-operand-order-is-canonical.md),
[mnemonic-histogram-diff-finds-the-wrong-idiom.md](mnemonic-histogram-diff-finds-the-wrong-idiom.md)

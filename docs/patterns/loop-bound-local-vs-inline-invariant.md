# A named `lim = m_x - 1` local before the loop colours differently from the inline bound
tags: cpp:loop cpp:local cpp:member | asm:dec asm:lea asm:cmp | topic:codegen-idiom topic:regalloc
symptoms: `dec reg` where retail has `lea reg,[other-0x1]`; whole-function register
permutation with IDENTICAL block topology; `push ecx` scratch slot on the base side
where retail spills into an incoming parameter's home slot; a family of near-identical
scan methods where exactly the ones carrying a `lim`/`end`/`count` local miss
confidence: 9/10

## Symptom

A family of tiny sibling loops (`CImageSet3::ScanUp/ScanRight/ScanDown/…ForValue`,
0x00166eb0..0x001670d0) splits cleanly in two:

| shape | score |
|---|---|
| `while (y > 0) { … }` — no bound local | **100%** |
| `i32 lim = m_width - 1; while (x < lim) { … }` | 67–82% |

`--blocks --diff --lite` reports `flow SAME` for every one of them. The instruction
STREAM is the same length and the same shape; only the register assignment is
permuted, plus one telltale pair:

```
  base  :  mov esi,[eax+0x4]      ; m_width straight into the lim register
           dec esi
  retail:  mov ecx,[esi+0x4]      ; m_width into a scratch register
           lea edi,[ecx-0x1]      ; ...and the bound lands somewhere else
```

On the wider ones (`ScanDown`, 116 B) the same source difference costs a whole
stack slot: the base opens with `push ecx` to make a spill home for `this`, while
retail keeps `this` in `ebp` and spills the bound into the *incoming `y` parameter's*
home slot (`mov [esp+0x18],ecx` after four pushes).

## Mechanism

A source-level local is a user variable: cl5 gives it its own vreg at the point of
the declaration statement, in the entry block, *before* the loop's live values are
coloured — so it competes with `this` and wins, pushing `this` into a caller-saved
register (or onto a fresh spill slot).

Spelled inline in the loop condition (`while (x < m_width - 1)`) the same value is an
**anonymous loop invariant**. LICM materializes it while building the rotated loop's
guard, so it is coloured *with* the loop-carried set — after `this` and the induction
variable — which is retail's assignment. The `dec` -> `lea` flip falls straight out
of that: `dec` only appears when the load's destination register IS the bound's
register, which is exactly what the early, user-variable colouring produces.

## Lever

Do not hoist a loop bound into a named local unless the disassembly shows it
evaluated *before* an unrelated statement. Spell it in the condition:

```cpp
// base 78.4%                       // retail, 100%
i32 lim = m_width - 1;              while (x < m_width - 1) {
while (x < lim) {                       ++x; ++off;
    ++x; ++off;                         …
    …                               }
}
```

Re-reading the member each iteration is NOT a cost: cl5 hoists it either way.

## Evidence

`imageset3g` 87.62% -> 100.00% in one change. `CImageSet3::ScanRight` 78.43,
`ScanRightForValue` 72.2 -> 79.4 (the pointer-form fix) -> 100, `ScanDown` 67.11,
`ScanDownForValue` 94.43 — all four to 100% EXACT, all four previously carrying
`// @early-stop`. The four siblings that were already 100% are precisely the four
with no bound local.

## Related

- [[named-local-picks-first-operand-load]] — the *inverse* lever: a named local is
  what you add when you need to pick a load order. Both say the same thing — a named
  local moves the value earlier in cl5's colouring order.
- [[loop-preheader-vs-exit-block-order]] — the other half of loop rotation.

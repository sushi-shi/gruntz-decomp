# Commutative `imul reg*mem`: cl loads the memory operand into eax, retail loads the register

## Symptom

An inner-loop index like `idx = j * m_c + i` (loop counter `j` in a register,
`m_c` a member re-read from memory via a base pointer) plateaus at ~99.9% with the
only residual being one byte-swapped pair per multiply:

```
  retail:   mov eax,edi          ; j (the register operand)
            imul eax,[ebp+0x0]   ; * m_c (memory)
  base(cl): mov eax,[ebp]        ; m_c (the memory operand)
            imul eax,edi         ; * j
```

Both forms are 6 bytes, re-read `m_c` from the SAME `[ebp]` base, preserve the loop
counter `edi`, and compute the identical product. Only *which* operand MSVC5
materializes into `eax` before the `imul r32, r/m32` differs.

## Mechanism

For a commutative `a * b` where one operand is already in a register (the loop
induction var) and the other is memory, MSVC5's `/O2` back-end **canonicalizes the
multiply and consistently loads the memory operand into the destination register**,
then `imul eax, <reg>`. Retail's build (same toolchain) emitted the opposite
selection here — `mov eax,<reg>; imul eax,<mem>`. This is a back-end operand-order
coin-flip, not driven by the source expression: `j*m_c`, `m_c*j`, `i + m_c*j`, and
the compound `idx=j; idx*=m_c` all normalize to the memory-into-eax form, and the
permuter's span-scoped mutations do not flip it either.

## Verdict

`topic:wall topic:regalloc`. When a serialize/grid loop is otherwise byte-exact and
the ONLY residue is this imul operand swap (verify with `gruntz walls diagnose --asm`: real
diff is exactly the `mov`/`imul` operand pair, everything else is `[ebp]`-vs-
`[ebp+0x0]` / `4*ecx`-vs-`ecx*4` display noise), it is a maximized `@early-stop`.
Related but distinct from
[[loop-invariant-multiply-strength-reduce-vs-memreread]] (where cl *hoists +
strength-reduces* the product; here both sides re-read memory and recompute the
`imul` in-loop — only the operand-in-eax differs).

Seen: `CMapMgr::Save` 0x09f840 (99.92%), `CMapMgr::Load` 0x09f9a0 (99.85%).

## Variant: BOTH operands in memory (`imul`) — and the same coin-flip on `add`

The identical residue shows up when *neither* operand is a live register, i.e. two
member loads off the same `this`. cl loads one member and folds the other as the
`r/m32`; retail picks the other one:

```
  retail:   mov ecx,[esi+0x28]   ; m_gridW
            imul ecx,[esi+0x2c]  ; * m_gridH
  base(cl): mov ecx,[esi+0x2c]
            imul ecx,[esi+0x28]
```

and on a two-member `+`, as which of the two loads becomes the `add`'s destination:

```
  retail:   mov edi,[ebp+0x38]   ; m_rect.top      -> the surviving y
            mov esi,[ebp+0x5c]   ; m_offsetY
            add edi,esi
  base(cl): mov edi,[ebp+0x5c]   ; the two loads swap registers
            mov esi,[ebp+0x38]
```

**THE PICK IS NOT A FUNCTION OF THE SOURCE — it is decided downstream, per function.**
Two independent proofs, both measured 2026-07-28:

* `CDDrawWorkerHost::Save` (0x163780) and `CDDrawWorkerHost::Load` (0x1638c0) hold the
  *same* `GridByteSize(m_gridH, m_gridW)` expression and cl picks OPPOSITE operands in
  the two of them — and each is the opposite of retail's pick for that function.
* retail is itself inconsistent: `CMenuPage::LayoutOne` (0x183e50) puts `m_rect.top` in
  the add's destination while its sibling `CMenuPage::Layout` (0x183b60) puts `m_offsetY`
  there, from the same `y = top + offsetY` statement.

So no spelling closes it. Tried and identical in every case: `a*b`, `b*a`, the
`x=a; x*=b;` compound, hoisting either side into its own local, calling through an
inline helper with the args swapped, and rewriting the helper body from `p *= q; return
p*4;` to `return p*q*4;`.

Also seen: `CDDrawWorker::GetMemoryUsage` 0x1523f0 (99.96%), `CDDrawWorkerHost::Save`
0x163780 (99.98%), `CDDrawWorkerHost::Load` 0x1638c0 (99.98%), `CMenuPage::LayoutOne`
0x183e50 (99.98%).

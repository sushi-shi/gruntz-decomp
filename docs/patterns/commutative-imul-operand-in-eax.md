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
the ONLY residue is this imul operand swap (verify with `sema disasm --diff`: real
diff is exactly the `mov`/`imul` operand pair, everything else is `[ebp]`-vs-
`[ebp+0x0]` / `4*ecx`-vs-`ecx*4` display noise), it is a maximized `@early-stop`.
Related but distinct from
[[loop-invariant-multiply-strength-reduce-vs-memreread]] (where cl *hoists +
strength-reduces* the product; here both sides re-read memory and recompute the
`imul` in-loop — only the operand-in-eax differs).

Seen: `CMapMgr::Save` 0x09f840 (99.92%), `CMapMgr::Load` 0x09f9a0 (99.85%).

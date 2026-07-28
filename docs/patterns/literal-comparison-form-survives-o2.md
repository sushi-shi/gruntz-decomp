# `>= 0` vs `> -1`, `>` vs `>=`: /O2 keeps the LITERAL comparison form, so the jcc names it
tags: cpp:branch cpp:loop | asm:cmp asm:test asm:jcc | topic:codegen-idiom topic:flags
symptoms: one jcc differs by exactly one "or-equal" (`jle`↔`jl`, `jge`↔`jg`) or retail spends an
extra `cmp reg,-1` where the recompile reuses `dec`'s flags with `test reg,reg`; everything else in
the function matches; the two spellings are behaviourally identical
confidence: 9/10

## Symptom

`jcc_sieve` reports a single `OTHER`/`POLARITY` flip that is an off-by-one in the *condition*, not
in the code:

```asm
; retail                       ; recompile           source we had
  cmp  ebp,eax                   cmp  ebp,eax
  jl   KEEP                      jle  KEEP           if (newSize > newMax) newMax = newSize;

; retail                       ; recompile           source we had
  dec  eax                       dec  eax
  cmp  eax,0xffffffff            test eax,eax
  jg   LOOP                      jge  LOOP           for (i = 0xf; i >= 0; i--)
```

## Cause

MSVC5 /O2 does **not** canonicalize a relational operator. `a > b` emits the `jle` skip; `a >= b`
emits `jl`. And `i >= 0` becomes `test reg,reg / jge` (cl reuses the flags a preceding `dec`
already set) while the *literally different* `i > -1` becomes `cmp reg,-1 / jg` — an extra
instruction cl will happily emit because the immediate is written in the source.

So the jcc is a direct readout of which operator the devs typed. When the two forms are
behaviourally equivalent at the boundary (a clamp that assigns the same value on equal; a loop
whose bound is one past), the *only* evidence for which was written is this one byte.

## Fix

Transcribe retail's operator literally.

**The inlined MFC `CArray::SetSize` growth** — five open-coded copies in the tree had
`if (newSize > newMax)`; retail is `cmp <newSize>,<newMax> / jl`, i.e. the clamp fires on `>=`.
That matches MFC's own phrasing, which tests the *other* direction:

```cpp
// MFC: if (nNewSize < m_nMaxSize + nGrowBy) nNewMax = m_nMaxSize + nGrowBy;
//      else                                nNewMax = nNewSize;
i32 newMax = m_arr.m_nMaxSize + grow;
if (newSize >= newMax) {     // was `>`  -> jle
    newMax = newSize;
}
```

`CShadeTableCache::AddFromFile` @0x14f8b0 80.93 → **81.29**, `AddFromArray` @0x14f6c0
76.90 → **77.24** (sites also fixed in `AddTable`, `CFaderMgr::Add`, `CFader`'s copy).

**A count-down loop written against −1** — `CShadeTableCache::SubTable` @0x14f310:

```cpp
for (i32 level = 0xf; level > -1; level--)   // was `level >= 0` -> test/jge
```

That one is worth reading as evidence rather than as points: it costs a `cmp` (73.96 vs 74.20 on
the parked score) but it is what retail's bytes say, and the loop tail now matches instruction for
instruction.

## Not this pattern

`jl`↔`jb` (and `jg`↔`ja`, `jle`↔`jbe`, `jge`↔`jae`) is a *signedness* difference, i.e. a real type
bug in an operand, not an operator choice — `jcc_sieve` classifies those as SIGNEDNESS and they are
almost always a member or global that wants to be unsigned.

## Related

- [masked-diff-hides-branch-target](masked-diff-hides-branch-target.md) — `python -m
  gruntz.audit.jcc_sieve` enumerates these tree-wide.
- [if-body-owns-the-fallthrough](if-body-owns-the-fallthrough.md) — the other single-flip family
  (block layout rather than operator).

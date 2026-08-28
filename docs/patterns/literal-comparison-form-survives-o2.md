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

## Same family: `a <= b` vs `b >= a` — the OPERAND ORDER survives too

The two spellings are identical in C and differ in *both* the `cmp` operand order and the
jcc, so a lone flip whose two sides also have their `cmp` operands swapped is this:

```asm
; retail  cmp edi,eax / jl   <- source `gx >= mid`
; base    cmp eax,edi / jg   <- source `mid <= gx`
```

`CBootyState::LevelMsgHudDriver` @0x1a700 had three of them (`(right+left)/2 <= gx`,
`g_levelMsgIconPos[s*2] <= gx`, `m_bomb[i]->m_screenX <= m_gokart[i]->m_screenX`);
rewriting each with retail's left operand first, 84.80 -> **85.23**, and the unit's sieve
went clean. Cheap and unambiguous - the left `cmp` operand names the left source operand.

## And: the loop bound must be in a REGISTER for cl to down-count

`for (x = 0; x < m_width; x++)` re-reads the member every iteration, which pins the guard
as `cmp edx,ecx / jl`; retail's `dec edi / jne` needs the trip count hoisted into a local
first. `CRezImage::FlipVertical` @0x176840, 41.61 -> **47.36** with `i32 wid = m_width;`
above the three byte loops. (The converse - retail comparing where we down-count - means
the index is still live, e.g. because the row addresses are computed from it.)

That isolated result remains a valid lowering observation, but it is not the final
`FlipVertical` reconstruction: the surviving 1996 `CDib::Invert` local census and three
ordinary forward loops move the function from the later 79.79 local maximum to 100.00
EXACT.

## And: `test x,x / jbe` is an UNSIGNED `> 0`, not `!= 0`

`CF` is always clear after `test`, so `jbe` and `je` are behaviourally the same branch here
- but cl only emits the `jbe` encoding when the *source operator* is `>`/`<=` on an
**unsigned** value. `if (want != 0)` gives `je`; `if (want > 0)` gives `jbe`.

`CRezItm::Read` @0x139af0 91.20 -> **92.72** (`if (want > 0)`, want u32), and
`CFaderShape::ApplyInit` @0x1817e0's mode guard 72.56 -> 72.68
(`if ((u32)pInit->m_14 <= 0) goto fail;` - the negated form, since that guard bails).
Both had the 1-byte difference filed as an unsteerable encoding choice.

## And: two range guards to ONE far exit need ONE `||`

`if (p < 0) goto fail; if (p > 100) goto fail;` lets cl place the FIRST guard's target
inline between the guards and the body, which forces the second guard to invert and jump
*forward to the body* (`jle <body>`). Retail sends both to the same far block
(`jl <fail> / jg <fail>`), which is one `||`:

```cpp
if (p < 0 || p > 100) {
    goto fail;
}
```

`CFaderSine::ApplyInit` @0x17fe00 71.58 -> **87.07** on that one edit - it had been filed
as "a callee-saved coloring swap that touches every ModRM byte", and the colouring came
right on its own.

## Not this pattern

`jl`↔`jb` (and `jg`↔`ja`, `jle`↔`jbe`, `jge`↔`jae`) is a *signedness* difference, i.e. a real type
bug in an operand, not an operator choice — `jcc_sieve` classifies those as SIGNEDNESS and they are
almost always a member or global that wants to be unsigned.

## Related

- [masked-diff-hides-branch-target](masked-diff-hides-branch-target.md) — `gruntz walls diagnose <fn>` names the class.
- [if-body-owns-the-fallthrough](if-body-owns-the-fallthrough.md) — the other single-flip family
  (block layout rather than operator).

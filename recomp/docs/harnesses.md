# The harnesses — what each one assumes, and why its target is reachable

Every harness lives in `recomp/harness/`, shares `recomp.h`, and is built with

    recomp/harness/build.sh <name> [unit ...]
    wine recomp/harness/<name>.exe "$GRUNTZ_EXE"

The `[unit ...]` arguments link our **compiled** objects
(`build/objdiff/base/<unit>.obj`) into the harness, so the comparison is
retail's bytes against *our* bytes — not against a transcription of our source
into the harness. Declaring the function with a signature that mangles the same
way is all it takes for the linker to bind it.

`/FORCE:UNRESOLVED` is what makes that practical: a decompiled unit's object
carries every external its TU references (~30 apiece — CRT, MFC, sibling TUs)
and we link none of them. The reachability audit is precisely the claim that the
function under test never reaches one. If that claim is wrong the harness faults
immediately, which is the correct failure mode.

| harness | target | RVA | kind | `this` | verdict |
|---|---|---|---|---|---|
| `pidrun`   | `CDDSurface::DecodeByteRun1Plane`            | `0x145270` | ISLAND    | unused | agrees, 9 821 sprites |
| `colorrun` | `CShadeTableCache::FindNearestColor`| `0x14fbf0` | ISLAND    | static | agrees, 17 959 936 queries |
| `saverun`  | `CSaveGame::Encode` / `::Decode`    | `0xe5410` / `0xe5460` | ISLAND | unused | agrees, 525 334 |
| `polyrun`  | `PolyIsConvexCW`                    | `0x145e30` | DATA-ONLY | free   | agrees, 1 301 184 |
| `rectrun`  | `CGrunt::RectSegProbe`              | `0x62b70`  | ISLAND    | unused | **disagreed — two bugs fixed** |

## pidrun — `CDDSurface::DecodeByteRun1Plane`

The original. No relocations and no `CALL` at all: a pure loop over a token
stream, writing into a caller-supplied buffer. Driven from
`gruntz-oracle recomp`, which feeds it every `Rle`-grammar sprite in a `.REZ`
in one batch (30 000 wine launches would prove nothing extra).

Assumes: nothing. `this` is stored to a frame slot in the prologue and never
read back, so any pointer serves.

## colorrun — `CShadeTableCache::FindNearestColor`

A **static** member, so plain `__cdecl` and no `this` at all. Inputs are one
256-entry `PALETTEENTRY` array plus three ints.

Two behaviours are aimed at deliberately, because a plausible reimplementation
could get either wrong without moving the byte-match needle:

* the three query channels are masked (`and reg,0xff` at `0x14fc09` /
  `0x14fc16` / `0x14fc25`), so out-of-range arguments are well defined;
* a distance tie keeps the **lower** index (`cmp ecx,ebp / jge` — strictly-less
  wins).

Coverage is honest about its shape: the query space is exhausted (all 2^24
`(r,g,b)` against one palette), the palette space is not and cannot be
(2^6144). See the file header for the per-case counts.

## saverun — `CSaveGame::Encode` / `::Decode`

Zero object fields, `this` unused: the cheapest possible harness and the right
one to copy. Both functions have **two** results and both are compared — the
returned checksum and the buffer they rewrite in place. Comparing only the
return value would miss a wrong XOR key entirely.

It also checks, entirely inside retail, that `Encode` then `Decode` restores the
input. If that ever failed, our reading of the format would be wrong regardless
of what our C++ does.

## polyrun — `PolyIsConvexCW`

The first **DATA-ONLY** harness and the first float one. Its relocations point
at x87 constants in `.rdata`, which move with the image because `recomp.h`
relocates the whole thing — that is the case the DATA-ONLY class exists to
describe, and it needs no fabrication.

Comparison is exact, not epsilon-based, and should be: the function only tests
the sign of a cross product, so a disagreement about a sign is a real
disagreement. Both sides run x87 in one process under one control word.

Not covered: `count == 0`. Both implementations compute `i % count`, so zero
faults on both sides and there is no answer to compare. Negative counts are
covered (the loop body never runs).

## rectrun — `CGrunt::RectSegProbe`

The one that found bugs, and the clearest illustration of the audit's
`this` question. It is a `__thiscall` member of `CGrunt`, which reads as "needs
a whole grunt", and it needs nothing: the prologue overwrites `ecx` with
`p->top` before `this` is ever read.

**Two real logic bugs, both invisible to byte-matching at 78.77 %:**

1. *Wrong interpolation numerator on the left and right edges.* Retail computes
   `t = (edge - e1x) / (e2x - e1x)` (`0x62c98` and `0x62d0e`, both
   `sub eax,edi`), interpolating forward from `e1` exactly as the top/bottom
   arms do. We had `t = (e2x - edge) / (e2x - e1x)` — same magnitude, wrong sign
   and wrong endpoint. It agreed only when the segment happened to be symmetric
   about the edge. **8 530 of 131 072 random cases wrong (6.5 %).**

2. *Wrong comparison strictness on the same two arms.* The left/right arms
   accept only `top < iy && iy < bottom` (`0x62cd0` `test ah,1 / je`, `0x62cdf`
   `test ah,0x41 / jne`), while the top/bottom arms admit equality
   (`0x62bdf` / `0x62bed`, the opposite jump polarity). We used `<=` on both.
   The asymmetry is retail's and it is load-bearing: a segment ending exactly on
   a **vertical** edge is not a crossing, while one ending on a **horizontal**
   edge is.

After both fixes: 1 044 480 comparisons, zero disagreements, and the byte match
moved 78.77 → 79.41 as a side effect. The score was never the point — before
the fix this function returned the wrong answer on one random input in fifteen.

## Adding a sixth

1. `python -m gruntz.audit.recomp_islands --max-fields 12` — the target must be
   ISLAND / SELF-CALL / DATA-ONLY, and the `no `this`` / `needs `this`` tag tells
   you whether an object has to be fabricated.
2. Copy the closest existing harness. `saverun` for a scalar/buffer function,
   `polyrun` for a free function over a POD array, `rectrun` for a `__thiscall`
   member that ignores `this`.
3. Declare the retail entry with `RECOMP_RVA`, declare ours with a signature
   that mangles identically, and write a generator that says in its header what
   it actually covers.
4. Say what you did **not** cover. A sweep is not a proof and should not be
   written up as one.

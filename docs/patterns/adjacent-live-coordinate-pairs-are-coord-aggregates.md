# Adjacent live coordinate pairs are `Coord` aggregates, not independent scalars

tags: cpp:struct cpp:local cpp:copy | asm:mov asm:sub | topic:codegen-idiom topic:eh
symptoms: retail loads an x/y pair from one object and stores it into adjacent frame
dwords, or derives x/y together and homes them at `base+0`/`base+4`; every later use
consumes the two values as one position; scalar reconstruction gives nonadjacent homes
and displaced EH cleanup operands
confidence: 10/10

## The signal

Two adjacent dwords are not enough by themselves. The stronger signature is complete
value flow: retail obtains both halves from one `Coord` (or derives both halves of one
position), stores them at adjacent field offsets, and later consumes both as the same
point in comparisons and calls. That is a source aggregate:

```cpp
Coord target = *CoordTail()->m_coord;

Coord start;
start.m_x = m_object->m_screenX >> TILE_SHIFT_PX;
start.m_y = m_object->m_screenY >> TILE_SHIFT_PX;
```

Two independent `i32` locals allow C1/C2 to assign unrelated homes even when the
expressions are equivalent. The primary body may barely move, while every cleanup
funclet reveals the corrected local identity.

## Calibration

`CGrunt::PathScan` (`0x57db0`) originally modeled the tail tile as `tcol`/`trow`.
Retail copied the source `Coord` into adjacent dwords; candidate placed the scalars in
nonadjacent slots. Restoring `Coord target` reduced the local reservation from `0x8c`
to `0x84` and moved 88.99% to 89.15%. Modeling the similarly adjacent start tile as
`Coord start` reduced it again to `0x7c`, moved the body only to 89.19%, and made both
`CPtrList` unwind funclets exact. All 60 conditional branches and the return shape
remained unchanged.

`CGrunt::StepCompassMove` (`0x51c00`) is the negative control that bounds this
pattern. An earlier reading treated its adjacent current/proposed pixel homes as two
`Coord` objects. Complete value-flow review refuted that inference: neither pair
escapes through a complete-object call or copy, and even the seed is written
field-by-field. Replacing the four scalars with two field-wise `Coord` locals moved an
EH cleanup operand but reduced the function from 61.74% to 60.64%; whole-object
assignment also collapsed the CFG. The scalar model is retained. Adjacent homes plus
field-wise traffic alone therefore do not prove an aggregate—the complete-object
evidence in the signal above is required.

This is not the dead-eight-byte-spill pattern. There, stores are never read and the
same values stay live in registers; inventing an aggregate adds unsupported source.
Here both fields have one object identity and are repeatedly read as that identity.

## The cheap MEMBER-side detector: `or reg,-1` pairs

When the aggregate being written is a member and the value is constant, the
signature is mechanical and greppable. Retail materialises `-1` in two
registers before copying both halves into the member; two independent field
stores use immediates:

```asm
; retail - the aggregate call            ; ours - two field assignments
or   eax,0xffffffff                      mov  DWORD PTR [esi+0x2f0],0xffffffff
or   ecx,0xffffffff                      mov  DWORD PTR [esi+0x2d4],0x0
mov  DWORD PTR [esi+0x2f0],eax           mov  DWORD PTR [esi+0x2f4],0xffffffff
mov  DWORD PTR [esi+0x2d4],ebx
mov  DWORD PTR [esi+0x2f4],ecx
```

Note the unrelated `m_defenderState` store scheduled between the two coordinate
stores on the retail side. The two stores belong to one aggregate copy, but cl
may separate them; a transcription can therefore look field-wise even when the
source was not.

SCREEN A WHOLE LANE IN ONE PASS - count `or reg,-1` in each unit's base obj
against its target obj; a positive delta counts missing register-fed halves,
subject to following their values into member stores. Measured 2026-08-21:
`battlezunitstep` base 8 /
target 24, `battlezmapconfig` 6 / 12, `gruntchargestep` 0 / 2,
`gruntdefensestep` 0 / 2, `gruntarrivalupdate` 1 / 2.

## The spelling is decided by a probe, not by score

A direct member write CSEs the equal constants regardless of whether it uses
`Set` or two field assignments. The two-register form comes from copying a
local aggregate into the member. A pinned cl 5.0 `/O2 /MT /GX /GR` probe gave:

| source | `or reg,-1` emitted |
|---|---|
| `g->c.Set(-1, -1);` | 1 |
| `g->c.m_x = -1; g->c.m_y = -1;` | 1 |
| `Coord t; t.Set(-1, -1); g->c = t;` | 2 |
| `g->c = *t.Set(-1, -1);` | 2 |
| `g->c = Coord(-1, -1);` with a two-argument constructor | 2 |

The three two-register spellings emitted identical bytes. Prefer the existing
POD plus `Set` over inventing a constructor. In
`CBattlezMapConfig::CheckQueuedSpawnTile`, changing the direct member `Set` to
a local aggregate copy moved 76.86% to 78.19% and reproduced both retail
constant temporaries. Calls, branch skeleton, operands, and referents remained
identical; the remaining wall is structural allocation/scheduling. A 32-state
campaign found only one compiler island, so further work belongs in structural
source recovery rather than TU-state search.

The 2026-08-21 sweep folded 13 proven sites. Every site gained, and the census
fell from 43 to 8 missing halves (`battlezunitstep` closed 8/24 to 24/24):

| function | was | now |
|---|---|---|
| `CBattlezMapConfig::TrackAssignedEnemy` | 86.30 | **93.45** |
| `CBattlezMapConfig::RetargetIdleUnit` | 84.88 | 86.59 |
| `CBattlezMapConfig::AdvanceToEnemyBase` | 80.71 | 82.61 |
| `CBattlezMapConfig::StepDefenderUnit` | 77.60 | 79.03 |
| `CGrunt::WanderStep` | 86.35 | 87.86 |
| `CBattlezMapConfig::CheckQueuedSpawnTile` | 76.86 | 78.19 |
| `CGrunt::ChargeStep` | 82.95 | 83.91 |
| `CGrunt::StepArrivalDefenseAlt` | 78.95 | 79.83 |
| `CGrunt::StepArrivalDefenseLean` | 76.64 | 77.49 |
| `CGrunt::StepArrivalDefense` | 84.98 | 85.68 |
| `CGrunt::UpdateArrival` | 90.63 | 90.92 |
| `CBattlezMapConfig::Step` | 87.13 | 87.23 |
| `CGrunt::ScanNearestTarget` | 94.60 | 94.78 |

An earlier bound claimed `TrySeedSpawnAt` and
`StepRowUnits` "really do store the two fields independently" because the fold
cost them score. The target objs refute that: `StepRowUnits` has FIVE
`or eax,-1 / or ecx,-1` pairs and `TrySeedSpawnAt`'s single `or edx,-1` is a
`cmp eax,edx` sentinel, not a store at all. Those two rows lost on the earlier
fold because the fold used the one-temp spelling, so read a lost score as
"wrong spelling", not as "retail wrote it field-wise". The site set is decided
by the target's own byte census, never by the delta of a score.

An `or reg,-1` feeding a comparison or arithmetic is not this signature
(`or eax,-1; sub eax,edi` computes `~edi`). Trace both values into the adjacent
member stores before applying it.

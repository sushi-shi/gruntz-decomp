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

When the aggregate being written is a MEMBER and the value is a constant, the
signature is mechanical and greppable. `Coord::Set(-1, -1)` makes the two
arguments IL temps, so cl materialises `-1` in TWO registers; two independent
field stores use immediates:

```asm
; retail - the aggregate call            ; ours - two field assignments
or   eax,0xffffffff                      mov  DWORD PTR [esi+0x2f0],0xffffffff
or   ecx,0xffffffff                      mov  DWORD PTR [esi+0x2d4],0x0
mov  DWORD PTR [esi+0x2f0],eax           mov  DWORD PTR [esi+0x2f4],0xffffffff
mov  DWORD PTR [esi+0x2d4],ebx
mov  DWORD PTR [esi+0x2f4],ecx
```

Note the unrelated `m_defenderState` store scheduled BETWEEN the two coordinate
stores on the retail side: with `Set` the two stores are one IL pair that cl may
separate, which is why a transcription often reads `m_x = -1; <other>; m_y = -1;`
and hides the aggregate.

SCREEN A WHOLE LANE IN ONE PASS - count `or reg,-1` in each unit's base obj
against its target obj; a positive delta is that many missing aggregate writes
(each `Set` contributes two). Measured 2026-08-21: `battlezunitstep` base 8 /
target 24, `battlezmapconfig` 6 / 12, `gruntchargestep` 0 / 2,
`gruntdefensestep` 0 / 2, `gruntarrivalupdate` 1 / 2.

STEERABLE. `CGrunt::StepArrivalDefense` 83.00 -> 84.98,
`CGrunt::StepArrivalDefenseLean` 75.39 -> 76.64, `CGrunt::ChargeStep`
81.94 -> 82.95, `CGrunt::UpdateArrival` 89.96 -> 90.63,
`CBattlezMapConfig::Step` 86.81 -> 87.12, `CheckQueuedSpawnTile` 76.78 -> 76.86.

BOUNDS - it is per-site, not per-file. In the same sweep
`CBattlezMapConfig::TrySeedSpawnAt` fell 94.59 -> 91.90 and `StepRowUnits`
84.89 -> 84.79 under the same fold; both keep the transcribed pair, so those
retail bodies really do store the two fields independently. Fold, build, and
revert the sites that lose - the delta count tells you how many sites take it,
never which ones.

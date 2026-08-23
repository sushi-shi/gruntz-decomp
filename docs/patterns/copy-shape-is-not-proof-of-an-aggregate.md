# A consecutive load pair feeding two member stores is NOT proof of an aggregate
tags: cpp:struct cpp:local cpp:store asm:mov | topic:evidence-discipline topic:codegen-idiom
symptoms: two loads then two stores at [obj+K]/[obj+K+4], struct copy shape, Coord, POINT, RECT, aggregate assignment, `*p++` walk, incoming stack argument, spilled scalar pair
confidence: 9/10

cl 5.0 lowers a whole-object assignment of a two-field aggregate as ONE COPY: a
load pair off consecutive addresses feeding the two member stores. That is the
signature `walls aggdecl` screens on, and it is how
[[switch-destination-is-scalars-not-an-aggregate]] was decided. But THREE other
sources emit byte-for-byte the same shape, and each one produced a confident
census row before it was separated out:

* **a `*p++` cursor.** C2 may pre-load two steps of a walk.
  `CDDrawWorkerHost::ReadPlaneObjects` reads its record with one `*p++` per
  member on both sides and retail pairs the loads at exactly ONE of sixty
  sites. Detection: the source base register is bumped by a constant next to
  the pair.
* **two incoming stack ARGUMENTS.** Storing parameter N at member M and
  parameter N+1 at member M+4 is two scalar stores that read two adjacent
  frame slots. `StreamFeeder::FeederStart` emits the identical 63 instructions
  on both sides and the pair reads COPY on one side and SEP on the other purely
  from which parameter landed in which register. Detection: nothing in the
  function ever writes those slots.
* **a SPILL of two scalars.** cl gives them adjacent slots, so the reload pair
  is indistinguishable from an aggregate's frame home. `CGrunt::ClaimSwitchTile`
  still reads copy-shape under the two-scalar model that its own default arm
  proves correct.

```asm
; all four of these are the same four instructions
mov eax,DWORD PTR [<src>]      ; aggregate | *p++ walk | argument slot | spill slot
mov ecx,DWORD PTR [<src>+4]
mov DWORD PTR [<obj>+K],eax
mov DWORD PTR [<obj>+K+4],ecx
```

**So the shape is a LEAD, not a verdict** - carry the sub-kind with the row and
adjudicate against the disassembly. Measured over the 569-row sub-100 queue of
2026-08-23: 2342 comparable member pair-keys, 1957 agree, and after the three
look-alikes are classified apart only 6 OVER and 5 UNDER rows remain, of which
`CBattlezMapConfig::AdvanceToEnemyBase` was the one real modelling defect
(83.04 -> 83.05, a `Coord` destination two arms join into).

**Two extraction bugs make this measurement lie**, and both are worth naming
because they are generic to any pair sieve:

* **find the source load by REACHING DEFINITION, not a fixed look-back window.**
  Searching N instructions before the FIRST store makes the answer depend on
  where the scheduler put the loads: `CBattlezMapConfig::TrackAssignedEnemy`
  emits the identical eight-instruction RECT copy on both sides and a
  ONE-INSTRUCTION rotation put one load inside the window on one side and
  outside it on the other. That alone was 44 of the first sweep's 55 rows, all
  in one direction, all plausible.
* **unfold the displacement through `lea R,[base+K]`.** cl parks `obj+0x60` in
  a register and stores at `[R+0x8]`, which is member 0x68; key on the raw
  displacement and every key on that side shifts at once. Same hazard
  `walls reloadscan` documents as the base fold.

Calibrating the finished sieve on the rows already at 100% gives 2243
comparable keys, 2243 agreeing, zero rows - a detector-bug rate of 0, which is
what makes the eleven surviving rows worth reading.

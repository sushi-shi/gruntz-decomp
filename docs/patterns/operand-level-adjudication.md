# The diagnose ladder classifies a wall; operands and referents adjudicate it
tags: topic:method topic:tooling topic:wall | asm:mov asm:fild asm:lea | cpp:member cpp:rect
symptoms: diagnose says regalloc but the function is wrong; masked diff shows "identical asm"; b0/tN; exclusive key; fild count differs; member displacement differs; store source differs
confidence: 9/10

`walls diagnose` answers "what CLASS of divergence is first" (referent ->
inline/call-set -> cfg -> regalloc). It cannot answer "is this body correct":
its call-set and branch-skeleton view is blind to a member read at the wrong
displacement, a store fed from the wrong field, a dropped conversion, or a
mask that lost a bit. Every substantive source-shape or runtime defect found
on 2026-08-20 sat under a `regalloc` or `inline` classification.

Adjudicate with `gruntz walls semdiff <fn>` (sweep a worklist with
`walls semsweep <tsv>`). Read the **EXCLUSIVE** section first: a key ONE side
uses and the other never touches. A `b3/t4` on a shared key is scheduling; a
`b0/t2` is a constant, offset or conversion that exists on one side only.

```asm
; the four signals, in the order they pay off
fp     fild   base 3 target 5   ; cl NEVER adds/drops a conversion to schedule
disp   +0x5c  base 4 target 5   ; a field one side reads and the other does not
store  +0x174 base 1 target 0   ; a member written on one side only
imm    0x1388 base 0 target 1   ; a constant that exists on one side only
```

WORKED EXAMPLE 1 - a dropped conversion is a wrong initializer.
`CProjectile::LoadProjectileSprites` 0xdf050 screened `fp fild b3/t5`,
`disp +0x5c b4/t5`, `disp +0x60 b4/t5`: retail files two more integers, from
`m_object->m_screenX/Y`.

```cpp
    m_posX = vx;                 // WRONG: the unit direction vector
    m_posY = dy / len;
    m_posX = m_object->m_screenX;   // retail: the shooter's position
    m_posY = m_object->m_screenY;
```
Every projectile integrated its flight from ~(+-1,+-1) - the wrong-spawn-
coordinate bug the user saw. Fixed in 6a45165ce.

WORKED EXAMPLE 2 - an inverted arm plus a missing tail.
`CBattlezMapConfig::RouteToNearbyEnemy` 0x2e3a0 carried `@early-stop`; the
exclusive immediates (0x1388, 0x366) and the store displacements 0x78/0x7c/
0x80/0x84 named a whole block retail runs and we did not.

```cpp
    if (RouteUnitTo(unit, bc.m_x, bc.m_y, 0x1000d8f, flags, 1) == 0) {   // WRONG arm
    if (RouteUnitTo(unit, bc.m_x, bc.m_y, 0x1000d8f, flags, 1) != 0) {   // retail
        ...  m_routeTimers[0].m_v = 0; ... SpawnVoiceDriver(unit, 0x366, ...)
```
Retail zeroes BOTH timers before re-arming - eight stores at 0x2e9e2 - and we
emitted six. Fixed in fd8706cd3.

WORKED EXAMPLE 3 - the same VALUES computed from a different basis.
`CStatusBarMgr::BuildTabzDialog` 0x10a340 screened exclusive immediates on
both sides (ours 0x11/0x12/0x16/0x31/0x3c/0x45/0x7c/0x7d/0x8e; retail 0x7a x5,
0xc4, 0xfd, 0x9e, 0x64, 0x11b, 0x11c, 0x90). Not a wrong rect: a different
ORIGIN.

```asm
10a619: sub esi,0x8e / sub edi,0x48   ; retail rebases the centre to the corner
        lea eax,[esi+0x11] ... add edi,0x7a
        ; ours, from the centre:  lea eax,[esi-0x7d] ... add edi,0x32
```
Each constant shifts by the same 0x8e / 0x48, so the geometry is identical and
the fix is arithmetic, not behavioural: 86.47 -> 90.30 (caed0e319).

WORKED EXAMPLE 4 - a loop-guard rotation IS a boundary change.
`WarpTextureBlit` 0x146a20 tested the shift bound at the BOTTOM
(`for(;;){ ...; shift++; if (shift >= 0x20) break; }`) where retail tests at
the top (`while ((u32)shift < 0x20)`) - one fewer branch, and a different
value on the exhausted-scan path. Fixed in 591491fa3.

WORKED EXAMPLE 5 - an exclusive member displacement plus a dead local of the
same member is a dropped call argument. `CUFO::CUFO` 0xb4a90 screened one key,
`disp +0x60 base 0 target 1`; the source read `i32 sy = o->m_screenY;` but
passed literal zero to `CreateSprite`, so cl deleted the load. Retail saves
obj+0x5c and obj+0x60 and pushes both as the second and third arguments. A dead
local whose member displacement exists only in retail is evidence for a
dropped statement or argument, not allocator residue.

WORKED EXAMPLE 6 - the mirror case: a store displacement present only in base
can identify one statement too many. `CGruntzMgr::Close` 0x855e0 screened
`disp/store +0xc base 1 target 0`; its hand-written `StateMgrBZ` teardown
cleared `m_mouse`, while retail skips that member. The sibling teardown in
`CGruntzMgr::Run`'s failure path independently emits the same five-store
sequence as retail. Compare repeated expansions of the same source entity
before treating an exclusive store as harmless scheduling.

TRAP - a referent-sequence DELETE of a call is not automatically a missing
call. `CBattlezMapConfig::AdvanceToEnemyBase` 0x33...  screened
`delete base[11:12] B ?RemoveAll@CPtrList@@QAEXXZ`, and the call SITE counts
agree with it: base 4, retail 3. It is still not a source difference. Retail
reaches the third site from a fourth logical path by cross-jumping into it
(`jmp 0xefe`, straight at another drain's `lea ecx,[esi+0x31c]`), so the
statement is executed on all four paths and only the emitted site count
differs. ALWAYS grep the retail side for a `jmp` into the surviving site's
address before deleting the statement: count the PATHS, not the call sites.

DISCIPLINE. Ordinary count deltas are not evidence: the taxonomy in
`gruntz.walls.semdiff`'s docstring lists eleven classes that produce a
difference with identical semantics (register mirrors, cross-jump merge
degree, lea-folded displacements, operand width, rep-stos first-dword split,
RMW split/fold, FP-stack housekeeping, jump-table data, byte-continuation
lines, frame-size immediates, one-past-end referent aliasing). Three are
filtered mechanically; the rest are read. Relocated operands are MASKED, so a
swapped pair of string keys or a mis-bound global is invisible to every value
multiset - that is what the referent-sequence and cmd/key sections are for.

Steerable: no - this is a method, not an idiom. Evidence: 379 rows screened in
one pass; four bugs found, all under a non-cfg classification; the same screen
cleared the whole resource-belt chain (LoadTabSprites 90.08, LoadChipMachine-
Config 94.15, Sync 90.43) as EQUAL.

# The emitted member-store run is NOT the source order - try declaration order

## Symptom

`walls diagnose` says REGALLOC/SCHEDULING with **identical** byte length,
instruction count, call count, branch count and relocation count, and the whole
divergence is one straight-line run of `mov [this+off],<reg>` stores whose
OFFSETS appear in a different order on the two sides, with the parameter loads
that feed them permuted the same way.

    BASE (ours)                     TARGET (retail)
    mov edx,DWORD PTR [esp+0x10]  | mov edx,DWORD PTR [esp+0x1c]
    mov DWORD PTR [esi+0x8],ecx   | mov DWORD PTR [esi+0x8],ecx
    mov ecx,DWORD PTR [esp+0x1c]  | mov ecx,DWORD PTR [esp+0x10]
    mov DWORD PTR [esi+0xc],eax   | mov DWORD PTR [esi],edx
    ...

## Mechanism

C2 schedules a straight-line store run for its own reasons - it stores a value
that is already register-resident before one that still needs a load, and it
picks which parameter load starts the merge block. So retail's EMITTED order is
an output of the scheduler, not a picture of the devs' source.

A reconstruction that transcribes the emitted order therefore feeds C2 an input
it never had, and C2 reschedules THAT, producing a different permutation. The
fixed point is usually the order a person would actually write: the class's
**declaration order**.

## Evidence

`CActionOptionsMenuBar::Init` 0x00009220 (`actionoptionsmenubar`) sat at 95.28
with 53 instructions, 0x8f bytes, 1 call, 6 branches, 2 relocations on both
sides - every count equal, first divergence at +0x54, in the six-store run.

The source carried retail's emitted order, transcribed:

    m_screenX = x;                      // +0x08
    m_gridX = gx;                       // +0x00
    m_screenY = yy;                     // +0x0c
    m_buttonState[1] = secondaryState;  // +0x18
    m_gridY = gy;                       // +0x04
    m_buttonState[0] = primaryState;    // +0x14

C2 emitted 8, c, 0, 18, 14, 4 from it - the source order with two adjacent pairs
transposed. Rewriting the run in the header's declaration order
(`m_gridX, m_gridY, m_screenX, m_screenY, m_buttonState[2]`):

    m_gridX = gx;
    m_gridY = gy;
    m_screenX = x;
    m_screenY = yy;
    m_buttonState[0] = primaryState;
    m_buttonState[1] = secondaryState;

emits 8, 0, c, 18, 4, 14 - retail's order, **100.00% EXACT**, no other change.

## The stronger form: the run is one OBJECT, so write one assignment

Declaration order is the remedy when the run really is N independent members.
When the permuted offsets are the fields of ONE embedded object, the humane
spelling is the object copy, and it is a better fixed point than any ordering,
because it removes the ordering question from the source entirely.

`CGrunt::IsDropReady` 0x00051510 (`gruntsteps`) carried the two `Coord` cursor
updates field-wise:

    m_lastTilePx.m_x = m_commitPx.m_x;      // +0x17c
    m_lastTilePx.m_y = m_commitPx.m_y;      // +0x180
    m_commitPx.m_x   = m_entrancePx.m_x;    // +0x184
    m_commitPx.m_y   = m_entrancePx.m_y;    // +0x188

That is already declaration order, and C2 still transposed the first pair
(emitting 180, 17c where retail emits 17c, 180) because the y value was
register-resident and ECX was wanted for the next argument. Writing what the
object copy actually is:

    m_lastTilePx = m_commitPx;
    m_commitPx   = m_entrancePx;

lands the run on retail's order: **98.76 -> 99.84**, residue reduced to a
five-instruction EDI/EDX rotation elsewhere in the body. Applied at the four
coord-pool clone sites the same way, `RepathAroundBlockedTiles` went
72.97 -> 73.68 and `PhaseStep` 83.37 -> 83.79.

The counter-case is worth knowing: `CTriggerMgr::UnregisterUnit`'s `Coord pt` is a
reconstruction of a retail SPILL PAIR, not a source local
(`dead-eight-byte-coord-temp-is-unreproduced.md`), and copy-initialising it
costs 5 points. Convert a run to an object copy only where the object is real.

## Use

On an all-counts-equal store-run residue, ask in this order:

1. are the permuted offsets the fields of one embedded object? write the copy;
2. otherwise try the members' declaration order.

Either costs one build, is the humane spelling, and removes a transcription
that was fitted to the output.

`gruntz walls storescan [--todo]` finds the rows mechanically: it reports every
paired function whose member-store runs are permutations of each other, and
separates the ones where bytes, instructions, calls, branches, returns,
relocations and the `semdiff` multisets are ALL equal. Of the 578-row campaign
queue on 2026-08-23: 274 rows carry a store run of 3 or more, 56 have a
permuted run, 13 of those also have every count equal, and 11 have equal
multisets too. `--values` is the companion screen for the live-bug case (two
offsets that EXCHANGED their constants rather than their order); it found none
in that queue.

## When the permutation is a consequence, not the defect

Four false-positive classes, each observed on a flagged row:

* **Shared-inline expansion.** The run comes from an inline expanded at N
  sites. If any sibling expansion is EXACT the order is already proven.
  `CGameLevel::SetCoordExtents` (84.28) flags on the scroll-parameter block,
  but `SetCoords`, `ResetSpatialDefaults`, `LoadFileWithCoords`,
  `LoadSourceWithCoords` and `LoadWwdWithCoords` all expand the same
  `SetSpatialDefaults()` at 100.00 - so the block's declaration order is
  correct and the residue is one immediate store filling a load-use shadow.
* **Regalloc serialization.** Retail keeps two constants live in two
  registers; ours funnels both through EAX, which FORCES the group reorder.
  `CBattlezMapConfig::CBattlezMapConfig`: retail holds `eax=0xbb8` and
  `edx=0x7d0` across the timer block, ours materialises 0xbb8, drains its two
  stores, then reloads EAX with 0x7d0. The store permutation is downstream of
  the register choice; no source ordering reaches it.
* **Frame / local-count wall.** A `sub esp` delta shifts every slot and the
  store swap rides on it (`CGrunt::ResolveArrivalReposition`, frame 0x8 vs
  0xc). Run `walls framescan` first.
* **vptr-stamp placement.** The moved "store" is the `??_7` stamp, which is
  not a source statement at all (`CActionArea` and `CGruntVoice` ctors, where
  retail sinks it past the trailing i64 zero stores and we do not).

Related: `param-store-last-forces-callee-saved.md` (same "emitted order does not
name the source order" mechanism, seen through the callee-saved push count) and
`imm-store-floats-to-end-of-store-run.md` (the same scheduler, immediate stores).

## The counter-case: when C2 emits the run verbatim, retail's order IS the source order

The "declaration order is the fixed point" rule assumes C2 permutes whatever you
feed it. It does not always. `CGrunt::LoadWandGruntItemConfig` 0x65a60 carried

    m_stamina = 0;                  // +0x3f0
    m_lowStaminaCued = 0;           // +0x460
    m_attackClockLo = g_frameTime;  // +0x860
    m_attackClockHi = 0;            // +0x864
    m_attackDowntimeLo = downtime;  // +0x868
    m_attackDowntimeHi = 0;         // +0x86c

which is ascending declaration order, and C2 emitted 3f0, 460, 868, 860, 864, 86c
- close to the source with one pair rotated. Retail emits 868, 86c, 860, 864, 460,
3f0: the two i64 halves stay paired, but the DOWNTIME pair precedes the CLOCK
pair and the two scalars come last, which is not the class's declaration order in
either direction. Writing exactly that order emitted it verbatim, every store on
retail's offset, mnemonic residue 17 -> 5.

So the diagnostic is whether the run C2 gave you is a PERMUTATION of your source
(scheduler at work - go to declaration order or the object copy) or a faithful
echo of it (no scheduling happened - transcribe retail's order, it is the source
order). Note the current fuzzy can DIP while the structure is corrected: this one
went 96.31 -> 95.13 because the following `m_healthSprite` test stopped floating
into the middle of the run.

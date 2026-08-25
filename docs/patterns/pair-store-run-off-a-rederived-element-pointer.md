# A pair-store run addressed off ONE re-derived element lea: the source bound a pointer to the MEMBER

tags: cpp:local cpp:member cpp:struct | asm:lea asm:mov | topic:codegen-idiom
confidence: 10/10 (six EXACT flips in one TU)
symptoms: diagnose says REGALLOC/SCHEDULING and EITHER (a) retail emits a
SECOND fresh `lea (%base,%idx_adj,scale)` whose index is the element index
PLUS a constant (`[idx + K]`) with the run at `+0/+4/+8/+0xc` off it while we
CSE one unfolded `this + idx*elem` base and scatter LARGE member
displacements, OR (b) a single pending store is SUNK past the next
statement's load/cmp into a pairing shadow (retail keeps source store order),
OR (c) an indexed walk's cursor IV anchors at the WRONG field offset

cl 5.0 folds a constant byte-offset D into a scaled index only when D is a
multiple of the element size (`D == K*elem` rewrites the address as
`[idx + K]`), and it does this PER ADDRESS EXPRESSION. Two consequences:

- If the source reaches a member sub-object through the SAME element
  expression as its neighbours (`m_slots[idx].m_startTimeLo = ...;` next to
  `sp->m_state = ...`), cl CSEs the common `this + idx*elem` sub-expression
  across the webs and every store becomes a big displacement off that one
  base. The fold never fires for the non-multiple field offsets.
- If the source BINDS A POINTER to the interior pair and stores through it,
  that pointer is a single address value: cl folds its (multiple-of-elem)
  offset into the index — `&m_slots[idx].m_startTime` with elem 0x18 and
  offset 0x228 emits `lea (%esi,%eax,8)` with `eax = (idx+0x17)*3` — and the
  run lands at `+0/+4/+8/+0xc` off it. The syntactically different folded
  form is NOT CSE'd with the sibling element pointer, which keeps retail's
  two separate leas.

So the retail signature above IS the statement structure: the run was written
through a bound pointer to the interior pair, not as per-member statements.

Worked flip: `CStatusBarMgr::LoadGooCookingSprite` 0x001055b0, 89.13 ->
100.00 EXACT on this change alone:

```cpp
// before (three member statements; cl CSEd the slot base, scattered
// +0x22c/+0x230/+0x234 displacements, folded only the +0x228 store):
m_slots[idx].m_interval = INT_MAX;
m_slots[idx].m_startTimeLo = g_frameTime;
m_slots[idx].m_startTimeHi = 0;

// after (the SyncClockPair i64* device - the slot tail IS the
// {startTime, interval} clock pair that unit serializes as a pair):
i64* clock = &m_slots[idx].m_startTime;
clock[1] = INT_MAX;          // interval lo/hi at +8/+0xc
clock[0] = g_frameTime;      // last lo/hi at +0/+4, hi zero-extended
```

The binding is load-bearing THREE ways, and the second works even at a
CONSTANT member offset where the address itself folds away:

1. INDEX-FOLD (above): a fresh folded lea that never CSEs with the sibling
   element pointer. LoadGooCookingSprite 0x1055b0 89.13 -> 100.00 EXACT.
2. ALIAS OPACITY: cl 5.0 cannot prove an `i64*` store does not alias a later
   MEMBER load, so pointer stores pin the source statement order against the
   scheduler's shadow-fill sink. (2026-08-18: this half is not special to
   `i64*` - ANY store pins the order of ANY following named-datum or
   distinct-base load, because cl's disambiguator only separates accesses that
   share a base and have disjoint CONSTANT offsets. So order-pinning is
   available from plain member statements placed in the right position; the
   `i64*` device earns its keep through mechanisms 1 and 3.
   docs/relevations/wall-reasons-globalopt.md §1, §4.) This broke the whole single-store-sink
   family that had been misclassified as C1 handle-state (probe-inert,
   /G3-/G6-invariant): SetHudRectA/B 0x1066f0/0x106740 71.83 -> 100.00
   EXACT, UpdateDestructWarningAnimation 0x10b320 94.67 -> 100.00 EXACT,
   LoadMultiplayerBattlezConfig 0x107ae0 98.06 -> 100.00 EXACT.
3. IV ANCHOR: in an indexed walk, the bound pair address becomes the cursor
   IV, so every sibling field reads at `-8/-4` and the pair at `+0/4/8/c`
   (UpdateRezConveyorStatusBar 0x105990 96.26 -> 100.00 EXACT: retail
   anchors `esi = &m_groupSlots[i].m_last`, sibling state/counter at
   -0x8/-0x4).

The alias-opacity signature also applies to a fixed-size scalar member array. In
`CTileTriggerContainer::SetCell` (0x117f60), writing four player flags directly let cl load
`elem->m_actionCode` before the store run. Binding `i32* flags = elem->m_playerFlags` kept the
four ascending stores together and pinned the action-code load after them, matching retail's
statement schedule and raising 82.5735% → 85.4412%. This is not a generic pointer-style rule:
the evidence is the retail run of `+0/+4/+8/+0xc` stores followed by a load from the owning
object, and the pointer names the real array subobject.

The i64-pair pointer is the established in-tree modeling of these
{timestamp, interval} tails (`SyncClockPair(CFileMemBase*, SerialMode, i64*)`
walks `pair` and `pair + 1` in `CStatusBarMgr::Sync`); prefer a real
aggregate member where the layout allows one.

THE POINTEE TYPE IS NOT PART OF THE MECHANISM (measured 2026-08-17): where
the pair IS a real aggregate (`SbiClockPair m_destructWarnClock`), spelling
the device as `SbiClockPair* clock = &m_destructWarnClock;` with
`clock->m_last` / `clock->m_interval` accesses is BYTE-IDENTICAL at all
three sites (UpdateDestructWarningAnimation held 100.00 EXACT, full-tree
score line unchanged, 0 fresh). All three powers - index-fold, alias
opacity, IV anchor - come from materializing the pair's address and storing
through a pointer cl cannot disambiguate; `i64*` indexing past a member is a
raw-offset access and is kept ONLY where the pair exists as loose fields
(the CSbiSlot/CSbiHlRow tails, whose SbiClockPair embedding is blocked by
MSVC5's ctor-in-union rule on the two-readings device).

Negative controls, same session: binding SIMPLE copies or single negations to
locals (`u32 t = z;`, `i32 negX = -topX;`, an `i32*` to a member that the
condition already loads once) does NOT survive C1 - plain copy webs are
forward-propagated and the emission is byte-identical. The pointer must
anchor a MULTI-ACCESS pair run. And it is SITE-SPECIFIC, not a blanket
spelling: UpdateRezConveyor's OWN m_beltClock writes match retail as plain
member statements, and converting Sync's nested hlGrid walk to a flowing
`i64*` cursor scored 90.43 -> 83.19 (reverted) - apply it only where the
retail bytes show one of the three signatures.

# Retail recomputes `v >> k` where the recompile CSEs it

tags: cpp:expr cpp:inline cpp:local cpp:loop | asm:mov asm:sar | topic:codegen-idiom topic:wall
symptoms: retail loads a member once, copies the raw value, and shifts both copies; the
recompile shifts once and reuses the result
confidence: 9/10

Do not classify this shape as an optimizer anomaly until the two consumers' source
abstraction has been recovered. A scalar `PxToTile(px)` helper may still fold, while a
pair-valued helper reached through the higher-level owner can give C1 two distinct trees:
one expansion produces the equality-test coordinates and the caller separately retains the
raw coordinates for later distance arithmetic.

The controlled positive is `CBattlezMapConfig::RepathToFreeCell` at `0x350d0`. Retail
loads `m_screenX` and `m_screenY` once, copies them for a same-cell test, shifts those
copies, and shifts the originals again in the distance block. The original direct
transcription CSE'd each shift and scored 77.7977 with a `0x8` frame. These one-layer
spellings did not recover retail:

- named shifted locals;
- shifts written directly at every use site;
- repeated member expressions with no local;
- a scalar or raw-pair pixel-to-tile helper.

They were incomplete controls, not proof that the source shape was unreachable. The
effective composition was:

```cpp
static inline Coord ScreenTile(CGrunt* unit) {
    Coord out;
    CGameObject* object = unit->m_object;
    out.m_x = object->m_screenX >> TILE_SHIFT_PX;
    out.m_y = object->m_screenY >> TILE_SHIFT_PX;
    return out;
}

CGameObject* object = unit->m_object;
i32 screenX = object->m_screenX;
i32 screenY = object->m_screenY;
Coord current = ScreenTile(unit);
```

That recovered retail's four shifts, cursor spill, and `0x10` frame, moving 77.7977 to
83.9326. The identity boundary is load-bearing: passing the cached `object` into the
helper lets C2 correlate the two layers and collapses back to the CSE island. A helper
that accepts the `CGrunt*` keeps the higher-level accessor expansion distinct from the
caller's raw object snapshot.

Two independent source facts then composed on this base. Reading a `CPtrList` node with
`GetAt(pos)` before advancing it with `GetNext(pos)` restored retail's payload-before-next
load order and reached 85.46. Declaring/initializing `POSITION pos` before `best` and
`bestDist` restored the callee-save push and list-head schedule and reached 89.6742.
Moving `bestDist` ahead of `best` instead fell to 88.3820, consistent with retail storing
the null best pointer before the distance sentinel.

Three target-adjacent 32-state campaigns were single-island controls: two on the earlier
four-shift bases and one on the final 89.6742 source hash. The retained function has exact
call, branch, return, relocation, semantic-operand, and frame counts except for two extra
retail `mov`s and register/schedule choices. That residue is bounded for this hash; the
source reconstruction above is still a reusable positive mechanism.

`CBattlezMapConfig::RerouteSwitchSeeker` (`0x35f10`) independently confirmed the
mechanism. A scalar `PxToTile(i32)` helper was byte-flat because it preserved one
expression layer. Keeping caller-owned raw screen coordinates, restoring a pair-valued
`ScreenTile(CGrunt*)`, and composing a second neighborhood boundary containing two such
expansions recovered all four bound shifts plus the two in-loop center shifts:
77.9664 -> 86.5714. The authored-order matrix mattered: X-before-Y pair construction and
upper-before-lower result declaration produced 120 instructions, exactly matching
retail's call/branch/return/relocation topology and instruction count; the reverse orders
landed on lower islands. A 64-state target-adjacent campaign on that final source hash
was completely flat. The retained layer remains an open base because its `0x1c` frame
and register assignment still differ from retail's `0x14` frame.

The remaining superficially similar row is `CTriggerMgr::FindNearestEnemy` (`0x77df0`).
Do not transfer either positive mechanically: first look for a pair-valued or owner-level
accessor boundary and independently recover its caller-side raw-value lifetime.

Reverse-use rule: when retail CSEs the loads but duplicates arithmetic, classify the
consumers by semantic layer. Test a real pair/aggregate return or higher-level inline
accessor while keeping the raw source value alive separately. Confirm the feature was
absent from baseline, and compose local declaration order before launching compiler-state
search. Do not use volatile carriers, fake calls, or duplicate loads to imitate the code.

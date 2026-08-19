# A post-call inline helper must receive call-stable inputs, not re-read members
tags: cpp:inline cpp:local cpp:call | asm:mov asm:sar | topic:correctness topic:wall
symptoms: retail enters an inlined collision/search region after an opaque call and reads tile coordinates from homes populated before the call; the candidate reloads receiver fields and shifts them again
confidence: 8/10

An opaque call kills knowledge of receiver-owned memory. If an inline helper reads
`g->position` after that call, cl 5.0 cannot replace the read with values computed
before the call: the callee may have changed the object. When retail instead consumes
the old values, the source carried call-stable locals into the helper (or contained the
equivalent open-coded region). Re-reading the member is a semantic reconstruction bug,
not harmless spelling.

`CGrunt::StepCompassMove` is the control. Its toy search follows CString and
`CButeMgr::GetDwordDef` calls; its fallback search follows `rand` and CByteArray calls.
Both retail collision expansions use the original `tx`/`ty` homes. The old helpers
reloaded `m_lastTilePx.m_x/.m_y` after those calls. Passing the original pixel locals
and deriving their tile coordinates inside each inline helper preserves the value that
retail actually uses:

```cpp
static __inline i32 CanCommit(
    CGrunt* g, i32 moveX, i32 moveY, i32 sourceX, i32 sourceY) {
    i32 tx = sourceX >> TILE_SHIFT_PX;
    i32 ty = sourceY >> TILE_SHIFT_PX;
    // ...
}
```

The ordinary early-return form reaches retail's 129-branch skeleton and lands within
three decoded instructions (887 candidate versus 884 retail). It remains a bounded
allocation/layout wall: 32 deterministic TU states yielded one meaningful instruction
island, and 128 syntax-aware declaration/statement-order variants did not recover the
retail cardinal-arm folding. The retained source therefore follows the value-flow
evidence, not the transient fuzzy score.

Negative controls matter here. Passing `tx`/`ty` directly produced 897 instructions
and 131 branches. Converting the toy helper to explicit single success/failure labels
raised fuzzy but changed the authoritative CFG to 130 branches; doing the same to the
fallback helper produced 132. Explicitly sharing the arrow arms with `goto` labels
collapsed the graph to 126 branches. None is a valid substitute for the evidenced
call-stable input.

Reverse-use rule: when retail reuses a pre-call stack home but the candidate reloads a
member after the call, first recover the stable source value and carry it across the
inline boundary. Do not add a cache, reference parameter, volatile carrier, or TU-state
probe merely to force the home. The remaining frame/register/layout residue is a
separate wall after value identity is corrected.

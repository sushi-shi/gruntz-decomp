# cl deletes a switch arm retail emits: the dominating member test was proved

tags: cpp:switch cpp:branch cpp:member | asm:cmp asm:jne | topic:wall
symptoms: `insn_seq --multiset` shows retail calling one MORE of two or three callees
that all live in the same `switch` arm, the base is 90-120 instructions short, and
editing that arm's source changes the base by exactly zero instructions
confidence: 9/10

A `CGrunt`-style step function has the shape

```cpp
if (m_poweredUp != 0) {
    ...                       // every path returns
    return 1;
}
switch (m_defenderState) {
    ...
    case AISTATE_ATTACK:
        if (m_poweredUp != 0) { <40-100 instructions> }
        m_defenderState = AISTATE_CHASE;
        m_dwell = DWELL_REPATH_MS;
        return 1;
}
```

The only edge into the `switch` is the `je` of the outer test, so `m_poweredUp == 0`
holds there; nothing between the branch and the arm calls or stores. Our cl propagates
that and deletes the arm's body outright, leaving the three-instruction tail.

**Retail does not.** It keeps the outer load in a register (`mov ecx,[esi+0x220]`,
`cmp ecx,ebp`), keeps `ecx` live across the jump-table/dec-chain dispatch, and
re-compares it in the arm (`cmp ecx,ebp; jne <body>`) — so the arm is emitted in full
even though it is unreachable. The arm body is dead code in retail too; only the
optimizer's reach differs.

**Diagnosis (do this before touching the arm):** edit anything inside the arm and
rebuild. If the instruction count does not move by one, the arm is not being emitted
at all and no amount of re-spelling inside it can matter — the deficit is this wall.

Measured negatives (do not repeat): hoisting `m_poweredUp` into an `i32` local used by
BOTH the outer test and the arm does not stop the fold (`CGrunt::ChargeStep` moved +4
instructions, all from the single load, and the arm stayed deleted); restructuring the
arm's shared `goto` tail is byte-invisible for the same reason.

Evidence: `CGrunt::ChargeStep` 0x000ef6b0 (retail 465 insns vs 374; extra GruntInRadius +
RectContains + a second ResetEntranceAnimation) and `CGrunt::UpdateArrival` 0x000f0130
(retail 568 vs 449; extra GruntInRadius + RectContains + SpawnVoiceDriver +
ResetEntranceAnimation). Both arms are `if (m_poweredUp != 0)` under an outer
`if (m_poweredUp != 0) { ... return 1; }`.

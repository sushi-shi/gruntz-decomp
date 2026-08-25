# Retail gives each entry guard its own `return` epilogue where cl shares one
tags: cpp:branch cpp:switch cpp:goto | asm:jcc asm:ret asm:jmp | topic:codegen-idiom topic:wall
symptoms: base and target agree instruction-for-instruction but the base has FEWER `ret` blocks; `gruntz walls diagnose --asm` shows `jcc <shared tail>` in the base against `jcc <continue> | fall <inline epilogue>` in the target; the base is short by a whole multiple of one epilogue; `insn_count` reports a negative delta with no operand differences anywhere
confidence: 9/10
variants: tail-block-placement-cross-jump-wall.md, single-predecessor-tail-block-gets-replicated.md, error-report-guard-falls-through-to-a-shared-return.md

A function with several source-identical `return <const>;` sites where retail
emits the epilogue INLINE at some of them and cl folds all of them onto one far
block. The instruction streams are otherwise identical, so the whole score gap
is the missing copies. It is the same asymmetry as
`single-predecessor-tail-block-gets-replicated.md` (cl5 replicates a small
`return` block into a predecessor only while that block has exactly ONE
predecessor) seen from the other side: cl merges the identical returns FIRST,
the merged block then has 4-5 predecessors, and the replication never fires.

```asm
; TARGET - each entry guard keeps its own epilogue, laid out as the fall-through
    cmp    esi,ecx
    jne    0x7a77f              ; jump OVER the return
    xor    eax,eax
    pop    edi
    pop    esi
    pop    ebp
    pop    ebx
    add    esp,0x18
    ret    0x4
0x7a77f:
    mov    eax,DWORD PTR [ebp+0x22c]
    cmp    eax,ecx
    jne    0x7a799
    xor    eax,eax             ; ... and again, byte-identical
    ...
; BASE - the branch is inverted and every return-0 site shares one far block
    cmp    esi,ecx
    je     <shared epilogue at the end of the function>
```

## STEERABLE when the sites are switch arms: `break` to ONE trailing `return`

`CStatusBarMgr::SetTabState` 0x100d70 was listed below as a wall and is now **100.00
EXACT**. Seventeen arms each ended in `return 1;` and cl chain-merged three of them
where retail merges two. Replacing every arm's `return 1;` with `break;` and letting
the single `return 1;` after the `switch` carry them closes it outright:

```cpp
// BEFORE - 88.53: each arm returns; cl cross-jumps arm1->arm2->arm4
    case SBICMD_TAB_STATZ:
        if (m_hlBusy) { return 1; }
        m_statzTabButton->SetState(state, 1);
        ...
        return 1;
// AFTER - 100.00: each arm breaks; cl replicates the tiny epilogue per arm
    case SBICMD_TAB_STATZ:
        if (m_hlBusy) { return 1; }        // the GUARD keeps its own return
        m_statzTabButton->SetState(state, 1);
        ...
        break;
    }
    return 1;
```

Counter-intuitive but consistent with
`single-predecessor-tail-block-gets-replicated.md`: a `return` per arm gives cl N
source-identical return statements to merge FIRST, and the merged block then has too
many predecessors to replicate. One `return` reached by N `break`s is a single block
that the layout pass duplicates into each arm - which is exactly retail's shape. Note
the early-exit guard inside an arm (`if (m_hlBusy) return 1;`) STAYS a `return`: retail
gives each of those its own epilogue copy too.

Untested but indicated by the same reasoning: the three sites below whose duplicated
epilogues are also switch-arm returns.

**Wall (the non-switch sites).** Measured and rejected: `if (p == NULL) return 0;` vs `if (!p) return 0;`
vs `if (p == NULL) { return 0; }` (identical); routing the deep sites through
`goto fail; ... fail: return 0;` so the entry guards are single-predecessor
(byte-identical - cl merges the returns before the layout pass); an explicit
`default:` arm instead of falling out of a switch (identical); and adding an
explicit `case <zero>:` arm, which additionally re-lowers the switch from a jump
table to a compare chain (91.37 -> 66.42). Sites:
`CStatusBarMgr::SetTabState` 0x100d70 - **CLOSED 100.00 by the `break` form above**,
`CDDrawFrontSurface::SetGeometry` 0x1644a0 (retail emits the
`WORLDERR_CREATE_DEVICE` block twice - once for the switch default, once for the
`err == 0` else - we fold them and both branches target one address, 91.37),
`CTriggerMgr::HandleTargetSelection` 0x79520 (retail merges all three cursor-spawn arms into
one tail, we leave the third out, 90.74), `CTriggerMgr::ScanGroup` 0x7a760 (three
`return 0` epilogues vs our one; every other byte matches, 89.48). Try the `break`
form on any of those whose sites are switch arms BEFORE calling it a wall.


## NOT always a wall - re-check for the hidden `||` first (2026-08-08)

`ScanGroup` was on that list and was **not** a wall: a single
`if (X) { ... return 0; } else { return 0; }` at the far end of the body is
byte-identical to `||`, i.e. the TOTAL merge regime, and it was collapsing five
guards written as plain separate `if`s. Flattening it plus `goto fail;` on the
deep sites took it **89.48 -> 99.19** with 31/31 blocks agreeing - and exposed a
real semantic bug on the way (see
[if-else-both-arms-return-is-the-or-regime.md](if-else-both-arms-return-is-the-or-regime.md)).
So before accepting this signature, grep the body for that shape; it does not
grep as `||`.

`HandleTargetSelection` 0x79520 IS still a wall, but for a different reason than the entry
above says: its unmerged arm's tail is genuinely **not identical** to the other
two. cl hoisted the `m_logicRecord` reload above the argument pushes in that one
arm (`mov eax,[esi+0x7c]` / `mov ecx,[eax+0x18]` before `push 0x1; push 0x3`),
where retail leaves the reload in the shared block. cl's suffix matcher then
correctly declines. The failed cross-jump is a CONSEQUENCE of a scheduling
choice, not a merge-policy difference - 45 of its 46 blocks are byte-identical.

## SOLVED for SetTabState - the arm terminator picks the pass (2026-08-08)

`SetTabState` 0x100d70 is **not** a wall either: `return 1;` in all fifteen arms is
what feeds cl's early cross-jump. Give every arm `break;` and let ONE trailing
`return 1;` carry them and the late layout pass replicates the epilogue into each
arm instead - **88.53 -> 100.00 EXACT** on that edit alone. The early-exit
`if (m_hlBusy) { return 1; }` guards INSIDE the arms keep their own `return`.
Full recipe and its discriminator:
[switch-arm-break-not-return-replicates-the-epilogue.md](switch-arm-break-not-return-replicates-the-epilogue.md).

Re-measured on the other two entries above, same session: the break form is
**byte-neutral** on `HandleTargetSelection` (90.7352 either way - its arms share an
`Activate(...)` suffix, not just an epilogue) and **91.37 -> 69.81** on
`SetGeometry` (retail's arms return directly). `SetGeometry`'s real residue is
an ENCODING accident, not a merge policy: retail's two `WORLDERR_CREATE_DEVICE`
blocks differ in exactly one byte, the `jne` displacement (`75 74` at 0x1645bc vs
`75 5a` at 0x1645d6), and cl's cross-jumper compares encoded bytes - so it declined
there and accepted in our layout. Nothing in the C++ reaches that.

## The unmerged unit is not always an EPILOGUE (2026-08-24)

Two rows widen the signature past `ret` blocks, in functions with NO /GX frame
(so the same-EH-state merge predicate in
[first-function-epilogue-merge-oracle.md](first-function-epilogue-merge-oracle.md)
cannot be the discriminator):

* `CBattlezMapConfig::AdvanceToEnemyBase` 0x32060 - retail keeps TWO copies of a
  twelve-instruction tail that is four member stores (`m_defenderState=7`,
  `m_routeBlockedMask=g_battlezRouteBlockedMask`, `m_routePassableMask=0x248`) plus `mov eax,1` and the
  epilogue; we cross-jump them. The accounting closes exactly: base 591 insns /
  65 branches against target 603 / 66, and 603-591 = the twelve, 66-65 = the
  `jmp` we emit instead. The two source sites are the `dist<=0x10` arm of the
  BATTLEZ_ROUTE_TARGET switch case and the bottom-of-function path; they are
  source-identical, and retail's two copies are byte-identical to each other.
* `CGrunt::StepGruntMovement` 0x4c170 - the duplicated unit is not a return at
  all. Retail emits SEVEN `sub esp,0xc` + three-store by-value pushes of the
  12-byte `GruntDirectionCell` feeding only THREE `SetFacing` calls (three of
  the seven `jmp` into a shared call); we emit four blocks for our four source
  sites. Retail additionally spills the direction record's second field into a
  per-arm slot at all eight direction arms (`mov [esp+0x2c],ebp`), which is its
  whole extra frame dword (0x38 vs our 0x30).

A `goto` to one shared block is NOT the fix: cl 5.0 rejects it outright here
(C2362 across the arm's initializations) and, per
`single-predecessor-tail-block-gets-replicated.md`, a two-predecessor block does
not replicate anyway. Treat both as the same era residue - recognize and stop.

Recognize the rest and stop - the code is already correct.

# Retail gives each entry guard its own `return` epilogue where cl shares one
tags: cpp:branch cpp:switch cpp:goto | asm:jcc asm:ret asm:jmp | topic:wall
symptoms: base and target agree instruction-for-instruction but the base has FEWER `ret` blocks; `--blocks --diff` shows `jcc <shared tail>` in the base against `jcc <continue> | fall <inline epilogue>` in the target; the base is short by a whole multiple of one epilogue; `insn_count` reports a negative delta with no operand differences anywhere
confidence: 8/10
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

**Wall.** Measured and rejected: `if (p == NULL) return 0;` vs `if (!p) return 0;`
vs `if (p == NULL) { return 0; }` (identical); routing the deep sites through
`goto fail; ... fail: return 0;` so the entry guards are single-predecessor
(byte-identical - cl merges the returns before the layout pass); an explicit
`default:` arm instead of falling out of a switch (identical); and adding an
explicit `case <zero>:` arm, which additionally re-lowers the switch from a jump
table to a compare chain (91.37 -> 66.42). Four independent sites this session:
`CStatusBarMgr::SetTabState` 0x100d70 (retail keeps two arms' `sprite1->Probe`
tails apart and merges the rest into arm 3; we chain-merge one level deeper),
`CDDrawSurfaceChildA::SetGeometry` 0x1644a0 (retail emits the
`WORLDERR_CREATE_DEVICE` block twice - once for the switch default, once for the
`err == 0` else - we fold them and both branches target one address, 91.37),
`CTriggerMgr::ResetGroup` 0x79520 (retail merges all three cursor-spawn arms into
one tail, we leave the third out, 90.74), `CTriggerMgr::ScanGroup` 0x7a760 (three
`return 0` epilogues vs our one; every other byte matches, 89.48).

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

`ResetGroup` 0x79520 IS still a wall, but for a different reason than the entry
above says: its unmerged arm's tail is genuinely **not identical** to the other
two. cl hoisted the `m_animWorker` reload above the argument pushes in that one
arm (`mov eax,[esi+0x7c]` / `mov ecx,[eax+0x18]` before `push 0x1; push 0x3`),
where retail leaves the reload in the shared block. cl's suffix matcher then
correctly declines. The failed cross-jump is a CONSEQUENCE of a scheduling
choice, not a merge-policy difference - 45 of its 46 blocks are byte-identical.

Recognize the rest and stop - the code is already correct.

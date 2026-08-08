# A trailing shared error block turns every other `ReportError; return 0` into a 2-insn stub

tags: cpp:if cpp:return | asm:jmp asm:jcc | topic:wall
symptoms: moving one guard to the `if (ok) { body; return 1; } Err(); return 0;` shape fixes the
first branch (`jcc <far block after the epilogue>`, matching retail) but every OTHER
`if (!x) { ReportError(TAG, code); return 0; }` site collapses from 5 instructions to
`push <code>` + `jmp`; net insn count falls hard
confidence: 8/10

`allocate-check-then-body-is-the-then-block.md` reads retail's `je <block placed after the
epilogue>` correctly: retail's source really does put the failure arm last. But our packaged
cl 5.0 cross-jumps far more aggressively than the toolchain that built retail, and a failure
arm sitting at the very end of the function is the perfect merge target for every earlier
error site with the same statement list:

```cpp
if (!a) { ReportError(IDX(IDS_INITIALIZE_GAME), 0x462); return 0; }   // 25 more like it
...
ReportError(IDX(IDS_INITIALIZE_GAME), 0x404); return 0;               // the trailing arm
```

All 26 statement lists differ only in the integer literal, so cl keeps ONE copy at the end and
rewrites the other 25 to `push <code>; jmp <shared>`. Retail emits all 26 in full (6 insns
each: two pushes, the receiver, the call, `xor eax,eax`, `jmp <epilogue>`).

Measured on `CGruntzMgr::Run` 0x83450 (2026-08-07): restructuring the `new CoordPoolNode[]`
guard to the trailing-arm shape moved B0 from `jcc B2` to `jcc B242` (retail: `jcc B257`) and
made B2/B5/B7/B8/B9 exact — but the 25 error sites went 5i -> 2i and the function fell
81.25 -> 78.38. Kept the inline `if (!pool) { ...; return 0; }` spelling.

So: **the polarity evidence is real, but do not act on it when the function has a family of
statement-identical early-error returns.** Fix it only once the cross-jump itself is
steerable. Related: with the guard inline, our cl still merges just the `xor eax,eax` ahead of
the shared epilogue at every site (5i vs retail's 6i) - that residue is the same mechanism at
one-instruction scale.

**UPDATE 2026-08-08 - this is NOT "our cl cross-jumps harder than retail's".** The three
regimes are measured in
[goto-fail-shares-one-exit-block.md](goto-fail-shares-one-exit-block.md): the collapse above
is the TOTAL regime, entered because the trailing arm makes every guard reach one shared
`return`. `goto fail;` + a trailing `fail:` label is the PARTIAL regime and shares only the
sites that say `goto` - which is what retail's guarded functions are. Re-attack `CGruntzMgr::Run`
with the goto spelling (25 sites keeping their inline copies, the trailing arm shared) before
treating this as a wall.

related: [goto-fail-shares-one-exit-block.md](goto-fail-shares-one-exit-block.md) (the lever),
[allocate-check-then-body-is-the-then-block.md](allocate-check-then-body-is-the-then-block.md),
[identical-arms-need-distinct-locals.md](identical-arms-need-distinct-locals.md)
(the same cl cross-jumper, where the fix DOES work because the receiver expression can
honestly differ),
[error-report-guard-falls-through-to-a-shared-return.md](error-report-guard-falls-through-to-a-shared-return.md)

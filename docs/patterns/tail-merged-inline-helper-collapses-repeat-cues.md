# cl TAIL-MERGES repeated inline-helper expansions; retail keeps them apart

- **confidence** c8
- **tags** `topic:wall` `topic:codegen-idiom` | `asm:call` `asm:jmp`
- **measured** `CTriggerMgr::LoadTileArrivalFx` @0x75e90 - four
  `CDDrawSubMgrLeafScan::PlayCue` sites collapse onto ONE
  `LeafCue::PlayIfElapsed` call in our obj; retail emits three separate calls plus a
  fourth site where `PlayIfElapsed` is expanded INLINE. Residual ~30 instructions.

## Symptom

`insn_seq` per-symbol counts show a helper's *interior* call short by N while the
call sites that lead to it all match:

    base   1 x ?PlayIfElapsed@LeafCue@@...      4 x ?Lookup@CMapStringToPtr@@...
    target 3 x ?PlayIfElapsed@LeafCue@@...      4 x ?Lookup@CMapStringToPtr@@...

Each of our sites ends `je <shared-tail> ; jmp <shared-tail>` into one block that
owns the only `push g_sndCueTag ; call PlayIfElapsed ; mov eax,1 ; jmp <epilogue>`.

## What it is (and is not)

It is NOT a missing statement and NOT a wrong callee - the source is already correct.
cl 5.0's flow optimizer cross-jumps identical block tails; retail's build did not, and
retail additionally expanded `PlayIfElapsed`'s BODY at one of the four sites
(`g_sndEnabled` / `g_killCueClock` / `DSoundCloneInst::ConfigureItem` appear inline
there), which is only possible if `LeafCue::PlayIfElapsed` was defined **in the class
body** in retail's header, leaving cl free to inline it where the budget allowed.

So the source-side lever, if it is ever worth paying for, is to move
`LeafCue::PlayIfElapsed`'s definition into `include/Gruntz/LeafCue.h`. That is a
cross-TU change (every caller re-inlines) and was NOT attempted here; the local sites
are byte-correct as written.

Do not "fix" this by duplicating the helper call at each site by hand - cl merges the
duplicates again.

related:
[reloc-sequence-diff-names-the-missing-statement.md](reloc-sequence-diff-names-the-missing-statement.md),
[inline-depth-splits-one-body-into-two-shapes.md](inline-depth-splits-one-body-into-two-shapes.md)

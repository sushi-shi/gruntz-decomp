# cl TAIL-MERGES repeated inline-helper expansions; retail keeps them apart

- **confidence** c8
- **tags** `topic:wall` `topic:codegen-idiom` | `asm:call` `asm:jmp`
- **measured** `CTriggerMgr::LoadTileArrivalFx` @0x75e90 - four
  `SoundCueRegistry::PlayCue` sites collapse onto ONE
  `SoundCue::PlayIfElapsed` call in our obj; retail emits three separate calls plus a
  fourth site where `PlayIfElapsed` is expanded INLINE. Residual ~30 instructions.

## Symptom

`insn_seq` per-symbol counts show a helper's *interior* call short by N while the
call sites that lead to it all match:

    base   1 x ?PlayIfElapsed@SoundCue@@...      4 x ?Lookup@CMapStringToPtr@@...
    target 3 x ?PlayIfElapsed@SoundCue@@...      4 x ?Lookup@CMapStringToPtr@@...

Each of our sites ends `je <shared-tail> ; jmp <shared-tail>` into one block that
owns the only `push g_soundVolumePercent ; call PlayIfElapsed ; mov eax,1 ; jmp <epilogue>`.

## What it is (and is not)

It is NOT a missing statement and NOT a wrong callee - the source is already correct.
cl 5.0's flow optimizer cross-jumps identical block tails; retail's build did not, and
retail additionally expanded `PlayIfElapsed`'s BODY at one of the four sites
(`g_soundEnabled` / `g_soundCueTimeMs` / `SoundSample::AcquireAndPlay` appear inline
there), which is only possible if `SoundCue::PlayIfElapsed` was defined **in the class
body** in retail's header, leaving cl free to inline it where the budget allowed.

So the source-side lever, if it is ever worth paying for, is to move
`SoundCue::PlayIfElapsed`'s definition into `include/Gruntz/SoundCue.h`. That is a
cross-TU change (every caller re-inlines) and was NOT attempted here; the local sites
are byte-correct as written.

Do not "fix" this by duplicating the helper call at each site by hand - cl merges the
duplicates again.

## It is the DOMINANT reading of a REL32 count row, and it goes BOTH ways (2026-08-08)

Six of the eight `global_refs --rel32` rows on one lane's units were this, and the
alignment settles each in one read - print the ordered relocation sequence for both
sides and look at where the run of identical offsets stops:

| row | what the sequence shows |
|---|---|
| `CGruntzMgr::HandleCommand` ReportError 2 v 4 | retail cross-jumps the `0x42b` arm onto the shared `push 0x8005 ; call ReportError` block at `0x89e42` and gives `0x42c` (`0x89b17`) its own copy; we merge both. Every error code `0x41e`-`0x479` retail pushes is already in the source - **there was nothing to derive.** |
| `CPlay::OnKeyDown` PlayIfElapsed 8 v 9 | our `CLEAR_TAB_HINT` at `+0x117` ends `jmp 0x1b0` into another site's `PlayIfElapsed` tail. |
| `CPlay::OnKeyDown` RebuildSelectionList 9 v 8 | the other direction, same function: retail's `'8'` arm (`0xccf81`) jumps into the `'9'` arm's `je/call` at `0xccf98`; we keep nine. |
| `CCheatMgr::LoadCheatConfig` GetInt 6 v 7 | our two `AddCheat` arms share one `GetInt`+`AddCheat` copy; retail's are 0x13b and 0x161. |
| `CFaderShape::RenderFrame` RenderTile 7 v 8, RenderWarpTile 5 v 6 | 16 source sites, both sides merge some. Retail keeps all 8 `RenderTile`; the `x` argument reaches the merge point in a register, so distinct arguments do **not** prevent the cross-jump. |
| `GameSerializationCallback` `??0CUserLogic` 4 short / `??0CUserBaseLink` 3 long | not a merge - the per-`new`-site ctor cut depth. `CUserBaseLink` is a MEMBER of `CUserLogic`, so a site that names it is a site where cl INLINED `CUserLogic::CUserLogic`. The first ELEVEN inlined sites agree at identical offsets (`0x343`..`0x6b7`) and the divergence starts at the twelfth. |

The discriminator against a real finding is cheap: dump both relocation sequences and
check whether the source already contains the call. If the constants, the arms and the
callee set are all present and only the count differs, it is the cross-jump.

related:
[reloc-sequence-diff-names-the-missing-statement.md](reloc-sequence-diff-names-the-missing-statement.md),
[inline-depth-splits-one-body-into-two-shapes.md](inline-depth-splits-one-body-into-two-shapes.md)

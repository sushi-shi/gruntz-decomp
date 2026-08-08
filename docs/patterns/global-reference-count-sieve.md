# The per-function reference COUNT names the cached global the diff cannot show
tags: cpp:local cpp:global | asm:mov topic:tooling topic:regalloc
symptoms: a function plateaus 45-95% with identical block topology; the masked `--diff`
shows a scatter of `-mov eax,_g_x` / `+mov eax,[esp+N]` that reads like regalloc noise;
`gruntz.audit.shrink_wrap` may show +/-1 push
confidence: 9/10 (calibrated to 0 false positives over 4301 paired functions)

A `CGruntzMgr* reg = g_gameReg;` at the top of a long method reads better than retail's
source and costs a callee-saved register for the whole body, so every later value spills.
`sema disasm --diff` MASKS the address operand, so the twelve reads that became three
appear as a handful of `-`/`+` pairs buried in scheduling noise. The signal that survives
masking is the **count**: cl 5.0 re-materialises a global at every use unless the source
hoisted it, so the number of DIR32 relocations naming a symbol inside one function is a
direct readout of how many times the SOURCE mentioned it.

    python -m gruntz.audit.global_refs              # DIR32 - data references
    python -m gruntz.audit.global_refs --rel32      # REL32 - call targets
    python -m gruntz.audit.global_refs --calibrate  # the false-positive rate

`base < target` = we over-cached (`CGrunt::LoadGruntCombatAnimations` read `_g_gameReg`
three times against retail's five: 45.93 -> 50.43, exactly-matching basic blocks 41 ->
120). `base > target` = we invented a read, or retail hoisted one we did not.

## The five filters, each of which was a false-positive family first

The raw comparison is useless - the first draft reported 1117 rows of which 752 were in
functions objdiff already scores at 100.00%. **Never read a raw per-function relocation
list**: three of the five families below re-appear the moment you dump relocations by hand
instead of going through the tool, and each one looks exactly like a finding.

1. **Window each side at the NEXT DEFINED SYMBOL in its own section.** The base is
   COMDAT-per-function; the delinked target packs a unit's whole `.text` into one section,
   so an unwindowed read counts the neighbour's relocations.
   *Do not* additionally clamp the target to the base function's length: our body is
   shorter than retail's in 213 of 4301 functions, and the clamp then hides retail's tail
   and reports everything in it as ours-only (`CStatusBarMgr::LoadTabSprites`, 0x274 bytes,
   seven phantom rows).
2. **Canonicalize the `$S<n>` / `$Sdata_data_<hash>` local-static suffix.**
   `canonicalize_data_symbols` hashes the datum's bytes *and its recorded relocations*,
   which the delinker does not always reproduce, so one logical `s_QUESTZ` is
   `_s_QUESTZ$Sdata_data_87db2c..._0` in the base and `..._5bc660..._0` in the target.
3. **Drop an addend PAST THE END of the symbol it names.** That is the delinker's
   unsized-datum fallback: `?s_table@?$CActRegPool@VCGrunt@@...` resolves to
   `?g_gruntDirNorthEast + 0x2b0` and `g_gruntDirNorthEast` is 12 bytes, which alone put
   twenty phantom `g_gruntDir*` deltas into `CGrunt::FireActivation` (258 B, 100.00%
   exact). **Bound it by the extent; do not demand a zero addend** - cl folds a field
   offset into the address of an indexed array element on one side and not the other, so
   `_g_rasterEdgeL + 0x14` and `_g_rasterEdgeL + 0` are the same reference. Same mechanism
   with a pooled string: `CTriggerMgr::ResetGroup`'s `"LightFx"` reads as
   `??_C@_09MHNK@DemoMover?$AA@ + 0xc` on the target side, which by hand looks like we
   passed the wrong sprite-logic name and is nothing but the pool's previous symbol.
4. **Drop a unit `report.json` does not score.** `build/objdiff/normalized/` is
   incremental: a unit a later `configure.py` dropped leaves its base/target pair there
   forever. One worktree held **72** of them, the oldest five days stale, and each scored
   0.00% - which sorts it to the very top of the worklist. `gamekeyhandler` (a superseded
   split of `play`) cost a re-derivation of `CPlay::OnKeyDown` before its `.symbols.tsv`
   turned out to be one empty header row. Paired functions 4428 -> 4301, exactly
   `report.json`'s count; 8 REL32 rows and 1 DIR32 row were phantoms.
5. **Drop a name only ONE side references inside the function.** This is what takes the
   rate to 0, and it is what makes the sieve answer "how many times" and never "which
   symbol". A wrong REFERENT (`??_C@_01PFH@A` against
   `??_C@_0BE@MAOF@GAME_ACTIONAREA_RED`, same offset, same byte;
   `?Attach@CGdiObject@@QAEHPAX@Z` against `?Attach@CImageList@@QAEHPAU_IMAGELIST@@@Z`,
   two FID labels for one address) belongs to a reloc-SEQUENCE comparison.
   `--one-sided` shows them again, at 21% false positives.

## Reading a row

A row is a lead about the SOURCE, and three shapes account for nearly all of them:

* **a cached global / member** - the thing the sieve is for. Delete the local, spell the
  global at each use.
* **cl TAIL-MERGED two arms that retail kept apart, or vice versa.** The tell is that the
  callee AND its argument string move together:
  `CCheatMgr::LoadCheatConfig` reads `?GetIntDef@CButeMgr@@` 6 vs 7 *and* `"Value"` 1 vs 2,
  because our two `AddCheat(code, GetIntDef(..,"Value",0x807b), 1/0)` arms cross-jumped
  onto one copy while retail's did not (retail defers `push 1` past the call in the then-arm
  and pushes `0` before it in the else-arm - different tails, no merge).
  `CTriggerMgr::ResetGroup` is the mirror image: retail merged all three `CLightFx::Activate`
  sites (1 call, 1 of each cursor string) and we merged only two.
  See [tail-merged-inline-helper-collapses-repeat-cues.md](tail-merged-inline-helper-collapses-repeat-cues.md).
  This is not a source bug; do not hand-duplicate the call.
* **cl inlined a ctor/dtor retail emitted OUT OF LINE.** The vtable stamp count and the
  ctor call count move in opposite directions: `CDDrawSurfaceMgr::Init` stamps
  `??_7CLoadable@@6B@` 7 times and calls `??0CLoadable@@QAE@PAVCDDrawSurfaceMgr@@HH@Z`
  0 times where retail stamps 4 and calls 4; `CDDrawSurfaceMgr::SnapshotChildren` stamps
  `??_7CFileMemBase@@6B@` 12 times and retail 0.
  See [ctor-inline-cut-depth-varies-per-new-site.md](ctor-inline-cut-depth-varies-per-new-site.md).

## What it found that nothing else did

`?GetTickCount@CTime@@SG?AV1@XZ` present on the target side of
`CDDrawSurfaceMgr::SnapshotChildren` and absent from ours: our `CTime now;` stamped a zero
date into every snapshot header where retail calls `CTime::GetCurrentTime()`. A correctness
bug with no percentage tell at all (the fix costs 0.37).

`ImagePolyClip`'s `0.0f` literals: retail loads the named `?g_c10@@3MB`, we pooled an
anonymous `$anon_f32_00000000_0`. `RotateRasterize` 52.15 -> 55.17.

related: [invented-member-pointer-local.md](invented-member-pointer-local.md),
[redundant-local-becomes-the-zero-register.md](redundant-local-becomes-the-zero-register.md),
[reloc-sequence-diff-names-the-missing-statement.md](reloc-sequence-diff-names-the-missing-statement.md)

# An `RVA()` extent must not run PAST the function into the linker's `0xCC` filler

tags: data:objdiff cpp:switch | asm:int3 asm:jmp | topic:scoring-artifact topic:tooling
symptoms: a big `switch`-heavy function stuck far below 100% with no plausible source
  bug; `gruntz sema disasm <rva>` ends in a long run of `int3`; the target
  instruction count is hundreds larger than the base's and the excess is ALL `int3`;
  `objdiff` `size` for the function is much larger than the base `.text` COMDAT
confidence: 10/10
variants: rva-extent-must-include-switch-tables.md

The mirror image of the short-claim artifact. The retail EXE was linked
`/INCREMENTAL`, so the linker leaves a **`0xCC` growth gap between functions** — up to
2 KB of it. The delinker cuts the target obj at exactly the `RVA()` size, so a claim
that over-runs into that gap hands objdiff hundreds of `int3` "instructions" with
nothing on the base side to match. cl pads a `.text` COMDAT to 16 with `0x90`;
**`0xCC` is always the linker, never cl**, so any `0xCC` inside a claim is proof the
claim is too long.

```cpp
RVA(0x0004dd50, 0x2880)   // 1168 B of 0xCC filler -> 59.76%
RVA(0x0004dd50, 0x2400)   // ends at the last table byte -> 87.11%
```
```asm
;  50144: 32 32 33                      <- last byte of the switch index LUT
;  50147: 90 90 90 90 90 90 90 90 90    <- cl's COMDAT alignment (part of the fn)
;  50150: cc cc cc cc ... x1168         <- the LINKER's gap (NOT part of the fn)
;  50a50: 83 ec 0c ...                  <- the next function
```

Steerable, and mechanical: the extent ends at the first `0xCC` run of >= 8 bytes before
the next admitted function start. Screen the whole tree with
`gruntz sema disasm --switch` (0/3455 false positives on the
byte-exact set; it reports both the LONG and the SHORT direction, and a long claim MASKS
a short one so fix these first).

Measured 2026-08-08, five functions in the tree: `CGrunt::LoadGruntTypeTable` 0x4dd50
**59.76 -> 87.11** (1168 B, the largest residual body in the tree),
`CMapMgr::ComputeCellFlags` 0x77790 **46.04 -> 76.40** (320 B),
`CInGameIcon::CInGameIcon` 0x95b10 **79.11 -> 92.23** (304 B),
`CGrunt::StepHitAndRunnerBehavior` 0xed9f0 **66.71 -> 83.44** (160 B),
`CGruntzMapMgr::BuildCellAttributes` 0x810f0 **71.43 -> 75.21** (48 B).
`CGrunt::LoadGruntCombatAnimations` 0x597a0 (64 B) went 46.35 -> 45.93: the score can
DROP when the filler happened to align against base padding, and that is the claim
starting to tell the truth — judge the extent by the bytes, never by the score.

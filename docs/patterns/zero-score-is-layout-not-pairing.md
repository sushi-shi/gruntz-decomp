# A function scored 0.00% (no `fuzzy_match_percent` key) is a BLOCK-LAYOUT swap, not a pairing failure
tags: cpp:branch cpp:return cpp:switch | asm:jmp asm:ret | topic:scoring-artifact topic:wall
symptoms: fuzzy_match_percent missing from report.json, match_percent 0.0, objdiff scores zero, epilogue in the middle, shared exit block, tail merge, cross jump
confidence: 9/10

objdiff scores a function `100 * (1 - (replaced + deleted + inserted) / target_instruction_count)`
and **clamps the result at 0**. A body whose CONTENT is right but whose BLOCK ORDER differs
generates one delete AND one insert per moved instruction, so the penalty double-counts and can
exceed the target's whole instruction count — the raw score goes negative and prints as `0.0`.
objdiff serializes the report with serde's skip-the-default rule, so the key then vanishes
entirely and the function looks unpaired. It is not: `objdiff-cli diff -1 <target> -2 <base>
--format json <sym>` shows a live `target_symbol` link in both directions.

```cpp
// The usual cause: a construct that funnels several sites into ONE physical block,
// which then steals the fall-through and drags the epilogue into the middle.
if (a != X || b != Y || c != Z || v != 1) { return 0; }   // ONE shared exit
// Retail spelled four guards, so cl emitted four inline `xor eax,eax; jmp <ret>`:
if (a != X) { return 0; }
if (b != Y) { return 0; }
if (c != Z) { return 0; }
if (v != 1) { return 0; }
```
```asm
; ours - epilogue planted mid-function, everything jumps back to it
  0xdf: xor eax,eax
  0xe1: mov ecx,[esp+0xc0] / pop edi / pop esi / pop ebp / mov fs:0,ecx / pop ebx / ret
  0xfc: ... the rest of the function ...
; retail - each return-0 inline, epilogue last
  xor eax,eax / jmp <ret>
  ...
  <ret>: mov ecx,[esp+0xc0] / pop edi / ...
```

STEERABLE when a `||` chain (or any single statement reached from many sites) is the funnel:
splitting `CRezMgr::Open`'s four-term magic guard into four `if`s moved the epilogue to
the end, took the body from 290 to 300/300 instructions and 938 to 952/952 bytes, and the score
from **0.00% to 98.46%**. WALL when the funnel is cl's own cross-jumper picking a different merge
factor (`CGameObject::SerializeDispatch` 0x151150 — retail merges the LOAD+POSTLOAD arms whole, cl merges
PRESAVE+SAVE+LOAD's restore; ~15 spellings did not move it). Screen with
`gruntz walls diagnose --asm`; the percentage is useless below the clamp, and
`gruntz.verify.scores.fn_fuzzy()` is the only correct way to read the missing key (it means 0.0,
never "unknown", never 100).

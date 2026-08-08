# cl cross-jumps a switch arm's 10-14 byte tail out of ~1 arm in 3; retail duplicates it

tags: cpp:switch cpp:branch | asm:jmp asm:or | topic:wall topic:regalloc
symptoms: a big per-`case` switch where MOST arms are byte-identical to retail but a
  periodic subset (every 2nd/3rd) is exactly 10 or 14 bytes SHORT; the short arm ends
  `mov edi,<imm>` + `jmp` to a label a few bytes BEFORE the label the full arms jump to;
  the target has N copies of a two- or three-instruction tail and the base has one
confidence: 9/10
variants: retail-duplicates-small-return-epilogues.md, goto-fail-shares-one-exit-block.md

Same compiler, same flags, same source: cl's cross-jump (tail-merge) pass hoists the
last two or three instructions of *some* arms into one shared block and jumps to it,
where retail's cl kept every copy. The arms it picks follow the /O2 register rotation
(cl cycles eax/ecx/edx across consecutive identical arms), so a merge lands roughly
every third arm - the one whose rotation makes its tail literally equal to the shared
copy's. Nothing about the C++ differs between a merged and an unmerged arm.

```asm
; TARGET - every arm keeps its own tail (54 B)
  call  Lookup
  mov   eax,[esp+0x20]
  mov   edi,0x3c4
  mov   [esi+0x3d8],eax
  jmp   <common>
; BASE - this arm (and every ~3rd) jumps to a shared 10-byte copy instead (44 B)
  call  Lookup
  mov   edi,0x3c4
  jmp   <shared>          ; <shared>: mov eax,[esp+0x10]; mov [esi+0x3d8],eax
```

A second face of the same pass: where an arm ends `flags |= 0x10`, whether cl emits the
mergeable `or DWORD PTR [esi+0x248],0x10` (7 B) or retail's `mov eax,[m] / or al,0x10 /
mov [m],eax` (14 B) is decided by whether EAX is free, and EAX is occupied exactly when
cl CSE'd the shared constant `1` into it instead of into EBX as retail did. So one
register choice flips instruction selection, which flips mergeability, which flips ~20
arm tails at once.

WALL, not steerable - measured on `CGrunt::LoadPickupSprites` 0x65e80 (18 of 60 arms
short by 10 B), `CGrunt::LoadGruntTypeTable` 0x4dd50 (12 of 20 `|= 0x10` sites) and
named by the 2026-08-08 lane as the mechanism behind `CTriggerMgr::ResetGroup` 0x79520,
which has a CLEAN extent and a CLEAN dispatch shape
(`python -m gruntz.audit.jump_tables` reports it clean). Do not read a periodic 10-byte
arm deficit as a missing statement: check first whether the short arm jumps a few bytes
short of where its siblings jump.

RETRACTED for `CStatusBarMgr::SetTabState` 0x100d70 (2026-08-08, later the same day):
it was listed here and it is NOT this pass. Its fifteen arms end in a statement-identical
`return 1;`, which feeds cl's EARLY cross-jump of return statements; `break;` in every
arm plus one trailing `return 1;` takes it **88.53 -> 100.00 EXACT**. Before assigning a
switch to this (unsteerable) family, check the arm terminator against
[switch-arm-break-not-return-replicates-the-epilogue.md](switch-arm-break-not-return-replicates-the-epilogue.md).

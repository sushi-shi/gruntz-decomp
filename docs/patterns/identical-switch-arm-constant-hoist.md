# Identical switch arms sharing constants: cl hoists them to callee-saved regs above the body, retail re-materializes per-arm
tags: cpp:switch cpp:local | asm:mov asm:or asm:lea | topic:wall topic:regalloc
symptoms: switch/jump-table where every arm writes the SAME constant block; `mov [esi+N],ebx`(held -1) vs `lea ebx,[esi+N];or eax,-1;mov [ebx],eax`; `mov [m],-1` immediate vs register; arm bodies structurally smaller in recompile; ~37%
confidence: 7/10

A jump-table switch whose arms all store an identical constant block (e.g.
`{-1,-1,1,1}`/`{0,0,0,0}` into two member regions) differing only in a per-arm
string/value. This cl build hoists the shared constants (`-1`,`1`,`0`) into
callee-saved `ebx/ebp/edi` ABOVE an intervening call (the CString ctor), so each
arm shrinks to one esi-relative `mov reg,[esi+N]` store; retail instead leaves the
`m_x=-1` member init as an immediate store and RE-MATERIALIZES the constants
inside each arm (`lea ebx,[esi+N]; or eax,-1; or ecx,-1; mov edx,1; mov [ebx],eax;
…`), keeping `kind`/`this` in `edi`/`esi`.

```asm
; retail arm (per-case rematerialize, region reached via a pointer):
lea  ebx,[esi+0x2b0]
or   eax,-1
or   ecx,-1
mov  edx,1
mov  [ebx],eax        ; vs recompile: mov [esi+0x2b0],ebx  (ebx=-1 hoisted at entry)
; and the member init: retail `mov dword [esi+0x1a0],0xffffffff` (immediate)
;                       recompile `mov [esi+0x1a0],ebx`        (register, hoisted)
```
WALL — same /O2 /Oy /GX frame; pointer-vs-index source spellings normalize to the
SAME bytes (so neither defeats the hoist), and /O1 only swaps in a wrong ebp
frame. A cl build-8034 constant-CSE/LICM heuristic, not steerable from source.
CGruntCmdObj::LoadVehicleGruntSprites 0x050ce0 ~37% (@early-stop). Cousin of
[[blowfish-feistel-unroll-regalloc]] / [[loop-invariant-multiply-strength-reduce-vs-memreread]].

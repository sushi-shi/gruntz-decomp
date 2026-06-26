# Repeated/large store constants: retail materializes them into registers, cl keeps immediates
tags: cpp:ctor cpp:local | asm:mov asm:xor asm:test | topic:wall topic:regalloc
symptoms: ctor/init body with same offsets+values as retail but reordered stores; `mov eax,0x140;mov ecx,0xf0` vs immediate `mov [..],0x140`; `xor edi,edi`+`cmp [..],edi`/`mov [..],edi` vs `test eax,eax`+`mov [..],0`; cascading register pin (idx in esi vs edi)
confidence: 7/10

A field-init body (ctor or array-record teardown) where retail pre-materializes a
store constant into a register and reuses it, while MSVC5 here emits an immediate
store (and `test reg,reg` instead of `cmp reg,<held-0>`). When the held constant is
`0` reused across many null-checks/stores, retail pins it in a callee-saved reg
(edi), which forces the OTHER live values (e.g. a loop index) into different
registers — so the whole allocation cascades and the diff looks large even though
every store has the right offset+value. Two triggers seen: (1) a ctor with two
distinct "large" constants (0x140 + 0xf0) — retail loads both to regs + stores in
offset order, cl keeps immediates + groups the zero-stores; (2) a function reusing
`0` for 7 pointer compares/stores — retail holds 0 in edi, cl uses test/immediate.

```cpp
// nothing reproduces it: store order, `int w=0x140;` locals, and `void* z=0;`
// reused for the null ops all fold back to cl's immediate/test choice.
m_18 = 0x140; m_1c = 0xf0;          // CFxModeT2 ctor 0x17e840 ~72%
if (rec->m_00) { free(rec->m_00); rec->m_00 = 0; }  // CDDPageMgr::RemoveAt 0x17d600 ~86%
```
```asm
mov eax,0x140 ; xor edx,edx ; mov ecx,0xf0   ; retail: constants to regs
mov [esi+0x18],eax ; mov [esi+0x1c],ecx       ; ...stored in offset order
xor edi,edi ; cmp [ebp+4],edi ; mov [ebx],edi ; retail: 0 held in edi, reused
```
WALL — not source-steerable; the register-vs-immediate + reuse decision is MSVC5's
constant scheduler. Sibling ctors with <=1 large constant (CFxModeT3/4/5/6) match
100%; FreeAll (no reused-0 store) matches 100%. Logic/layout are byte-correct.

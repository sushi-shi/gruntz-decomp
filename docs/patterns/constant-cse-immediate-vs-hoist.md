# A literal reused in cmp + a later call: retail materializes immediates, MSVC may hoist/copy-prop
tags: cpp:local cpp:branch | asm:cmp asm:mov | topic:wall topic:regalloc
symptoms: cmp mem,imm vs `mov ecx,imm; cmp mem,ecx`; entry `mov ecx,0x280; mov eax,0x1e0`; field-copy store becomes `mov $imm,[field]` (copy-prop); ~54-61% on a tiny resolution/dimension guard
confidence: 6/10

A small function compares two members against fixed literals (e.g. a 640x480
mode check) AND passes the same literals to a later call. Retail materializes
each literal as an **immediate** at every use (`cmp [esi+0x8c],0x280` … `push
0x280`) and re-loads the compared fields **fresh** in the save block with no
constant propagation. MSVC 5.0 under wine instead makes one of two more
aggressive choices that no source spelling here reproduces:
- direct member compare `if (m_8c == 0x280 …)` → hoists `mov ecx,0x280; mov
  eax,0x1e0` to entry and reuses the registers in cmp, store, and call;
- compare via locals `int w = m_8c; if (w == 0x280 …)` → immediate cmp/call
  operands (good) but copy-props the known value into the field-copy store
  (`mov $0x280,[esi+0x94]`).

```cpp
int w = m_8c, h = m_90;           // best-scoring spelling (~61%): immediate cmp + call
if (w == 0x280 && h == 0x1e0) { if (save) { m_94 = w; m_98 = h; } return 1; }
if (SetVideoMode(0x280, 0x1e0, save)) return 1;
```
```asm
8ddd3: cmp DWORD PTR [esi+0x8c],0x280   ; retail: immediate operand, no entry hoist
8ddf3: mov eax,[esi+0x8c]               ; retail: fresh field re-load, no copy-prop
8ddff: mov [esi+0x94],eax
```
WALL — logic is exact; the residue is MSVC 5.0's constant-CSE/copy-prop heuristic
choosing hoist-and-reuse where retail materialized immediates. Evidence:
CGruntzMgr::RestoreVideoMode @0x08ddd0 — direct-member 54.3%, compare-via-locals
61.1%; both correct, neither reaches retail's immediate-cmp + fresh-reload shape.

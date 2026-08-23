# A destination-register difference at the same instruction index is cursor PHASE, and never a work item
tags: asm:mov | topic:wall topic:regalloc topic:negative-control
symptoms: same slot same symbol different destination register, `mov edx,[g_gameReg]` vs `mov ecx,[g_gameReg]`, retail is one byte earlier, moffs32, A1 encoding, register rotation, EAX ECX EDX ESI EDI EBX EBP
confidence: 10/10

Two aligned instructions with the same mnemonic and the same (relocation-resolved)
source operand but different destination registers. The rotation order is
`EAX ECX EDX ESI EDI EBX EBP`, read off a cursor that advances once per IL tuple
and resets per basic block, so the delta is a small signed rotation.

```asm
   base    mov edx,DWORD PTR ds:?g_gameReg@@3PAVCGruntzMgr@@A
   retail  mov ecx,DWORD PTR ds:?g_gameReg@@3PAVCGruntzMgr@@A     ; delta -1
```

**Do not rank these by BYTE offset.** `mov eax,moffs32` is the 5-byte `A1` form
and `mov <other-reg>,[disp32]` is 6, so a byte-offset ranking turns every colour
site involving EAX into an apparent "retail is one byte earlier from here on".
Measured over the global-reference class: 21 of 63 sites make retail one byte
SHORTER, 9 make it one byte LONGER, and 33 carry no size delta at all. Rank on
INSTRUCTION INDEX; the byte offset is only the question when the byte offset IS
the question.

**Wall, and not even a wall worth naming per row.** Tree-wide census (2026-08-23,
871 sub-100 functions): 699 colour sites across 261 functions, direction
`-3:29 -2:103 -1:208 +1:240 +2:94 +3:25`, sign split 340 negative / 359 positive,
and 88 of the 261 functions carry BOTH signs among their own sites. Restricted to
the global-reference subclass (same relocation symbol on both sides) it is 63 sites
across 40 functions, `-3:1 -2:13 -1:16 +1:25 +2:8`, 30 negative / 33 positive. A
symmetric sign means there is no systematic bias to correct.

**A colour site is a readout, not a cause.** Only 3 of the 261 functions have a
colour site as their ONLY divergence — `SaveVideoCheckboxes` 0x378c0 (99.50),
`CDDrawSurfacePair::DrawCross` (99.73) and `ReadMenuOptionsDialog` 0x36a30 (99.90).
The other 258 have an independent divergence in the same body, and THAT is the row's
work. The positive control shows the direction of causation: re-introducing a
redundant `CoordPoolNode* fl` local in `CGrunt::LoadStateRecord` 0x555e0 makes the
free-list publish store read EAX where retail reads EDX, and the sieve reports
exactly one colour site there with delta +2; deleting the local again closes the
colour AND the function (99.99 -> 100.00). The colour moved because the IL tuple
moved, never on its own.

**Why no colour-targeted edit exists.** To change the register of tuple N you must
change the number of tuples before it in the same basic block, which changes other
instructions. On the three pure rows the two sides' streams are otherwise identical,
so the tuple count is identical and only the cursor's entry PHASE differs - a
property of C1 handle state, not of the function's own source. Four inert A/Bs on
`SaveVideoCheckboxes`: named locals for the results, one reused local, the TU-state
probe, and inverting `if (x == NULL) return;` into `if (x != NULL) { ... }` are all
byte-identical.

**Detector control.** The same sieve over the 6,596 EXACT functions reports 0 sites,
so the signature has no false-positive rate.

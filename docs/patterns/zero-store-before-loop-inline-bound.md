# Pre-loop member `=0` store + inline masked loop bound: keep MSVC from fusing the index-zero with the member-zero and from masking the count in place
tags: cpp:local cpp:loop | asm:xor asm:mov asm:and | topic:codegen-idiom topic:scheduling
symptoms: a 4-instr window before a counted loop permutes — `xor eax,eax` vs `mov $0,eax`, the `and $0xff` lands in the count register vs a copy, and a `movw $0,[mem]` becomes `movw reg,[mem]` (the mask-zero store reuses the index's just-zeroed register)
confidence: 8/10

A `mask = 0` member store followed by `for (int i = 0; i < (count & 0xFF); i++)` over a
zero-cleared accumulator. The two source spellings below are byte-identical in *logic* but the
optimizer's pre-loop setup differs:

```cpp
// DIVERGES: MSVC fuses the two zeros — `xor eax,eax` (i=0) is reused as the value
// for the member store (`movw %ax,[mem]`), and `and $0xff` masks the count IN PLACE.
int n = count & 0xff;
*(unsigned short*)&m_10 = 0;
for (int i = 0; i < n; i++) ...
```
```cpp
// MATCHES: zero the member FIRST (forces an immediate `movw $0,[mem]`, not a
// shared register), and inline the masked bound in the condition (count stays in
// its register; the mask `and $0xff` goes to a fresh copy used as the loop limit).
*(unsigned short*)&m_10 = 0;
for (int i = 0; i < (count & 0xff); i++) ...
```
```asm
mov  ecx, ebx          ; ecx = count (ebx kept)
xor  eax, eax          ; i = 0
and  ecx, 0xff         ; bound = count & 0xff  (into the COPY)
mov  WORD PTR [esi+0x10], 0   ; mask = 0  (immediate store, last)
```

STEERABLE. Two independent levers: (1) emit the `=0` member store *before* the loop's index
init so the optimizer can't collapse `i=0` and `member=0` into one register (gives `movw $0,[mem]`
+ separate `xor eax,eax`); (2) write the masked count `(count & 0xff)` *inline in the for-condition*
(not a hoisted `int n=...`) so the AND targets a scratch copy and leaves the count in its original
callee/arg register. Evidence: CGruntzCommand::SetMaskFromList (0x023ed0) — the hoisted-`n` +
member-zero-second form left a 4-instr permutation (~80%); zero-first + inline-bound closed it to
byte-exact.

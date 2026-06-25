# Switch range-checks emit unsigned `ja`/`jbe` only for an unsigned key
tags: cpp:switch cpp:type | asm:ja asm:jbe asm:jg asm:jle | topic:codegen-idiom
symptoms: switch on a member/int; retail `cmp eax,K; ja/jbe` but recompile `jg/jle`; body otherwise byte-identical; ~97-98% plateau, one or more compare-jump diffs
confidence: 9/10

A `switch` whose key is a **signed `int`/`i32`** lowers its binary-search range
checks with **signed** conditionals (`jg`/`jle`/`jl`/`jge`). The same switch keyed
on an **unsigned `u32`/`unsigned int`** lowers them with **unsigned** conditionals
(`ja`/`jbe`/`jb`/`jae`). Nothing else in the dispatch changes — same `cmp`
immediates, same case ordering, same tail. So a state-tag / enum / message-id
switch that the retail emits with `ja`/`jbe` only matches when the key field is
typed unsigned; a signed `i32` field caps the function at the entropy plateau with
exactly the compare-jump opcode flipped.

```cpp
struct Worker {
    u32 m_1c;            // state tag — UNSIGNED so the switch uses ja/jbe
};
switch (rec->m_1c) { case 0: ...; case 0x1d: ...; default: ...; }
```
```asm
cmp    eax,0x1d
ja     <hi>          ; unsigned (u32 key); signed key would be `jg`
je     <eq>
```
Steerable: type the switch-key field `u32` (matching-neutral — same offset/size,
no mangling change). Flipped all three InGameWorkerHandlers handlers
(0x095750/0x095890/0x0aa6e0) and all three SiriusWorkerHandlers handlers
(0x03d670/0x07db20/0x07dda0) from 97.86% to 100%.

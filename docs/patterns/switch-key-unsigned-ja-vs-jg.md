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

## An enum-typed accessor puts the `jg` BACK

MSVC 5.0 sizes every `enum` as `int` and treats it as **signed**, so routing an
unsigned field through a domain accessor re-introduces the signed compare even
though the storage is unsigned. Measured on a 20-line probe: `switch ((Act)k)`
gives `jg`, `switch (u32 k)` gives `ja`, and the two functions are otherwise
byte-identical.

That is why this pattern kept coming back on the `Create<Leaf>` worker pumps:
the switch reads `CLogicRecord::LogicEvent()`, which returned `LogicRecordEvent`.
The fix at the domain layer, not the field:

```cpp
GZ_ENUM_BEGIN_SPLIT(LogicRecordEvent, u32)      // the domain is stored unsigned
    ...
GZ_ENUM_END_SPLIT(LogicRecordEvent, u32)

GZ_ENUM_RETURN(LogicRecordEvent, u32) LogicEvent() const {
    return static_cast<LogicRecordEvent>(m_eventCode);   // u32 in the matching build
}
```

and switch **directly on the accessor** — an `LogicRecordEvent act = ...;` local in
between silently re-signs the key (and is dead anyway). 2026-08-08: **63**
functions 97.857140% -> 100.00% EXACT in one build, every `_Create<Leaf>` pump
across thirteen files plus the three hand-written copies
(`_CreateStatusBarSprite`, `_CreateLevelTime`, `CTileTriggerLogic`'s
`TILE_LOGIC_RECORD_DISPATCH`). A whole-tree histogram of the residual queue found
them: 63 functions at the *identical* fuzzy value is one mechanism, never 63.

# A timed phase reuses ONE variable (`t = tg() - t`) — two locals give it two frame slots

tags: cpp:local cpp:call | asm:mov asm:sub | topic:codegen-idiom topic:regalloc
symptoms: `sub eax,[esp+0x14]; mov [esp+0x1c],eax` vs retail `sub eax,[esp+0x18]; mov [esp+0x18],eax`,
every `[esp+N]` displacement permuted, same frame size, "spill to a different set of stack slots"
confidence: 9/10

An instrumented body with N `timeGetTime`-bracketed phases: every instruction pairs but the
`[esp+N]` slot NUMBERS are permuted. Look for retail **writing a phase result back into the
slot it read the start time from** (`sub eax,[esp+0x18]; mov [esp+0x18],eax`) — that is one
variable per phase, not a start-time local plus a duration local. Two locals cannot share a
slot (both are live at the same point), so their live ranges — and therefore the whole
function's slot assignment — differ.

```cpp
// before - t1 and activateMs are two variables, two slots
u32 t1 = tg();  Work();  i32 activateMs = static_cast<i32>(tg() - t1);

// after - one variable per phase, the slot is reused
i32 activateMs = static_cast<i32>(tg());
Work();
activateMs = static_cast<i32>(tg() - static_cast<u32>(activateMs));
```

```asm
call esi | mov [esp+0x18],eax | ... | call esi | sub eax,[esp+0x18] | mov [esp+0x18],eax
```

STEERABLE. The same file already used the idiom for the cross-frame globals
(`g_profAccB = (i32)(tg() - (u32)g_profAccB)`), which is the corroboration that the locals
were spelled inconsistently. Evidence: `CPlay::ProfileInputFrame` @0x0c9e40 99.90 → **100%
EXACT** (seven phases rewritten), filed as a "profiler-scheduling wall … the slot-reuse
schedule diverges despite identical logic".

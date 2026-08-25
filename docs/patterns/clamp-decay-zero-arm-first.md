# A decaying timer/clamp spells the ZERO arm first — `>= ? 0 : diff`, not `< ? diff : 0`

tags: cpp:branch cpp:ternary | asm:jae asm:jb asm:sub | topic:codegen-idiom
symptoms: jcc_sieve POLARITY with a `jae`/`jb` twin, base `cmp a,b; jae L; sub b,a;
mov [g],b; jmp E; L: mov [g],0` vs retail `cmp a,b; jb L; mov [g],0; jmp E; L: sub b,a;
mov [g],b` — same instructions, the two arms swapped around the branch
confidence: 9/10

`g -= elapsed` clamped at zero is written one of two equivalent ways, and cl keeps
whichever arm the source puts FIRST as the branch-not-taken fall-through:

| source | cl emits |
|---|---|
| `if (elapsed < g) g -= elapsed; else g = 0;` | `jae <zero-arm>` … subtract falls through |
| `if (elapsed >= g) g = 0; else g -= elapsed;` | `jb <subtract-arm>` … zero store falls through |

Retail is the **second** form everywhere this appeared. The tell is purely the
condition-code twin (`jae` vs `jb`), which `gruntz walls diagnose` and
`gruntz walls diagnose <rva>` surfaces it directly — the instruction multiset is
identical, so a flat `--diff` looks like harmless "block reordering" and is easy to
dismiss as a layout wall.

```cpp
// before
if (g_frameDelta < t1) { g_timer32 = t1 - g_frameDelta; } else { g_timer32 = 0; }
// after
if (g_frameDelta >= t1) { g_timer32 = 0; } else { g_timer32 = t1 - g_frameDelta; }
```

```asm
target: cmp eax,ecx | jb  SUB | mov ds:g,ebp | jmp END | SUB: sub ecx,eax | mov ds:g,ecx
base:   cmp eax,ecx | jae ZER | sub ecx,eax  | mov ds:g,ecx | jmp END | ZER: mov ds:g,ebp
```

STEERABLE, and it multiplies: the same block usually repeats once per timer.
Evidence (2026-07-28, `src/Gruntz/Multi.cpp`): `CMulti::AdvanceGameFrame` @0x0b6b40 91.4 → 98.0
with the five stat-timer decays swapped, `CMulti::Render` @0x0b6890 the m_drainTimer
decay — both had been filed "MSVC5's register/branch choices … not steerable from
source". The sibling shape for a value/value (not value/zero) select is
[default-then-override-flag.md](default-then-override-flag.md).

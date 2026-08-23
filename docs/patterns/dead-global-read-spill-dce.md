# A dead global-read spill beside a live sibling field is a by-value struct return
tags: cpp:local cpp:member cpp:inline cpp:temporary | asm:mov asm:sub | topic:codegen-idiom topic:regalloc
variants: a-dead-stack-store-is-not-an-era-anomaly.md, dead-unreachable-recheck-block-dce.md

symptoms: `sub esp,8` + a `mov [esp+N],reg` of a freshly-loaded global field
that is NEVER read back; the load + spill repeat per branch; recompile omits both
the frame slot and the store and frees the register, cascading regalloc shifts.

Retail loads two ADJACENT fields off a global (`g_gameReg->m_modeSize.cx` and
`.cy`), uses one, and spills the other dead — once per arm. That is not a weaker
dead-store pass on retail's side: it is an inlined accessor returning the pair BY
VALUE. cl materialises the frame temp, folds the read half into its consumer and
leaves the unread half's store behind. Read the two offsets as ONE object and give
the class the by-value accessor.

```cpp
// tagSIZE GetModeSize() { return m_modeSize; }   -- in CGruntzMgr, not per TU
m_originY = g_gameReg->GetModeSize().cy - 66;    // .cx's store is the dead spill
```
```asm
sub    esp,0x8
...
mov    edx,DWORD PTR [eax+0x8c]   ; .cx  -> the temp
mov    DWORD PTR [esp+0x8],edx    ; spill - never read back
mov    eax,DWORD PTR [eax+0x90]   ; .cy  - folded into its consumer
```
STEERABLE. `CChatBoxOwner::Configure` 0x20530 (~69%) and `::HitTest` 0x21140
(~38%, 4 dead width spills) are both **100.00 EXACT**. The naming-a-local spellings
this file used to park behind — an unread local, an unread array element, `(void)vx`,
a `*(volatile i32*)` read — all fail because none of them creates the temp; the
struct RETURN is what does. Mechanism and its controls:
[[a-dead-stack-store-is-not-an-era-anomaly]].

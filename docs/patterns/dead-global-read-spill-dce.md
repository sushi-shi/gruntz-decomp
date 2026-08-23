# Retail keeps a dead global-read spill our MSVC5 DCE eliminates
tags: cpp:local cpp:member | asm:mov asm:sub | topic:wall topic:regalloc
variants: a-dead-stack-store-is-not-an-era-anomaly.md, dead-unreachable-recheck-block-dce.md

symptoms: `sub esp,8` + a `mov [esp+N],reg` of a freshly-loaded global field
that is NEVER read back; the load + spill repeat per branch; recompile omits both
the frame slot and the store and frees the register, cascading regalloc shifts.

A function reads two adjacent fields off a global (e.g. `g_gameReg->m_8c` and
`->m_90`, the viewport width/height) into locals but uses only one. Retail's
MSVC5 keeps the unused field's load AND spills it to a dead stack slot (`sub
esp,8` / `mov [esp+N],reg`), duplicated into each arm before a merge. Our same
MSVC5 cl runs a stronger dead-store pass: it DCEs the unused read entirely (no
frame, no spill) and reuses the freed register, so the whole register allocation
and per-arm code motion diverge.

```cpp
// the real dev shape (read both viewport coords, use only height):
i32 vx = g_gameReg->m_8c;   // DCE'd by our cl; retail spills it dead to [esp+N]
m_4 = g_gameReg->m_90 - 66;
```
```asm
sub    esp,0x8
...
mov    edx,DWORD PTR [eax+0x8c]   ; load width
mov    DWORD PTR [esp+0x8],edx    ; spill — never read back
mov    eax,DWORD PTR [eax+0x90]   ; height (the only used coord)
```
WALL: no natural C++ spelling reproduces it — naming the unused local, an unread
array element, or `(void)vx` all get DCE'd; only a `*(volatile i32*)` read forces
the load, but it still lands in a register (no `[esp]` spill) and is a tooling
hack, not the dev shape. Accept the plateau. Evidence: CChatBoxOwner::Configure
(0x20530, ~69%) and ::HitTest (0x21140, ~38%, 4 dead width spills); logic + offsets
exact, residual is purely the missing dead spills + their regalloc fallout.

CORRECTED 2026-08-23: "our cl runs a stronger dead-store pass" is too strong and
invites the RTM-provenance reflex. cl 5.0 SP3 DOES emit dead stack stores from
ordinary source — `SaveScreenshot` writes `srcRect.right = 0;` and overwrites it two
statements later, keeps the store, and is EXACT. The elimination is conditional, not
unconditional; a both-sides census finds the shape in 18 base objs. Run that census
before parking a row here — see [[a-dead-stack-store-is-not-an-era-anomaly]].

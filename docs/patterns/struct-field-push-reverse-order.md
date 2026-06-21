# Many-arg WinAPI call from a params struct: declare the struct in REVERSE arg order so the ascending-offset push falls out
tags: cpp:struct asm:push | topic:codegen-idiom
symptoms: a 12-arg CreateWindowExA-style call loads struct fields `[eax+0], [eax+4], … [eax+0x2c]` in ASCENDING order, each pushed immediately; natural-order struct loads +0x2c first → ~97.6%
confidence: 8/10

When a many-arg WinAPI call is fed from a params struct, the target loads `[struct+0]`,
`[struct+4]`, … in ASCENDING offset order, each pushed immediately. Because MSVC pushes call
args RIGHT-TO-LEFT, the FIRST-loaded field (`[+0]`) is the LAST (rightmost) call arg and the
LAST field (`[+0x2c]`) is the first call arg. So declare the params struct in REVERSE
call-arg order (lpParam @+0 … dwExStyle @+0x2c) and call normally — the ascending-offset push
sequence falls out.

```cpp
struct Params { void* lpParam; /*…*/ DWORD dwExStyle; };   // reverse of CreateWindowExA arg order
CreateWindowExA(p->dwExStyle, …, p->lpParam);
```
STEERABLE. Evidence: CGameWnd::CreateAndShow (@0x13cf20) — natural-order struct 97.6% → reverse
99.8%. GOTCHA: a header-only struct reorder does NOT retrigger the .cpp recompile in ninja —
`rm` the unit's base/current .obj (or touch the .cpp) before rebuild.

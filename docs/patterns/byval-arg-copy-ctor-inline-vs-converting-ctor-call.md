# By-value class arg built two ways: trivial copy-ctor INLINES, converting-ctor CALLs

**Tags:** cpp:ctor cpp:local | asm:call asm:mov asm:sub | topic:codegen-idiom topic:regalloc

## Symptom

A function passes the same small class **by value** at two call sites, once from a
local of a layout-compatible type and once from a pointer deref:

```cpp
RECT sh; CopyRect(&sh, rc); OffsetRect(&sh, 2, 3);
g_textObj.RenderText(*str, drawFn, WapRect(sh),  1, flag, 0);   // shadow (local)
g_textObj.RenderText(*str, drawFn, WapRect(*rc), 1, flag, 0);   // main   (deref)
```

where `WapRect(const RECT&)` is an **external** engine ctor (its own RVA, e.g.
0x115b30, a `CopyRect` wrapper). Retail's /O2 SPLITS the two builds:

- **shadow pass**: INLINES a 4-mov field copy of `sh` straight into the outgoing arg
  slot (`sub esp,0x10; mov ecx,esp; mov [ecx],..; mov [ecx+4],..; ..`), no call.
- **main pass**: CALLs the converting ctor (`sub esp,0x10; mov ecx,esp; push rc;
  call <ctor>`).

Modeling the ctor as **external for both** calls it twice AND materializes a
**persistent** temp slot -> `sub esp,0x20` instead of retail's `0x10`, shifting every
`[esp+N]` and cascading (~53%). Modeling it **inline for both** fixes the frame but
INLINES the main pass too (retail calls there) -> the main "Copy" reloc vanishes
(~58%).

## Cause

MSVC5 inlines a **compiler-generated trivial copy ctor** (member-wise, always
available) but emits a **call** to a user-declared **external** converting ctor. Retail
got the split because the shadow arg was effectively a copy of a same-layout local
while the main arg went through the converting ctor.

## Fix (steerable)

Keep the converting ctor **external** (declaration only) so `WapRect(*rc)` CALLs it,
and pass the shadow's rect as a **`WapRect` lvalue** so the trivial copy ctor inlines:

```cpp
g_textObj.RenderText(*str, drawFn, *(WapRect*)&sh, 1, flag, 0);  // trivial copy -> inline
g_textObj.RenderText(*str, drawFn, WapRect(*rc),   1, flag, 0);  // converting ctor -> call
```

The `*(WapRect*)&sh` reinterpret is binary-proven authentic — retail emits exactly the
inline 4-mov byte copy of `sh` (WapRect and RECT are layout-identical). Frame collapses
`0x20 -> 0x10`, all callees pair. `EngStr_RenderText` (0x115930): 53 -> 60 (residual is
an unrelated switch jump-table artifact).

## Confidence

c7 — reproduced live (engstrrendertext). The external-vs-copy-ctor distinction is the
lever; the reinterpret only works because the two types share layout.

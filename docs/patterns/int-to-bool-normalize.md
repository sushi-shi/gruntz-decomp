# `return Fn(...) != 0;` emits the int→bool `neg/sbb` normalize — a bare `return Fn(...)` omits it
tags: cpp:branch cpp:return | asm:neg asm:sbb | topic:codegen-idiom
symptoms: target has a trailing `neg eax; sbb eax,eax; neg eax` (or `neg/sbb/inc`) that your recompile lacks, on a function returning a call result as bool
confidence: 8/10

When a function returns a non-{0,1} value (a call result, an int member) coerced to bool, MSVC5
emits the canonicalizing idiom `neg eax; sbb eax,eax; neg eax` (for `!= 0`) or `neg eax; sbb
eax,eax; inc eax` (for `== 0`). A bare `return base(...)` / `return val` omits it. Spell the
coercion explicitly:

```cpp
return base(args) != 0;     // neg/sbb/neg  (0/1 from nonzero)
return RegFn(args) == 0;    // neg/sbb/inc  (1 when zero)
```
STEERABLE. Evidence: CGruntzApp::Init (@0x80930) — the `!= 0` is the load-bearing detail that
emits the int→bool idiom (a bare `return base(...)` omits it); the L1712 RegFn==0 one-liner.
Contrast: a bool spanning a /GX SEH teardown needs `? true : false` (setne), not this — see
seh-bool-return-canonicalize.md.

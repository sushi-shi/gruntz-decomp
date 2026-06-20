# A `new`d unmatched engine class must be padded to its REAL size so `operator new` pushes the right constant
tags: cpp:ctor cpp:new | asm:push asm:call | topic:codegen-idiom
symptoms: `push <wrong-N>; call ??2@YAPAXI@Z` desyncs everything from the alloc onward, a thin shell emitted `push 0xc`
confidence: 8/10

`new T(...)` emits `push sizeof(T); call operator new`. If you model an unmatched engine class
you `new` as a thin/minimal shell, `cl` pushes the shell's size (e.g. `push 0xc`) instead of the
real size, and the byte stream desyncs from the allocation onward. Pad the modeled class to its
REAL byte size (read it from the target's `push <N>` immediate) — `char m_pad[N]` or fields out
to N — and the `push <N>` and everything downstream falls into place.

```cpp
struct CPlane { char raw[0x158]; };   // model unmatched new'd classes at FULL size, not minimal
```
STEERABLE. Evidence: WwdFile::ReadPlane (@0x15d8d0) — CPlane is 0x158 B; the thin shell pushed
`0xc`, padding to `[0x158]` flipped it to `push 0x158` and the whole body fell into place (99.19%).
related: external-nobody-callee.md (the unmatched class's methods are external no-body),
dummy-virtual-slots.md (declare N dummy virtuals so its real method lands at the right offset).

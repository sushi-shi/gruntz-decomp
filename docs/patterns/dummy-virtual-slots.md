# Declare N dummy virtuals so the real method lands at the right vtable offset → `mov eax,[this]; call [eax+slot]` for free
tags: cpp:method cpp:virtual | asm:mov asm:call | topic:codegen-idiom
symptoms: need to emit `mov edx,[ecx]; call [edx+0xNN]` (a specific vtable-slot indirect) but your class's method lands at the wrong index
confidence: 8/10

To reproduce an indirect vtable-slot call `mov eax,[this]; call [eax+slot]` you only need the
target method to sit at the right vtable INDEX. Declare `slot/4` placeholder virtuals (empty
inline stubs, not in symbol_names.csv) ahead of it so the real method lands at the correct
offset; the indirect call then falls out, reloc-masked. The placeholders cost nothing and do not
shift data members (the vptr is already @+0). Adding virtuals does NOT regress scalar-delete
sites (they use slot 0 regardless of how many slots follow).

```cpp
struct CPlane { virtual void v0(); ... virtual int Read(...); };  // Read at slot 10 = +0x28
```
STEERABLE. Evidence: WwdFile::ReadPlane CPlane Read @slot 10 / dtor @slot 1; CGameApp idle
tail-call `jmp [eax+0x10]` (modeled m_8 with 4 slots so index-4 PerFrameTick is real); CGameLevel
LoadWwd (17 placeholder virtuals before Reset @+0x44); CMapMgr 6-slot vftable emitted in-TU.
related: newd-class-real-size.md, external-nobody-callee.md, emit-vtable-in-tu.md.

# Opened scalar `new T(...)` shows as `operator new` + null-guarded ctor
tags: cpp:new cpp:ctor | asm:call asm:test asm:jcc | topic:codegen-idiom
symptoms: operator new(size); if(p) p=p->Ctor(...); else p=0; push <sizeof>; call ??2; test eax,eax; je; mov ecx,eax; call ctor
confidence: 8/10
variants: newd-class-real-size.md, rezalloc-placement-new-no-eh-frame.md, inline-multiderived-ctor-vtable-stores.md

MSVC5 commonly lowers a source `new T(args...)` into an allocation, a compiler
null guard, and a constructor call. The recovered source may look like a real
`if (p) p = p->Ctor(...); else p = 0;`, but that `if` is usually codegen, not
author-written logic.

```cpp
struct T {
    inline T(A a, B b) { Ctor(a, b); }

    // Use this only when the reconstructed shell size cannot safely become
    // sizeof(T), or when the allocation size is load-bearing for matching.
    inline void* operator new(u32) { return ::operator new(0x58); }

    T* Ctor(A a, B b);
};

T* p = new T(a, b);
```

```cpp
// Decompiler-opened shape to replace when the constructor semantics are known.
T* p = (T*)operator new(0x58);
if (p) {
    p = p->Ctor(a, b);
} else {
    p = 0;
}
```

```asm
push 58h
call ??2@YAPAXI@Z        ; operator new(uint)
add  esp,4
test eax,eax
je   short null_alloc
mov  ecx,eax             ; this
push b
push a
call ?Ctor@T@@QAEPAV1@HH@Z
jmp  short done
null_alloc:
xor  eax,eax
done:
```

Treat this as steerable only when the recovered source already proves the ctor
body and arguments. Prefer a real inline constructor and, if necessary, a
class-local fixed-size `operator new`; do not invent names or constructor effects
from assembly alone.

Do not convert buffers, arrays, file-read storage, or allocator blocks that are
not object construction. For `RezAlloc` plus placement-new in /GX functions, see
`rezalloc-placement-new-no-eh-frame.md`: spelling a real `new` can add the wrong
EH frame or lower the score.

Evidence: real `new` improved or held metrics for `CFaderMgr`, `DirectSoundMgr`
clone construction, `TriggerMgrEh`, and draw-pool items. Revert on any exact,
fuzzy, or generated-`README.md` regression; attempted conversions in
`DirectInputMgr2::AddController`, `CButeValueNode`, `WwdPlaneRender`,
`SoundDevice`, and `SoundStream` either lost exactness or lowered module fuzzy
percentages.

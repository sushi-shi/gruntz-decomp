# Opened scalar `new T(...)` shows as `operator new` + null-guarded ctor
tags: cpp:new cpp:ctor | asm:call asm:test asm:jcc | topic:codegen-idiom
symptoms: operator new(size); if(p) p=p->Ctor(...); else p=0; push <sizeof>; call ??2; test eax,eax; je; mov ecx,eax; call ctor
confidence: 10/10
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

Do not classify the source by adjacency alone. A search such as
`rg -U ' new .*\n.*if'` also finds an author-written allocation-failure check or
the first method call after construction. Build a `/Z7` object and use
`gruntz walls diagnose <rva> --asm`: the allocation, compiler null guard,
and constructor body are attributed to the `new` line, while a second test
attributed to the following `if` is source control flow. Retail's six
`CWorldSoundSet` factories and `WinMain` contain both tests and are exact with
both source statements retained.

The complete tree audit confirms that this distinction is mechanical, not an
example-based heuristic. All 209 lexical results of
`rg -U ' new .*\n.*if' src` were checked: three are substring false positives;
the other 206 contain a retail branch for the following condition. Of those,
113 have byte-exact machine-code confirmation, 82 have `/Z7` line attribution
plus normalized instruction mapping, and 11 need direct retail inspection
because VC5 tail-merges or reorders the relevant block. Exactness does not
prove a unique original source spelling. A full constructor/caller remodel can
move that condition, but deleting it alone cannot explain retail's second
branch.

The inverse trap is an implicit default constructor that is too weak. If retail
has member/base constructor calls followed by scalar stores inside the
allocation guard, but the base `/Z7` block stops after the member/base calls,
those stores belong to an explicit inline constructor even though there is no
opened store block in the caller. This recovered `CCheatMgr`,
`DirectInputMgr2`, `CFontConfig`, and `CVoiceManager`: their stores moved
onto the `new` source line and `CGruntzMgr::Run` rose from 85.2590% to 86.3292%.
Check the complete guarded interval, not only source-visible `if (p)` blocks.

Do not convert buffers, arrays, file-read storage, or allocator blocks that are
not object construction. For `RezAlloc` plus placement-new in /GX functions, see
`rezalloc-placement-new-no-eh-frame.md`: spelling a real `new` can add the wrong
EH frame or lower the score.

Evidence: real `new` improved or held historical MAX for `CFaderMgr`,
`SoundBuffer` clone construction, `TriggerMgrEh`, and draw-pool items.
Retain evidence-backed constructor ownership even when unrelated current scores
move; investigate only a MAX-gate failure or structural contradiction. Attempted
conversions in `DirectInputMgr2::CreateDeviceGroup`, `CButeValueNode`,
`WwdPlaneRender`, `SoundDevice`, and `SoundStream` changed the wrong call/CFG
shape and were rejected on that evidence.

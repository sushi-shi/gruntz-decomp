# MSVC 5.0 has no `operator new[]` — the allocation SYMBOL cannot tell you the source form

tags: cpp:new cpp:array cpp:class | asm:push asm:call | topic:codegen-idiom topic:identity

symptoms: a `push <n>; call ??2@YAPAXI@Z; add esp,4` site, and the question "was this
`new T[n]`, `new BYTE[n * sizeof(T)]`, or a bare `::operator new(...)`?". Or a
CArray-shaped growth function stuck a point or two below 100% with only a two-register
rotation in the block after the `call ??3@YAXPAX@Z`.

confidence: 10/10

## `??_U` does not exist in this toolchain

`??_U@YAPAXI@Z` (`operator new[]`) appears **zero** times in the MSVC 5.0 `LIBCMT.LIB`
(`??2@YAPAXI@Z` appears 4). Every array `new` — of a POD or of a class — lowers to plain
`??2@YAPAXI@Z`. So the callee at an allocation site distinguishes *nothing*: these three
emit byte-identical code (measured, `/O2 /MT /GX /GR`):

```cpp
static_cast<RezElem40*>(::operator new(n * sizeof(RezElem40)))   // lea/shl/push/call ??2
reinterpret_cast<RezElem40*>(new BYTE[n * sizeof(RezElem40)])    // identical
new BYTE[n * sizeof(RezElem40)]                                   // identical
```

## What DOES read out: the vector constructor iterator

`new T[n]` where `T` has a **user-declared constructor** is a different animal. cl emits

- a `/GX` EH frame (`push -1 / push <handler> / mov fs:0,esp`) plus a `.text$x` unwind
  funclet that calls `??3@YAXPAX@Z`,
- an inline count-down loop calling `??0T@@QAE@XZ` (or the `??_H@YGXPAXIHP6EX0@Z@Z`
  vector-constructor-iterator helper),

and *no* array cookie (MSVC 5.0 only needs the count back at `delete[]` time, i.e. when
`T` has a destructor). So:

> **Retail allocating exactly `n * sizeof(T)` bytes with no `??0T` loop after the
> `call ??2` proves the source did NOT write `new T[n]`** — it wrote a raw byte
> allocation. If the class has a real `??0T` in the image (ours does: `RezElem40::RezElem40`
> at `0x0017f300`) and the allocation site never calls it, the elements are constructed
> somewhere else — e.g. MFC's `ConstructElements<TYPE>` placement-new loop.

That is exactly MFC's `CArray<TYPE,ARG_TYPE>::SetSize`, whose real source line is
`m_pData = (TYPE*) new BYTE[nNewSize * sizeof(TYPE)];`. `CRezBufferObject` is a
hand-written clone of it (same four members, same `/8` clamp-to-`[4,1024]` growth
heuristic, same `ConstructElements` body), so read the whole family off MFC's source
rather than re-deriving each branch.

## The `delete` spelling is regalloc-load-bearing (the steerable half)

`::operator delete(p)` and `delete[] p` / `delete p` are byte-identical **in isolation** —
all four spellings compile to `push p; call ??3@YAXPAX@Z; add esp,4` when `T` has no
destructor. Inside a real function they are **not** interchangeable: the statement form
and the call-expression form build different expression trees and the allocator colours
the surrounding block differently.

`CRezBufferObject::SetSize` 0x17f390, the block right after the `call ??3`:

```
retail                              ::operator delete(m_pData)      delete[] m_pData
  mov eax,[esp+0x18]                  mov ecx,[esp+0x18]              mov eax,[esp+0x18]
  mov ecx,[esp+0x1c]                  mov edx,[esp+0x1c]              mov ecx,[esp+0x1c]
  add esp,0x4                         add esp,0x4                     add esp,0x4
  mov [ebx+0x8],ebp                   mov [ebx+0x8],ebp               mov [ebx+0x8],ebp
  mov [ebx+0x4],eax                   mov [ebx+0x4],ecx               mov [ebx+0x4],eax
  mov [ebx+0xc],ecx                   mov [ebx+0xc],edx               mov [ebx+0xc],ecx
```

Switching the seven sites to the statement form took `SetSize` 95.90 -> 96.04,
`CRezBufferObject::Serialize` 96.47 -> 97.39 and `CFaderMesh::ApplyInit` 68.30 -> 69.79,
and moved **nothing** else in the tree. The typed `delete[] m_pData` and MFC's
`delete[] (BYTE*)m_pData` are equally good here (no destructor, so no cookie arithmetic);
prefer the typed one — it needs no `reinterpret_cast`, which is a FATAL ratchet.

The `new` half of the same rewrite moved **zero** functions, so spell it whichever way the
cast board prefers; `static_cast<T*>(::operator new(n * sizeof(T)))` costs no ratcheted
cast and is honest about the raw byte buffer.

## Corollary: never hardcode a size in a per-class `operator new`

```cpp
void* operator new(u32) { return ::operator new(0x6c); }   // BANNED
```

This inverts the project's own class-size oracle
([class-layout-has-three-retail-oracles.md](class-layout-has-three-retail-oracles.md)):
`push 0x6c` then proves nothing, because we told cl to push it. It also hides a layout bug
from `alloc_size` — the hardcoded literal can disagree with the real
`sizeof` and nothing fires. Delete the member `operator new` and let `new CFoo` compute the
size; cl inlines a forwarding member `operator new` away entirely, so no `??2CFoo@@SAPAXI@Z`
COMDAT is emitted either way (retail has none, and neither do our base objs).

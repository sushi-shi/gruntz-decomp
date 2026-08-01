# `#include <new>` silently deletes a /GX unwind frame

**Tags:** `topic:codegen-idiom` `cpp:include` `asm:eh` `asm:prologue` `topic:eh`

## Symptom

A destructor (or any function with a sub-object to unwind) is missing retail's whole
`/GX` EH prologue/epilogue, and nothing about the body or the class explains it:

```
retail                                base (ours)
push   0xffffffff                     push   esi
push   <__ehhandler>                  mov    esi,ecx
mov    eax,fs:0x0                     mov    eax,[esi+0x4]
push   eax                            mov    [esi],<derived vtbl>
mov    fs:0x0,esp                     test   eax,eax
push   ecx                            ...
push   esi                            mov    [esi],<base vtbl>
mov    esi,ecx                        pop    esi
mov    [esp+0x4],esi   ; `this`       ret
mov    [esi],<derived vtbl>
mov    eax,[esi+0x4]
mov    [esp+0x10],0x0  ; EH state 0
...
mov    [esi],<base vtbl>
mov    fs:0x0,ecx
pop    esi
add    esp,0x10
ret
```

The unit already has `/GX` (the `eh`/`mfc`/`framedeh` profile), the class already has the
right polymorphic base, and no body spelling brings the frame back. Score sits far below
100 (measured: `??1CRezBufferObject@@UAE@XZ` at **46%**).

## Mechanism

MSVC 5.0 only registers a base/member sub-object as an *unwind action* if something
between the sub-object's construction and its destruction **can throw**. In a destructor
whose only call is `::operator delete`, that call IS the only throw point.

`<new>` (the C++ standard header) declares

```cpp
void __cdecl operator delete(void*) throw();
```

The `throw()` spec makes the delete a **non-throwing** call, so the region has no throw
point, so cl drops the unwind state *and the entire EH frame*. `<new.h>` (the pre-standard
MSVC header) declares the same operator **without** the exception specification, and also
supplies the placement `operator new(size_t, void*)` you were including `<new>` for.

So the header choice — not the body, not the class, not the flags — decides whether the
frame exists.

## Fix

Include `<new.h>`, never `<new>`, in a TU that (a) needs placement new and (b) contains a
`/GX` function whose unwind depends on a `delete`/`operator delete` call:

```cpp
// <new.h>, NOT <new>: the C++ <new> declares `operator delete(void*) throw()`, and a
// nothrow delete makes cl drop the /GX unwind frame retail has.
#include <new.h>
```

## Scope — it bites ONLY when the delete is the region's SOLE throw point

Swept all 11 remaining `#include <new>` TUs (2026-08-01, `<new>` vs `<new.h>`, per-symbol
objdiff on each): **10 of 11 are exact no-ops**, and the 11th moved one unrelated function
by +0.7. Nothing else in the tree was losing a frame. The reason is the mechanism itself —
the exception spec only decides the unwind state when the `delete` is the *only* call in
the protected region:

| TU class | why it is a no-op |
|---|---|
| `flags = "base"` (no `/GX` at all) — `zvec`, `gruntstatestep`, `grunttilescan` | there is no frame to drop |
| no destructor with a base sub-object — `warlord`, `gruntcombat`, `gruntarrivalscan`, `modeobjinit` | nothing registers an unwind action in the first place |
| a destructor that ALSO calls something else — `grunt` (`~CGrunt`), `gruntzmgr` (`~CGruntzMgr`, `~CDemo`), `tiletriggercontainer`, `battlezmapconfig` | member/base dtors and MFC calls are already throw points, so the frame exists either way and they are already 100% |

So the diagnostic is narrow and precise: **a minimal destructor whose entire body is
`if (m_p) delete m_p;`** (or `::operator delete`), in a `/GX` unit, missing retail's frame.

**Do not sweep the tree for this.** `<new.h>` is a *smaller* header, so it shifts cl's
internal symbol counter and RENUMBERS every `$S<n>` local-static COMDAT in the TU
(measured: `_s_FreezeRadius$S33024` -> `$S32890` in `GruntCombat.cpp`). In a TU that has
such statics that is churn for no gain. Change the include only where the frame is
actually missing.

## Evidence

- `src/Rez/RezBufferObjectDtor.cpp`, `??1CRezBufferObject@@UAE@XZ` @ 0x17f330:
  **46.26% -> 100.00% EXACT** from that one-character include change; no other edit.
- Isolation probe (2026-08-01): the identical class + identical dtor body compiled in a
  scratch TU produced retail's frame byte-for-byte; adding `#include <new>` to that
  scratch TU deleted it, and `#include <new.h>` restored it. `<Rez/RezAlloc.h>`,
  `<rva.h>` and the `RezElem40::RezElem40()` definition were each ruled out by the same
  bisect.
- Six destructor-body spellings (`delete[] (u8*)`, `delete[] T*`, explicit destruct loop,
  explicit `CObject::~CObject()` call, `RezFree`, `!= 0`) all scored an identical 46%
  while `<new>` was included — i.e. the body is not the lever, the header is.

## See also

- `docs/seh-eh.md` — the `/GX` frame shapes and when cl emits them.
- `docs/patterns/INDEX.md` — `topic:eh` entries.

# __thiscall foreign vtable: a declared-only virtual interface is the ONLY MSVC5 form — a char-pad fn-ptr Vtbl struct is impossible
tags: cpp:virtual cpp:method | asm:mov asm:call | topic:wall topic:codegen-idiom
symptoms: C4234 "'__thiscall' keyword reserved for future use"; virtual vNN()/dummyN()/SlotNN placeholder run for a slot dispatched as `mov eax,[this]; mov ecx,this; call [eax+off]`
confidence: 9/10
variants: emit-vtable-in-tu.md, reloc-typing-vptr-global.md

A foreign engine object we only *call through* (its ctor/vtable live in other, unmatched TUs)
dispatches a used slot as a **__thiscall** virtual: `mov eax,[this]; mov ecx,this; call [eax+off]`
(`this` in ECX). The source-doctrine "unroll a manual vtable into a typed `struct Vtbl { void* pad[N];
T(*Fn)(...); } *m_vptr`" idiom (`p->m_vptr->Fn(p)`) — used to KILL a fake `virtual vNN()` placeholder
run — **only works for `__stdcall`/COM vtables** (DirectDraw/DirectSound: `this` is pushed on the STACK,
so a plain/`__stdcall` fn-ptr reproduces the bytes). For a **__thiscall** vtable a fn-ptr CANNOT put
`this` in ECX: MSVC 5.0 **rejects the `__thiscall` keyword on a function-pointer type** — `i32(__thiscall*
Fn)(void*)` is a hard **error C4234** ("reserved for future use"). A `__stdcall` fn-ptr would push `this`
→ wrong bytes. So the char-pad Vtbl struct is unreachable; the declared-only virtual interface (a run of
`virtual void vNN()` up to the used slot's INDEX, then the named used virtual, `this` cast to it) is the
**only** MSVC5-expressible form, and its placeholder-slot run is FORCED, not lazy — do not try to
"reduce" it to a char-pad struct.

```cpp
// __thiscall foreign vtable, only slot +0x20 used — REQUIRED form (no char-pad alternative):
struct IFoo { virtual void v00(); /* … up to */ virtual void v1c(); virtual i32 Used(); /* +0x20 */ };
((IFoo*)this)->Used();                       // mov eax,[this]; mov ecx,this; call [eax+0x20]
// struct FooVtbl { char pad[0x20]; i32(__thiscall* Used)(void*); };  // C4234 — DOES NOT COMPILE
// struct FooVtbl { char pad[0x20]; i32(__stdcall*  Used)(void*); };  // wrong: pushes `this`, diverges
```
Wall (toolchain limit, not a codegen choice). Real reductions of such a run need the class made fully
polymorphic with an EMITTED `??_7` (emit-vtable-in-tu.md) — which requires the ctor in-TU; when the ctor
is external the declared-only interface stays. Evidence: WwdSelf (CWwdGameObject vtable @0x5f0020, slots
+0x20/+0x40), CSerialArchive (Read @+0x2c / Write @+0x30), CPlane/CPlaneRenderPoly (WwdFile.h) — all
__thiscall, all necessarily declared-only. A char-pad conversion attempt fails to build with C4234.

# A `mov ecx,<receiver>` before a call we model as a FREE function proves the callee is a `__thiscall` METHOD
tags: cpp:method cpp:call | asm:mov asm:call | topic:identity topic:codegen-idiom
symptoms: our version is missing exactly one `mov ecx,[reg+N]` (or `mov ecx,<this>`) immediately before a `call`; the callee itself is already 100% EXACT, so it "cannot" be wrong; the loaded value is never used again
confidence: 9/10

A one-argument `__stdcall` free function and a one-argument `__thiscall` method that
ignores `this` compile to **identical bodies** — both read the argument at `[esp+4]` and
`ret 4`. So a callee can score 100% under the wrong linkage forever, while every CALL
SITE silently drops the receiver setup. The tell is at the call sites, never in the body:

```asm
; CRezDir::Find @0x13a0b2                 ; CRezMgr::ReadEmulationDirectory @0x13b71a
mov  ecx,DWORD PTR [ebx+0x18]  ; m_owner  ; mov ecx,DWORD PTR [esp+0x14]  ; its own `this`
add  esp,0x4                              ; add esp,0x4
lea  edx,[esp+0xc]                        ; lea edx,[esp+0x18]
push edx                                  ; push edx
call 0x13b910                             ; call 0x13b910
```
Two independent call sites both loading a receiver ⇒ the callee is a method of that class.

```cpp
// WRONG - scores 100% on the body, loses one instruction at every call site
u32 __stdcall PackTag(const char* s);
// RIGHT
class CRezMgr { u32 PackTag(const char* s); };   // 0x13b910
// and the other class reaches it through its owner:
fourcc = m_owner->PackTag(tmp);
```
The mangled name changes (`?PackTag@@YGIPBD@Z` -> `?PackTag@CRezMgr@@QAEIPBD@Z`) and
that is fine — names are ours, the RVA binding is what matters.

**Do not "fix" this with an inline forwarder.** A wrapper whose body ignores `this` gets
its receiver load deleted by cl, so it reproduces nothing; only real `__thiscall` linkage
emits the load. STEERABLE: `CRezDir::Find` @0x13a040 98.28 -> **100 EXACT**.

## The inverse tell: the CALLER spills `this` for no other reason

`CDDrawChildGroup::CollideBroadcast` @0x159f00 opened its frame with `sub esp,0x30` +
`mov [esp],ecx` where we emitted `sub esp,0x2c` and kept `this` in a register — and the
only two consumers of that slot were dead `mov ecx,[esp+0x14]` / `mov ecx,[esp+0x10]`
loads immediately before `call RectsOverlap` and `call BoxesOverlap`. Both callees are
`__stdcall`-shaped (they clobber `ecx` with their first argument) and both were modelled
as free functions; both were really `__thiscall` members of `CDDrawChildGroup`. So the
receiver-load tell also shows up as **a spill slot the function has no other use for**:
if `this` is spilled and never read except right before a call, that call takes it.
Confirming evidence is one-sided xrefs — `sema xref` gave each helper exactly one caller,
`CollideBroadcast` itself. 90.93 -> 94.39 in one build; the callee bodies did not move
(`RectsOverlap` stayed 100% EXACT through the re-mangling).

## Third tell: the EH band finds it when neither call site is being read

`CChatBoxOwner::HandleTextInputKey` @0x205c0 carried one unwind funclet retail has not
got — `??1CButeTail@@QAE@XZ` — which shifted every later state by one and made the whole
chain read as a wrong-TYPE divergence in `eh_band --census`. The extra state came from a
fabricated `CButeTail cryptTail;` local, and chasing the class through `sema xref` put
the object where it belongs and handed over the receiver-load tell for free:

```
$ gruntz sema xref 0x0016f680          # ??0CButeTail
  <- call 0x00170210 ??0CButeMgr@@QAE@XZ        # `lea ecx,[esi+0x10f]` at 0x17029d
$ gruntz sema xref 0x0016f6b0          # ??1CButeTail
  <- call 0x000213c0 ??1CButeMgr@@QAE@XZ
  <- call 0x000205c0 ?HandleTextInputKey@...     # `lea ecx,[esp+0x14b]`, the SAME slot
```

A type constructed only by another class's constructor and destroyed only by its
destructor is a MEMBER of it — `CButeMgr::m_crypt` at +0x10f — so the caller uses
`bute.m_crypt`, not a second object. And the two remaining `CButeTail` entry points read
the same way: retail loads `ecx` before both of them, `lea ecx,[esp+0x14b]` here and
`mov ecx,0x6454e7` (`g_buteMgr` + 0x10f) in `CGruntzMgr::Run` @0x83450, so the
`__stdcall` free function `Blowfish_InitKey` @0x16f6c0 was really
`CButeTail::InitKey(const char*)`. Its 18-byte body is identical either way because
`this` is unused — exactly the blind spot at the top of this file. 81.92 -> 83.66, and
the funclet chain became type-correct at all 26 indices.

**So the EH band is a third route into this pattern**: when a funclet names a type that
should not be in that frame, `sema xref` on the type's ctor/dtor usually resolves BOTH
the ownership and the linkage of everything else it touches.

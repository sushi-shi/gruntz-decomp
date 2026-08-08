# A `mov ecx,<receiver>` before a call we model as a FREE function proves the callee is a `__thiscall` METHOD
tags: cpp:method cpp:call | asm:mov asm:call | topic:identity topic:codegen-idiom
symptoms: our version is missing exactly one `mov ecx,[reg+N]` (or `mov ecx,<this>`) immediately before a `call`; the callee itself is already 100% EXACT, so it "cannot" be wrong; the loaded value is never used again
confidence: 9/10

A one-argument `__stdcall` free function and a one-argument `__thiscall` method that
ignores `this` compile to **identical bodies** — both read the argument at `[esp+4]` and
`ret 4`. So a callee can score 100% under the wrong linkage forever, while every CALL
SITE silently drops the receiver setup. The tell is at the call sites, never in the body:

```asm
; CSymTab::Find @0x13a0b2                 ; CSymParser::ParseRecords @0x13b71a
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
class CSymParser { u32 PackTag(const char* s); };   // 0x13b910
// and the other class reaches it through its owner:
fourcc = m_owner->PackTag(tmp);
```
The mangled name changes (`?PackTag@@YGIPBD@Z` -> `?PackTag@CSymParser@@QAEIPBD@Z`) and
that is fine — names are ours, the RVA binding is what matters.

**Do not "fix" this with an inline forwarder.** A wrapper whose body ignores `this` gets
its receiver load deleted by cl, so it reproduces nothing; only real `__thiscall` linkage
emits the load. STEERABLE: `CSymTab::Find` @0x13a040 98.28 -> **100 EXACT**.

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

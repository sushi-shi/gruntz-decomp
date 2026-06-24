# RezAlloc + placement-new ctor: MSVC5 omits the retail /GX EH frame
tags: cpp:eh cpp:new cpp:ctor | asm:mov asm:push | topic:wall topic:eh
symptoms: push -1 / push <state> / mov fs:0,esp prologue MISSING, every `return 0` is a frameless pop;ret instead of `jmp <shared-epilogue>`, sub esp,0x28 vs retail sub esp,0x34, zero pinned in edi vs retail esi, ~45-50%
confidence: 8/10

Retail allocates an engine resource object with RezAlloc (its global/class `operator
new`, inlined to `_RezAlloc`), null-guards it, and constructs in place — the throwing
ctor runs in a /GX ctor-in-flight try, so the function carries a full EH frame
(`mov fs:0; push -1; push <state>; push handler; mov fs:0,esp`), a trylevel local, and
ALL early-out `return`s funnel through one shared fs:0-restoring epilogue via `jmp`.

```cpp
// What the retail source did (and what we cannot reproduce on MSVC5):
StreamVoice* v = new StreamVoice(buf, this, a, b); // op-new == RezAlloc, EH-tracked ctor
```
```cpp
// What we write (body byte-exact, EH frame absent):
StreamVoice* v = (StreamVoice*)RezAlloc(0xb0);
if (v) v = new (v) StreamVoice(buf, this, a, b);   // placement new
```
```asm
; retail prologue (EH-framed)            ; our recompile (frameless)
mov eax,fs:0 / push -1 / push <state>    sub esp,0x28
push handler / mov fs:0,esp / sub 0x34   push ebx/ebp/esi/edi
... xor esi,esi (zero pinned in esi)     ... xor edi,edi (zero in edi)
return 0:  jmp <one shared epilogue>     return 0:  pop;pop;pop;pop;add esp,0x28;ret  (inlined)
```
WALL: MSVC5 predates C++ placement `operator delete`, so placement-new with a throwing
ctor emits NO ctor-in-flight EH state — the body is byte-identical but the whole /GX frame
+ shared epilogue is absent (the offset/zero-reg/epilogue cascade caps it ~47%). The
class-scoped `operator new` form (`new T`) DOES emit a frame but diverges on per-return
inline epilogues + early-out EH handling (~42%, worse). Evidence: SoundStream::
CreateStreamBuffer (0x137780) — body exact, 47.1%. Defer to the final sweep when the
StreamVoice ctor (0x1375b0) is modelled and a `new T`-with-real-allocator path can emit the
retail frame.

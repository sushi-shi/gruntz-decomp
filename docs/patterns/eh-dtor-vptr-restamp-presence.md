# /GX virtual dtor: polymorphic model re-stamps the most-derived vptr; retail elided it
tags: cpp:dtor cpp:eh cpp:virtual | asm:mov | topic:wall topic:eh
symptoms: ~Class body byte-identical except ONE extra `mov [esi],&??_7Class` re-stamp right after `mov [esp+N],this`, before the trylevel write; recompile ~92-93% with the /GX frame + base-dtor call already exact
confidence: 6/10

A `/GX` (`flags="eh"`) **virtual destructor** of a class that derives from a real
polymorphic base (e.g. `CPtrList`) and overrides only `~Base`: the polymorphic
model (declare `virtual ~Derived()`, let the compiler emit `??_7Derived`/`??_G`/
`??_E`) makes cl re-stamp the most-derived vptr at dtor entry — `mov [this],&??_7Derived`
— before the trylevel-1 write and the body. Retail's `~Class` has the SAME EH frame
(`push -1`/`mov fs:0`), the SAME trylevel stamps, and the SAME trailing base-dtor
call, but **no vptr re-stamp at all** (cl elided it because the dtor body makes no
virtual call on `this`, so the vptr value is dead). The single extra `mov` is the
only divergence; the function caps ~92-93% with everything else exact.

```cpp
class CFontConfig : public CPtrList {  // real polymorphic base -> /GX frame from base subobj
    ~CFontConfig();                    // virtual override (UAE), auto ??_7/??_G/??_E
};
CFontConfig::~CFontConfig() { Reset(); }  // body calls a non-virtual method; vptr never observed
```
```asm
6ff: mov esi,ecx
701: mov [esp+4],esi          ; retail goes straight to the trylevel write...
705: mov [esp+0x10],1         ;   (NO mov [esi],&??_7CFontConfig here)
70d: call Reset
; our recompile inserts `mov [esi],&??_7CFontConfig@@6B@` between 701 and 705.
```
WALL: cousin of eh-ctor-vptr-restamp-position.md (ctor: re-stamp present but scheduled
LATE) — here the dtor re-stamp is ELIDED entirely by retail and NOT by our cl, and the
presence/absence is decided by cl's /GX EH-state machine, not by any source spelling
(both the polymorphic auto-emit and a manual `*(void**)this=&vtbl` produce the store).
Modeling the base subobject is still REQUIRED (it supplies the EH frame + base-dtor call,
without which you fall to the ~50-60% eh-dtor-needs-base-subobject.md plateau) — so the
polymorphic model is correct; only this one re-stamp is unreachable. Defer to the final
sweep. Evidence: `CFontConfig::~CFontConfig` (0x85f40) 92.7%, body+frame otherwise exact;
siblings Reset/FreeNodes/AddItem/Scroll (same TU) all 100%.
related: eh-ctor-vptr-restamp-position.md, eh-dtor-needs-base-subobject.md

# Destructor's /GX EH frame comes from a non-trivial base subobject — can't get it with a manual-vptr non-polymorphic model
tags: cpp:dtor cpp:eh cpp:virtual | asm:mov | topic:wall topic:eh
symptoms: retail ~DtorBody has push -1/push handler/mov fs:0,esp + [esp+0x10]=1/-1 trylevel stamps around the body and a trailing base-dtor call; recompile emits a frameless push reg;…;pop reg;ret with no fs:0 / no trylevel, ~50-60%
confidence: 7/10

A destructor whose class has a base subobject with a non-trivial destructor gets
a full `/GX` SEH frame: the derived body runs inside a try region (trylevel stamped
1), then the implicit base-dtor call runs at trylevel -1, so the base is unwound if
the body throws. When the class is modeled NON-polymorphically (vptr stamped by a
manual `*(void**)this = &g_Vtbl` store, virtuals not declared, no real base class),
the recompiled dtor has nothing destructible and MSVC emits a frameless body — the
whole prologue/epilogue + trylevel machinery is missing, capping the function ~50-60%.

```cpp
// What retail wants (NOT recoverable without the real base hierarchy):
//   struct Base { virtual ~Base(); };          // ~Base == the trailing base-dtor call
//   struct Derived : Base { ~Derived() { ...clone loop... } };
// What we write while the class is only partially modeled (transitional manual stamp):
DirectSoundMgr::~DirectSoundMgr() {
    *(void**)this = (void*)g_DirectSoundCloneVtbl; // manual vptr stamp
    while (m_58_head != 0) RemoveClone(m_58_head->m_inst);
    BaseDtor();                                    // base-subobject dtor, modeled as extern
}
```
```asm
135bb0: push -1 / push 0x5e08a3 / mov fs:0,esp   ; the /GX frame we cannot emit
        mov [esi],0x5ef6bc                        ; vptr stamp (matches)
        mov [esp+0x10],1                           ; trylevel = 1 (base live)
        ...clone loop...                           ; (matches)
        mov [esp+0x10],-1                           ; trylevel = -1
        call 0x136260                              ; BaseDtor (matches)
```
WALL: the clone-loop body + vptr stamp + BaseDtor call are byte-exact; only the EH
frame is missing. Converting the class to a real polymorphic base hierarchy WOULD
emit the frame but re-shapes the ctor + emits a `??_7`/`??_G` and risks regressing
every already-exact sibling method — defer to the final sweep when the whole class
(its base + all virtuals) is modeled. Evidence: `DirectSoundMgr::~DirectSoundMgr`
(0x135bb0) 54.45%, body otherwise exact; sibling ctor 0x1351d0 is frameless + 100%
(no base-dtor → no frame), confirming the base subobject is the frame's only cause.

RESOLVED (2026-07-16, matcher-1 / Fable lane): the prescription above works — twice.
(1) `StreamVoice : DSoundCloneInst` (real base insertion + `Cleanup()` renamed to the
real non-virtual `~StreamFeeder`): ctor 0x1375b0 34%→100, dtor 0x137650 45%→100,
member dtor 0x137cf0 91%→100 — the /GX frame, the 1→0→-1 trylevel machine and both
vptr stamps all fall out of the language. (2) The DinMgr2 device chain: the
standalone base-dtor copies at 0x133370/0x1333b0 are cl's own out-of-line COMDAT
emissions of the header-inline `~CInputDevRoot`/`~CInputDevBase` (the leaf dtors'
EH funclets take their addresses) — bind them with `@rva-symbol`, never a
placeholder view; both 100. If you are staring at this wall, model the real base:
the frame is not hand-reachable but it is compiler-trivial.

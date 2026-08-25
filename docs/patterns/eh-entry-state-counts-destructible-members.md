# /GX entry state counts destructible sub-objects — an off-by-one means a MISSING member
tags: cpp:ctor cpp:dtor cpp:eh | asm:mov | topic:codegen-idiom topic:eh
symptoms: `mov dword ptr [esp+N],K` entry-state immediate off by one vs retail, dtor/ctor otherwise byte-identical, ~99.9%
confidence: 9/10
variants: eh-state-numbering-base.md

A `/GX` destructor stores its "everything alive" unwind state as the first thing in the body,
and that immediate is exactly **(number of destructible sub-objects) − 1** (base sub-object
included, members counted whether or not their destructor emits any code). The matching ctor
stores the same number once every sub-object is constructed. So when the whole body is
byte-identical and ONLY that immediate is low by one, the class model is **missing a
destructible member** — this is a diagnostic, not a wall. Read off WHERE it is from the state
the code drops to before each teardown call: the members are numbered in DECLARATION order
(member 0 destroyed last), so a missing state at the top means a member declared AFTER the
last one you already model.

```cpp
// Two independent dtors said IntrusiveList owns a do-nothing destructor:
struct IntrusiveList {
    IntrusiveLink* m_head;
    IntrusiveLink* m_tail;
    ~IntrusiveList() {} // no teardown code anywhere - it exists ONLY in the state count
};
```
```asm
; ~SoundSample 0x135bb0 - base SoundBufferInstance(0) + m_cloneList(1) => entry 1
    mov    DWORD PTR [esp+0x10],0x1     ; retail; without the list dtor cl emits 0x0
    ...
    mov    DWORD PTR [esp+0x10],0xffffffff  ; -1, then the base dtor
    call   0x136260
```
STEERABLE. Adding `~IntrusiveList(){}` flipped `??1SoundSample` (0x135bb0, entry 0→1) and
`??1CRezArchive` (0x13abc0, entry 1→2) to EXACT in one build. The same read cracked
`??1GruntzPlayer` (0x083260): entry 2 with only `CString m_name` + `CBattlezMapConfig m_038`
modelled proved a third sub-object declared after m_038 — the +0x22c `PlayerLatency`
{avg,count} accumulator, whose inline ctor is the pair of zero-stores retail emits right after
`??0CBattlezMapConfig` (the old model had to duplicate those two statements by hand). That
flipped the dtor to EXACT and took the ctor 0x0da790 down to a single residual store.

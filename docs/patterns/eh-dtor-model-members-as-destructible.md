# /GX member-teardown dtor: model the destructible members as REAL subobjects with destructors

tags: cpp:dtor cpp:eh cpp:member | asm:mov asm:call | topic:codegen-idiom topic:eh
symptoms: retail ~Dtor has push -1/mov fs:0,esp + a descending [esp+N]=3/2/1/0/-1 trylevel chain around N `call ~Member` invocations in reverse declaration order; a hand-written `void Dtor(){ m_a.Dtor(); m_b.Dtor(); }` (explicit method calls) emits a FRAMELESS body with no fs:0 / no trylevels, ~42%
confidence: 9/10

A destructor that tears down several owned subobjects (here four `CObList`
members at +0x00/+0x1c/+0x38/+0x54) gets a full `/GX` SEH frame: each member
destructor runs at its own descending trylevel so a throw mid-teardown unwinds
exactly the members already constructed. The retail shape is:

```asm
push -1 / push <handler> / mov fs:0,esp        ; /GX frame
...
mov [esp+0x10],3                                ; (most-derived body / DtorBase) trylevel 3
call <DtorBase>
mov [esp+0x10],2 ; lea ecx,[esi+0x54]; call ~CObList   ; m_list3
mov [esp+0x10],1 ; lea ecx,[esi+0x38]; call ~CObList   ; m_list2
mov [esp+0x10],0 ; lea ecx,[esi+0x1c]; call ~CObList   ; m_list1
mov [esp+0x10],-1; mov ecx,esi;       call ~CObList   ; m_base
```

The fix is to let the compiler EMIT this: write a **real destructor**
`~Class()` and give each member type a **real destructor** that calls the
(reloc-masked) engine dtor — do NOT hand-roll the teardown as explicit method
calls.

```cpp
class TtcObList {                       // the owned subobject (one CObList, 0x1c bytes)
public:
    void Dtor();                        // 0x1b48c6  ~CObList (reloc-masked rel32 callee)
    ~TtcObList() { Dtor(); }            // <-- REAL dtor: this is what drives the /GX frame
    void* m_vptr; TtcNode* m_pNodeHead; char _pad08[0x14];
};
class CTileTriggerContainer {
public:
    ~CTileTriggerContainer();           // 0xc8640  (mangles ??1...@@QAE@XZ)
    TtcObList m_base, m_list1, m_list2, m_list3;  // declared in teardown-reverse order
};
CTileTriggerContainer::~CTileTriggerContainer() {
    DtorBase();                          // the most-derived body (trylevel 3)
    // m_list3 / m_list2 / m_list1 / m_base auto-destroyed here, reverse decl order,
    // each at its own trylevel — the compiler emits the whole /GX machinery.
}
```

This took `CTileTriggerContainer::~CTileTriggerContainer` (0xc8640) from **42% →
100%**: the trylevel chain, the `lea ecx,[this+off]` member-pointer setup, the
reverse-declaration order, and the `~CObList` calls all fall out of the
member-destructor model.

STEERABLE (modeling choice). This is the steerable counterpart of
[eh-dtor-needs-base-subobject](eh-dtor-needs-base-subobject.md): when the
non-trivial teardown lives in MEMBER subobjects (not a hidden base class you
can't reproduce), giving the members real `~T(){ EngineDtor(); }` destructors
recovers the frame — no manual trylevel stamping, no `__try`. The base-subobject
case stays a wall only when the destructible thing is an UNMODELED polymorphic
base. Distinct from the `new`-with-EH allocator frame (RezAlloc + placement-ctor
+ guard, e.g. 0x116a40/0x116cf0) which this does NOT fix — that frame comes from
the exception cleanup of a partially-constructed heap object, a separate
mechanism. related: inline-base-dtor-folds-into-leaves.md.

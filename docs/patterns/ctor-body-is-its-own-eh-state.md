# An extra /GX entry state between the last member ctor and the first real call is a CTOR BODY

tags: cpp:ctor cpp:eh | asm:mov | topic:codegen-idiom topic:eh
symptoms: a `/GX` function whose local object's construction emits N+1 entry states where the
model only accounts for N destructible sub-objects; the last transition lands AFTER a call that
looks like an ordinary member call on the fresh local
confidence: 9/10
variants: eh-entry-state-counts-destructible-members.md

`eh-entry-state-counts-destructible-members.md` reads the entry-state count as the number of
destructible sub-objects. There is a second producer of a state transition: the **constructor
body itself**. While a ctor body runs, only the base and members are alive, so cl keeps the
partially-constructed state; the "fully constructed" state is stored once the ctor RETURNS, sunk
to just before the next throwing call.

So the tell for `T x;` where `T` has a user-written ctor body is:

```asm
    call  <base ctor>              ; ecx = &x
    mov   [esp+STATE],0            ; base alive
    call  <member ctor>            ; ecx = &x.m_member
    mov   BYTE PTR [esp+STATE],1   ; base + member alive; ctor body may now run
    mov   [esp+0xc],<??_7T>        ; vptr stamp - still part of the ctor
    call  <T's ctor body callee>   ; e.g. Reset()
    ...args for the next call...
    mov   [esp+STATE],2            ; x fully constructed
    call  <first real statement>
```

Modelling it as `T x; x.Reset();` (an empty ctor plus an explicit statement) collapses states 1
and 2 into one: cl emits the vptr stamp BEFORE the state store and never needs a third state.
The state store also moves to the wrong side of the stamp, which is the visible first divergence.

```cpp
// NO - two entry states, vptr stamp before the state store
CFileMem S;
S.Reset();

// YES - three entry states (0/1/2), stamp between states 1 and 2
class CFileMem : public CFileMemBase {
public:
    CFileMem() { Reset(); }
    ...
};
CFileMem S;
```

STEERABLE. `CDDrawSurfaceMgr::RestoreChildren` 0x156530 and `SnapshotChildren` 0x156020 both
carry twelve teardown arms whose state numbers are all shifted by one against retail until the
ctor body is modelled; adding `CFileMem::CFileMem() { Reset(); }` realigned every one of them
(RestoreChildren 55.4 -> 58.6, SnapshotChildren 73.8 -> 77.4, and it is a prerequisite for the
frame/register match). `LoadRecordFile` 0x156ad0, which inlines every ctor in the chain, shows
the full ladder 0/1/2/3 with the `Reset()` body expanded in place - independent confirmation.

related: [eh-entry-state-counts-destructible-members.md](eh-entry-state-counts-destructible-members.md),
[ob1-inline-budget-divergence.md](ob1-inline-budget-divergence.md)

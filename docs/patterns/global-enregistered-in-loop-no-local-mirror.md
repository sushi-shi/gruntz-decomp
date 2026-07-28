# Call-free loop over a global: read/write the GLOBAL, never a local mirror
tags: cpp:global cpp:loop cpp:local | asm:mov | topic:codegen-idiom topic:regalloc
symptoms: one instruction differs, `mov [<glob>],eax` where retail has `mov [<glob>],esi`, the loop-carried copy `mov esi,eax` is already byte-exact, free-list push/pop drain loop
confidence: 9/10
variants: member-store-direct-not-via-temporary.md

In a loop body with **no calls** (MFC `GetNext` and friends inline away), cl5 keeps
a global in a register across iterations by itself: it loads it once before the
loop, forwards reads from the register, and still writes memory each iteration —
writing **from the register that holds the enregistered value**. Mirroring the
global in a hand-written local produces the same instruction sequence except the
store, which cl sources from the *other* register holding the same value.

```cpp
// NO - the local mirror makes cl store from eax (the fresh node), not from the
// register that holds the global's current value
CoordPoolNode* head = g_coordPool.m_freeHead;
do {
    CoordPoolNode* slot = g_coordPool.NodeOf(m_recList.GetNext(pos));
    slot->m_next = head;
    head = slot;
    g_coordPool.m_freeHead = head;      // -> mov [g_freeHead],eax
} while (pos != 0);

// YES - just use the global; cl enregisters it for you
do {
    CoordPoolNode* slot = g_coordPool.NodeOf(m_recList.GetNext(pos));
    slot->m_next = g_coordPool.m_freeHead;
    g_coordPool.m_freeHead = slot;      // -> mov esi,eax / mov [g_freeHead],esi
} while (pos != 0);
```
```asm
mov  esi,ds:0x645544       ; hoisted read of the global, ONCE
...
mov  [eax],esi             ; slot->m_next = <global>
mov  esi,eax               ; the enregistered copy advances
mov  [ds:0x645544],esi     ; ...and the write-back comes FROM it
```
STEERABLE, and it also deletes a `void*`/mirror local. Chained assignment
(`g = head = slot;`) does NOT fix it — cl folds it back to the eax form.
`CTriggerMgr::ClearRecords` 99.75 -> **100.00 EXACT**,
`CTriggerMgr::ClearSelections` 99.84 -> **100.00 EXACT** (both were filed as
prologue/scheduling walls).

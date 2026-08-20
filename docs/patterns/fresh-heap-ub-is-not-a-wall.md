# Byte-exact code can still misbehave: retail reads an uninitialized member
tags: topic:method topic:runtime-defect topic:negative-control | cpp:ctor cpp:array | asm:call
symptoms: byte-exact functions but wrong in-game behaviour; guard reads a member nobody wrote; vector ctor iterator zeroes only part of the element; works on the first level then breaks; ??_H
confidence: 9/10

A runtime symptom is not automatically a reconstruction defect. When every
function on the path is byte-exact and the behaviour still differs, the input
is not the code - and one of the inputs is HEAP HISTORY.

DETECTION SIGNATURE, all three together:
  1. a guard reads a member and treats one value as "free"
     (`if (m_state != HLROW_OFF) return 0;`);
  2. the element constructor writes only PART of the element;
  3. no fresh-start writer covers the rest.

```cpp
struct CSbiHlRow {
    CSbiHlRow() {            // 0xc86d0, EXACT, 17 bytes
        m_lastLo = 0; m_intervalLo = 0;    // +0x8 +0x10
        m_lastHi = 0; m_intervalHi = 0;    // +0xc +0x14
    }                        // m_state (+0) and m_value (+4) are NEVER written
    i32 m_state; i32 m_value; /* ... */
};
```
```asm
; CStatusBarMgr's inlined ctor, retail 0x10a1c6..0x10a1fa
push <??0CSbiHlRow> ; push 0xc ; lea ecx,[esi+0x378] ; push 0x18 ; push ecx
call ??_H@YGXPAXIHP6EX0@Z@Z     ; vector ctor iterator over m_hlGrid[12]
; the seven memsets in the same ctor cover 0x114/0x150/0x18c/0x204/0x308/
; 0x498/0x61c - NONE of them covers 0x378
```

ADJUDICATION ROUTE - enumerate the WRITER SET before blaming the
reconstruction:
  * every writer of the member (source grep is the map of the image; confirm
    no other TU touches it);
  * the fresh-start call chain, with each step's score;
  * which writers are reachable on that chain versus only from save/network
    deserialization (`gruntz sema xref <rva> --tree` answers this).

For `CStatusBarMgr::m_hlGrid` (this+0x378, 12 x 0x18): the writers are the
vector ctor above, `SetHlCell` 0x106b40, `ClearHlCell` 0x1069c0, `EnterHlRow`
0x106820 (all EXACT, all per-cell) and `Deserialize` 0x109520 (bulk) - and
Deserialize is reachable ONLY via `Sync <- CPlay::SyncState <- BroadcastCmd <-
SerialObjectFactory`, the serialization path. The fresh path is
`new CStatusBarMgr(0x630)` -> ctor -> `LoadBattlezItemConfig` 0xfdc00 ->
`Reset` 0x105920 -> ResetSlots/ArmSlot/ResetGroupA/UpdateRezMachineSnooze-
StatusBar/InitTabRects, every one 100% EXACT and none of them writing
m_hlGrid.

CONCLUSION SHAPE. "Retail relies on fresh-heap zeros" is a legitimate,
reportable outcome: the shipped game has uninitialized-memory UB, our build
reproduces it faithfully, and the behaviour then depends on allocation
history - typically working on the first load of a session (OS-zeroed pages)
and failing once the block is recycled. That changes what gets shipped as the
fix (a deliberate, documented divergence-from-retail initializer) and it must
not be mistaken for a matching target: there is no sub-100 writer to correct.

Wall: no - it is not a codegen question at all. Evidence: the resource-belt
slot-landing symptom, 2026-08-20; the whole chain screened EQUAL at operand
level and the asset side was cleared independently
(`gruntz tool rez list --path RESOURCETAB/BELT --index` shows FRAME001..024,
covering both the 0xa..0x12 and 0x13..0x18 ramps).

# Two reads of one member `tagSIZE` never CSE — the BY-VALUE accessor temp does

tags: cpp:struct cpp:local cpp:member | asm:mov asm:sub | topic:codegen-idiom
symptoms: `mov reg,[obj+0x8c]` / `mov reg,[obj+0x90]` appearing TWICE where retail
  loads each once; a frame 8 bytes short of retail with an 8-byte hole between the
  first local and the next; retail dead-stores the size pair into a slot nothing
  reads
confidence: 9/10

`CGruntzMgr::m_modeSize` (and any member POD returned by an inline by-value
accessor) is read at two sites - `.cx` here, `.cy` there. Three source spellings
give three different codegens and only one is retail's:

| spelling | what cl 5.0 does |
|---|---|
| `rc.right = g->m_modeSize.cx; rc.bottom = g->m_modeSize.cy;` | two INDEPENDENT loads, no temp, no frame slot |
| `tagSIZE m; m.cx = g->m_modeSize.cx; m.cy = g->m_modeSize.cy;` (field-by-field) | forwards both fields and **DCEs the local entirely** - frame unchanged |
| `tagSIZE m = g->m_modeSize;` (whole-struct copy) | still DCE'd once both members are read |
| **`tagSIZE m = g->GetModeSize();`** (inline accessor returning BY VALUE) | **materializes the 8-byte return object in the frame, stores both members, and CSEs the two loads** |

Only the fourth keeps the temp: the return object is address-taken by the NRV
hidden pointer, so the stores survive even though nothing reads them again; and
because there is now ONE materialization, the two member loads are CSE'd into one
pair of registers.

```cpp
// WRONG - 2 loads of each member, no frame slot (frame 8 B short of retail)
i32 right  = w->m_modeSize.cx;
i32 bottom = w->m_modeSize.cy;

// RIGHT - one materialization, CSE'd loads, retail's frame
tagSIZE mode = w->GetModeSize();
i32 right  = mode.cx;
i32 bottom = mode.cy;
```
```asm
; retail: one load pair + two dead stores into the temp
mov  ecx,DWORD PTR [eax+0x90]     ; cy
mov  edx,DWORD PTR [eax+0x8c]     ; cx
mov  ebx,ecx
mov  DWORD PTR [esp+0x24],edx     ; temp.cx  <- never read
mov  DWORD PTR [esp+0x3c],ebx     ; temp.cy  <- never read
```

STEERABLE, measured 2026-08-08 (the accessor already existed in
`include/Gruntz/GruntzMgr.h`; the comment above it documents the same fact):

| fn | before | after |
|---|---|---|
| `CPlay::ResetViewport` 0xd8c60 | 95.55 | **100.00 EXACT** |
| `CMulti::WaitForOtherPlayers` 0xbb700 | 94.42 | **100.00 EXACT** |
| `CPlay::DrawCursorSaveUnder` 0xd0b30 | 90.00 | 99.99 |
| `CState::InputVirtual` 0xface0 | 96.18 | 99.54 |

`WaitForOtherPlayers` needed one extra knob on top: with the `RECT` built from the
temp, the two `rc.left = rc.top = 0` stores must be written **LAST** (after the
size fields), which is what schedules the block exactly.

NOT universal - it is a no-op where cl already CSEs (`CStatusBarMgr::LoadBattlezItemConfig`,
`CTriggerMgr::LoadCameraSprite`, `CPlay::LoadScrollSpeedOptions`,
`CPlay::PositionBridgeToggle` all unchanged) and it REGRESSES
`CPlay::ClampViewport2` (91.27 -> 89.72), so measure per function.

SECOND DETECTION SIGNATURE (2026-08-18): the temp can hide inside an OVERSIZED
untyped local. `SaveScreenshot` 0x114ff0 declared `i32 descB[6]` and passed it as
`BltEx`'s dest rect; the trailing two dwords were the accessor temp, not part of
the rect. Typing the parameter `RECT*` is what exposes it - a `RECT` local plus a
hand-written `SIZE` loses 8 bytes of frame (cl DCEs the SIZE), which PROVES the
pair belongs to a compiler-materialized object. Two calls to the accessor in one
statement pair (`rc.right = m->GetModeSize().cx; rc.bottom = m->GetModeSize().cy;`)
share ONE slot and write both halves twice, which is retail's double load here -
so the CSE clause above applies to a temp BOUND to a local, not to the call-in-
expression form. 96.76 -> 100.00 EXACT.

related: struct-copy-dead-member-store-frame.md, frame-size-counts-the-locals.md

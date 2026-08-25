# A member retail does NOT re-read after a call was a source local — and a doubled store names TWO of them
tags: cpp:local cpp:member cpp:call | asm:mov | topic:correctness topic:codegen-idiom
symptoms: an if/else-if chain over a neighbour object where the recompile re-loads
`other->m_field` after an intervening call (`mov eax,[ebx+0x17c]`) and retail instead
moves a register it loaded BEFORE the call (`mov eax,edi`); and, upstream, retail stores
the SAME loaded pair into TWO pairs of stack slots in one basic block
confidence: 9/10

cl 5.0 has no cross-call alias analysis: a member read written in source after a call is
ALWAYS re-emitted as a load. So when retail carries a register across the call instead,
retail's source did not re-read — it used a local. That is a CORRECTNESS reading, not a
scheduling one: it decides whether the value handed to the next callee is the pre-call or
the post-call one.

```cpp
// what the codegen says, at CGrunt::AdvanceMotion 0x5f79d..0x5f7f6:
i32 lastX = other->m_lastTilePx.m_x;      // 0x5f79d, before either probe
i32 lastY = other->m_lastTilePx.m_y;
i32 targetX = lastX;                      // the doubled store: X to two slots,
i32 targetY = lastY;                      // Y to two slots, one block
if (RectContains(x, y) != 0) {
    targetX = otherPxX; targetY = otherPxY;          // 0x5f7ca reloads the UNMASKED homes
} else if (RectContains(lastX, lastY) != 0) {        // 0x5f7d4 reloads the SAVED slots
    other->SnapToLastTile(0);
    targetX = lastX; targetY = lastY;                // 0x5f7f2 `mov eax,edi / mov ecx,ebp`
} else {
    targetX = m_arrivalTargetPx.m_x; targetY = m_arrivalTargetPx.m_y;
}
```

Two independent readings, both mechanical:

- **no re-load after the call ⇒ a local held the value.** Spelling the probe
  `RectContains(other->m_lastTilePx.m_x, other->m_lastTilePx.m_y)` and re-reading the
  member after `SnapToLastTile` costs two loads retail does not have.
- **one loaded value stored to TWO slot pairs in one block ⇒ TWO source locals**, not a
  live-range split. Collapsing them to one (`targetX = other->m_lastTilePx.m_x` and
  probing with `targetX`) is semantically identical and re-colours the whole arm set
  DOWNWARD; declaring the second pair explicitly beats both spellings.

Measured on `CGrunt::AdvanceMotion` @0x5f310, both trigger arms: one pair 91.09 -> 90.08,
two pairs **91.09 -> 91.18**. The same file's `CGrunt::ArrivalRecycle` @0x59230 is the
pointer form of the same rule — retail parks the record POINTER in ebx across
`ActNameConstructGrownSlots` (`mov ebx,eax`) and only then loads `[ebx]`, so
`char** rec0 = ...GetNameRecordRaw(key); ...; strcmp(*rec0, s_codeH)` is the source and
`char* nm0 = *...` reads a buffer pointer the reconstruction can replace (94.80 -> 94.95).

**The mirror reading is just as mechanical, and it decides array-element statements.**
When retail RE-LOADS `arr[i]` after a call, the source did not carry a local across it -
it spelled the array element again; when retail loads it ONCE for a run of consecutive
stores, the source did carry a local. Both directions cost real points on the same file:

```cpp
// retail reloads slot[i] after SetImageFrameByName (`mov eax,[ebx]` where the cursor
// already advanced), so the last statement is the ARRAY ELEMENT, not the local `a`:
a->SetImageFrameByName("GAME_STATUSBAR_TABZ_GAMETAB_WARPSTONE", i + 2);
slot[i]->m_stateFlags |= SPRITE_STATE_HIDDEN;      // 86.23 -> 91.42

// retail loads [esi] ONCE and stores three fields off it, so these three are a LOCAL:
{ CWwdSpriteObject* o = m_sprintSprites[i];
  o->m_drawActive = 1; o->m_drawFillCmd = SHADE_PAL_16; o->m_drawFillArg = h; }
                                                    // 94.31 -> 98.40
```

`CBootyState::BuildWarpStoneGlitterAnimation` @0x19540 and
`CBootyState::BuildGruntSprintAnimation` @0x19920. Read WHICH SIDE reloads before
deciding; the two spellings differ by one `mov` per statement and by nothing else.

The scalar form also appears in `CStaticHazard::CStaticHazard` @0xfb7a0. Retail loads
`CAniElement::m_durationMs` into `edi` before `CButeMgr::GetIntDef` and adds that saved
value after the call; the reconstruction formerly spelled `entry->m_durationMs` in the
addition and therefore reloaded it afterward. Introducing
`i32 durationMs = entry->m_durationMs` restored retail's pre-call lifetime, made both
sides 199 instructions, and raised the function
from 88.56% to 91.68%. The remaining four-byte frame difference belongs to the typed
map-output boundary, not this member lifetime.

The same pointer reading recovered `CRezImage::SaveBmp` @0x176b30. Retail loads
`m_pixels` once before its null guard, keeps the pointer in `edi` across construction,
open, and two header writes, then adds each saved row offset to that register. Spelling
`m_pixels` again in the scanline loop forced a member reload inside every iteration.
Capturing `u8* pixels = m_pixels`, testing `pixels`, and writing through `pixels`
removed the reload, made both sides 141 instructions with 7 calls, 12 branches, and 9
ordered relocations, and raised the function from 95.87% to 98.99%. The remaining byte
is an equivalent whole-function EBX/EBP role swap, not missing source structure.

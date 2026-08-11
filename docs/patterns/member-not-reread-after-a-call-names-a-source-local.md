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

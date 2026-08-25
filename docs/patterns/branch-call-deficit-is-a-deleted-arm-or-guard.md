# A branch AND call DEFICIT vs retail is source somebody deleted as redundant
tags: cpp:branch cpp:if cpp:goto cpp:switch cpp:new | asm:jcc asm:cmp asm:call | topic:codegen-idiom
symptoms: `walls diagnose` says INLINE/CALL-SET or CFG with the base SHORTER on BOTH
counters (`base: N insns, C calls, B branches` vs `target: N+, C+, B+`), the call
multiset difference names ordinary callees, and the function's `hist_pct` is well
above `best_pct`
confidence: 9/10

cl 5.0 does not invent calls or branches, so when the base is short on both the
missing code is in the source, not in the allocator. Two recurring shapes, both
of which a later reader deletes because they look redundant.

**A switch arm shared with a `goto` into a sibling arm.** Retail tail-duplicates;
a cross-arm `goto` merges. Count the retail arm's own calls before sharing.

```cpp
// WRONG - one label serving two arms
case DEATH_QUICKFALL:
    tag = 0x357;
    m_poseDeath = Lookup(s_DEATHZ_QUICKFALL);
    goto fallSnap;                      // -3 calls, -5 branches
// RIGHT - the arm carries its own copy, as retail emits it
case DEATH_QUICKFALL:
    m_object->m_screenX = (m_object->m_screenX & ~TILE_MASK_PX) + TILE_HALF_PX;
    m_object->m_screenY = (m_object->m_screenY & ~TILE_MASK_PX) + TILE_HALF_PX;
    m_poseDeath = Lookup(s_DEATHZ_QUICKFALL);
    m_value = m_wwdObject->m_animationCursor.m_animation;
    m_wwdObject->SetAnimation(m_poseDeath, 0);
    m_wwdObject->SetImageFrameByName(s_DEATHZ_FALL, DEATH_FRAME());
    DEATH_CUE(0x357);
    goto finalize;
```

**A `p == NULL` guard on a member that a `new` just filled.** It reads as dead
code, but retail emits it, and several such guards cross-jump onto one shared
`xor eax,eax; jmp <epilogue>`. It is a DIFFERENT branch from `delete p`'s own
implicit null test, which can sit right after it on the same register.

```asm
0c82b2: mov  esi,[ebx+0x2dc]     ; m_statusBar
0c82b8: cmp  esi,ebp             ; ebp == 0     <- the SOURCE guard
0c82ba: je   0xc8485             ; -> shared `xor eax,eax; jmp`
0c82c0: cmp  esi,ebp             ; <- `delete m_statusBar`'s own test
0c82c6: je   0xc830a
```

STEERABLE. `CGrunt::LoadGruntDeathAnimations` 0x60150 86.28 -> 90.70 (calls
79 -> 82 = retail's 82, relocs 150 = 150) by un-sharing the QUICKFALL arm;
`CPlay::LoadGameAssetNamespaces` 0xc7ec0 83.31 -> 84.46 by restoring the two
`new`-result guards. Screen: `gruntz walls diagnose <rva>` and compare the
branch/call columns before reading a single instruction.

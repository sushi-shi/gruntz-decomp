# A member chain re-loaded AFTER a call proves the local was captured below it
tags: cpp:call cpp:local | asm:mov asm:call | topic:codegen-idiom
symptoms: diagnose says REGALLOC/SCHEDULING; retail loads the same `[this+A]` / `[obj+B]` chain twice with a `call` between, while the base loads it once before the call and keeps the pointer in a callee-saved register or spilled slot
confidence: 9/10
variants: subexpression-position-names-its-statement.md

cl 5.0 never CSEs a member chain across an opaque call. So when retail's
stream shows `mov eax,[this+0x38]; mov ecx,[eax+0x2c]; call ...;
mov edx,[this+0x38]; mov ebx,[edx+0x2c]` - the SAME chain on both sides of
the call - the source read it twice, i.e. the local holding the pointer was
declared AFTER the call, and the call's receiver was spelled as the full
chain inline. A single pre-call capture (`CState* sp = m_mgr->m_state;
sp->Update();`) loads once and re-uses, which is what the base emits when
the reconstruction hoists the local above the call.

```cpp
// ours (one load, kept live across the call):
CState* sp = m_manager->m_curState;
i32 isPlay = (sp->Update() == GAMESTATE_MULTI);

// retail (chain re-derived after the call):
i32 isPlay = (m_manager->m_curState->Update() == GAMESTATE_MULTI);
CState* sp = m_manager->m_curState;
```

Detection is mechanical: two loads of the same displacement pair bracketing a
call = capture below; one load = capture above. The same barrier logic holds
for byte/word STORES in loops (u8 writes alias members, forcing per-iteration
re-loads when the loop body reads the member directly - see FlipVertical in
imagepool and dword-flag-mask-narrows-to-byte-rmw.md).

## Measured

- CGruntzCmdMgr::ScanTargets 0x23a10 93.84 -> 100.00 EXACT from moving the
  `sp` capture below the `Update()` call.
- CRezImage::FlipVertical 0x176840: all three row-copy loops re-load
  m_pixels (`[ebx+0x42c]`) inside the loop body - only member-direct
  indexing (`m_pixels[i*wid+x]`), not a cached `u8* top` local, reproduces
  the reload (store-aliasing arm of the same barrier).

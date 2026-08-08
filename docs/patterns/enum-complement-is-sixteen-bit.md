# `~SPRITE_STATE_HIDDEN` masks 16 bits, not 32 — MSVC 5.0 promotes a small enum to `unsigned short`
tags: cpp:enum cpp:flags cpp:expr | asm:and | topic:codegen-idiom topic:correctness
symptoms: retail has `and dword ptr [reg+ofs],0xfffffffe` (`83 /4 imm8`, 4 bytes) where the base has `and dword ptr [reg+ofs],0xfffe` (`81 /4 imm32`, 7 bytes); the clear-a-flag line reads `x &= ~SOME_FLAG;` and `SOME_FLAG` is an enumerator, not a literal
confidence: 10/10

An enumerator's type is the enum's type, and MSVC 5.0 picks the enum's underlying
type from the enumerator RANGE. For a flag bag whose largest value is small, that
type is 16-bit, so the integral promotion of `~ENUMERATOR` yields a 16-bit
complement, which then zero-extends:

```cpp
GZ_ENUM_FLAGS_BEGIN(SpriteStateFlags, i32)    // expands to `enum { ... };` + `typedef i32`
    SPRITE_STATE_NONE     = 0,
    SPRITE_STATE_HIDDEN   = 0x1,
    SPRITE_STATE_MIRROR_X = 0x2,
    SPRITE_STATE_FLASHING = 0x8
GZ_ENUM_FLAGS_END(SpriteStateFlags, i32)

m_stateFlags &= ~SPRITE_STATE_HIDDEN;              // and dword,0x0000fffe   << WRONG
m_stateFlags &= ~static_cast<i32>(SPRITE_STATE_HIDDEN);  // and dword,-2     << retail
```

**This is a CORRECTNESS bug before it is a matching bug.** `0x0000fffe` clears
bits 16..31 of the word as well, so every flag above bit 15 is destroyed on every
clear. It is invisible in a `--diff` read-through because both lines say `and
dword ptr [...]` and the eye slides past the immediate — exactly the
"`--diff` MASKS large immediates" trap from `docs/gotchas.md`, except here the
immediate is small enough to print and still gets skimmed.

`|=` is unaffected: `x |= ENUMERATOR` needs no complement, so the OR sites are all
correct and only the `&= ~` sites are wrong. That asymmetry is why the bug survives
a reading of the surrounding code.

## Finding them

    rg -n '&= ~[A-Z_]+;' src/            # the whole family
    rg -n 'and .*,0x[0-9a-f]{0,4}fffe'   # after a build, against the base objs

Measured 2026-08-08 on `CPlay::LoadCursorSprites` 0xd0120, `CPlay::HandleDragMove`
0xd0db0 and `CPlay::ResetPlayState` 0xd60b0: retail is `83 60 40 fe` at all three
and the base was `81 60 40 fe ff 00 00`. Tree-wide the same expression appears at
about 45 more sites across 14 files (`GameMode.cpp`, `BootyStateActivate.cpp`,
`InGameIcon.cpp`, `Wormhole.cpp`, `BootyWalkAnim.cpp`, ...) - each is three bytes
of encoding AND a live flag-word corruption.

The named-cast spelling `~static_cast<i32>(X)` is the one already used in
`src/DinMgr2/DinMgr2.cpp` for the same reason, so it is not a new idiom. A
domain-wide fix would have to change what `GZ_ENUM_FLAGS_BEGIN` emits, which
touches every TU; see `docs/enum-modeling-plan.md`.

related: [enum-domains.md](enum-domains.md)

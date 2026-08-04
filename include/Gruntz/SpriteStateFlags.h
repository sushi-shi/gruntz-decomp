#ifndef GRUNTZ_GRUNTZ_SPRITESTATEFLAGS_H
#define GRUNTZ_GRUNTZ_SPRITESTATEFLAGS_H

#include <Enums.h>

// Bits in CResolveNode::m_stateFlags, the per-sprite state word.
//
// Named behaviourally - what the code does around each bit - because no bit
// table has been recovered. Each is corroborated across unrelated files:
//
//   0x1  HIDDEN.  Cleared immediately before a sprite is positioned and named,
//        set when it should stop showing. CBootyWalkAnim clears it to reveal
//        each walking sprite and sets it when the walk ends; CGruntToySprite
//        clears it when the toy should appear; CWormhole clears it when placing;
//        CPlay sets it to suppress the scroll indicator and clears it only on
//        level 0. Nothing in the tree uses it any other way.
//
//   0x2  MIRROR_X.  CWwdFactoryObject's move arm is the same two lines twice,
//        differing only in sign: with the bit, `m_screenX = x - dx`; without it,
//        `x + dx`. A flipped sprite walks the other way.
//
//   0x8  FLASHING.  Set in the same breath as m_flashInterval and
//        m_flashCountdown, guarded by `(m_stateFlags & 8) == 0` so the effect
//        arms once, and cleared where it ends.
//
// NOT to be confused with CWwdGameObjectA::m_flags, a different and much wider
// word on the same objects (0x10000, 0x20000, 0x2000, ...), whose bits are still
// unrecovered.
GZ_ENUM_FLAGS_BEGIN(SpriteStateFlags, i32)
    SPRITE_STATE_HIDDEN = 0x1,
    SPRITE_STATE_MIRROR_X = 0x2,
    SPRITE_STATE_FLASHING = 0x8
GZ_ENUM_FLAGS_END(SpriteStateFlags, i32)

#endif // GRUNTZ_GRUNTZ_SPRITESTATEFLAGS_H

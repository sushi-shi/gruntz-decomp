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
//        `x + dx`. CImage renders the bit with DDBLTFX_MIRRORLEFTRIGHT.
//
//   0x4  MIRROR_Y. CImage renders the bit with DDBLTFX_MIRRORUPDOWN and uses
//        the corresponding Y-origin/plot-offset geometry.
//
//   0x8  FLASHING.  Set in the same breath as m_flashInterval and
//        m_flashCountdown, guarded by the flag being clear so the effect arms
//        once, and cleared where it ends.
//
//   0x10000000 FLASH_VISIBLE. CImage toggles this at every flashing interval
//        and suppresses drawing during the clear phase.
//
// NOT to be confused with CWwdGameObjectA::m_flags, the separate
// WwdGameObjectFlags word on the same objects.
GZ_ENUM_FLAGS_BEGIN(SpriteStateFlags, i32)
    SPRITE_STATE_NONE = 0,
    SPRITE_STATE_HIDDEN = 0x1,
    SPRITE_STATE_MIRROR_X = 0x2,
    SPRITE_STATE_MIRROR_Y = 0x4,
    SPRITE_STATE_FLASHING = 0x8,
    SPRITE_STATE_FLASH_VISIBLE = 0x10000000
GZ_ENUM_FLAGS_END(SpriteStateFlags, i32)

#endif // GRUNTZ_GRUNTZ_SPRITESTATEFLAGS_H

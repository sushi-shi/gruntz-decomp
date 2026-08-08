#ifndef GRUNTZ_WWD_WWDANIDRAWVALUE_H
#define GRUNTZ_WWD_WWDANIDRAWVALUE_H

#include <Enums.h>

// An ANI record's `draw_value` - field 6 of the ten i16s, the per-frame event
// code CAniAdvanceCursor::Advance returns to its caller through m_pendingDraw /
// m_curDraw. Advance itself is a generic integer-valued interface: it never
// interprets the value, so this domain is the union of what the on-disk records
// carry and what the callers test for. Corpus: 13 480 records in 1038 ANI
// resources, see docs/formats/ani-v1.md.
GZ_ENUM_BEGIN(WwdAniDrawValue)
// Not an on-disk value: Advance returns it when no animation is bound.
    WWDDRAW_NO_ANIMATION = -1,
    // No event. Advance also zeroes m_curDraw after a consuming read. 12 955.
    WWDDRAW_NONE = 0,
    // Animation complete - 222 of the 280 occurrences sit on the last record of
    // their animation. Tested by CGruntDecay, CRockBreakEffect, CWarlord and the
    // Grunt entrance/arrival states.
    WWDDRAW_ANIMATION_COMPLETE = 1,
    // The effect / impact frame: the attack lands, the hazard fires, the dirt
    // particle spawns. 222 records; four consumers, CGrunt::StepAttackFire
    // @0x61cb0, CStaticHazard @0xfc1a0, CDroppedObjectShadow::Advance @0xc7ab0
    // and CTriggerMgr::LoadTileArrivalFx @0x75e90.
    WWDDRAW_EFFECT_FRAME = 2,
    // The tool effect applies NOW - LoadTileArrivalFx returns early from every
    // arm unless the cue is this, then it uncovers the powerup or breaks the
    // brick. 18 records, each at or near the end of a Grunt tool-use animation.
    WWDDRAW_TOOL_APPLIES = 0x63
// Values 3 (2 records, GRUNTZ\ANIZ\GAUNTLETZGRUNT\ITEM) and 100 (3 records,
// WELDERGRUNT\PROJECTILE{2,3,4}) are on disk with NO consumer located in
// src/. Our reconstruction is incomplete there, so that is "not found", not
// "absent" - leave the gap rather than invent a name for either.
GZ_ENUM_END(WwdAniDrawValue)

#endif // GRUNTZ_WWD_WWDANIDRAWVALUE_H

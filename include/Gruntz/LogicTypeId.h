// LogicTypeId.h - the per-class "logic-type id" tag space returned by every
// CUserLogic-derived leaf's GetTypeTag() virtual (the 6-byte `mov eax,<id>; ret`
// accessor). Each concrete tile-logic / game-object leaf returns a UNIQUE magic
// constant identifying its logic kind; the engine's logic dispatch keys off it.
//
// Named here so every GetTypeTag body returns the named enumerator, not a bare
// literal (matching-NEUTRAL: a named enumerator lowers to the exact same immediate,
// so each `mov eax,<id>; ret` stays byte-identical). Mirrors LoadableClassId in
// CLoadable.h.
//
// A few leaves' owning class names are not yet pinned (their plain dtors are
// COMDAT-folded, so RTTI can't attribute them) - those are named by their tag
// (LOGIC_TAG_<hex>), matching the placeholder CTileLogicTag<hex> classes, rather
// than inventing a fake class name. The id itself is the recovered fact.
#ifndef GRUNTZ_LOGICTYPEID_H
#define GRUNTZ_LOGICTYPEID_H

enum LogicTypeId {
    LOGIC_DONOTHING = 0x3ec,   // CDoNothing::GetTypeTag              @0x0f6b0
    LOGIC_TAG_3EF = 0x3ef,     // CTileLogicTag3ef::GetTypeTag        @0x0fa40 (owner unpinned)
    LOGIC_BEHINDCANDY = 0x3f0, // CBehindCandy::GetTypeTag            @0x0fb70
    LOGIC_EYECANDY = 0x3f1,    // CEyeCandy::GetTypeTag               @0x0fca0
    LOGIC_EXITTRIGGER = 0x3f7, // CExitTrigger::GetTypeTag            @0x10870
    LOGIC_TILETRIGGERTRANSITION = 0x405, // CTileTriggerTransition::GetTypeTag  @0x11730
    LOGIC_GRUNTSTAMINASPRITE = 0x410,    // CGruntStaminaSprite::GetTypeTag     @0x12020
    LOGIC_GRUNTTOYTIMESPRITE = 0x411,    // CGruntToyTimeSprite::GetTypeTag     @0x120e0
    LOGIC_GRUNTWINGZTIMESPRITE = 0x417,  // CGruntWingzTimeSprite::GetTypeTag   @0x121a0
    LOGIC_PARTICLEZ = 0x41c,             // CParticlez::GetTypeTag              @0x12cd0
    LOGIC_TAG_41D = 0x41d,      // CTileLogicTag41d::GetTypeTag        @0x12ff0 (owner unpinned)
    LOGIC_ACTIONAREA = 0x423,   // CActionArea (GetTypeTag not yet reconstructed)
    LOGIC_PATHHAZARD = 0x425,   // CPathHazard::GetTypeTag             @0x132f0
    LOGIC_VOICETRIGGER = 0x426, // CVoiceTrigger::GetTypeTag           @0x133b0
    LOGIC_TAG_428 = 0x428,      // CTileLogicTag428::GetTypeTag        @0x11bf0 (owner unpinned)
    LOGIC_TAG_429 = 0x429,      // CTileLogicTag429::GetTypeTag        @0x10f00 (owner unpinned)
};

#endif // GRUNTZ_LOGICTYPEID_H

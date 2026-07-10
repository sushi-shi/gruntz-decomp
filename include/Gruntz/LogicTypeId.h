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
    LOGIC_ANICYCLE = 0x3ea,       // CAniCycle::GetTypeTag               @0x0f450
    LOGIC_DONOTHING = 0x3ec,      // CDoNothing::GetTypeTag              @0x0f6b0
    LOGIC_FRONTCANDY = 0x3ef,     // CFrontCandy::GetTypeTag             @0x0fa40 (vtable 0x1e84ec, slot 2)
    LOGIC_BEHINDCANDY = 0x3f0,    // CBehindCandy::GetTypeTag            @0x0fb70
    LOGIC_EYECANDY = 0x3f1,       // CEyeCandy::GetTypeTag               @0x0fca0
    LOGIC_BEHINDCANDYANI = 0x3f3, // CBehindCandyAni::GetTypeTag         @0x10030
    LOGIC_EYECANDYANI = 0x3f4,    // CEyeCandyAni::GetTypeTag            @0x0ff00
    LOGIC_EXITTRIGGER = 0x3f7,    // CExitTrigger::GetTypeTag            @0x10870
    LOGIC_TELEPORTER = 0x3fc,     // CTeleporter::GetTypeTag             @0x10d80
    LOGIC_TILETRIGGERTRANSITION = 0x405, // CTileTriggerTransition::GetTypeTag  @0x11730
    LOGIC_BRICKZ = 0x409,                // CBrickz::GetTypeTag                 @0x11300
    LOGIC_OBJECTDROPPER = 0x40f,         // CObjectDropper::GetTypeTag          @0x124a0
    LOGIC_GRUNTSTAMINASPRITE = 0x410,    // CGruntStaminaSprite::GetTypeTag     @0x12020
    LOGIC_GRUNTTOYTIMESPRITE = 0x411,    // CGruntToyTimeSprite::GetTypeTag     @0x120e0
    LOGIC_STATICHAZARD = 0x416,          // CStaticHazard::GetTypeTag           @0x12ae0
    LOGIC_GRUNTWINGZTIMESPRITE = 0x417,  // CGruntWingzTimeSprite::GetTypeTag   @0x121a0
    LOGIC_TOOBSPIKEZ = 0x418,            // CToobSpikez::GetTypeTag             @0x12ba0
    LOGIC_PARTICLEZ = 0x41c,             // CParticlez::GetTypeTag              @0x12cd0
    LOGIC_SPOTLIGHT = 0x41d,    // CSpotLight::GetTypeTag              @0x12ff0 (vtable 0x1e75bc, slot 2)
    LOGIC_WAYPOINT = 0x420,     // CWayPoint::GetTypeTag               @0x10220
    LOGIC_ACTIONAREA = 0x423,   // CActionArea::GetTypeTag             @0x07f80
    LOGIC_PATHHAZARD = 0x425,   // CPathHazard::GetTypeTag             @0x132f0
    LOGIC_VOICETRIGGER = 0x426, // CVoiceTrigger::GetTypeTag           @0x133b0
    LOGIC_FORTRESSFLAG = 0x427, // CFortressFlag::GetTypeTag           @0x10e40
    LOGIC_TOYPEEK = 0x428,      // CToyPeek::GetTypeTag                @0x11bf0 (vtable 0x1e7204, slot 2)
    LOGIC_WARPSTONEPAD = 0x429, // CWarpStonePad::GetTypeTag           @0x10f00 (vtable 0x1e71ac, slot 2)
    LOGIC_GUARDPOINT = 0x42a,   // CGuardPoint::GetTypeTag             @0x10350
};

#endif // GRUNTZ_LOGICTYPEID_H

// BoundaryLeafLogic.cpp - the two byte-exact CUserLogic-family leaf dtors that
// CANNOT be homed onto their real class (structural residue, not fold-able views).
// The other 13 leaf dtors / Serialize overrides that lived here have been re-homed
// onto their real class TUs (AniCycle/SingleFrameMessage/FrontCandy/FrontCandyAni/
// EyeCandyAni/SingleAnimation/GruntStartingPoint/ToyPeek/StatusBarSpriteActs/
// SpotLightCtor + CMenuSparkle/CWarpStonePad into UserLogic.cpp).
#include <Gruntz/BoundaryLeafLogicViews.h> // the two placeholder CUserLogic leaves

// @early-stop
// @flag: one-source/N-COMDAT residue (no MSVC5 ICF). 0x8860 IS a ~CUserLogic COMDAT
// copy (its sdd 0x8a10 sits in ??_7CUserLogic slot 0); 0x117f0 is a SECOND byte-
// identical ~CUserLogic copy (sdd 0x117c0 in ??_7CTileTriggerTransition slot 0) that
// already wears the real ??1CUserLogic@@UAE@XZ symbol (TileTriggerTransition.cpp).
// ~CUserLogic is inline in the header (so leaf dtors fold the teardown), so both out-
// of-line copies are un-folded COMDATs; only ONE can wear ??1CUserLogic. This copy
// therefore REQUIRES a distinct synthetic symbol - folding it onto CUserLogic would
// collide with / orphan 0x117f0's match. NOT a foldable view. 100% byte-exact.
RVA(0x00008860, 0x44)
L_8860::~L_8860() {}

// @early-stop
// @flag: MSVC5 /O2 dead-vptr-store elimination wall (byte-proven). 0x13400 IS
// CUFO::~CUFO, but retail's /O2 collapsed the CUFO:CPathHazard:CUserLogic dtor chain
// to a FLAT CUserLogic teardown - the disasm stamps ONLY 0x5e705c (CUserLogic vtable)
// then 0x5e70b4 (CUserBase vtable), never CUFO's (0x5e72b4) or CPathHazard's vptr (the
// intermediate stamps are dead stores, eliminated). The real CUFO:CPathHazard model
// (Ufo.h) emits those intermediate stamps -> byte-proven crater to 4.7%. This flat
// `: CUserLogic` model is the SAME faithful shape as the sibling CPathHazard leaf
// (UserLogic.cpp models CPathHazard `: CUserLogic` directly) - the TILE_LOGIC_TAIL
// precedent (the binary rejects the RTTI-proper hierarchy for these fold-flat dtors).
// 100% byte-exact. A future pass could rename this to CUFO by remodeling Ufo.h as a
// flat `: CUserLogic` leaf (touches GameObjectCtors.cpp's CUFO ctor - deferred).
RVA(0x00013400, 0x44)
L_13400::~L_13400() {}

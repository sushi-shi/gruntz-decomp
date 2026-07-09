// BoundaryLeafLogic.cpp - tile-logic leaf destructors + Serialize overrides
// recovered from the engine_boundary backlog (C:\Proj\Gruntz).
//
// Each function sits at a class boundary in GRUNTZ.EXE; RTTI cannot attribute the
// COMDAT-folded leaf methods, so the owning class names here are placeholders
// (L_<rva> / S_<rva>), declared in <Gruntz/BoundaryLeafLogicViews.h>. The recovered
// FACTS are: each is a CUserLogic leaf (the shared game-object hierarchy from
// <Gruntz/UserLogic.h>) whose
//   * destructor folds the bare CUserLogic teardown - store the CUserLogic vptr
//     (0x5e705c), inline-destruct the +0x18 link (the embedded ~EngStr call
//     0x16d2a0), store the CUserBase vptr (0x5e70b4); the destructible link forces
//     the /GX EH frame (the established leaf-dtor archetype: ~CSimpleAnimation
//     @0xf9d0, ~CLevelTime @0x11a50, ~CCursorSnapSprite @0x11920 - all 100%).
//   * Serialize override chains the shared CUserLogic::SerializeChain (0x16e7f0) on
//     `this`, then (only on success) the +0x34 sub-object's Chain (0x8c00 via the
//     0x1aff thunk), normalizing the result to a bool (the retail neg/sbb/neg
//     idiom) - the SAME archetype as CCursorSnapSprite::Serialize @0x11880 (100%).
//
// Only OFFSETS + the inheritance chain are load-bearing; the empty dtor body and
// the two-chain Serialize body are enough for cl to reproduce the retail bytes.
// Real leaf-class identities recovered from the classes' vtable slot 0 (the
// scalar-deleting-dtor -> base-dtor chain, gruntz sema xref --tree). Each dtor is
// the canonical vtable-referenced ??1 COMDAT; UserLogic.cpp / the class ctor TU
// hold the RVA-less vtable-emission copies.
#include <Gruntz/AniCycle.h>
#include <Gruntz/EyeCandyAni.h>
#include <Gruntz/FrontCandy.h>
#include <Gruntz/WarpStonePad.h>
#include <Gruntz/SingleFrameMessage.h>
#include <Gruntz/GruntStartingPoint.h>
#include <Gruntz/MenuSparkle.h>
#include <Gruntz/SingleAnimation.h>
#include <Gruntz/SpotLight.h>
#include <Gruntz/StatusBarSprite.h>
#include <Gruntz/ToyPeek.h>
#include <Gruntz/BoundaryLeafLogicViews.h> // placeholder CUserLogic leaf classes (pulls UserLogic.h)
#include <Gruntz/SerialObjRef.h>           // the shared serialized-object-reference (Chain @0x8c00)

// ---------------------------------------------------------------------------
// Leaf destructors (the /GX leaf-dtor archetype). Each is byte-identical to
// ~CSimpleAnimation; the only per-class difference is the EH funcinfo table the
// compiler emits (reloc-masked).
// ---------------------------------------------------------------------------
// @early-stop
// L_8860 (0x8860) IS CUserLogic::~CUserLogic - its scalar-deleting dtor 0x8a10 sits
// in ??_7CUserLogic slot 0 (gruntz sema xref --tree). But 0x117f0 is a SECOND
// byte-identical ~CUserLogic COMDAT (emitted in CTileTriggerTransition's TU, its
// sdd 0x117c0 in ??_7CTileTriggerTransition slot 0) and already carries the real
// ??1CUserLogic@@UAE@XZ symbol (TileTriggerTransition.cpp @rva-symbol). MSVC5 has
// no ICF, so both un-folded copies survive; only ONE can wear ??1CUserLogic. This
// second copy therefore REQUIRES its own synthetic symbol (??1L_8860) to hang RVA
// 0x8860 - folding it onto CUserLogic would collide/orphan 0x117f0's match. Genuine
// one-source/N-COMDAT residue (the mechanism the brief flags), not a fake-view miss.
RVA(0x00008860, 0x44)
L_8860::~L_8860() {}

// CAniCycle::~CAniCycle @0xf510 - the canonical vtable-referenced dtor (??_7CAniCycle
// slot 0 -> sdd 0xf4e0 -> this). UserLogic.cpp holds the RVA-less vtable-emission copy.
RVA(0x0000f510, 0x44)
CAniCycle::~CAniCycle() {}

// CSingleFrameMessage::~CSingleFrameMessage @0xf640 (??_7CSingleFrameMessage slot 0).
RVA(0x0000f640, 0x44)
CSingleFrameMessage::~CSingleFrameMessage() {}

// CFrontCandy::~CFrontCandy @0xfb00 (??_7CFrontCandy slot 0). Extracted to
// <Gruntz/FrontCandy.h>; UserLogic.cpp holds the RVA-less vtable-emission copy.
RVA(0x0000fb00, 0x44)
CFrontCandy::~CFrontCandy() {}

// S_fdf0 (0x0000fdf0) VERIFIED = CFrontCandyAni::Serialize (slot-1). Read the retail
// vtable directly: ??_7CFrontCandyAni (0x1e83e4) slot1 = thunk 0x19a6 -> 0xfdf0. The
// existing FrontCandyAni.cpp "CFrontCandyAni::Serialize @0xfa60" is a MIS-ATTRIBUTION:
// 0xfa60's data-ref is ??_7CFrontCandy (0x1e84ec) slot1 -> it is CFrontCandy's slot-1,
// not CFrontCandyAni's. FOLD BLOCKED: CFrontCandyAni is double-defined (UserLogic.cpp
// local "genuine ctor" facet + FrontCandyAni.h "acts" facet, documented NOT-foldable);
// folding needs those merged to one header def first (+ correcting the 0xfa60 owner to
// CFrontCandy) - a 5-file reconcile on a 100%-matched fn, deferred to avoid a match-cost.
RVA(0x0000fdf0, 0x47)
i32 S_fdf0::Serialize(i32 ar, i32 tag, i32 c, i32 d) {
    if (!SerializeChain(ar, tag, c, d)) {
        return 0;
    }
    return ((CSerialObjRef*)((char*)this + 0x34))
               ->Chain((CSerialArchive*)ar, tag, c, (CSerialObj*)d)
           != 0;
}

// L_fe90 (0x0000fe90) VERIFIED = CFrontCandyAni::~CFrontCandyAni (??_7CFrontCandyAni
// slot 0 -> sdd 0xfe60 -> this). FOLD BLOCKED by the same CFrontCandyAni double-def as
// S_fdf0 above (the UserLogic.cpp def has the dtor but is .cpp-local; FrontCandyAni.h
// lacks it and has no VTBL) - reconcile the two defs to one header first.
RVA(0x0000fe90, 0x44)
L_fe90::~L_fe90() {}

// CEyeCandyAni::~CEyeCandyAni @0xffc0 (??_7CEyeCandyAni slot 0).
RVA(0x0000ffc0, 0x44)
CEyeCandyAni::~CEyeCandyAni() {}

// CMenuSparkle::~CMenuSparkle @0x101b0 (??_7CMenuSparkle slot 0 -> sdd 0x10180). The
// canonical CMenuSparkle extracted to <Gruntz/MenuSparkle.h>; UserLogic.cpp holds the
// RVA-less vtable-emission copy. (The Grunt.h-world MenuSparkleSerial.h view is a
// separate TU-local model; the two never coexist in a TU, so no redefinition.)
RVA(0x000101b0, 0x44)
CMenuSparkle::~CMenuSparkle() {}

// CSingleAnimation::Serialize @0x104a0 - the vtable slot-1 body (thunk 0x3602 in
// ??_7CSingleAnimation slot 1). The two-chain serialize (base chain + the +0x34
// CSerialObjRef), same archetype as CFrontCandyAni::Serialize.
RVA(0x000104a0, 0x47)
i32 CSingleAnimation::Serialize(i32 ar, i32 tag, i32 c, i32 d) {
    if (!SerializeChain(ar, tag, c, d)) {
        return 0;
    }
    return ((CSerialObjRef*)&m_34)->Chain((CSerialArchive*)ar, tag, c, (CSerialObj*)d) != 0;
}

// CGruntStartingPoint::Serialize @0x105d0 - slot-1 body (thunk 0x4291 in
// ??_7CGruntStartingPoint slot 1).
RVA(0x000105d0, 0x47)
i32 CGruntStartingPoint::Serialize(i32 ar, i32 tag, i32 c, i32 d) {
    if (!SerializeChain(ar, tag, c, d)) {
        return 0;
    }
    return ((CSerialObjRef*)&m_34)->Chain((CSerialArchive*)ar, tag, c, (CSerialObj*)d) != 0;
}

// CWarpStonePad::~CWarpStonePad @0x10fc0 (??_7CWarpStonePad slot 0). Extracted to
// <Gruntz/WarpStonePad.h>; UserLogic.cpp holds the RVA-less vtable-emission copy.
RVA(0x00010fc0, 0x44)
CWarpStonePad::~CWarpStonePad() {}

// CStatusBarSprite::~CStatusBarSprite @0x11b80 (??_7CStatusBarSprite slot 0). Extracted
// to <Gruntz/StatusBarSprite.h>; StatusBarSpriteActs.cpp holds the vtable-emission copy.
RVA(0x00011b80, 0x44)
CStatusBarSprite::~CStatusBarSprite() {}

// CToyPeek::~CToyPeek @0x11c40 (??_7CToyPeek slot 0).
RVA(0x00011c40, 0x44)
CToyPeek::~CToyPeek() {}

// CSpotLight::~CSpotLight @0x13040 (??_7CSpotLight slot 0).
RVA(0x00013040, 0x44)
CSpotLight::~CSpotLight() {}

// @early-stop
// L_13400 (0x13400) IS CUFO::~CUFO (??_7CUFO slot 0). CODEGEN WALL, not a naming miss:
// CUFO : CPathHazard : CUserLogic with a 0x130-byte layout. Folding onto the real
// CUFO (tried: `CUFO::~CUFO(){}` w/ Ufo.h) craters 0x13400 to 4.7% - the 2-level model
// with its non-trivial members emits a different dtor than retail's pure 0x44-byte
// CUserLogic-teardown shape (MSVC5 empty-dtor optimization collapses the intermediate
// vptr stores only for direct/trivial leaves). Needs a matcher to pare CUFO's member
// model so its empty dtor optimizes to the base teardown; the CUserLogic-shaped
// placeholder is required until then.
RVA(0x00013400, 0x44)
L_13400::~L_13400() {}

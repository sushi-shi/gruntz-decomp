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
#include <Gruntz/SingleFrameMessage.h>
#include <Gruntz/SpotLight.h>
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

// L_fb00 (0x0000fb00) IS CFrontCandy::~CFrontCandy (??_7CFrontCandy slot 0). Foldable
// like CAniCycle, but CFrontCandy is a .cpp-local class in UserLogic.cpp (no header) -
// extract it to include/Gruntz/ first, then fold (kills placeholder + a .cpp-local view).
RVA(0x0000fb00, 0x44)
L_fb00::~L_fb00() {}

// S_fdf0 (0x0000fdf0) is a slot-1 SerializeMove; xref --tree puts it at
// ??_7CFrontCandyAni+0x4, but CFrontCandyAni already claims 0xfa60 as its slot-1
// override - the +0x4 data-ref likely lands in the NEXT unlabeled vtable after
// ??_7CFrontCandyAni. Attribution is AMBIGUOUS; verify the vtable contents before
// folding (do not trust the nearest-preceding-symbol +0x4).
RVA(0x0000fdf0, 0x47)
i32 S_fdf0::Serialize(i32 ar, i32 tag, i32 c, i32 d) {
    if (!SerializeChain(ar, tag, c, d)) {
        return 0;
    }
    return ((CSerialObjRef*)((char*)this + 0x34))
               ->Chain((CSerialArchive*)ar, tag, c, (CSerialObj*)d)
           != 0;
}

// L_fe90 (0x0000fe90) IS CFrontCandyAni::~CFrontCandyAni (??_7CFrontCandyAni slot 0).
// CFrontCandyAni is double-defined (UserLogic.cpp local + FrontCandyAni.h) with a
// conflicting slot-1 attribution - reconcile the two class defs before folding.
RVA(0x0000fe90, 0x44)
L_fe90::~L_fe90() {}

// CEyeCandyAni::~CEyeCandyAni @0xffc0 (??_7CEyeCandyAni slot 0).
RVA(0x0000ffc0, 0x44)
CEyeCandyAni::~CEyeCandyAni() {}

// L_101b0 (0x000101b0) IS CMenuSparkle::~CMenuSparkle (??_7CMenuSparkle slot 0).
// CMenuSparkle is a documented dual-model (canonical CTileLogic in UserLogic.cpp vs
// Grunt.h-world CUserLogic view in MenuSparkleSerial.h) - reconcile before folding.
RVA(0x000101b0, 0x44)
L_101b0::~L_101b0() {}

RVA(0x000104a0, 0x47)
i32 S_104a0::Serialize(i32 ar, i32 tag, i32 c, i32 d) {
    if (!SerializeChain(ar, tag, c, d)) {
        return 0;
    }
    return ((CSerialObjRef*)((char*)this + 0x34))
               ->Chain((CSerialArchive*)ar, tag, c, (CSerialObj*)d)
           != 0;
}

RVA(0x000105d0, 0x47)
i32 S_105d0::Serialize(i32 ar, i32 tag, i32 c, i32 d) {
    if (!SerializeChain(ar, tag, c, d)) {
        return 0;
    }
    return ((CSerialObjRef*)((char*)this + 0x34))
               ->Chain((CSerialArchive*)ar, tag, c, (CSerialObj*)d)
           != 0;
}

// L_10fc0 (0x00010fc0) IS CWarpStonePad::~CWarpStonePad (??_7CWarpStonePad slot 0).
// Foldable; CWarpStonePad is a .cpp-local class in UserLogic.cpp - extract to a header first.
RVA(0x00010fc0, 0x44)
L_10fc0::~L_10fc0() {}

// L_11b80 (0x00011b80) IS CStatusBarSprite::~CStatusBarSprite (??_7CStatusBarSprite slot 0).
// Foldable; CStatusBarSprite is a .cpp-local class in StatusBarSpriteActs.cpp - extract first.
RVA(0x00011b80, 0x44)
L_11b80::~L_11b80() {}

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

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
#include <Gruntz/BoundaryLeafLogicViews.h> // placeholder CUserLogic leaf classes (pulls UserLogic.h)
#include <Gruntz/CSerialObjRef.h>          // the shared serialized-object-reference (Chain @0x8c00)

// ---------------------------------------------------------------------------
// Leaf destructors (the /GX leaf-dtor archetype). Each is byte-identical to
// ~CSimpleAnimation; the only per-class difference is the EH funcinfo table the
// compiler emits (reloc-masked).
// ---------------------------------------------------------------------------
RVA(0x00008860, 0x44)
L_8860::~L_8860() {}

RVA(0x0000f510, 0x44)
L_f510::~L_f510() {}

RVA(0x0000f640, 0x44)
L_f640::~L_f640() {}

RVA(0x0000fb00, 0x44)
L_fb00::~L_fb00() {}

RVA(0x0000fdf0, 0x47)
i32 S_fdf0::Serialize(i32 ar, i32 tag, i32 c, i32 d) {
    if (!SerializeChain(ar, tag, c, d)) {
        return 0;
    }
    return ((CSerialObjRef*)((char*)this + 0x34))
               ->Chain((CSerialArchive*)ar, tag, c, (CSerialObj*)d)
           != 0;
}

RVA(0x0000fe90, 0x44)
L_fe90::~L_fe90() {}

RVA(0x0000ffc0, 0x44)
L_ffc0::~L_ffc0() {}

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

RVA(0x00010fc0, 0x44)
L_10fc0::~L_10fc0() {}

RVA(0x00011b80, 0x44)
L_11b80::~L_11b80() {}

RVA(0x00011c40, 0x44)
L_11c40::~L_11c40() {}

RVA(0x00013040, 0x44)
L_13040::~L_13040() {}

RVA(0x00013400, 0x44)
L_13400::~L_13400() {}

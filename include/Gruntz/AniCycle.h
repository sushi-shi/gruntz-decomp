// AniCycle.h - an animation-cycle eyecandy game-object (C:\Proj\Gruntz), a
// CUserLogic leaf in the same animation family as CSimpleAnimation /
// CSingleFrameMessage: a per-class activation registry (g_aniCycleActReg
// @0x646088) bound by RegisterActs to a per-frame AdvanceAnim handler under the
// shared activation-name key "A".
//
// CAniCycle : CUserLogic. Only offsets / code bytes are load-bearing; names are
// placeholders for the recovered engine identities.
#ifndef GRUNTZ_CANICYCLE_H
#define GRUNTZ_CANICYCLE_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h> // LogicTypeId (GetTypeTag return type)
#include <Gruntz/UserLogic.h>   // CUserLogic base (CAniCycle : CUserLogic)

class CAniCycle : public CUserLogic {
public:
    TILE_LOGIC_TAIL
public:
    CAniCycle(CGameObject* obj); // 0x0aad20 (ctor body in UserLogic.cpp)
    // The vtable slot-2 logic-type id accessor (returns 0x3ea).
    // 0x0000f450 vtable slot 2: per-class logic-type id, inline (one
    // deduped COMDAT copy in retail; see docs on header-inline members).
    RVA(0x0000f450, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_ANICYCLE;
    }
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    virtual i32 UserLogicVfunc2() OVERRIDE;                            // slot 4
    // The vtable slot-1 override (two-chain Serialize): the shared CUserLogic
    // serialize helper on `this`, then the +0x34 sub-object's chain.
    i32 Serialize(i32 ar, i32 tag, i32 c, i32 d); // 0x00f470
    // Construct the class's activation-coordinate registry singleton over the
    // fixed [2000, 2010] range via the shared registry ctor (0x408710).
    static void InitActReg(); // 0x0aaf00
    // Look up the activation-registry entry for id and, if a handler is bound, run
    // it as a PMF on this (the same ResolveEntry archetype inlined twice, once for
    // the null-check, once for the dispatch). 0x0aaf80.
    i32 RunAct(i32 id);
    // Bind the per-frame handler (AdvanceAnim) to the activation key "A" via the
    // shared name registry; the same archetype as CBehindCandyAni::RegisterActs.
    static void RegisterActs(); // 0x0ab0e0
    // The per-frame handler (@0x0ab2e0); Ghidra did not carve it (recovery gap),
    // so it is declared only - RegisterActs takes its address as a reloc-masked
    // handler-store operand.
    i32 AdvanceAnim();
    virtual ~CAniCycle() OVERRIDE; // empty vtable-anchor dtor (folds the CUserLogic teardown)

    i32 m_40;                  // +0x40 (geometry-id cache; written by the ctor)
    char m_pad44[0x54 - 0x44]; // +0x44..0x53 (leaf tail; sizeof from `new CAniCycle` @0xa9a40)
};
VTBL(CAniCycle, 0x001e86a4);
SIZE(CAniCycle, 0x54);

#endif // GRUNTZ_CANICYCLE_H

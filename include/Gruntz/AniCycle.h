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

class CAniCycle : public CTileLogic {
public:
    CAniCycle(CGameObject* obj); // 0x0aad20 (ctor body in UserLogic.cpp)
    // The vtable slot-2 logic-type id accessor (returns 0x3ea).
    LogicTypeId GetTypeTag(); // 0x00f450
    // The vtable slot-1 override (two-chain Serialize): the shared CUserLogic
    // serialize helper on `this`, then the +0x34 sub-object's chain.
    i32 Serialize(i32 ar, i32 tag, i32 c, i32 d); // 0x00f470
    // Construct the class's activation-coordinate registry singleton over the
    // fixed [2000, 2010] range via the shared registry ctor (0x408710).
    static void InitActReg(); // 0x0aaf00
    // Bind the per-frame handler (AdvanceAnim) to the activation key "A" via the
    // shared name registry; the same archetype as CBehindCandyAni::RegisterActs.
    static void RegisterActs(); // 0x0ab0e0
    // The per-frame handler (@0x0ab2e0); Ghidra did not carve it (recovery gap),
    // so it is declared only - RegisterActs takes its address as a reloc-masked
    // handler-store operand.
    i32 AdvanceAnim();
    virtual ~CAniCycle() OVERRIDE; // empty vtable-anchor dtor (folds the CUserLogic teardown)

    i32 m_40; // +0x40 (geometry-id cache; written by the ctor)
};

#endif // GRUNTZ_CANICYCLE_H

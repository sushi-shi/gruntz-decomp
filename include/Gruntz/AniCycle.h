#ifndef GRUNTZ_CANICYCLE_H
#define GRUNTZ_CANICYCLE_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h> // LogicTypeId (GetTypeTag return type)
#include <Gruntz/UserLogic.h>   // CUserLogic base (CAniCycle : CUserLogic)

class CAniCycle : public CUserLogic, public CWapX {
public:
public:
    CAniCycle(CGameObject* obj); // 0x0aad20 (ctor body in UserLogic.cpp)
    // The vtable slot-2 logic-type id accessor (returns 0x3ea).
    // 0x0000f450 vtable slot 2: per-class logic-type id, inline (one
    // deduped COMDAT copy in retail; see docs on header-inline members).
    RVA(0x0000f450, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_ANICYCLE;
    }
    virtual i32 SerializeMove(CFileMemBase*, i32, i32, i32) OVERRIDE; // slot 1
    // The vtable slot-1 override (two-chain Serialize): the shared CUserLogic
    // serialize helper on `this`, then the +0x34 sub-object's chain.
    // Construct the class's activation-coordinate registry singleton over the
    // fixed [2000, 2010] range via the shared registry ctor (0x408710).
    static void InitActReg(); // 0x0aaf00
    // Look up the activation-registry entry for id and, if a handler is bound, run
    // it as a PMF on this (the same ResolveEntry archetype inlined twice, once for
    // the null-check, once for the dispatch). 0x0aaf80.
    virtual void FireActivation(i32 id) OVERRIDE;
    // Bind the per-frame handler (AdvanceAnim) to the activation key "A" via the
    // shared name registry; the same archetype as CBehindCandyAni::RegisterActs.
    static void RegisterActs(); // 0x0ab0e0
    // The per-frame handler (@0x0ab2e0); Ghidra did not carve it (recovery gap),
    // so it is declared only - RegisterActs takes its address as a reloc-masked
    // handler-store operand.
    i32 AdvanceAnim();
    // NO user-declared dtor: retail's is COMPILER-GENERATED (implicit
    // elides the leaf-vptr restamp; RVA_COMPGEN pin in the home TU).
};
SIZE(0x54);

typedef i32 (CUserLogic::*AniCycleHandler)();
struct CAniCycleActEntry {
    AniCycleHandler m_fn;
};
SIZE_UNKNOWN();

#include <Gruntz/ActReg.h>       // CActReg (extern below)
extern CActReg g_aniCycleActReg; // 0x00246088

#endif // GRUNTZ_CANICYCLE_H

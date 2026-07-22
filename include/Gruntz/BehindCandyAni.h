#ifndef GRUNTZ_CBEHINDCANDYANI_H
#define GRUNTZ_CBEHINDCANDYANI_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h> // LogicTypeId (GetTypeTag return type)
#include <Gruntz/UserLogic.h>   // CUserLogic base (CBehindCandyAni : CUserLogic)

class CBehindCandyAni : public CUserLogic, public CWapX {
public:
public:
    CBehindCandyAni(CGameObject* obj); // 0x0ad540 (ctor body in UserLogic.cpp)
    // Resolve the registry entry for id; run its bound handler as a PMF on this
    // (ResolveEntry inlined twice). 0x0ad850.
    virtual void FireActivation(i32 id) OVERRIDE;
    // Construct the class's activation-coordinate registry (g_behindCandyActReg
    // @0x645f98) over the fixed [2000,2010] range; free init thunk, reloc-masked.
    static void InitActReg(); // 0x0ad7d0
    // Bind the per-frame handler (AdvanceAnim) to the activation key "A" via the
    // shared name registry (the same archetype as CSecretLevelTrigger::RegisterActs).
    static void RegisterActs(); // 0x0ad9b0
    i32 AdvanceAnim();          // 0x0adbb0 (re-target bound anim to the draw-delta; ret 0)
    // 0x00010030 vtable slot 2: per-class logic-type id, inline (one
    // deduped COMDAT copy in retail; see docs on header-inline members).
    RVA(0x00010030, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_BEHINDCANDYANI;
    }
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    // NO user-declared dtor: retail's is COMPILER-GENERATED (implicit
    // elides the leaf-vptr restamp; RVA_COMPGEN pin in the home TU).
};
SIZE(0x54);

typedef i32 (CUserLogic::*BehindCandyHandler)();
struct CBehindCandyActEntry {
    BehindCandyHandler m_fn;
};
SIZE_UNKNOWN();

#endif // GRUNTZ_CBEHINDCANDYANI_H

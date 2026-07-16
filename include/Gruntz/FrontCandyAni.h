// FrontCandyAni.h - a front-candy eyecandy animation game-object
// (C:\Proj\Gruntz). The sibling of CBehindCandyAni (the eyecandy that draws in
// FRONT of the gruntz rather than behind), sharing the same CUserLogic leaf
// archetype: a per-coordinate activation registry (g_frontCandyActReg @0x6460b0)
// that FireActivation dispatches through, bound by RegisterActs to the per-frame
// AdvanceAnim handler under the shared activation-name key "A".
//
// CANONICAL CFrontCandyAni : CUserLogic (vtable 0x1e83e4). Unifies the former
// UserLogic.cpp-local "genuine ctor" view (ctor 0xacf40 + full vtable + m_40) with
// the "acts" facet (InitActReg/FireActivation/RegisterActs/AdvanceAnim); the added
// non-virtual/static methods are layout-neutral so the ctor emission is unchanged.
// Only offsets / code bytes are load-bearing; names are placeholders.
#ifndef GRUNTZ_CFRONTCANDYANI_H
#define GRUNTZ_CFRONTCANDYANI_H

#include <rva.h>
#include <Gruntz/LogicTypeId.h> // LogicTypeId (GetTypeTag return type)
#include <Gruntz/UserLogic.h>   // CUserLogic base (CFrontCandyAni : CUserLogic)

class CFrontCandyAni : public CUserLogic {
public:
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    RVA(0x0000fdd0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_FRONTCANDYANI;
    } // slot 2
    virtual i32 UserLogicVfunc2() OVERRIDE; // slot 4
    TILE_LOGIC_TAIL
public:
    CFrontCandyAni(CGameObject* obj);   // 0x0acf40
    virtual ~CFrontCandyAni() OVERRIDE; // 0xfe90 (slot 0; folds the CUserLogic teardown)
    // The vtable slot-1 two-chain body (0xfdf0): the shared CUserLogic serialize
    // helper on `this`, then the +0x34 sub-object's chain. (0xfa60 was mis-attributed
    // here; the vtable read proves it is CFrontCandy::Serialize.)
    i32 Serialize(i32 ar, i32 tag, i32 c, i32 d); // 0xfdf0
    // Construct the class's activation-coordinate registry (g_frontCandyActReg
    // @0x6460b0) over the fixed [2000,2010] range; free init thunk, reloc-masked.
    static void InitActReg(); // 0x0ad130
    // Look the activation coordinate up in the class registry; if its entry has a
    // bound handler, look it up again and dispatch it __thiscall on this. The SAME
    // archetype as CParticlez::FireActivation (0x046d30).
    void FireActivation(i32 coord); // 0x0ad1b0
    // Bind the per-frame handler (AdvanceAnim) to the activation key "A" via the
    // shared name registry; the same archetype as CBehindCandyAni::RegisterActs.
    static void RegisterActs(); // 0x0ad310
    i32 AdvanceAnim();          // 0x0ad510 (re-target bound anim to the draw-delta; ret 0)
    CAniElement* m_40;                   // +0x40  saved active-anim descriptor (ctor snapshot)
    char m_pad44[0x54 - 0x44];  // +0x44..0x53 (leaf is 0x54: its only new-site, the
                                // logic-worker pump @0xaa460, pushes 0x54)
};
VTBL(CFrontCandyAni, 0x1e83e4);

// The per-coordinate activation registry entry (g_frontCandyActReg's element): its
// first dword receives the per-frame handler PMF (AdvanceAnim, a 4-byte code ptr on
// this single-inheritance class). FireActivation/RegisterActs cast the CActReg entry
// to this. A faithful 4-byte PMF record, hoisted out of FrontCandyAni.cpp.
typedef i32 (CFrontCandyAni::*FrontCandyHandler)();
struct CFrontCandyActEntry {
    FrontCandyHandler m_fn;
};
SIZE_UNKNOWN(CFrontCandyActEntry);

#endif // GRUNTZ_CFRONTCANDYANI_H

#ifndef GRUNTZ_CFORTRESSFLAG_H
#define GRUNTZ_CFORTRESSFLAG_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h> // LogicTypeId (GetTypeTag return type)
#include <Gruntz/UserLogic.h>   // CUserLogic base (CFortressFlag : CUserLogic)

class CFortressFlag : public CUserLogic, public CWapX {
public:
public:
    CFortressFlag(CGameObject* obj); // 0x045d30
    // Construct the class's activation-coordinate registry (g_fortressFlagActReg
    // @0x644638) over the fixed [2000,2010] range; free init thunk, reloc-masked.
    static void InitActReg(); // 0x046000
    // Bind the per-frame handler (AdvanceAnim) to the activation key "A" via the
    // shared name registry (the same archetype as CBehindCandyAni::RegisterActs).
    static void RegisterActs();                   // 0x0461e0
    virtual void FireActivation(i32 id) OVERRIDE; // 0x046080 (per-coord PMF dispatcher)
    i32 AdvanceAnim();          // 0x0463e0 (re-target bound anim to the draw-delta; ret 0)
    void HandleFortConquered(); // 0x03f5f0 (per-frame fort-conquest check)
    // vtable slot 2 (per-class logic-type id); regular method - the fat CUserLogic
    // base models this slot with a placeholder signature (see CGuardPoint.cpp).
    // 0x00010e40 vtable slot 2: per-class logic-type id, inline (one
    // deduped COMDAT copy in retail; see docs on header-inline members).
    RVA(0x00010e40, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_FORTRESSFLAG;
    }
    virtual i32 SerializeMove(CFileMemBase*, i32, i32, i32) OVERRIDE; // slot 1
    // NO user-declared dtor: retail's is COMPILER-GENERATED (implicit
    // elides the leaf-vptr restamp; RVA_COMPGEN pin in the home TU).
};
SIZE(0x54);

typedef i32 (CUserLogic::*FortressFlagHandler)();
struct CFortressFlagActEntry {
    FortressFlagHandler m_fn;
};
SIZE_UNKNOWN();

#include <Gruntz/ActReg.h>           // CActReg (extern below)
extern CActReg g_fortressFlagActReg; // 0x00244638

#include <Gruntz/ActReg.h>                  // CLogicActTable (extern below)
extern CLogicActTable g_logicActReg_6447f8; // 0x002447f8

// TU-local thunk/table names this TU registers (moved from the .cpp; the
// addresses are ILT thunk VAs, reloc-masked at every use).
extern "C" void FortressFlagAct(); // thunk 0x4041ec -> 0x466b0

// --- the TU's extern surface (moved out of the .cpp; addresses/thunk
// VAs are reloc-masked at use) ---
extern CActReg g_partColl;

#endif // GRUNTZ_CFORTRESSFLAG_H

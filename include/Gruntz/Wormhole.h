#ifndef GRUNTZ_CWORMHOLE_H
#define GRUNTZ_CWORMHOLE_H

#include <rva.h>
#include <Gruntz/UserLogic.h> // CUserLogic base (CWormhole : CUserLogic)

class CWormhole : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeMove(CFileMemBase*, i32, i32, i32) OVERRIDE; // slot 1
    RVA(0x00010930, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_WORMHOLE;
    } // slot 2
public:
    // (The ex "CWormhole act cluster" - InitActReg 0x3f210 / FireAct 0x3f290 /
    // RegisterActs 0x3f3f0 / AdvanceAnim 0x3f5f0 / g_wormholeActReg 0x6445c0 - is
    // RE-ATTRIBUTED to CExitTrigger (RTTI: CExitTrigger[4] -> ILT 0x0042e6 -> jmp
    // 0x3f290). This class's OWN slot 4 is 0x0034e5 -> 0x040050, below. See
    // <Gruntz/ExitTrigger.h>.
    virtual void FireActivation(i32 id) OVERRIDE; // slot 4: 0x040050 (logic-command
                                                  // dispatch via g_wormholeDispatch)
    CWormhole(CGameObject* obj);                  // 0x03fc70 (1-arg leaf ctor, /GX frame)
    void SpawnPartners();                         // 0x0403b0
    // NO user-declared dtor: retail's is COMPILER-GENERATED (implicit
    // elides the leaf-vptr restamp; RVA_COMPGEN pin in the home TU).
};
SIZE_UNKNOWN();

#include <Gruntz/ActReg.h> // CActReg (extern below)

// The act-table slot type (the registry stores CUserLogic member pointers).
typedef void (CUserLogic::*WormholeActHandler)();

// TU-local thunk/table names this TU registers (moved from the .cpp; the
// addresses are ILT thunk VAs, reloc-masked at every use).
extern "C" void TeleporterActB(); // 0x403846 (teleporter "B")
extern "C" void TeleporterActA(); // 0x40187a (teleporter "A")
extern "C" void PuddleActB();     // 0x403418 (puddle "B")
extern "C" void PuddleActA();     // 0x4021f8 (puddle "A")

// --- the TU's extern surface (moved out of the .cpp; addresses/thunk
// VAs are reloc-masked at use) ---
#include <Gruntz/LogicFnTable.h> // CActReg (for the extern below)

#endif // GRUNTZ_CWORMHOLE_H

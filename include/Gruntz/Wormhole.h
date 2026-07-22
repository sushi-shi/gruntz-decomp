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
    void LoadColors();                            // 0x0411f0
    i32 ReapplyConfig();                          // 0x0412c0 (per-partner config re-run)
    // NO user-declared dtor: retail's is COMPILER-GENERATED (implicit
    // elides the leaf-vptr restamp; RVA_COMPGEN pin in the home TU).

    // CWormhole's own data begins at +0x40 (CUserLogic base ends at +0x40). Only
    // the offsets the matched methods write are modeled; +0x54/+0x68 are config
    // state flags whose exact roles are unproven (kept as offset placeholders).
    i32 m_54; // +0x54  config flag (set to 1 by ReapplyConfig)
    char m_pad58[0x68 - 0x58];
    i32 m_68; // +0x68  config flag (cleared by ReapplyConfig)
};
SIZE_UNKNOWN();

#include <Gruntz/ActReg.h> // CTeleporterActReg (extern below)
extern CTeleporterActReg g_teleporterActReg; // 0x002446b0

// TU-local thunk/table names this TU registers (moved from the .cpp; the
// addresses are ILT thunk VAs, reloc-masked at every use).
extern "C" void TeleporterActB(); // 0x403846 (teleporter "B")
extern "C" void TeleporterActA(); // 0x40187a (teleporter "A")
extern "C" void PuddleActB(); // 0x403418 (puddle "B")
extern "C" void PuddleActA(); // 0x4021f8 (puddle "A")
extern i32 WormholeLogic_40181b();
extern "C" void WormholeTypeMarker();

// --- the TU's extern surface (moved out of the .cpp; addresses/thunk
// VAs are reloc-masked at use) ---
extern CLogicActTable g_logicDispatch_6445e8; // owner-TU definition; its 0x24-byte CActReg
#include <Gruntz/LogicFnTable.h> // CLogicActTable (for the extern below)
extern CLogicActTable g_wormholeDispatch;

#endif // GRUNTZ_CWORMHOLE_H

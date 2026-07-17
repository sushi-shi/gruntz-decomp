// Wormhole.h - the world teleport-node game-object (C:\Proj\Gruntz), a CUserLogic
// leaf. The class adds no data members the dtor sees, so its dtor folds the bare
// CUserLogic teardown (the /GX leaf-dtor archetype). Split across two TUs:
//   Wormhole.cpp      - the object logic (~CWormhole, SpawnPartners, LoadColors,
//                       the config-rerun stubs).
//   WormholeActs.cpp  - now the CEXITTRIGGER act cluster (re-attributed).
// The vtable is stamped by ~CWormhole (its key function, in Wormhole.cpp); no other
// TU instantiates the class, so none re-emits it. Field names/offsets + code bytes
// are load-bearing.
#ifndef GRUNTZ_CWORMHOLE_H
#define GRUNTZ_CWORMHOLE_H

#include <rva.h>
#include <Gruntz/UserLogic.h> // CUserLogic base (CWormhole : CUserLogic)

SIZE_UNKNOWN(CWormhole);
class CWormhole : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
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
    // elides the leaf-vptr restamp; @rva-symbol pin in the home TU).

    // CWormhole's own data begins at +0x40 (CUserLogic base ends at +0x40). Only
    // the offsets the matched methods write are modeled; +0x54/+0x68 are config
    // state flags whose exact roles are unproven (kept as offset placeholders).
    i32 m_54; // +0x54  config flag (set to 1 by ReapplyConfig)
    char m_pad58[0x68 - 0x58];
    i32 m_68; // +0x68  config flag (cleared by ReapplyConfig)
};
VTBL(CWormhole, 0x1e817c);

// (CWormholeActEntry moved to <Gruntz/ExitTrigger.h> as CExitActEntry - the act
// cluster is CExitTrigger's.)

#endif // GRUNTZ_CWORMHOLE_H

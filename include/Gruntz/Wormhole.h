// Wormhole.h - the world teleport-node game-object (C:\Proj\Gruntz), a CUserLogic
// leaf. The class adds no data members the dtor sees, so its dtor folds the bare
// CUserLogic teardown (the /GX leaf-dtor archetype). Split across two TUs:
//   Wormhole.cpp      - the object logic (~CWormhole, SpawnPartners, LoadColors,
//                       the config-rerun stubs).
//   CWormholeActs.cpp - the activation-registration side (InitActReg / RegisterActs
//                       / the AdvanceAnim handler PMF).
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
    static void InitActReg();   // 0x03f210
    static void RegisterActs(); // 0x03f3f0
    // @identity-TODO  0x03f290 is NOT CWormhole's - the retail bytes say it is
    // CExitTrigger's vtable slot 4: the ILT thunk at 0x0042e6 is `e9 a5 af 03 00`
    // = `jmp 0x03f290`, and CExitTrigger's RTTI vtable (0x1e822c) slot 4 holds
    // exactly 0x0042e6 (CWormhole's own slot 4 is 0x0034e5 -> 0x040050 below, on
    // the SEPARATE g_wormholeDispatch registry - so CWormhole does not need, and
    // cannot have, a second slot-4 body). Corroborated by the RVA band:
    // ExitTrigger.cpp runs 0x3ecf0..0x3f187 and this act cluster (InitActReg
    // 0x3f210 / this 0x3f290 / RegisterActs 0x3f3f0 / AdvanceAnim 0x3f5f0)
    // continues it contiguously, while CWormhole's own methods start at 0x3fc70.
    // MSVC5 has no /OPT:ICF, so 0x3f290 has exactly ONE owner. The "wormhole"
    // naming of the cluster + g_wormholeActReg (0x6445c0) is a previous lane's
    // guess from RVA proximity - the global's name is DATA-reloc-masked and proves
    // nothing. Re-attributing the whole cluster (4 fns + the registry global + the
    // entry PMF type) to CExitTrigger is a follow-up: it re-mangles 4 RVA-bound
    // methods, which needs its own per-unit %-verification. Recipe + precedent:
    // CToyPeek::FireActivation @0x97de0 (was CInGameIcon::RunState), done this
    // session - byte-neutral, 100% held.
    void FireAct(i32 coord); // 0x03f290 (act-registry PMF dispatcher, g_wormholeActReg)
    virtual void FireActivation(i32 id) OVERRIDE; // slot 4: 0x040050 (logic-command
                                                  // dispatch via g_wormholeDispatch)
    i32 AdvanceAnim();                            // 0x03f5f0 (the per-frame handler PMF)
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

// The per-class activation-registry entry: its first dword receives the per-frame
// handler PMF (AdvanceAnim; a 4-byte code ptr on this single-inheritance class).
// Declared here (not .cpp-local) with the class it binds to (same pattern as
// CLightFxActEntry / CTeleporterActEntry).
typedef i32 (CUserLogic::*WormholeHandler)();
struct CWormholeActEntry {
    WormholeHandler m_fn;
};
SIZE_UNKNOWN(CWormholeActEntry);

#endif // GRUNTZ_CWORMHOLE_H

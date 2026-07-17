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
class CWormhole : public CUserLogic {
public:
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    RVA(0x00010930, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_WORMHOLE;
    } // slot 2
    virtual i32 UserLogicVfunc2() OVERRIDE; // slot 4
    TILE_LOGIC_TAIL
public:
    static void InitActReg();                     // 0x03f210
    static void RegisterActs();                   // 0x03f3f0
    void FireActivation(i32 coord);               // 0x03f290 (per-coord PMF dispatcher)
    void Dispatch(i32 idx);                       // 0x040050 (logic-command dispatch via
                                                  //           g_wormholeDispatch; same as
                                                  //           CSimpleAnimation::Dispatch)
    i32 AdvanceAnim();                            // 0x03f5f0 (the per-frame handler PMF)
    CWormhole(CGameObject* obj);                  // 0x03fc70 (1-arg leaf ctor, /GX frame)
    void SpawnPartners();                         // 0x0403b0
    void LoadColors();                            // 0x0411f0
    i32 ReapplyConfig();                          // 0x0412c0 (per-partner config re-run)
    virtual ~CWormhole() OVERRIDE;                // 0x010980 (folded leaf teardown, /GX frame)

    // CWormhole's own data begins at +0x40 (CUserLogic base ends at +0x40). Only
    // the offsets the matched methods write are modeled; +0x54/+0x68 are config
    // state flags whose exact roles are unproven (kept as offset placeholders).
    CAniElement* m_prevAnimNode; // +0x40  snapshot of the bound object's active-anim descriptor
    char m_pad44[0x54 - 0x44];
    i32 m_54; // +0x54  config flag (set to 1 by ReapplyConfig)
    char m_pad58[0x68 - 0x58];
    i32 m_68; // +0x68  config flag (cleared by ReapplyConfig)
};
VTBL(CWormhole, 0x1e817c);

// The per-class activation-registry entry: its first dword receives the per-frame
// handler PMF (AdvanceAnim; a 4-byte code ptr on this single-inheritance class).
// Declared here (not .cpp-local) with the class it binds to (same pattern as
// CLightFxActEntry / CTeleporterActEntry).
typedef i32 (CWormhole::*WormholeHandler)();
struct CWormholeActEntry {
    WormholeHandler m_fn;
};
SIZE_UNKNOWN(CWormholeActEntry);

#endif // GRUNTZ_CWORMHOLE_H

// CWormhole.h - the world teleport-node game-object (C:\Proj\Gruntz), a CUserLogic
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
    static void InitActReg();                     // 0x03f210
    static void RegisterActs();                   // 0x03f3f0
    i32 AdvanceAnim();                            // 0x03f5f0 (the per-frame handler PMF)
    CWormhole(CGameObject* obj);                  // 0x03fc70 (1-arg leaf ctor, /GX frame)
    void SpawnPartners();                         // 0x0403b0
    void LoadColors();                            // 0x0411f0
    i32 Serialize(i32 ar, i32 tag, i32 c, i32 d); // 0x03fed0 (two-chain + tag-8 color fixup)
    i32 Stub_0412c0();                            // 0x0412c0 (per-partner config re-run)
    virtual ~CWormhole() OVERRIDE;                // 0x010980 (folded leaf teardown, /GX frame)
};

#endif // GRUNTZ_CWORMHOLE_H

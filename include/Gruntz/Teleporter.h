#ifndef GRUNTZ_CTELEPORTER_H
#define GRUNTZ_CTELEPORTER_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h> // LogicTypeId (GetTypeTag return type)
#include <Gruntz/UserLogic.h>   // CUserLogic base (CTeleporter : CUserLogic)

class CFileMemBase;

struct CTeleAnimSink {
    char m_pad00[0x20];
    i32 m_20; // +0x20 idle-state flag
    char m_pad24[0x28 - 0x24];
    i32 m_28; // +0x28 active flag
};
SIZE_UNKNOWN();

extern "C" u32 g_engineFrameDelta;

extern "C" u32 g_frameTime; // VA 0x645588 (?g_clock@@3IA, unsigned)

#include <Bute/ButeMgr.h>

class CTeleporter : public CUserLogic, public CWapX {
public:
public:
    CTeleporter(CGameObject* obj); // 0x41020 (base init + name/state setup; body in UserLogic.cpp)
    // InitActReg (0x414a0): construct the class's activation-coordinate registry
    // singleton (g_teleporterActReg @0x6446b0) over the fixed [2000, 2010] range.
    // FireActivation (0x41520): per-coordinate activation dispatcher - look coord up
    // in g_teleporterActReg; if the entry has a registered handler, dispatch it on
    // `this`. Same archetype as CParticlez::FireActivation (double ResolveEntry).
    virtual void FireActivation(i32 id) OVERRIDE;
    // Serialize (0x41350): slot-1 (SerializeMove) override - chain the shared
    // CUserLogic serialize helper + the +0x34 sub-object's chain, then tag-dispatch
    // the leaf state: tag 4 writes / tag 7 reads the two i64 arm/interval snapshots
    // (+0x58/+0x60) then the two i32 fields (m_armed/+0x54, m_tickHandled/+0x68);
    // tag 8 (post-load) re-applies the config via LoadColors. Same archetype as
    // CGruntPuddle::Serialize / CWormhole::Serialize.
    void LoadColors();   // 0x411f0: resolve the teleporter/wormhole display color
    i32 ReapplyConfig(); // 0x412c0: reopen and arm the teleporter
    // Begin (0x419e0): advance the anim sub-mgr; on its first idle frame, snapshot
    // the bound geometry, apply the teleporter lookup-geometry and re-bind the "B"
    // bute node. Returns 0.
    i32 Begin();
    // Update (0x41aa0): the per-frame teleporter tick - advance the anim sub-mgr,
    // poll the on-screen render flag, and when armed (m_54) test the cell for a
    // grunt, spawn the "Teleporter"/"Wormhole" sprite, close the gate and scroll
    // the camera to the warped grunt. Returns 0.
    i32 Update();
    // vtable slot 2 (per-class logic-type id), inline. The ctor now lives in
    // Teleporter.cpp so cl+clang emit this COMDAT (@0x10d80) + the ??_7CTeleporter
    // vtable in this TU.
    RVA(0x00010d80, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_TELEPORTER;
    }
    virtual i32 SerializeMove(CFileMemBase*, i32, i32, i32)
        OVERRIDE; // slot 1 (body: Serialize 0x41350)
    // NO user-declared dtor: retail's is COMPILER-GENERATED (implicit
    // elides the leaf-vptr restamp; RVA_COMPGEN pin in the home TU).
    i32 m_armed; // +0x54  armed flag (a resolved target id)
    // The armed-at running-clock snapshot (m_58) and the bound object's per-tile-time
    // interval (m_60), each a manually zero-extended i64 (lo stored, hi forced 0) so
    // the per-frame delta test compares them 64-bit; kept as lo/hi i32 pairs because
    // retail emits two separate 32-bit stores (not a sign-extending i64 assign).
    i32 m_armClockLo;          // +0x58  running-clock snapshot (g_frameTime)
    i32 m_armClockHi;          // +0x5c
    i32 m_intervalLo;          // +0x60  bound object's per-tile-time (m_10->m_7c->m_bc)
    i32 m_intervalHi;          // +0x64
    i32 m_tickHandled;         // +0x68  "tick handled" latch
    char m_pad6c[0x70 - 0x6c]; // +0x6c  (unmodeled tail; size proven 0x70 from
                               //         AnimWorkerHandlers `new CTeleporter`)
};
SIZE(0x70);

typedef i32 (CUserLogic::*TeleporterHandler)();
struct CTeleporterActEntry {
    TeleporterHandler m_fn;
};
SIZE_UNKNOWN(); // only the first dword (the handler) is modeled

#endif // GRUNTZ_CTELEPORTER_H

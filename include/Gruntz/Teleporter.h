// Teleporter.h - the teleporter tile-logic game object (C:\Proj\Gruntz).
//
// CTeleporter : CUserLogic (RTTI: .?AVCTeleporter@@ at 0x609598). A tile-logic
// leaf in the same game-object hierarchy as CGruntPuddle / CPathHazard, proven
// by its dtor (0x10dd0) stamping the CUserLogic vftable 0x5e705c then the
// CUserBase vftable 0x5e70b4, tearing down the +0x18 link via the embedded
// ~EngStr at 0x16d2a0 (the /GX leaf-dtor archetype). It adds no destructible
// members beyond CUserLogic, so the dtor folds the bare teardown.
//
// Begin (0x419e0) is the per-frame/initial bring-up: advance the +0x1a0 anim
// sub-mgr to the current draw-delta, and once it is idle, snapshot the bound
// object's geometry, apply the "GAME_TELEPORTER" lookup-geometry, and swap the
// +0x14 sub-object's "B" bute node - the SAME archetype as CGruntPuddle::Place's
// finalize block.
//
// Field names are placeholders; only the OFFSETS + the inheritance chain are
// load-bearing.
#ifndef GRUNTZ_CTELEPORTER_H
#define GRUNTZ_CTELEPORTER_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h> // LogicTypeId (GetTypeTag return type)
#include <Gruntz/UserLogic.h>   // CUserLogic base (CTeleporter : CUserLogic)

// The serialize stream is the REAL CFileMemBase (<Gruntz/SerialArchive.h> typedefs
// CSerialArchive onto it); a fwd decl of the OLD placeholder name here would
// re-declare a distinct class and silently out-rank the typedef (MSVC5).
class CFileMemBase;
typedef CFileMemBase CSerialArchive;

// The +0x1a0 animation sub-mgr the bring-up advances once each frame (Advance
// 0x15c360, __thiscall ret 4, takes the g_engineFrameDelta draw-delta). Its +0x20/+0x28
// int fields gate the one-shot finalize (run once, when +0x28==0 && +0x20!=0).
// The SAME engine sub-mgr CPathHazard/CSimpleAnimation drive; modeled NO-body so
// the call reloc-masks.
struct CTeleAnimSink {
    char m_pad00[0x20];
    i32 m_20; // +0x20 idle-state flag
    char m_pad24[0x28 - 0x24];
    i32 m_28; // +0x28 active flag
};

// The per-frame draw-delta mirror (BSS @0x6bf3bc) the sub-mgr Advance consumes.
// Already named g_engineFrameDelta in Projectile.cpp; re-declared here, address-pinned.
extern "C" u32 g_engineFrameDelta;

// The lookup-geometry key "GAME_TELEPORTER" (VA 0x60bd38) the finalize applies to
// the bound object via CGameObject::ApplyLookupGeometry (0x1505b0).

// The running game clock (g_frameTime .data int) stashed into the leaf's +0x58.
extern "C" u32 g_frameTime; // VA 0x645588 (?g_clock@@3IA, unsigned)

// The "B" bute key (0x60d1bc) - the SAME rdata as CInGameIcon.h's s_actKeyB;
// reuse the identical declaration so the reloc pairs.
extern char s_actKeyB[]; // DAT_0060d1bc

// g_buteTree (the global bute store) is declared canonically in <Bute/ButeTree.h>,
// reached here transitively via <Bute/ButeMgr.h>.
#include <Bute/ButeMgr.h>

// ---------------------------------------------------------------------------
// CTeleporter : CUserLogic - the teleporter tile-logic leaf. The inherited
// m_10/m_38 (CUserLogic) hold the bound CGameObject; the leaf adds its bring-up
// state at +0x40 (CUserLogic ends at +0x40). The CUserLogic base gives the +0x18
// destructible link, so the dtor folds the shared teardown.
// ---------------------------------------------------------------------------
class CTeleporter : public CUserLogic {
public:
    TILE_LOGIC_TAIL
public:
    CTeleporter(CGameObject* obj); // 0x41020 (base init + name/state setup; body in UserLogic.cpp)
    // InitActReg (0x414a0): construct the class's activation-coordinate registry
    // singleton (g_teleporterActReg @0x6446b0) over the fixed [2000, 2010] range.
    static void InitActReg();
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
    // LoadColors (0x411f0) IS CWormhole::LoadColors - there is ONE such function at
    // 0x411f0 (MSVC5 has no /OPT:ICF); the tag-8 post-load reapply calls it on the
    // teleporter `this` (cast to CWormhole* at the call site), so no fake shadow decl.
    // Begin (0x419e0): advance the anim sub-mgr; on its first idle frame, snapshot
    // the bound geometry, apply the teleporter lookup-geometry and re-bind the "B"
    // bute node. Returns 0.
    i32 Begin();
    // Update (0x41aa0): the per-frame teleporter tick - advance the anim sub-mgr,
    // poll the on-screen render flag, and when armed (m_54) test the cell for a
    // grunt, spawn the "Teleporter"/"Wormhole" sprite, close the gate and scroll
    // the camera to the warped grunt. Returns 0.
    i32 Update();
    // Leaf-state entry setup this-methods the ctor runs (0x1771 / 0x27d9;
    // reloc-masked no-body).
    void EnterField1(); // 0x1771
    void EnterField2(); // 0x27d9
    // vtable slot 2 (per-class logic-type id), inline. The ctor now lives in
    // Teleporter.cpp so cl+clang emit this COMDAT (@0x10d80) + the ??_7CTeleporter
    // vtable in this TU.
    RVA(0x00010d80, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_TELEPORTER;
    }
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32)
        OVERRIDE;                    // slot 1 (body: Serialize 0x41350)
    virtual ~CTeleporter() OVERRIDE; // 0x10dd0 (folds the CUserLogic teardown)

    CAniElement* m_savedGeoId; // +0x40  snapshot of m_38->m_1a0.m_14
    char m_pad44[0x54 - 0x44];
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
SIZE(CTeleporter, 0x70);

// The registry entry FireActivation dispatches: its first dword is the registered
// handler (stored as a free-fn ptr by CTeleporter_RegisterActs, dispatched on
// `this` -> modeled as a 4-byte single-inheritance PMF so the call lowers to
// `mov ecx,this; call [entry]`).
typedef i32 (CTeleporter::*TeleporterHandler)();
struct CTeleporterActEntry {
    TeleporterHandler m_fn;
};
SIZE_UNKNOWN(CTeleporterActEntry); // only the first dword (the handler) is modeled

#endif // GRUNTZ_CTELEPORTER_H

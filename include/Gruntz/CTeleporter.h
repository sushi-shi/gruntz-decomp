// CTeleporter.h - the teleporter tile-logic game object (C:\Proj\Gruntz).
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

// The +0x1a0 animation sub-mgr the bring-up advances once each frame (Advance
// 0x15c360, __thiscall ret 4, takes the g_6bf3bc draw-delta). Its +0x20/+0x28
// int fields gate the one-shot finalize (run once, when +0x28==0 && +0x20!=0).
// The SAME engine sub-mgr CPathHazard/CSimpleAnimation drive; modeled NO-body so
// the call reloc-masks.
struct CTeleAnimSink {
    void Advance(u32 ctx); // 0x15c360
    char m_pad00[0x20];
    i32 m_20; // +0x20 idle-state flag
    char m_pad24[0x28 - 0x24];
    i32 m_28; // +0x28 active flag
};

// The per-frame draw-delta mirror (BSS @0x6bf3bc) the sub-mgr Advance consumes.
// Already named g_6bf3bc in Projectile.cpp; re-declared here, address-pinned.
DATA(0x002bf3bc)
extern "C" u32 g_6bf3bc;

// The lookup-geometry key "GAME_TELEPORTER" (VA 0x60bd38) the finalize applies to
// the bound object via CGameObject::ApplyLookupGeometry (0x1505b0).
DATA(0x0020bd38)
extern char g_teleporterGeoKey[]; // s_GAME_TELEPORTER_0060bd38

// The running game clock (g_645588 .data int) stashed into the leaf's +0x58.
DATA(0x00245588)
extern i32 g_645588; // VA 0x645588

// The "B" bute key (0x60d1bc) - the SAME rdata as CInGameIcon.h's g_iconBute;
// reuse the identical declaration so the reloc pairs.
DATA(0x0020d1bc)
extern char g_iconBute[]; // DAT_0060d1bc

// The global bute store (g_buteTree @0x6bf620; Find 0x16d190 __thiscall ret 4).
#include <Bute/ButeMgr.h>
extern CButeTree g_buteTree;

// ---------------------------------------------------------------------------
// CTeleporter : CUserLogic - the teleporter tile-logic leaf. The inherited
// m_10/m_38 (CUserLogic) hold the bound CGameObject; the leaf adds its bring-up
// state at +0x40 (CUserLogic ends at +0x40). The CUserLogic base gives the +0x18
// destructible link, so the dtor folds the shared teardown.
// ---------------------------------------------------------------------------
class CTeleporter : public CUserLogic {
public:
    // InitActReg (0x414a0): construct the class's activation-coordinate registry
    // singleton (g_teleporterActReg @0x6446b0) over the fixed [2000, 2010] range.
    static void InitActReg();
    // Begin (0x419e0): advance the anim sub-mgr; on its first idle frame, snapshot
    // the bound geometry, apply the teleporter lookup-geometry and re-bind the "B"
    // bute node. Returns 0.
    i32 Begin();
    // Update (0x41aa0): the per-frame teleporter tick - advance the anim sub-mgr,
    // poll the on-screen render flag, and when armed (m_54) test the cell for a
    // grunt, spawn the "Teleporter"/"Wormhole" sprite, close the gate and scroll
    // the camera to the warped grunt. Returns 0.
    i32 Update();
    // vtable slot 2 (per-class logic-type id); regular method - the fat CUserLogic
    // base models this slot with a placeholder signature (see CGuardPoint.cpp).
    LogicTypeId GetTypeTag();        // 0x10d80
    virtual ~CTeleporter() OVERRIDE; // 0x10dd0 (folds the CUserLogic teardown)

    i32 m_savedGeoId; // +0x40  snapshot of m_38->m_geoId
    char m_pad44[0x54 - 0x44];
    i32 m_armed; // +0x54  armed flag (a resolved target id)
    // The armed-at running-clock snapshot (m_58) and the bound object's per-tile-time
    // interval (m_60), each a manually zero-extended i64 (lo stored, hi forced 0) so
    // the per-frame delta test compares them 64-bit; kept as lo/hi i32 pairs because
    // retail emits two separate 32-bit stores (not a sign-extending i64 assign).
    i32 m_armClockLo;  // +0x58  running-clock snapshot (g_645588)
    i32 m_armClockHi;  // +0x5c
    i32 m_intervalLo;  // +0x60  bound object's per-tile-time (m_10->m_7c->m_bc)
    i32 m_intervalHi;  // +0x64
    i32 m_tickHandled; // +0x68  "tick handled" latch
};

#endif // GRUNTZ_CTELEPORTER_H

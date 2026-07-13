// PathHazard.h - the path-following hazard game object (C:\Proj\Gruntz).
//
// CPathHazard : CUserLogic (RTTI; most-derived vftable 0x5e7394, the
// CPathHazard base ctor 0xb35a0 stamps it; CRainCloud / CUFO derive from it).
// A hazard that walks a precomputed waypoint path each frame: BeginLeg
// (0xb47e0, virtual slot 19) computes the unit-vector toward the current
// waypoint, and Tick (0xb4020, virtual slot 16) integrates the sub-pixel
// movement vector into the bound object's tile position each frame, snapping to
// the waypoint on overshoot. The leaf dtor (0x13340) folds the bare CUserLogic
// teardown (the /GX leaf-dtor archetype shared with CTimeBomb / CKitchenSlime).
//
// CUserLogic / CUserBase / EngStr / CGameObject come from <Gruntz/UserLogic.h>.
// Only the OFFSETS + the code bytes are load-bearing; field names are
// placeholders for the recovered engine identities.
#ifndef GRUNTZ_CPATHHAZARD_H
#define GRUNTZ_CPATHHAZARD_H

#include <rva.h>

#include <Gruntz/GameRegistry.h>
#include <Gruntz/LogicTypeId.h> // LogicTypeId (GetTypeTag return type)
#include <Gruntz/UserLogic.h>

// The waypoint the path array (this+0x90, 8-byte stride) stores: {x:int, y:int}.
struct CPathWaypoint {
    i32 x; // +0x00
    i32 y; // +0x04
};

// The hazard reads its bound CGameObject (this->m_10 == this->m_38) directly:
// screen pos (+0x5c/+0x60), the +0x7c aux (per-tile time m_7c->m_bc), leg/segment
// count (+0x120), the on-screen rect base (+0x144, passed to QueryAt) and the
// +0x198 layer descriptor (CGameObjLayer, its +0x18/+0x1c base offsets) - all
// modeled on CGameObject / CGameObjLayer in <Gruntz/UserLogic.h>, no per-TU view.

// The entity QueryAt returns; +0x258 is its type/state tag (0x38 == this hazard
// itself, so its own footprint is ignored).
struct CPathEntity {
    char m_pad00[0x258];
    i32 m_258; // +0x258
};

// The visibility / cue gate is reached cast-free as g_gameReg->m_cmdGrid (typed
// CTriggerMgr*): QueryAt IS CTriggerMgr::FindGruntAt @0x75c60, Strike IS
// CTriggerMgr::CellDispatch @0x6bcb0 (both via <Gruntz/TriggerMgr.h>).

DATA(0x0024556c)

// The +0x1a0 sub-mgr the per-frame Tick advances once (SetGeoSource 0x15c360,
// __thiscall ret 4, takes the g_pathTick frame counter as its int arg).
struct CPathSubMgr {};

// A frame/tick counter (BSS, @0x6bf3bc) the sub-mgr Advance consumes. Already
// named g_6bf3bc in src/Gruntz/Projectile.cpp; re-declared here, address-pinned.
DATA(0x002bf3bc)
extern "C" u32 g_pathTick;

// The integer step seed the integrator scales by the per-frame speed
// (g_645584 = .data int). Already used as g_slimeFrameScale in KitchenSlime.cpp.
DATA(0x00245584)
extern i32 g_pathStepSeed; // VA 0x645584

// (The +0x108 new-leg seed at 0x645588 is the running game clock. It used to be
// re-declared here as `g_pathLegTag` under C++ linkage - a third name for a datum whose
// ONE definition is Projectile.cpp's extern-"C" g_645588, so ?g_pathLegTag@@3HA was an
// unresolved external. PathHazard.cpp now reads g_645588 directly.)

// 0.0 (the velocity-sign comparand). Already g_slimeZero in KitchenSlime.cpp.
DATA(0x001ea400)
extern const double g_pathZero; // VA 0x5ea400

// 0.03125 (= 1/32, the per-tile-time -> speed reciprocal scale).
DATA(0x001ea408)
extern const double g_pathTimeScale; // VA 0x5ea408

// 1.0 (the unit-vector numerator).
DATA(0x001ea410)
extern const double g_pathOne; // VA 0x5ea410

// The "B" bute key the new-leg path query passes (0x60d1bc) - the SAME rdata as
// CInGameIcon.h's g_iconBute; reuse the identical declaration so the reloc pairs.
DATA(0x0020d1bc)
extern char g_iconBute[]; // DAT_0060d1bc

// The global bute store (g_buteTree @0x6bf620; Find 0x16d190 __thiscall ret 4).
// Owned by another TU; declared extern so `ecx=&g_buteTree; call Find` masks.
#include <Bute/ButeMgr.h>
extern "C" CGameRegistry* g_gameReg; // *0x24556c canonical singleton
extern CButeTree g_buteTree;

// sqrt lowers inline (d9 fa); __ftol (0x11f570) lowers the (int) casts.
extern "C" i32 __ftol(); // 0x11f570 (declared so the call reloc-masks if needed)

// ---------------------------------------------------------------------------
// CPathHazard : CUserLogic - the path-following hazard. The inherited m_10/m_38
// (CUserLogic) hold the bound CGameObject; the hazard reads it directly. The
// leaf adds the movement-integrator state at +0x58 and the path/waypoint state
// at +0x90.. The CUserLogic base gives the +0x18 destructible link, so the dtor
// folds the shared teardown (the /GX leaf-dtor archetype).
// ---------------------------------------------------------------------------
class CPathHazard : public CUserLogic {
public:
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    virtual i32 UserLogicVfunc2() OVERRIDE;                            // slot 4
    // The vtable slot-4 (UserLogicVfunc2) activation dispatcher body (0x0b3b60;
    // shared by CUFO/CRainCloud); a plain method - the base placeholder blocks the
    // int-arg OVERRIDE spelling.
    i32 RunAct(i32 id);
    TILE_LOGIC_TAIL
public:
    CPathHazard(); // 0x13170 (no-arg deserialize-path ctor; zeroes the leg/strike i64s)
    CPathHazard(CGameObject* obj); // 0xb35a0 (folds CUserLogic(obj) + the waypoint setup)
    i32 StartPath();               // 0x29be thunk (find/seed the first leg; reloc-masked no-body)
    // GetTypeTag (0x132f0): the 6-byte per-class logic-type id accessor (0x425).
    // 0x000132f0 vtable slot 2: per-class logic-type id, inline (one
    // deduped COMDAT copy in retail; see docs on header-inline members).
    RVA(0x000132f0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_PATHHAZARD;
    }
    // The five virtuals CPathHazard adds over CUserLogic's 16 slots (16..20), so cl
    // emits the real 21-slot ??_7CPathHazard@@6B@ (CRainCloud/CUFO derive it). Tick
    // and BeginLeg carry bodies; slots 17/18/20 are declared-only (reloc-masked).
    virtual i32 Tick();        // slot 16 (body 0xb4020): the per-frame driver.
    virtual i32 SiblingTick(); // slot 17 (body 0xb43f0 - the timed strike driver)
    // Arm the strike-window timer (deadline = now, window = the RainCloudFlashTime bute),
    // fire the cue gate + the positional kill sound. Non-virtual. (Was the duplicate
    // CLightningHazard view's method.)
    i32 ArmStrike(i32 a, i32 b); // 0x0b4640
    // Arrive (slot 18, body 0xb47a0): advance to the next waypoint, wrapping the
    // index back to 0 once the path is exhausted. Returns 1.
    virtual i32 Arrive(); // slot 18
    // BeginLeg (slot 19, body 0xb47e0): compute the unit vector toward the current
    // waypoint (m_f8) and seed the movement state. Returns 1.
    virtual i32 BeginLeg();        // slot 19
    virtual i32 HitTest(i32, i32); // slot 20 (declared-only; per-frame hit test)
    // ForwardTick (0xb5070): a thin non-virtual forwarder to virtual slot 16 (Tick).
    // Tail-jumps `this->vtbl[16]()` through the raw vtable view (kept indirect).
    void ForwardTick(); // 0x0b5070 (out-of-line: tail-jump to Tick(), virtual slot 16)
    // INLINE (like ~CUserLogic): the derived leaves (CRainCloud/CUFO) must FOLD this
    // teardown into their own dtors - retail's ~CRainCloud @0x13340 is a flat CUserLogic
    // teardown, byte-identical to ~CPathHazard, with every intermediate vptr stamp
    // dead-store-eliminated. An out-of-line base dtor would emit `call ??1CPathHazard`
    // there instead. The out-of-line COMDAT the vtable dispatches to (0x13280) is pinned
    // by @rva-symbol in PathHazard.cpp (an inline dtor cannot hang an RVA()).
    virtual ~CPathHazard() OVERRIDE {} // slot 0 (COMDAT @0x13280)

    i32 m_savedGeoId; // +0x40  saved m_38->m_1b4 geometry id (before GAME_CYCLE100)
    char m_pad44[0x58 - 0x44];
    double m_speed;         // +0x58  per-frame speed (1 / (m_bc / 32))
    double m_posX;          // +0x60  sub-pixel X position accumulator
    double m_posY;          // +0x68  sub-pixel Y position accumulator
    double m_unitX;         // +0x70  unit-vector x (toward waypoint)
    double m_unitY;         // +0x78  unit-vector y
    double m_roundBiasX;    // +0x80  sign(unitX) * 0.5 (round-to-tile bias)
    double m_roundBiasY;    // +0x88  sign(unitY) * 0.5
    CPathWaypoint m_wp[13]; // +0x90  waypoint path (wp[0]=start, wp[1..12]=scaled)
    i32 m_wpIndex;          // +0xf8  current waypoint index
    i32 m_wpX;              // +0xfc  current waypoint X (int)
    i32 m_wpY;              // +0x100 current waypoint Y (int)
    i32 m_wpCount;          // +0x104 waypoint count (path length)
    // The leg/strike timers are REAL i64s (the strike gate compares them with signed
    // 64-bit arithmetic - see SiblingTick). The old split-i32 lo/hi spelling was the
    // artifact of the CLightningHazard dual-view, now folded in.
    i64 m_legDeadline; // +0x108 leg start-clock deadline
    i64 m_legWindow;   // +0x110 leg window duration
    i32 m_strikeArmed; // +0x118 strike-armed gate
    char m_pad11c[0x120 - 0x11c];
    i64 m_strikeDeadline; // +0x120 strike start-clock deadline
    i64 m_strikeWindow;   // +0x128 strike window duration
};
SIZE(CPathHazard, 0x130);
VTBL(CPathHazard, 0x001e7394); // vtable_names -> code (RTTI game class)

#endif // GRUNTZ_CPATHHAZARD_H

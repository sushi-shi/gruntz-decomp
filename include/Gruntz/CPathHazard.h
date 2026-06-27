// CPathHazard.h - the path-following hazard game object (C:\Proj\Gruntz).
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
#include <Gruntz/UserLogic.h>

// The waypoint the path array (this+0x90, 8-byte stride) stores: {x:int, y:int}.
struct CPathWaypoint {
    i32 x; // +0x00
    i32 y; // +0x04
};

// The +0x198 layer descriptor the screen-rect bounds poll: +0x18 base X, +0x1c
// base Y. The bound CGameObject's m_5c/m_60 are screen coords relative to it.
struct CPathLayer {
    char m_pad00[0x18];
    i32 m_18; // +0x18 layer base X
    i32 m_1c; // +0x1c layer base Y
};

// The bound CGameObject viewed by the hazard (this->m_10 == this->m_38). Only
// the touched offsets are modeled; it overlays the CGameObject from UserLogic.h
// (the bodies reinterpret m_10/m_38 at each use, codegen-neutral).
struct CPathObj {
    char m_pad00[0x5c];
    i32 m_5c; // +0x5c  current tile X (the integrator writes back here)
    i32 m_60; // +0x60  current tile Y
    char m_pad64[0x7c - 0x64];
    CPathObj* m_7c; // +0x7c  the per-tile-time owner (m_7c->m_bc)
    char m_pad80[0xbc - 0x80];
    i32 m_bc; // +0xbc  per-tile time (the speed reciprocal numerator)
    char m_padc0[0x120 - 0xc0];
    i32 m_120; // +0x120 leg/segment count remaining
    i32 m_124; // +0x124
    char m_pad128[0x144 - 0x128];
    i32 m_144; // +0x144 the on-screen rect base (Tick passes &m_144 to QueryAt)
    char m_pad148[0x198 - 0x148];
    CPathLayer* m_198; // +0x198 the layer descriptor
};

// The entity QueryAt returns; +0x258 is its type/state tag (0x38 == this hazard
// itself, so its own footprint is ignored).
struct CPathEntity {
    char m_pad00[0x258];
    i32 m_258; // +0x258
};

// The visibility / cue gate reached as g_gameReg->m_68; QueryAt resolves the
// entity under a screen rect. Same shared engine fn (0x75c60) KitchenSlime uses;
// modeled NO-body so the call reloc-masks.
struct CPathCueGate {
    void* QueryAt(i32 x, i32 y, i32* rect, i32* outA, i32* outB, i32* outC); // 0x75c60
    void Strike(i32 a, i32 b, i32 c, i32 d); // 0x402e96 (the strike cue, __thiscall)
};

struct CPathGameReg {
    char m_pad00[0x68];
    CPathCueGate* m_68; // +0x68 visibility/cue gate
    char m_pad6c[0x118 - 0x6c];
    i32 m_118; // +0x118 has-window flag
    char m_pad11c[0x134 - 0x11c];
    i32 m_134; // +0x134 mode discriminator (==1 -> skip the visibility scroll)
};
DATA(0x0024556c)
extern CPathGameReg* g_pathGameReg;

// The +0x1a0 sub-mgr the per-frame Tick advances once (SetGeoSource 0x15c360,
// __thiscall ret 4, takes the g_pathTick frame counter as its int arg).
struct CPathSubMgr {
    void Advance(i32 tick); // 0x15c360
};

// A frame/tick counter (BSS, @0x6bf3bc) the sub-mgr Advance consumes. Already
// named g_6bf3bc in src/Gruntz/Projectile.cpp; re-declared here, address-pinned.
DATA(0x002bf3bc)
extern "C" u32 g_pathTick;

// The integer step seed the integrator scales by the per-frame speed
// (g_645584 = .data int). Already used as g_slimeFrameScale in KitchenSlime.cpp.
DATA(0x00245584)
extern i32 g_pathStepSeed; // VA 0x645584

// A bute-type tag (g_645588 = .data int) stashed into +0x108 when a new leg with
// remaining segments begins.
DATA(0x00245588)
extern i32 g_pathLegTag; // VA 0x645588

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
extern CButeTree g_buteTree;

// sqrt lowers inline (d9 fa); __ftol (0x11f570) lowers the (int) casts.
extern "C" i32 __ftol(); // 0x11f570 (declared so the call reloc-masks if needed)

// ---------------------------------------------------------------------------
// CPathHazard : CUserLogic - the path-following hazard. The inherited m_10/m_38
// (CUserLogic) hold the bound CGameObject; the hazard reads it as CPathObj. The
// leaf adds the movement-integrator state at +0x58 and the path/waypoint state
// at +0x90.. The CUserLogic base gives the +0x18 destructible link, so the dtor
// folds the shared teardown (the /GX leaf-dtor archetype).
// ---------------------------------------------------------------------------
class CPathHazard : public CUserLogic {
public:
    // GetTypeTag (0x132f0): the 6-byte per-class logic-type id accessor (0x425).
    i32 GetTypeTag();
    // Tick (virtual slot 16, body 0xb4020): the per-frame driver.
    i32 Tick();
    // BeginLeg (virtual slot 19, body 0xb47e0): compute the unit vector toward
    // the current waypoint (m_f8) and seed the movement state. Returns 1.
    i32 BeginLeg();
    // ForwardTick (0xb5070): a thin non-virtual forwarder to the virtual slot 16
    // (Tick). Tail-jumps `this->vtbl[16]()`.
    void ForwardTick();
    ~CPathHazard(); // 0x13340 (folds the CUserLogic teardown)

    char m_pad40[0x58 - 0x40];
    double m_58; // +0x58  per-frame speed (1 / (m_bc * 1/32))
    double m_60; // +0x60  accumulated x (double)
    double m_68; // +0x68  accumulated y (double)
    double m_70; // +0x70  unit-vector x
    double m_78; // +0x78  unit-vector y
    double m_80; // +0x80  sign(ux) * 0.5  (the round-to-tile bias)
    double m_88; // +0x88  sign(uy) * 0.5
    char m_pad90[0xf8 - 0x90];
    i32 m_f8;  // +0xf8  current waypoint index
    i32 m_fc;  // +0xfc  current waypoint X (int)
    i32 m_100; // +0x100 current waypoint Y (int)
    char m_pad104[0x108 - 0x104];
    i32 m_108; // +0x108 leg bute tag
    i32 m_10c; // +0x10c
    i32 m_110; // +0x110 leg segments remaining
    i32 m_114; // +0x114
};

// The waypoint path array at this+0x90 (8-byte stride). Accessed as
// ((CPathWaypoint*)((char*)this + 0x90))[m_f8].
#define PATH_WAYPOINTS(self) ((CPathWaypoint*)((char*)(self) + 0x90))

#endif // GRUNTZ_CPATHHAZARD_H

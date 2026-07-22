#ifndef GRUNTZ_CPATHHAZARD_H
#define GRUNTZ_CPATHHAZARD_H

#include <rva.h>

#include <Gruntz/GameRegistry.h>
#include <Gruntz/LogicTypeId.h> // LogicTypeId (GetTypeTag return type)
#include <Gruntz/UserLogic.h>

struct CPathWaypoint {
    i32 x; // +0x00
    i32 y; // +0x04
};
SIZE_UNKNOWN();

struct CPathEntity {
    char m_pad00[0x258];
    i32 m_258; // +0x258
};
SIZE_UNKNOWN();

struct CPathSubMgr {};
SIZE_UNKNOWN();

extern "C" u32 g_engineFrameDelta;

extern "C" i32 g_frameDelta; // VA 0x645584

DATA(0x001ea400)
extern const double g_pathZero; // VA 0x5ea400

DATA(0x001ea408)
extern const double g_pathTimeScale; // VA 0x5ea408

DATA(0x001ea410)
extern const double g_pathOne; // VA 0x5ea410

#include <Bute/ButeMgr.h>
#include <Gruntz/GameRegMfcPtr.h> // g_gameReg at its REAL type (CGruntzMgr)

extern "C" i32 __ftol(); // 0x11f570 (declared so the call reloc-masks if needed)

class CPathHazard : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    // The vtable slot-4 (UserLogicVfunc2) activation dispatcher body (0x0b3b60;
    // shared by CUFO/CRainCloud); a plain method - the base placeholder blocks the
    // int-arg OVERRIDE spelling.
    virtual void FireActivation(i32 id) OVERRIDE;
public:
    CPathHazard(); // 0x13170 (no-arg deserialize-path ctor; zeroes the leg/strike i64s)
    CPathHazard(CGameObject* obj); // 0xb35a0 (folds CUserLogic(obj) + the waypoint setup)
    i32 StartPath();               // 0x29be thunk (find/seed the first leg; reloc-masked no-body)
    // GetTypeTag (0x13210, ??_7CPathHazard slot 2 -> this body): the 6-byte
    // per-class logic-type id accessor (0x424). 0x132f0 (the old binding) is
    // CRainCloud's - the derived vtables each hold their OWN 6-byte copy.
    RVA(0x00013210, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_PATHHAZARD;
    }
    // The five virtuals CPathHazard adds over CUserLogic's 16 slots (16..20), so cl
    // emits the real 21-slot ??_7CPathHazard@@6B@ (CRainCloud/CUFO derive it). Tick
    // and BeginLeg carry bodies; slots 17/18/20 are declared-only (reloc-masked).
    virtual i32 Tick();        // slot 16 (body 0xb4020): the per-frame driver.
    virtual i32 SiblingTick(); // slot 17 (body 0xb43f0 - the timed strike driver)
    // (The 0xb4640 strike-arm body that used to be declared here as a non-virtual
    //  "ArmStrike" is CRainCloud's slot-20 HitTest override - ??_7CRainCloud[20]
    //  is its only referent; see RainCloud.cpp.)
    // Arrive (slot 18, body 0xb47a0): advance to the next waypoint, wrapping the
    // index back to 0 once the path is exhausted. Returns 1.
    virtual i32 Arrive(); // slot 18
    // BeginLeg (slot 19, body 0xb47e0): compute the unit vector toward the current
    // waypoint (m_f8) and seed the movement state. Returns 1.
    virtual i32 BeginLeg();        // slot 19
    // slot 20 (0x13230, ??_7CPathHazard[20] -> this body): the per-coord hit
    // probe's base default - `return 1`. CUFO inherits it; CRainCloud overrides
    // it with the strike-arm body @0xb4640.
    RVA(0x00013230, 0x8)
    virtual i32 HitTest(i32, i32) {
        return 1;
    }
    // ForwardTick (0xb5070): a thin non-virtual forwarder to virtual slot 16 (Tick).
    // Tail-jumps `this->vtbl[16]()` through the raw vtable view (kept indirect).
    void ForwardTick(); // 0x0b5070 (out-of-line: tail-jump to Tick(), virtual slot 16)
    // NO user-declared dtor: retail 0x13280 is the COMPILER-GENERATED one (implicit
    // elides the leaf-vptr restamp a user `{}` would emit now that the CWapX base EH
    // state blocks the old dead-store elision). Still implicitly-inline, so the derived
    // leaves (CRainCloud/CUFO) FOLD the teardown; the out-of-line COMDAT keeps its
    // RVA_COMPGEN pin in PathHazard.cpp.
    char m_pad54[0x58 - 0x54];
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
SIZE(0x130);

struct CPathHazardActEntry {
    i32 (CUserLogic::*m_fn)();
};
SIZE_UNKNOWN();

#endif // GRUNTZ_CPATHHAZARD_H

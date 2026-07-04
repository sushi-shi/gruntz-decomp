// RollingBall.h - the rolling-ball hazard game object (CRollingBall : CUserLogic,
// vftable 0x5e86fc, sizeof 0xa0). A CUserLogic leaf: its own state begins after
// the shared CUserLogic base region (ends +0x40) and runs to +0xa0.
//
// Modeled polymorphically (like every UserLogic leaf) so the empty dtor folds to
// the bare CUserLogic teardown (store the CUserLogic vptr 0x5e705c, ~EngStr on the
// +0x18 link, store the CUserBase vptr 0x5e70b4) under a /GX frame, the shape every
// UserLogic leaf dtor matches. Serialize is the slot-1 override (modeled as a plain
// method so its ?Serialize@...@@QAE.. name + RVA pin; the vtable slot is reloc-
// masked, the same way CSecretTeleporterTrigger::Serialize is wired).
//
// Field names are placeholders (m_<hexoffset>); only the OFFSETS + code bytes are
// load-bearing (campaign doctrine). Layout recovered from Update (0xb0140) and
// Serialize (0xb0fe0): +0x58 double (per-tile time), +0x60/+0x68 double (sub-tile
// pos), +0x70/+0x74 int (step dir), +0x78/+0x7c int (target tile coords), +0x80/
// +0x84 latches, +0x88..+0x94 (explosion timing), +0x98/+0x9c (move delta).
#ifndef GRUNTZ_GRUNTZ_ROLLINGBALL_H
#define GRUNTZ_GRUNTZ_ROLLINGBALL_H

#include <rva.h>

#include <Mfc.h> // CObject base + the two CString diagnostic temps in Update (/GX)

#include <Gruntz/UserLogic.h> // CUserLogic : CUserBase, EngStr, CGameObject

#include <Gruntz/CSerialObjRef.h> // the shared +0x34 serialized-object-reference (Chain @0x8c00)

// The CArchive-like serializer the record is streamed through (Serialize's arg1) is
// the shared WAP32 CSerialArchive (Read @ vtable +0x2c / Write @ +0x30), pulled in via
// <Gruntz/CSerialObjRef.h> above - the former local `CRbArchive` view is folded away.

// ---------------------------------------------------------------------------
// CRollingBall : CUserLogic (vftable 0x5e86fc). Own state from +0x40 onward.
// The dtor (0x12f80) adds no destructible members, so it folds the bare
// CUserLogic teardown.
// ---------------------------------------------------------------------------
class CRollingBall : public CUserLogic {
public:
    CRollingBall(CGameObject* obj);   // 0x0af820 (folds CUserLogic(obj) + the ball setup)
    virtual ~CRollingBall() OVERRIDE; // 0x012f80 (folds the bare CUserLogic teardown)

    // Construct the class's activation-coordinate registry (g_rollingBallActReg
    // @0x6461b0) over the fixed [2000,2010] range; free init thunk, reloc-masked.
    static void InitActReg(); // 0x0afd60
    // Bind the per-frame handler (Update) to the activation key "A" via the shared
    // name registry (the same archetype as CBehindCandyAni::RegisterActs).
    static void RegisterActs(); // 0x0aff40

    i32 Serialize(CSerialArchive* ar, i32 tag, i32 c, i32 d); // 0x0b0fe0 (vtable slot 1)
    i32 Update();                                             // 0x0b0140

    // --- CRollingBall own fields (offsets load-bearing) ---
    i32 m_savedGeoId;          // +0x40  saved m_38->m_1b4 geometry id
    char m_pad44[0x58 - 0x44]; // CUserLogic ends +0x40
    double m_moveSpeed;        // +0x58  per-frame speed (numerator / RollingBallTimePerTile)
    double m_subX;             // +0x60  sub-tile X position
    double m_subY;             // +0x68  sub-tile Y position
    i32 m_stepDirX;            // +0x70  X step direction (-1/0/1)
    i32 m_stepDirY;            // +0x74  Y step direction (-1/0/1)
    i32 m_targetX;             // +0x78  target tile X (<<5)
    i32 m_targetY;             // +0x7c  target tile Y (<<5)
    i32 m_explodeLatch;        // +0x80  explosion one-shot latch
    i32 m_fallLatch;           // +0x84  fall one-shot latch
    i32 m_explodeStartLo;      // +0x88  explosion start clock (i64 lo)
    i32 m_explodeStartHi;      // +0x8c  explosion start clock (i64 hi)
    i32 m_explodeWindowLo;     // +0x90  explosion window (i64 lo)
    i32 m_explodeWindowHi;     // +0x94  explosion window (i64 hi)
    i32 m_moveDeltaLo;         // +0x98  move delta (i64 lo)
    i32 m_moveDeltaHi;         // +0x9c  move delta (i64 hi)
};

SIZE(CRollingBall, 0xa0);

#endif // GRUNTZ_GRUNTZ_ROLLINGBALL_H

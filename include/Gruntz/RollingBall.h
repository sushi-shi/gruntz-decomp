#ifndef GRUNTZ_GRUNTZ_ROLLINGBALL_H
#define GRUNTZ_GRUNTZ_ROLLINGBALL_H

#include <rva.h>

#include <Mfc.h> // CObject base + the two CString diagnostic temps in Update (/GX)

#include <Gruntz/UserLogic.h> // CUserLogic : CUserBase, EngStr, CGameObject

#include <Gruntz/SerialArchive.h> // CFileMemBase (the inherited CWapX::Chain arg; ex SerialObjRef.h)

class CFileMemBase;

class CRollingBall : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeMove(CFileMemBase*, i32, i32, i32) OVERRIDE; // slot 1
    RVA(0x00012f30, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_ROLLINGBALL;
    } // slot 2
public:
    CRollingBall(CGameObject* obj); // 0x0af820 (folds CUserLogic(obj) + the ball setup)
    // NO user-declared dtor: retail's is COMPILER-GENERATED (implicit
    // elides the leaf-vptr restamp; RVA_COMPGEN pin in the home TU).

    // Construct the class's activation-coordinate registry (g_rollingBallActReg
    // @0x6461b0) over the fixed [2000,2010] range; free init thunk, reloc-masked.
    // Resolve the registry entry for id; run its bound handler as a PMF on this
    // (ResolveEntry inlined twice). 0x0afde0.
    virtual void FireActivation(i32 id) OVERRIDE;
    // Bind the per-frame handler (Update) to the activation key "A" via the shared
    // name registry (the same archetype as CBehindCandyAni::RegisterActs).
    static void RegisterActs(); // 0x0aff40

    i32 Update(); // 0x0b0140

    // --- CRollingBall own fields (offsets load-bearing) ---
    char m_pad54[0x58 - 0x54]; // CUserLogic ends +0x40
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
SIZE(0xa0);

typedef i32 (CUserLogic::*RollingBallHandler)();
struct CRollingBallActEntry {
    RollingBallHandler m_fn;
};
SIZE_UNKNOWN();

#include <Gruntz/ActReg.h> // CActReg (extern below)

// TU-local thunk/table names this TU registers (moved from the .cpp; the
// addresses are ILT thunk VAs, reloc-masked at every use).
extern "C" i32 __ftol(double x); // 0x11f570

#endif // GRUNTZ_GRUNTZ_ROLLINGBALL_H

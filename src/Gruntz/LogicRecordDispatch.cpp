// LogicRecordDispatch.cpp - two trace-discovered __cdecl logic-record state
// dispatchers re-homed from src/Stub/Discovered.cpp (matcher-1, traced under
// the worker record). Each takes a game owner, reaches the embedded worker
// record at owner->m_7c (the canonical AnimWorkerObj), and dispatches on its
// state tag (m_1c) to the bound CUserLogic leaf's vtable - except state 0,
// which lazily constructs the leaf. The dispatchers differ only in the leaf
// type built by state 0. Offsets + code bytes are load-bearing.
//
// (2026-07-13 worker fold: the former LogicSubRec/LogicDispatchRecord/
// LogicDispatchOwner views are DISSOLVED - LogicSubRec was a 16-slot lens of
// the real CUserLogic vtable (Init = Activate slot 6, Op1e/Op1d/Op52/Op51/
// Op50/Op53 = UserLogicVfunc8/9/A/B/C/D slots 10-15), the record was the
// canonical AnimWorkerObj, the owner the canonical CGameObject.)
#include <Ints.h>
#include <rva.h>

#include <Gruntz/StaticHazard.h> // CStaticHazard (state-0 leaf of LogicDispatchA, ctor 0xfb7a0)
#include <Gruntz/TimeBomb.h>     // CTimeBomb    (state-0 leaf of LogicDispatchD, ctor 0xe1b90)
#include <Gruntz/Projectile.h> // CProjectile  (state-0 leaf of LogicDispatchE, ctor 0xdec60, size 0x228)
#include <Gruntz/Boomerang.h> // CBoomerang   (state-0 leaf of LogicDispatchBoomerang, ctor 0xe0650, size 0x260)
#include <Gruntz/XferArchive.h> // ProjTypeXfer (0x16e4f0) = the default-case fall-through
#include <Gruntz/UserLogic.h>   // CGameObject + CUserLogic (the bound leaf) + AnimWorkerObj

// global operator new (engine NAFXCW _RezAlloc @0x1b9b46); external/no-body.
void* operator new(u32 n);

// The record's state tag (m_1c, an int|ptr role-union field - the int states cast).
// State kLogicStateInit lazily builds the leaf then latches kLogicStateBuilt; each
// Op state dispatches the matching CUserLogic virtual; kLogicStateBuilt is the
// built/idle no-op. Same immediates as the bare labels -> naming is matching-neutral.
// The switch key is (u32)m_1c so it keeps its unsigned ja/jbe codegen.
enum LogicRecordState {
    kLogicStateInit = 0,      // build the leaf, then -> kLogicStateBuilt
    kLogicStateOp1d = 0x1d,   // dispatch UserLogicVfunc9 (slot 11)
    kLogicStateOp1e = 0x1e,   // dispatch UserLogicVfunc8 (slot 10)
    kLogicStateOp50 = 0x50,   // dispatch UserLogicVfuncC (slot 14)
    kLogicStateOp51 = 0x51,   // dispatch UserLogicVfuncB (slot 13)
    kLogicStateOp52 = 0x52,   // dispatch UserLogicVfuncA (slot 12)
    kLogicStateOp53 = 0x53,   // dispatch UserLogicVfuncD (slot 15)
    kLogicStateBuilt = 0x3e8, // leaf built / idle (no-op)
};

// State-0 leaf types (built lazily) are all real engine classes, `new`'d directly
// through their shared headers so the ctor CALL binds to the retail RVA: LogicDispatchA
// -> CStaticHazard (0x6c, ctor 0xfb7a0), D -> CTimeBomb (0x68, ctor 0xe1b90),
// E -> CProjectile (0x228, ctor 0xdec60), Boomerang -> CBoomerang (0x260, ctor 0xe0650).
//
// LogicSubRecB (the state-0 sub-record built by LogicDispatchB@0x10d3d0) is really CBrickz
// (ctor thunk 0x3701 -> 0x10e800); LogicDispatchB was folded into src/Gruntz/TileLogicPump.cpp
// (waveM-strays: it sits inside the tile-trigger obj's contiguous .text block) and modeled on
// the real CBrickz, so the LogicSubRecB view is gone.

// The default-case fall-through helper IS the real shared type-registry resolve at
// 0x16e4f0 (?ProjTypeXfer@@YAHPAUCXferArchive@@@Z, __cdecl). Thin forwarder so the
// callers emit the bound rel32 (was the fake, UNBOUND _LogicSubDefault_16e4f0).
inline void LogicSubDefault_16e4f0(CUserLogic* sub) {
    ProjTypeXfer((CXferArchive*)sub);
}

// LogicDispatchE @0x0de8a0 - state-0 builds a CProjectile (0x228, ctor 0xdec60).
// The `new CProjectile` size push is the imm32 0x228 (3 bytes wider than the
// imm8-size CTimeBomb/CStaticHazard siblings).
RVA(0x000de8a0, 0xf4)
i32 LogicDispatchE(CGameObject* owner) {
    AnimWorkerObj* rec = owner->m_7c;
    switch ((u32)rec->m_1c) {
        case kLogicStateInit:
            rec->m_1c = (void*)kLogicStateBuilt;
            {
                CUserLogic* obj = new CProjectile(owner);
                obj->Activate();
                rec->m_logic = obj;
            }
            break;
        case kLogicStateOp1d:
            rec->m_logic->UserLogicVfunc9();
            break;
        case kLogicStateOp1e:
            rec->m_logic->UserLogicVfunc8();
            break;
        case kLogicStateOp50:
            rec->m_logic->UserLogicVfuncC();
            break;
        case kLogicStateOp51:
            rec->m_logic->UserLogicVfuncB();
            break;
        case kLogicStateOp52:
            rec->m_logic->UserLogicVfuncA();
            break;
        case kLogicStateOp53:
            rec->m_logic->UserLogicVfuncD();
            break;
        case kLogicStateBuilt:
            break;
        default:
            LogicSubDefault_16e4f0(rec->m_logic);
            break;
    }
    return 1;
}

// LogicDispatchBoomerang @0x0de9e0 - state-0 builds a CBoomerang (0x260, ctor 0xe0650 =
// ??0CBoomerang@@QAE@PAUCGameObject@@@Z, CBoomerang : CProjectile). Same dispatch shape
// as the siblings; the `new CBoomerang` size push is the imm32 0x260. Spatially re-homed
// from src/Stub/DiscoveredEh.cpp (was BoomerangCmdDispatch_de9e0). The default handler
// ProjTypeXfer @0x16e4f0 IS LogicSubDefault_16e4f0.
// @identity-TODO: the dispatcher's own owner class is unrecovered (only inbound edge
// is ILT thunk 0x158c from an unrecovered fn).
RVA(0x000de9e0, 0xf4)
i32 LogicDispatchBoomerang(CGameObject* owner) {
    AnimWorkerObj* rec = owner->m_7c;
    switch ((u32)rec->m_1c) {
        case kLogicStateInit:
            rec->m_1c = (void*)kLogicStateBuilt;
            {
                CUserLogic* obj = new CBoomerang(owner);
                obj->Activate();
                rec->m_logic = obj;
            }
            break;
        case kLogicStateOp1d:
            rec->m_logic->UserLogicVfunc9();
            break;
        case kLogicStateOp1e:
            rec->m_logic->UserLogicVfunc8();
            break;
        case kLogicStateOp50:
            rec->m_logic->UserLogicVfuncC();
            break;
        case kLogicStateOp51:
            rec->m_logic->UserLogicVfuncB();
            break;
        case kLogicStateOp52:
            rec->m_logic->UserLogicVfuncA();
            break;
        case kLogicStateOp53:
            rec->m_logic->UserLogicVfuncD();
            break;
        case kLogicStateBuilt:
            break;
        default:
            LogicSubDefault_16e4f0(rec->m_logic);
            break;
    }
    return 1;
}

// LogicDispatchD @0x0deb20 - state-0 builds a CTimeBomb (0x68, ctor 0xe1b90).
RVA(0x000deb20, 0xf1)
i32 LogicDispatchD(CGameObject* owner) {
    AnimWorkerObj* rec = owner->m_7c;
    switch ((u32)rec->m_1c) {
        case kLogicStateInit:
            rec->m_1c = (void*)kLogicStateBuilt;
            {
                CUserLogic* obj = new CTimeBomb(owner);
                obj->Activate();
                rec->m_logic = obj;
            }
            break;
        case kLogicStateOp1d:
            rec->m_logic->UserLogicVfunc9();
            break;
        case kLogicStateOp1e:
            rec->m_logic->UserLogicVfunc8();
            break;
        case kLogicStateOp50:
            rec->m_logic->UserLogicVfuncC();
            break;
        case kLogicStateOp51:
            rec->m_logic->UserLogicVfuncB();
            break;
        case kLogicStateOp52:
            rec->m_logic->UserLogicVfuncA();
            break;
        case kLogicStateOp53:
            rec->m_logic->UserLogicVfuncD();
            break;
        case kLogicStateBuilt:
            break;
        default:
            LogicSubDefault_16e4f0(rec->m_logic);
            break;
    }
    return 1;
}

// LogicDispatchA @0x0fb660 - state-0 builds a CStaticHazard (0x6c, ctor 0xfb7a0).
RVA(0x000fb660, 0xf1)
i32 LogicDispatchA(CGameObject* owner) {
    AnimWorkerObj* rec = owner->m_7c;
    switch ((u32)rec->m_1c) {
        case kLogicStateInit:
            rec->m_1c = (void*)kLogicStateBuilt;
            {
                CUserLogic* obj = new CStaticHazard(owner);
                obj->Activate();
                rec->m_logic = obj;
            }
            break;
        case kLogicStateOp1d:
            rec->m_logic->UserLogicVfunc9();
            break;
        case kLogicStateOp1e:
            rec->m_logic->UserLogicVfunc8();
            break;
        case kLogicStateOp50:
            rec->m_logic->UserLogicVfuncC();
            break;
        case kLogicStateOp51:
            rec->m_logic->UserLogicVfuncB();
            break;
        case kLogicStateOp52:
            rec->m_logic->UserLogicVfuncA();
            break;
        case kLogicStateOp53:
            rec->m_logic->UserLogicVfuncD();
            break;
        case kLogicStateBuilt:
            break;
        default:
            LogicSubDefault_16e4f0(rec->m_logic);
            break;
    }
    return 1;
}

// LogicDispatchC @0x046850 (state-0 news a CPARTICLEZ: ctor thunk 0x2a04 ->
// 0x46ad0 == ??0CParticlez) was re-homed to FortressFlag.cpp (wave3-I): its
// retail body is text-contained in the ff+particlez+explosion obj.
// LogicSubRecC (its former local sub-record view) went with it, dissolved
// onto the real CParticlez.

// LogicDispatchB @0x10d3d0 (state-0 news a CBrickz: ctor thunk 0x3701 -> 0x10e800) was folded
// into src/Gruntz/TileLogicPump.cpp (waveM-strays): its retail body is text-contained in the
// tile-trigger logic obj's contiguous first-link .text block, and it is CBrickz's state pump.

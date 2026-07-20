#include <Ints.h>
#include <rva.h>

#include <Gruntz/StaticHazard.h> // CStaticHazard (state-0 leaf of LogicDispatchA, ctor 0xfb7a0)
#include <Gruntz/TimeBomb.h>     // CTimeBomb    (state-0 leaf of LogicDispatchD, ctor 0xe1b90)
#include <Gruntz/Projectile.h> // CProjectile  (state-0 leaf of LogicDispatchE, ctor 0xdec60, size 0x228)
#include <Gruntz/Boomerang.h> // CBoomerang   (state-0 leaf of LogicDispatchBoomerang, ctor 0xe0650, size 0x260)
#include <Gruntz/XferArchive.h> // ProjTypeXfer (0x16e4f0) = the default-case fall-through

void* operator new(u32 n);

#include <Gruntz/UserLogic.h>

enum LogicRecordState {
    kLogicStateInit = 0,      // build the sub-record, then -> kLogicStateBuilt
    kLogicStateOp1d = 0x1d,   // dispatch LogicSubRec::Op1d()
    kLogicStateOp1e = 0x1e,   // dispatch LogicSubRec::Op1e()
    kLogicStateOp50 = 0x50,   // dispatch LogicSubRec::Op50()
    kLogicStateOp51 = 0x51,   // dispatch LogicSubRec::Op51()
    kLogicStateOp52 = 0x52,   // dispatch LogicSubRec::Op52()
    kLogicStateOp53 = 0x53,   // dispatch LogicSubRec::Op53()
    kLogicStateBuilt = 0x3e8, // sub-record built / idle (no-op)
};

inline void LogicSubDefault_16e4f0(CUserLogic* sub) {
    ProjTypeXfer(sub);
}

RVA(0x000de8a0, 0xf4)
i32 LogicDispatchE(CGameObject* owner) {
    AnimWorkerObj* rec = owner->m_7c;
    switch (reinterpret_cast<u32>(rec->m_1c)) { // m_1c: the documented int|ptr union (unsigned key -> ja/jbe)
        case kLogicStateInit:
            rec->m_1c = reinterpret_cast<void*>(kLogicStateBuilt);
            {
                CUserLogic* obj = new CProjectile(owner);
                obj->Activate(); // [6]
                rec->m_logic = obj;
            }
            break;
        case kLogicStateOp1d:
            rec->m_logic->UserLogicVfunc9(); // [11] +0x2c
            break;
        case kLogicStateOp1e:
            rec->m_logic->UserLogicVfunc8(); // [10] +0x28
            break;
        case kLogicStateOp50:
            rec->m_logic->UserLogicVfuncC(); // [14] +0x38
            break;
        case kLogicStateOp51:
            rec->m_logic->UserLogicVfuncB(); // [13] +0x34
            break;
        case kLogicStateOp52:
            rec->m_logic->UserLogicVfuncA(); // [12] +0x30
            break;
        case kLogicStateOp53:
            rec->m_logic->UserLogicVfuncD(); // [15] +0x3c
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
// from src/Stub/DiscoveredEh.cpp (was BoomerangCmdDispatch_de9e0). The LogicSubRec view
// is CBoomerang's shared vtable lens (Init = slot 6); the default handler ProjTypeXfer
// @0x16e4f0 IS LogicSubDefault_16e4f0.
// @identity-TODO: the dispatcher's own owner class is unrecovered (only inbound edge
// is ILT thunk 0x158c from an unrecovered fn).
RVA(0x000de9e0, 0xf4)
i32 LogicDispatchBoomerang(CGameObject* owner) {
    AnimWorkerObj* rec = owner->m_7c;
    switch (reinterpret_cast<u32>(rec->m_1c)) { // m_1c: the documented int|ptr union (unsigned key -> ja/jbe)
        case kLogicStateInit:
            rec->m_1c = reinterpret_cast<void*>(kLogicStateBuilt);
            {
                CUserLogic* obj = new CBoomerang(owner);
                obj->Activate(); // [6]
                rec->m_logic = obj;
            }
            break;
        case kLogicStateOp1d:
            rec->m_logic->UserLogicVfunc9(); // [11] +0x2c
            break;
        case kLogicStateOp1e:
            rec->m_logic->UserLogicVfunc8(); // [10] +0x28
            break;
        case kLogicStateOp50:
            rec->m_logic->UserLogicVfuncC(); // [14] +0x38
            break;
        case kLogicStateOp51:
            rec->m_logic->UserLogicVfuncB(); // [13] +0x34
            break;
        case kLogicStateOp52:
            rec->m_logic->UserLogicVfuncA(); // [12] +0x30
            break;
        case kLogicStateOp53:
            rec->m_logic->UserLogicVfuncD(); // [15] +0x3c
            break;
        case kLogicStateBuilt:
            break;
        default:
            LogicSubDefault_16e4f0(rec->m_logic);
            break;
    }
    return 1;
}

RVA(0x000deb20, 0xf1)
i32 LogicDispatchD(CGameObject* owner) {
    AnimWorkerObj* rec = owner->m_7c;
    switch (reinterpret_cast<u32>(rec->m_1c)) { // m_1c: the documented int|ptr union (unsigned key -> ja/jbe)
        case kLogicStateInit:
            rec->m_1c = reinterpret_cast<void*>(kLogicStateBuilt);
            {
                CUserLogic* obj = new CTimeBomb(owner);
                obj->Activate(); // [6]
                rec->m_logic = obj;
            }
            break;
        case kLogicStateOp1d:
            rec->m_logic->UserLogicVfunc9(); // [11] +0x2c
            break;
        case kLogicStateOp1e:
            rec->m_logic->UserLogicVfunc8(); // [10] +0x28
            break;
        case kLogicStateOp50:
            rec->m_logic->UserLogicVfuncC(); // [14] +0x38
            break;
        case kLogicStateOp51:
            rec->m_logic->UserLogicVfuncB(); // [13] +0x34
            break;
        case kLogicStateOp52:
            rec->m_logic->UserLogicVfuncA(); // [12] +0x30
            break;
        case kLogicStateOp53:
            rec->m_logic->UserLogicVfuncD(); // [15] +0x3c
            break;
        case kLogicStateBuilt:
            break;
        default:
            LogicSubDefault_16e4f0(rec->m_logic);
            break;
    }
    return 1;
}

RVA(0x000fb660, 0xf1)
i32 LogicDispatchA(CGameObject* owner) {
    AnimWorkerObj* rec = owner->m_7c;
    switch (reinterpret_cast<u32>(rec->m_1c)) { // m_1c: the documented int|ptr union (unsigned key -> ja/jbe)
        case kLogicStateInit:
            rec->m_1c = reinterpret_cast<void*>(kLogicStateBuilt);
            {
                CUserLogic* obj = new CStaticHazard(owner);
                obj->Activate(); // [6]
                rec->m_logic = obj;
            }
            break;
        case kLogicStateOp1d:
            rec->m_logic->UserLogicVfunc9(); // [11] +0x2c
            break;
        case kLogicStateOp1e:
            rec->m_logic->UserLogicVfunc8(); // [10] +0x28
            break;
        case kLogicStateOp50:
            rec->m_logic->UserLogicVfuncC(); // [14] +0x38
            break;
        case kLogicStateOp51:
            rec->m_logic->UserLogicVfuncB(); // [13] +0x34
            break;
        case kLogicStateOp52:
            rec->m_logic->UserLogicVfuncA(); // [12] +0x30
            break;
        case kLogicStateOp53:
            rec->m_logic->UserLogicVfuncD(); // [15] +0x3c
            break;
        case kLogicStateBuilt:
            break;
        default:
            LogicSubDefault_16e4f0(rec->m_logic);
            break;
    }
    return 1;
}

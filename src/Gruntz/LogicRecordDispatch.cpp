// LogicRecordDispatch.cpp - two trace-discovered __cdecl logic-record state
// dispatchers re-homed from src/Stub/Discovered.cpp (matcher-1, traced under
// CLogicRecord). Each takes a game owner, reaches the embedded logic record at
// owner+0x7c, and dispatches on its state tag (m_1c) to the polymorphic
// sub-record's vtable - except state 0, which lazily constructs the sub-record.
// The two differ only in the sub-record type built by state 0 (size 0x6c ctor
// 0x2c70 vs 0x54 ctor 0x3701). Offsets + code bytes are load-bearing.
#include <Ints.h>
#include <rva.h>

#include <Gruntz/StaticHazard.h> // CStaticHazard (state-0 leaf of LogicDispatchA, ctor 0xfb7a0)
#include <Gruntz/TimeBomb.h>     // CTimeBomb    (state-0 leaf of LogicDispatchD, ctor 0xe1b90)
#include <Gruntz/Projectile.h> // CProjectile  (state-0 leaf of LogicDispatchE, ctor 0xdec60, size 0x228)
#include <Gruntz/Boomerang.h> // CBoomerang   (state-0 leaf of LogicDispatchBoomerang, ctor 0xe0650, size 0x260)
#include <Gruntz/XferArchive.h> // ProjTypeXfer (0x16e4f0) = the default-case fall-through

// global operator new (engine NAFXCW _RezAlloc @0x1b9b46); external/no-body.
void* operator new(u32 n);

// The polymorphic logic sub-record (m_18). Only the dispatched slots are named;
// the leading slots fix the vtable offsets. External (foreign vtable).
class LogicSubRec {
public:
    virtual void Slot00(); // 0x00
    virtual void Slot04(); // 0x04
    virtual void Slot08(); // 0x08
    virtual void Slot0c(); // 0x0c
    virtual void Slot10(); // 0x10
    virtual void Slot14(); // 0x14
    virtual void Init();   // 0x18  slot 6  (state-0 init)
    virtual void Slot1c(); // 0x1c
    virtual void Slot20(); // 0x20
    virtual void Slot24(); // 0x24
    virtual void Op1e();   // 0x28  slot 10 (state 0x1e)
    virtual void Op1d();   // 0x2c  slot 11 (state 0x1d)
    virtual void Op52();   // 0x30  slot 12 (state 0x52)
    virtual void Op51();   // 0x34  slot 13 (state 0x51)
    virtual void Op50();   // 0x38  slot 14 (state 0x50)
    virtual void Op53();   // 0x3c  slot 15 (state 0x53)
};

// The record's state tag (m_1c). State kLogicStateInit lazily builds the sub-record
// then latches kLogicStateBuilt; each Op<NN> state dispatches the like-named
// LogicSubRec vtable slot; kLogicStateBuilt is the built/idle no-op. Same immediates
// as the bare labels -> naming is matching-neutral. m_1c stays u32 so the switch key
// keeps its unsigned ja/jbe codegen (retyping it to the enum would flip to signed).
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

struct LogicDispatchRecord {
    char m_pad00[0x18];
    LogicSubRec* m_18; // +0x18 owned sub-record
    u32 m_1c;          // +0x1c state tag (LogicRecordState; unsigned switch key -> ja/jbe)
};

struct LogicDispatchOwner {
    char m_pad00[0x7c];
    LogicDispatchRecord* m_7c; // +0x7c embedded record
};

// State-0 sub-record types (built lazily) are all real engine classes, `new`'d directly
// through their shared headers so the ctor CALL binds to the retail RVA: LogicDispatchA
// -> CStaticHazard (0x6c, ctor 0xfb7a0), D -> CTimeBomb (0x68, ctor 0xe1b90),
// E -> CProjectile (0x228, ctor 0xdec60), Boomerang -> CBoomerang (0x260, ctor 0xe0650).
// No fake LogicSubRec{E,Boomerang} views remain (CProjectile is now size-exact 0x228 and
// CBoomerang got a shared header <Gruntz/Boomerang.h>).
//
// LogicSubRecB (the state-0 sub-record built by LogicDispatchB@0x10d3d0) is really CBrickz
// (ctor thunk 0x3701 -> 0x10e800); LogicDispatchB was folded into src/Gruntz/TileLogicPump.cpp
// (waveM-strays: it sits inside the tile-trigger obj's contiguous .text block) and modeled on
// the real CBrickz, so the LogicSubRecB view is gone.

// The default-case fall-through helper IS the real shared type-registry resolve at
// 0x16e4f0 (?ProjTypeXfer@@YAHPAUCXferArchive@@@Z, __cdecl). Thin forwarder so the
// callers emit the bound rel32 (was the fake, UNBOUND _LogicSubDefault_16e4f0).
inline void LogicSubDefault_16e4f0(LogicSubRec* sub) {
    ProjTypeXfer((CXferArchive*)sub);
}

// LogicDispatchE @0x0de8a0 - state-0 builds a CProjectile (0x228, ctor 0xdec60). The
// object is CUserLogic-derived; the LogicSubRec view is its shared vtable lens (Init =
// slot 6) so the dispatch bytes are unchanged. The `new CProjectile` size push is the
// imm32 0x228 (3 bytes wider than the imm8-size CTimeBomb/CStaticHazard siblings).
RVA(0x000de8a0, 0xf4)
i32 LogicDispatchE(LogicDispatchOwner* owner) {
    LogicDispatchRecord* rec = owner->m_7c;
    switch (rec->m_1c) {
        case kLogicStateInit:
            rec->m_1c = kLogicStateBuilt;
            {
                LogicSubRec* obj = (LogicSubRec*)new CProjectile((CGameObject*)owner);
                obj->Init();
                rec->m_18 = obj;
            }
            break;
        case kLogicStateOp1d:
            rec->m_18->Op1d();
            break;
        case kLogicStateOp1e:
            rec->m_18->Op1e();
            break;
        case kLogicStateOp50:
            rec->m_18->Op50();
            break;
        case kLogicStateOp51:
            rec->m_18->Op51();
            break;
        case kLogicStateOp52:
            rec->m_18->Op52();
            break;
        case kLogicStateOp53:
            rec->m_18->Op53();
            break;
        case kLogicStateBuilt:
            break;
        default:
            LogicSubDefault_16e4f0(rec->m_18);
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
i32 LogicDispatchBoomerang(LogicDispatchOwner* owner) {
    LogicDispatchRecord* rec = owner->m_7c;
    switch (rec->m_1c) {
        case kLogicStateInit:
            rec->m_1c = kLogicStateBuilt;
            {
                LogicSubRec* obj = (LogicSubRec*)new CBoomerang((CGameObject*)owner);
                obj->Init();
                rec->m_18 = obj;
            }
            break;
        case kLogicStateOp1d:
            rec->m_18->Op1d();
            break;
        case kLogicStateOp1e:
            rec->m_18->Op1e();
            break;
        case kLogicStateOp50:
            rec->m_18->Op50();
            break;
        case kLogicStateOp51:
            rec->m_18->Op51();
            break;
        case kLogicStateOp52:
            rec->m_18->Op52();
            break;
        case kLogicStateOp53:
            rec->m_18->Op53();
            break;
        case kLogicStateBuilt:
            break;
        default:
            LogicSubDefault_16e4f0(rec->m_18);
            break;
    }
    return 1;
}

// LogicDispatchD @0x0deb20 - state-0 builds a CTimeBomb (0x68, ctor 0xe1b90). The
// object is CUserLogic-derived; the LogicSubRec view is its shared vtable lens
// (Init = CUserLogic::Activate, slot 6) so the dispatch bytes are unchanged.
RVA(0x000deb20, 0xf1)
i32 LogicDispatchD(LogicDispatchOwner* owner) {
    LogicDispatchRecord* rec = owner->m_7c;
    switch (rec->m_1c) {
        case kLogicStateInit:
            rec->m_1c = kLogicStateBuilt;
            {
                LogicSubRec* obj = (LogicSubRec*)new CTimeBomb((CGameObject*)owner);
                obj->Init();
                rec->m_18 = obj;
            }
            break;
        case kLogicStateOp1d:
            rec->m_18->Op1d();
            break;
        case kLogicStateOp1e:
            rec->m_18->Op1e();
            break;
        case kLogicStateOp50:
            rec->m_18->Op50();
            break;
        case kLogicStateOp51:
            rec->m_18->Op51();
            break;
        case kLogicStateOp52:
            rec->m_18->Op52();
            break;
        case kLogicStateOp53:
            rec->m_18->Op53();
            break;
        case kLogicStateBuilt:
            break;
        default:
            LogicSubDefault_16e4f0(rec->m_18);
            break;
    }
    return 1;
}

// LogicDispatchA @0x0fb660 - state-0 builds a CStaticHazard (0x6c, ctor 0xfb7a0).
// The object is CUserLogic-derived; the LogicSubRec view is its shared vtable lens
// (Init = CUserLogic::Activate, slot 6) so the dispatch bytes are unchanged.
RVA(0x000fb660, 0xf1)
i32 LogicDispatchA(LogicDispatchOwner* owner) {
    LogicDispatchRecord* rec = owner->m_7c;
    switch (rec->m_1c) {
        case kLogicStateInit:
            rec->m_1c = kLogicStateBuilt;
            {
                LogicSubRec* obj = (LogicSubRec*)new CStaticHazard((CGameObject*)owner);
                obj->Init();
                rec->m_18 = obj;
            }
            break;
        case kLogicStateOp1d:
            rec->m_18->Op1d();
            break;
        case kLogicStateOp1e:
            rec->m_18->Op1e();
            break;
        case kLogicStateOp50:
            rec->m_18->Op50();
            break;
        case kLogicStateOp51:
            rec->m_18->Op51();
            break;
        case kLogicStateOp52:
            rec->m_18->Op52();
            break;
        case kLogicStateOp53:
            rec->m_18->Op53();
            break;
        case kLogicStateBuilt:
            break;
        default:
            LogicSubDefault_16e4f0(rec->m_18);
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

SIZE_UNKNOWN(LogicDispatchOwner);
SIZE_UNKNOWN(LogicDispatchRecord);
SIZE_UNKNOWN(LogicSubRec);

// --- vtable catalog ---

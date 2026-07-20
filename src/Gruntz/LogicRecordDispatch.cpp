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

// DISSOLVED (Fable A2, 2026-07-14): the "LogicSubRec" 16-slot lens WAS the
// canonical CUserLogic (<Gruntz/UserLogic.h>, ??_7 @0x1e705c, 16 slots) - every
// state-0 leaf built here (CStaticHazard/CTimeBomb/CProjectile/CBoomerang) is
// CUserLogic-derived, and the view's slots map 1:1: "Init" = Activate [6],
// "Op1e" = UserLogicVfunc8 [10 +0x28], "Op1d" = UserLogicVfunc9 [11 +0x2c],
// "Op52" = UserLogicVfuncA [12 +0x30], "Op51" = UserLogicVfuncB [13 +0x34],
// "Op50" = UserLogicVfuncC [14 +0x38], "Op53" = UserLogicVfuncD [15 +0x3c].
// The owner/record shells were CGameObject (+0x7c) and its AnimWorkerObj (m_logic
// @+0x18, the m_1c state word - the documented int|ptr union, reinterpreted at
// the pump per the canonical's note). TileLogicPump.cpp's parallel
// CTileTransitionController/State pump models the SAME record/slots.
#include <Gruntz/UserLogic.h>

// The record's state tag (m_1c). State kLogicStateInit lazily builds the sub-record
// then latches kLogicStateBuilt; each Op<NN> state dispatches the like-named
// LogicSubRec vtable slot; kLogicStateBuilt is the built/idle no-op. Same immediates
// as the bare labels -> naming is matching-neutral. The switch key is (u32)m_1c
// (AnimWorkerObj's documented int|ptr union member) so it keeps its unsigned ja/jbe
// codegen (an enum-typed key would flip to signed).
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
inline void LogicSubDefault_16e4f0(CUserLogic* sub) {
    ProjTypeXfer(sub);
}

// LogicDispatchE @0x0de8a0 - state-0 builds a CProjectile (0x228, ctor 0xdec60). The
// object is CUserLogic-derived; the LogicSubRec view is its shared vtable lens (Init =
// slot 6) so the dispatch bytes are unchanged. The `new CProjectile` size push is the
// imm32 0x228 (3 bytes wider than the imm8-size CTimeBomb/CStaticHazard siblings).
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

// LogicDispatchD @0x0deb20 - state-0 builds a CTimeBomb (0x68, ctor 0xe1b90). The
// object is CUserLogic-derived; the LogicSubRec view is its shared vtable lens
// (Init = CUserLogic::Activate, slot 6) so the dispatch bytes are unchanged.
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

// LogicDispatchA @0x0fb660 - state-0 builds a CStaticHazard (0x6c, ctor 0xfb7a0).
// The object is CUserLogic-derived; the LogicSubRec view is its shared vtable lens
// (Init = CUserLogic::Activate, slot 6) so the dispatch bytes are unchanged.
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

// LogicDispatchC @0x046850 (state-0 news a CPARTICLEZ: ctor thunk 0x2a04 ->
// 0x46ad0 == ??0CParticlez) was re-homed to FortressFlag.cpp (wave3-I): its
// retail body is text-contained in the ff+particlez+explosion obj.
// LogicSubRecC (its former local sub-record view) went with it, dissolved
// onto the real CParticlez.

// LogicDispatchB @0x10d3d0 (state-0 news a CBrickz: ctor thunk 0x3701 -> 0x10e800) was folded
// into src/Gruntz/TileLogicPump.cpp (waveM-strays): its retail body is text-contained in the
// tile-trigger logic obj's contiguous first-link .text block, and it is CBrickz's state pump.

// --- vtable catalog ---

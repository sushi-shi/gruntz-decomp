// UserLogicCtorEmit.cpp - the two out-of-line base-ctor COMDATs of the
// game-object spine (C:\Proj\Gruntz), re-homed from src/Stub/CUserLogic.cpp.
//
//   CUserLogic::CUserLogic()            -> 0x138d0 (75 B)
//   CUserLogic::CUserLogic(CGameObject*)-> 0x58cd0 (405 B)
//
// Both are inline in <Gruntz/UserLogic.h> so the game-object leaf ctors FOLD them;
// retail ALSO emits the standalone out-of-line copies (called by the big factory
// fns - the no-arg by the serial-parse loop @0xd2xx, the 1-arg by
// CProjectile/CDoNothingNormal/Grunt through the 0x3828 ILT thunk). An inline ctor
// can't hang RVA() directly, so the standalone copies are pinned by mangled name
// via @rva-symbol; the non-inlined references below force MSVC to emit them.
//
// WHY ITS OWN UNIT: the 1-arg standalone ctor INLINES the built-in logic-type
// registration (the 0x8a40 BuildLogicTypeTable body, Lookup/0x1b8008 based),
// whereas the copies folded into a leaf 1-arg ctor keep it a CALL at inline
// depth 2. To reproduce the inline, BuildLogicTypeTable needs a visible body here;
// but if that body were visible in src/Gruntz/UserLogic.cpp it would fold into
// EVERY leaf 1-arg ctor there and regress them all (measured: ~25 leaves dropped
// from ~90% to ~10-50%). Isolating the forcer + the inline body in this TU emits
// the standalone COMDATs while UserLogic.cpp's leaves keep calling the helper.
#include <Mfc.h> // operator new + the afx-first windows.h order UserLogic.h needs
#include <Gruntz/UserLogic.h>
#include <rva.h>

// @rva-symbol: ??0CUserLogic@@QAE@XZ 0x000138d0 0x4b  (100% - byte-exact)
//
// @early-stop
// 1-arg ctor 0x58cd0: 0.89% stub -> 89.0%, byte-shaped (prologue + throwing-link
// /GX frame + EngStr temp + inlined Lookup-based BuildLogicTypeTable + AddLogic*3
// + int stores all match; verified base-vs-target with llvm-objdump -dr). Two
// residuals, both documented walls, both also present in the reference standalone
// BuildLogicTypeTable @0x8a40 (itself only 87.92%):
//   1. Factory-fn DIR32 naming: the three RegisterType factory pushes reference
//      _LogicHitFactory/_LogicAttackFactory/_LogicBumpFactory, but retail's symbols
//      at 0x56e4c0/d0/e0 are delinked as ?KeyPrefixBits_16e480@@YAHPBD0@Z (a Ghidra
//      mislabel shared across the logic-type TUs) -> DIR32 name mismatch. Renaming
//      is a cross-unit symbol-naming fix (would also touch logictypetable), out of
//      this TU's scope.
//   2. MSVC scheduling coin-flip inside the inlined BuildLogicTypeTable: retail
//      schedules the `found = 0` store + the `obj->m_c` reload AFTER the Lookup
//      arg-pushes; MSVC5 here emits them before (x3 blocks). Not source-steerable
//      (a scalar store vs call-arg ordering the scheduler owns).
// @rva-symbol: ??0CUserLogic@@QAE@PAUCGameObject@@@Z 0x00058cd0 0x195

extern "C" {
    i32 LogicHitFactory(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)    // 0x56e4c0
    i32 LogicAttackFactory(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant) // 0x56e4d0
    i32 LogicBumpFactory(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)   // 0x56e4e0
}
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawWorkerCache.h>
inline void CUserLogic::BuildLogicTypeTable(CGameObject* obj) {
    {
        CObject* found = 0;
        obj->OwnerMgr()->m_workerCache->m_10.Lookup("LogicHit", found);
        if (!found) {
            obj->OwnerMgr()->m_workerCache->CreateWorker(LogicHitFactory, "LogicHit", 2);
        }
    }
    {
        CObject* found = 0;
        obj->OwnerMgr()->m_workerCache->m_10.Lookup("LogicAttack", found);
        if (!found) {
            obj->OwnerMgr()->m_workerCache->CreateWorker(LogicAttackFactory, "LogicAttack", 2);
        }
    }
    {
        CObject* found = 0;
        obj->OwnerMgr()->m_workerCache->m_10.Lookup("LogicBump", found);
        if (!found) {
            obj->OwnerMgr()->m_workerCache->CreateWorker(LogicBumpFactory, "LogicBump", 2);
        }
    }
}

static CUserLogic* volatile g_forceEmitSink;
#pragma inline_depth(0)
void ForceEmitCUserLogicNoArg() {
    g_forceEmitSink = new CUserLogic();
}
#pragma inline_depth()
void ForceEmitCUserLogic1Arg(CGameObject* o) {
    g_forceEmitSink = new CUserLogic(o);
    g_forceEmitSink = new CUserLogic(o);
    g_forceEmitSink = new CUserLogic(o);
    g_forceEmitSink = new CUserLogic(o);
}

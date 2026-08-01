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
// via RVA_COMPGEN; the non-inlined references below force MSVC to emit them.
//
// WHY ITS OWN UNIT: the 1-arg standalone ctor INLINES the built-in logic-type
// registration (the 0x8a40 BuildLogicTypeTable body, Lookup/0x1b8008 based),
// whereas the copies folded into a leaf 1-arg ctor keep it a CALL at inline
// depth 2. To reproduce the inline, BuildLogicTypeTable needs a visible body here;
// but if that body were visible in src/Gruntz/UserLogic.cpp it would fold into
// EVERY leaf 1-arg ctor there and regress them all (measured: ~25 leaves dropped
// from ~90% to ~10-50%). Isolating the forcer + the inline body in this TU emits
// the standalone COMDATs while UserLogic.cpp's leaves keep calling the helper.
#include <Gruntz/UserLogicCtorEmit.h> // this TU's external declarations
#include <Mfc.h> // operator new + the afx-first windows.h order UserLogic.h needs
#include <Gruntz/UserLogic.h>
#include <rva.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawWorkerCache.h>

RVA_COMPGEN(0x000138d0, 0x4b, ??0CUserLogic@@QAE@XZ) // (100% - byte-exact)
//
// @early-stop
RVA_COMPGEN(0x00058cd0, 0x195, ??0CUserLogic@@QAE@PAUCGameObject@@@Z)

i32 LogicHitFactory(
    CGameObject* obj
); // GameObjNotifyFn ABI (CreateWorker registrant)    // 0x56e4c0
// TU-LOCAL INLINING DEVICE - keep textually identical to the ONE real definition,
// CUserLogic::BuildLogicTypeTable @0x8a40 in src/Gruntz/UserLogic.cpp (which is where
// the symbol comes from; this copy must NOT emit a COMDAT of its own, hence the
// inline_depth(0) on the 1-arg forcer below - without it the forcer's 3 non-inlined
// ctor copies call the helper and drag out a duplicate definition).
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
// inline_depth(0): emit the standalone ctor COMDAT WITHOUT inlining it here. Inlined
// copies stop folding BuildLogicTypeTable at that depth and reference it instead, which
// used to emit a second definition of it in this obj (see the note on the body above).
#pragma inline_depth(0)
void ForceEmitCUserLogic1Arg(CGameObject* o) {
    g_forceEmitSink = new CUserLogic(o);
}
#pragma inline_depth()

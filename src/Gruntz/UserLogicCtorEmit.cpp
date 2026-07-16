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
// The inline CUserLogic(obj) now inits only through m_2c (the true-0x30 base), so the
// standalone COMDAT it forces already matches retail 0x58cd0 with no macro. The leaf-
// only m_34/m_38/m_3c tail stores moved to CTileLogic(obj) (see <Gruntz/UserLogic.h>);
// CProjectile/CDoNothingNormal/Grunt call this same base ctor out-of-line via 0x3828.
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

// ---------------------------------------------------------------------------
// Lookup-based inline body of CUserLogic::BuildLogicTypeTable (the 0x8a40 helper,
// mirrored from src/Gruntz/LogicTypeTable.cpp): each of the three built-in types
// re-reads obj->m_c->m_14 (the logic-type registry), Lookup()s the key (0x1b8008,
// __thiscall ret 8, on registry+0x10), and RegisterType()s (vtable slot +0x24) it
// when absent. Visible only in this TU, so it inlines into the standalone 1-arg
// COMDAT but not into any leaf.
// ---------------------------------------------------------------------------
extern "C" {
    void LogicHitFactory();    // 0x56e4c0
    void LogicAttackFactory(); // 0x56e4d0
    void LogicBumpFactory();   // 0x56e4e0
}
// REGISTRY = the canonical CDDrawWorkerCache (Fable A2, 2026-07-14; see
// LogicTypeTable.cpp): the "+0x24 registrar" is its slot-9 CreateWorker and the
// +0x10 lookup sub-object its real CMapStringToOb m_10 (Lookup @0x1b8008 - the
// old CMapStringToPtr cast here bound the reloc to the WRONG library symbol,
// 0x1b8438; the inverted-label pair struck again). The CLogicType* chain shells
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawWorkerCache.h>
inline void CUserLogic::BuildLogicTypeTable(CGameObject* obj) {
    {
        CObject* found = 0;
        obj->m_0c->m_workerCache->m_10.Lookup("LogicHit", found);
        if (!found) {
            obj->m_0c->m_workerCache->CreateWorker((i32)LogicHitFactory, "LogicHit", 2);
        }
    }
    {
        CObject* found = 0;
        obj->m_0c->m_workerCache->m_10.Lookup("LogicAttack", found);
        if (!found) {
            obj->m_0c->m_workerCache->CreateWorker((i32)LogicAttackFactory, "LogicAttack", 2);
        }
    }
    {
        CObject* found = 0;
        obj->m_0c->m_workerCache->m_10.Lookup("LogicBump", found);
        if (!found) {
            obj->m_0c->m_workerCache->CreateWorker((i32)LogicBumpFactory, "LogicBump", 2);
        }
    }
}

// ---------------------------------------------------------------------------
// The forcers. The no-arg ctor is small (~75 B) - MSVC always inlines it, so
// inline_depth(0) is used to force the standalone COMDAT (it has no inlinable
// callees, so depth-0 is harmless). The 1-arg ctor is compiled at DEFAULT depth
// so BuildLogicTypeTable folds into it as in retail 0x58cd0; the ~405 B body then
// overflows the caller's inline budget across the repeated constructions and is
// emitted standalone rather than folded into the forcer.
// ---------------------------------------------------------------------------
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

// --- vtable catalog (view/base classes bound to their unit vtable rva) ---

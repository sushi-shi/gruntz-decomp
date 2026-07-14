#include <Mfc.h> // real MFC CMapStringToOb (the logic-name map's Lookup @0x1b8438)
#include <rva.h>
#include <Gruntz/Grunt.h>     // CAnimLookupNode (the m_14 aux the guard reads at +0x1c)
#include <Gruntz/UserLogic.h> // CUserLogic - the real owner of 0x8b90 (ex CFinalize8b90)
// LogicTypeTable.cpp - CLogicTypeBuilder::BuildLogicTypeTable. A one-shot
// registrar that ensures three built-in tile-logic
// types ("LogicHit", "LogicAttack", "LogicBump") are present in the engine's
// logic-type registry, lazily registering each (with its factory callback) only
// if a lookup for that key comes back empty (C:\Proj\Gruntz).
//
// For each of the three keys it:
//   1. looks the key up in the registry's string-keyed table (the matched lookup
//      helper CLogicRegistry::Lookup, external), addressed
//      at (this->m_c->m_14)+0x10;
//   2. if the lookup result is null, calls the registry's virtual registrar
//      (vtable slot +0x24) with (factoryFn, key, 2) to install the type.
//
// Only offsets / code bytes are load-bearing; names are placeholders.
//
// BYTE-EXACT body modulo the same MSVC5 scheduling coin-flip as spriteloaders:
// the target SINKS each block's lookup out-param zero-init store (`mov [&found],0`)
// past the lea/push of &found; our cl HOISTS it before - the same instructions,
// permuted (byte-content identical). See config/units.toml. Kept wip, not
// strict-exact.

// ---------------------------------------------------------------------------
// The three built-in logic-type factory callbacks. These are real engine .text
// routines we are NOT matching here; declared as external no-body functions so
// that pushing their address emits a DIR32 relocation (reloc-masked in objdiff,
// like the target's LAB_0056e4c0/d0/e0 .text data references).
// ---------------------------------------------------------------------------
extern "C" {
    void LogicHitFactory();
    void LogicAttackFactory();
    void LogicBumpFactory();
}

// ---------------------------------------------------------------------------
// The logic-type registry IS the canonical CDDrawWorkerCache (Fable A2,
// 2026-07-14; <DDrawMgr/DDrawWorkerCache.h>, ??_7 @0x1efd00): the "+0x24
// registrar" is its slot-9 CreateWorker (0x1652c0) and the +0x10 lookup
// sub-object its CMapStringToOb m_10 (Lookup @0x1b8008). The former
// CLogicRegistry (10-slot filler view) / CLogicCtx / CLogicTypeBuilder chain
// shells are dissolved: the builder arg is the bound CGameObject, whose +0xc
// world ctx (the CGameObjWorld view, <Gruntz/UserLogic.h>) carries the cache at
// +0x14 (== the canonical CDDrawSurfaceMgr's m_workerCache).
// ---------------------------------------------------------------------------
#include <DDrawMgr/DDrawWorkerCache.h>

// ---------------------------------------------------------------------------
// BuildLogicTypeTable - __stdcall: the builder object is the
// single explicit stack arg, read into ESI; not a __thiscall.
//
// Each block re-reads obj->m_c->m_14 (the registry pointer) for BOTH the lookup
// and the register call - the target does NOT cache it across the two (it reloads
// `mov edx,[esi+0xc]; mov ecx,[edx+0x14]` at each site), so the lookup expression
// is repeated rather than hoisted into a local.
RVA(0x00008a40, 0xc8)
void __stdcall BuildLogicTypeTable(CGameObject* obj) {
    {
        CObject* found = 0;
        ((CGameObjWorld*)obj->m_0c)->m_workerCache->m_10.Lookup("LogicHit", found);
        if (!found) {
            ((CGameObjWorld*)obj->m_0c)
                ->m_workerCache->CreateWorker((i32)LogicHitFactory, "LogicHit", 2);
        }
    }
    {
        CObject* found = 0;
        ((CGameObjWorld*)obj->m_0c)->m_workerCache->m_10.Lookup("LogicAttack", found);
        if (!found) {
            ((CGameObjWorld*)obj->m_0c)
                ->m_workerCache->CreateWorker((i32)LogicAttackFactory, "LogicAttack", 2);
        }
    }
    {
        CObject* found = 0;
        ((CGameObjWorld*)obj->m_0c)->m_workerCache->m_10.Lookup("LogicBump", found);
        if (!found) {
            ((CGameObjWorld*)obj->m_0c)
                ->m_workerCache->CreateWorker((i32)LogicBumpFactory, "LogicBump", 2);
        }
    }
}

// ---------------------------------------------------------------------------
// CUserLogic::FinalizeStep (0x008b90) - fire up to two registered __thiscall callbacks
// (m_04, m_08) with `this`, the m_08 one guarded by m_14->m_1c == m_28, null each fired
// slot, and reset m_28 to 0x3e9 (1001). __thiscall(i32) - retail `ret 4` - and the arg
// is genuinely unused.
//
// IDENTITY RECOVERED (2026-07-13, Fable lane; found by gruntz.analysis.thunk_alias_dups):
// this ONE body had THREE source names, two of them phantoms.
//   * `CFinalize8b90::Finalize` - the fake class it was defined under here, with an
//     invented PMF-holder layout. DISSOLVED: its m_0/m_4/m_8/m_14/m_28 land EXACTLY on
//     CUserLogic's m_00/m_04/m_08/m_14/m_28 (the m_14 union is already the
//     CAnimLookupNode* whose +0x1c this guards against m_28).
//   * `CGrunt::FinalizeStep` (Grunt.h) - a decl with NO definition anywhere, i.e. a
//     guaranteed unresolved external; its one call site (GruntEntranceArrival.cpp) runs
//     it on a CGrunt `this`, which INHERITS this CUserLogic method. Phantom killed: the
//     call now resolves here, cast-free, and CGrunt's duplicate decl is deleted.
//   * `?UserLogicVfunc3@CUserLogic@@UAEHXZ` - the vtable SLOT-5 binding (an @rva-symbol
//     on ILT thunk 0x3913, which jmps straight here). STILL OPEN, and it is a genuine
//     SIGNATURE defect rather than a naming one: the slot is `void f(i32)` (retail
//     `ret 4`), but UserLogic.h declares `virtual i32 UserLogicVfunc3()` - a DROPPED
//     PARAMETER plus a wrong return type. Grunt.h:1945 already records the fallout
//     ("the no-arg UserLogicVfunc3() base placeholder blocks the OVERRIDE spelling"),
//     which is exactly why CGrunt's real slot-5 override (0x5ecd0, also `ret 4`) has to
//     be spelled as the plain method RunPositionInterpStep(i32). Correcting the virtual
//     migrates three headers (UserLogic.h / Grunt.h / MovingLogic.h) and first needs
//     0x5ecd0's and CMovingLogic's override RETURN types settled from disasm - NOT
//     guessed. Left for a dedicated pass; reported, not fabricated.
//
// The two callback slots stay i32 IN THE HEADER on purpose: a pointer-to-member typed
// inside its own (still-incomplete) class makes MSVC pick the most-general PMF
// representation, which is LARGER than 4 bytes and would silently shift every field
// after it (docs/patterns/pmf-complete-class-4byte.md). The PMF type is therefore formed
// HERE, where CUserLogic is complete, and applied at the two call sites - language-
// forced, not a modeling shortcut.
// ---------------------------------------------------------------------------
typedef void (CUserLogic::*UserLogicCallback)(); // 4 bytes (complete class, single inheritance)

RVA(0x00008b90, 0x40)
void CUserLogic::FinalizeStep(i32 /*unused*/) {
    if (m_04 == 0) {
        return;
    }
    if (m_08 != 0 && (i32)m_14->m_1c == m_28) {
        (this->*(UserLogicCallback&)m_08)();
        m_08 = 0;
    }
    (this->*(UserLogicCallback&)m_04)();
    m_04 = 0;
    m_28 = 0x3e9;
}


// --- vtable catalog ---

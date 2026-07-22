#include <Gruntz/TriggerMgr.h>       // CTriggerMgr::NotifyCell (winapi_064540 arrival anim)
#include <Gruntz/AniAdvanceCursor.h> // CAniAdvanceCursor::Advance (winapi_064540)
#include <Mfc.h>                     // CString / RECT / PostMessageA
#include <Bute/SymTab.h>             // CSymTab::ResolveQualified (winapi_064540 level lookup)
#include <Gruntz/LogicTypeId.h>      // LogicTypeId (CUserBase/CUserLogic GetTypeTag)
#include <Gruntz/Grunt.h> // complete CGrunt (its slot-3 XferName stub is homed here)
#include <Gruntz/UserLogic.h>
#include <rva.h>
#include <Gruntz/TypeKeyColl.h>

// The two out-of-line base-ctor COMDATs (CUserLogic() @0x138d0 / CUserLogic(obj)
// @0x58cd0) are emitted + RVA_COMPGEN pinned in a SEPARATE unit,
// src/Gruntz/UserLogicCtorEmit.cpp. They must NOT be forced here: the 1-arg copy
// needs an inline (Lookup-based) BuildLogicTypeTable body to match retail's
// inlined registration, and that body, if visible in THIS TU, folds into every
// leaf 1-arg ctor at depth 2 and regresses them all (retail leaves CALL 0x8a40 at
// depth 2). Isolating the forcer + inline body in its own TU keeps the leaves here
// calling the out-of-line helper.

i32 CUserBase::SerializeMove(CFileMemBase*, i32, i32, i32) {
    return 1; // retail 0x87d0: mov eax,1; ret 0x10
}
LogicTypeId CUserBase::GetTypeTag() {
    return static_cast<LogicTypeId>(0);
}

void CUserLogic::XferName(char* name) {
    // ret-4 one-arg no-op (the base 0x00106e shape); leaves override to receive
    // their serialized type name.
}
void CUserLogic::FireActivation(i32) {}
i32 CUserLogic::Activate() {
    return 0;
}
i32 CUserLogic::UserLogicVfunc5() {
    return 1; // retail 0x88f0: mov eax,1; ret
}
i32 CUserLogic::UserLogicVfunc6() {
    return 1; // retail 0x8910: mov eax,1; ret
}
i32 CUserLogic::StepAttackFire() {
    return 1; // retail 0x8930: mov eax,1; ret
}
void CUserLogic::UserLogicVfunc8() {} // retail 0x8950: bare ret
i32 CUserLogic::UserLogicVfunc9() {
    return 0;
}
void CUserLogic::UserLogicVfuncA() {} // retail 0x8990: bare ret
void CUserLogic::UserLogicVfuncB() {} // retail 0x89b0: bare ret
void CUserLogic::UserLogicVfuncC() {} // retail 0x89d0: bare ret
void CUserLogic::UserLogicVfuncD() {} // retail 0x89f0: bare ret

// reloc-fidelity: pin the obj-defined base virtuals above to their real retail BODY
// RVAs (CUserBase vtbl 0x5e70b4 slots 1-2; CUserLogic vtbl 0x5e705c slots 3-15, from
// `sema class`). Every tile-logic leaf obj emits ??_7CUserBase / ??_7CUserLogic with
// these slots as DIR32 relocs to the base virtual symbols; unpinned they bound to NO
// rva (reloc-masked but wrong). Bodies are NOT matched (dummy anchors) - the pin only
// binds the slot symbol to the body's address.
//
// These pins previously carried the vtable slot's stored VALUE (0x1361, 0x242d, ...),
// which is NOT a body: retail is linked /INCREMENTAL, so each slot holds the address
// of a 5-byte `jmp rel32` incremental-link thunk (band 0x1005..0x44a8) that forwards
// to the body. Pinning a symbol onto a thunk claims a 5-byte jmp IS the function. The
// addresses below are the thunks' destinations - the contiguous 0x87d0..0x89f0 cluster
// of tiny CUserLogic virtuals (sizes 1-8B) that the slots actually reach.
// See docs/patterns/ilt-thunk-indirection.md.
RVA_COMPGEN(0x000087d0, 0x0, ?SerializeMove@CUserBase@@UAEHPAVCFileMemBase@@HHH@Z)
RVA_COMPGEN(0x000087f0, 0x0, ?GetTypeTag@CUserBase@@UAE?AW4LogicTypeId@@XZ)
RVA_COMPGEN(0x000088d0, 0x0, ?Activate@CUserLogic@@UAEHXZ)
RVA_COMPGEN(0x000088f0, 0x0, ?UserLogicVfunc5@CUserLogic@@UAEHXZ)
RVA_COMPGEN(0x00008910, 0x0, ?UserLogicVfunc6@CUserLogic@@UAEHXZ)
RVA_COMPGEN(0x00008930, 0x0, ?StepAttackFire@CUserLogic@@UAEHXZ)
RVA_COMPGEN(0x00008950, 0x0, ?UserLogicVfunc8@CUserLogic@@UAEXXZ)
RVA_COMPGEN(0x00008970, 0x0, ?UserLogicVfunc9@CUserLogic@@UAEHXZ)
RVA_COMPGEN(0x00008990, 0x0, ?UserLogicVfuncA@CUserLogic@@UAEXXZ)
RVA_COMPGEN(0x000089b0, 0x0, ?UserLogicVfuncB@CUserLogic@@UAEXXZ)
RVA_COMPGEN(0x000089d0, 0x0, ?UserLogicVfuncC@CUserLogic@@UAEXXZ)
RVA_COMPGEN(0x000089f0, 0x0, ?UserLogicVfuncD@CUserLogic@@UAEXXZ)
RVA_COMPGEN(0x00008b50, 0x0, ?XferName@CUserLogic@@UAEXPAD@Z)
RVA_COMPGEN(0x00008b70, 0x0, ?FireActivation@CUserLogic@@UAEXH@Z)

// @confidence: low
// @source: winapi:CopyRect
// @stub
RVA(0x0004d800, 0x423)
i32 CUserLogic::winapi_04d800_CopyRect(i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32) {
    return 0;
}

// XREF-RECOVERED IDENTITY (matcher-1): LoadGruntTypeTable (0x4dd50, 8896 B) and
// LoadGruntTuningConstants (0x5d210, 5187 B) are CGrunt methods, NOT CUserLogic - both
// run on a CGrunt `this` (fields to +0x3f0), and 0x5d210 is CGrunt vtable slot 3
// (?LoadGruntTuningConstants is data-ref'd at ~??_7CGrunt@@6B@+0xc; it calls 0xee800 =
// CGrunt::ArrivalReticleScan). Supporting singletons are the canonical classes:
// CGameRegistry (g_gameReg @0x64556c), CMapMgr (+0x70), g_typeColl (zDArray,
// tuning), g_resButeMgr (config strings: FadeTransparency/SafeFlashTime/AccelerateFlash/
// EntranceSafeTime). Both are DEFERRED to the final sweep: they are decompiler-gated
// mega-methods (MSVC /O2 stack-slot aliasing + a local CByteArray + the tile grid double-
// loops + switch tables), of the same shape as CGrunt::ArrivalReticleScan whose front is
// banked in GruntReticle.cpp. Kept as stubs here (RVA-anchored, so re-homing to a real
// CGrunt TU is a follow-up); reconstructing them needs the Ghidra decompiler C.
// LoadGruntTypeTable is SYMBOL-exported under its real ?LoadGruntTypeTable@CGrunt@@ name
// (i32 return) so the ~9 CGrunt-side callers bind - it is NOT a CUserLogic method.
// @confidence: med
// @source: string-xref;vtable-slot
// @stub
SYMBOL(?LoadGruntTypeTable@CGrunt@@QAEHHHHH@Z)
RVA(0x0004dd50, 0x22c0)
i32 Stub_LoadGruntTypeTable_4dd50(i32, i32, i32, i32) {
    return 0;
}

// @confidence: med
// @source: string-xref;vtable-slot
// @stub
RVA(0x0005d210, 0x1443)
void CGrunt::XferName(char* name) {}

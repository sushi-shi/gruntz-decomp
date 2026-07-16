// UserLogic.cpp - Gruntz game-object base hierarchy + the tile-logic leaf ctors
// (C:\Proj\Gruntz).
//
// Reconstructs CUserBase / CUserLogic (see include/Gruntz/UserLogic.h) and the
// game-object leaf constructors that fold them. Two ctor shapes:
//   * NO-ARG leaf ctors (75 B): base prologue + leaf vptr, members untouched.
//   * 1-ARG leaf ctors `(CGameObject*)`: fold the inline CUserLogic(obj) shared
//     init, then store the leaf vptr and run a per-class tail.
//
// The one out-of-line ctor the family chains is CUserBaseLink::CUserBaseLink
// (0x16d710, the +0x18 member); it + the EngStr/registrar externs are in
// src/Gruntz/UserBaseLink.cpp. Functions are defined in ascending-RVA order.
#include <Gruntz/TriggerMgr.h>       // CTriggerMgr::NotifyCell (winapi_064540 arrival anim)
#include <Gruntz/AniAdvanceCursor.h> // CAniAdvanceCursor::Advance (winapi_064540)
#include <Mfc.h>                     // CString / RECT / PostMessageA
#include <Bute/SymTab.h>             // CSymTab::ResolveQualified (winapi_064540 level lookup)
#include <Gruntz/LogicTypeId.h>      // LogicTypeId (CUserBase/CUserLogic GetTypeTag)
#include <Gruntz/UserLogic.h>
#include <rva.h>
#include <Globals.h>
#include <Gruntz/TypeKeyColl.h>

// The two out-of-line base-ctor COMDATs (CUserLogic() @0x138d0 / CUserLogic(obj)
// @0x58cd0) are emitted + @rva-symbol pinned in a SEPARATE unit,
// src/Gruntz/UserLogicCtorEmit.cpp. They must NOT be forced here: the 1-arg copy
// needs an inline (Lookup-based) BuildLogicTypeTable body to match retail's
// inlined registration, and that body, if visible in THIS TU, folds into every
// leaf 1-arg ctor at depth 2 and regresses them all (retail leaves CALL 0x8a40 at
// depth 2). Isolating the forcer + inline body in its own TU keeps the leaves here
// calling the out-of-line helper.

// ---------------------------------------------------------------------------
// Out-of-line vtable anchors. These give each base class a real vftable in this
// TU so the inline ctors emit the right vptr stores. Bodies are not matched.
// (~CUserBase / ~CUserLogic are now inline in the header so leaf dtors fold the
// whole base teardown; the remaining out-of-line virtuals still anchor the
// vftables.)
i32 CUserBase::SerializeMove(CGruntArchive*, i32, i32, i32) {
    return 0;
}
LogicTypeId CUserBase::GetTypeTag() {
    return (LogicTypeId)0;
}

i32 CUserLogic::UserLogicVfunc1() {
    return 0;
}
i32 CUserLogic::UserLogicVfunc2() {
    return 0;
}
i32 CUserLogic::Activate() {
    return 0;
}
i32 CUserLogic::UserLogicVfunc5() {
    return 0;
}
i32 CUserLogic::UserLogicVfunc6() {
    return 0;
}
i32 CUserLogic::StepAttackFire() {
    return 0;
}
i32 CUserLogic::UserLogicVfunc8() {
    return 0;
}
i32 CUserLogic::UserLogicVfunc9() {
    return 0;
}
i32 CUserLogic::UserLogicVfuncA() {
    return 0;
}
i32 CUserLogic::UserLogicVfuncB() {
    return 0;
}
i32 CUserLogic::UserLogicVfuncC() {
    return 0;
}
i32 CUserLogic::UserLogicVfuncD() {
    return 0;
}

// reloc-fidelity: pin the obj-defined base virtuals above to the real retail vtable-
// slot RVAs (CUserBase vtbl 0x5e70b4 slots 1-2; CUserLogic vtbl 0x5e705c slots 3-15,
// from `sema class`). Every tile-logic leaf obj emits ??_7CUserBase / ??_7CUserLogic
// with these slots as DIR32 relocs to the base virtual symbols; unpinned they bound
// to NO rva (reloc-masked but wrong). Bodies are NOT matched (dummy anchors) - the
// pin only binds the slot symbol to the address retail's slot uses.
// @rva-symbol: ?SerializeMove@CUserBase@@UAEHPAVCFileMemBase@@HHH@Z 0x000039e0
// @rva-symbol: ?GetTypeTag@CUserBase@@UAE?AW4LogicTypeId@@XZ 0x0000242d
// @rva-symbol: ?UserLogicVfunc1@CUserLogic@@UAEHXZ 0x00003413
// @rva-symbol: ?UserLogicVfunc2@CUserLogic@@UAEHXZ 0x0000246e
// @rva-symbol: ?Activate@CUserLogic@@UAEHXZ 0x000033dc
// @rva-symbol: ?UserLogicVfunc5@CUserLogic@@UAEHXZ 0x00002162
// @rva-symbol: ?UserLogicVfunc6@CUserLogic@@UAEHXZ 0x000026b7
// @rva-symbol: ?StepAttackFire@CUserLogic@@UAEHXZ 0x00001361
// @rva-symbol: ?UserLogicVfunc8@CUserLogic@@UAEHXZ 0x000023f6
// @rva-symbol: ?UserLogicVfunc9@CUserLogic@@UAEHXZ 0x0000225c
// @rva-symbol: ?UserLogicVfuncA@CUserLogic@@UAEHXZ 0x0000150a
// @rva-symbol: ?UserLogicVfuncB@CUserLogic@@UAEHXZ 0x00003116
// @rva-symbol: ?UserLogicVfuncC@CUserLogic@@UAEHXZ 0x00001730
// @rva-symbol: ?UserLogicVfuncD@CUserLogic@@UAEHXZ 0x00003607

// ---------------------------------------------------------------------------
// CSecretTeleporterTrigger virtual support. Two engine externs the Serialize
// override (0x010a10) chains; both __thiscall ret 0x10 (4 args), modeled NO-body
// so the calls reloc-mask:
//   * CUserLogic::SerializeChain (0x16e7f0) - run on `this`.
//   * the +0x34 serializable sub-object's chain (0x8c00) - run on `&this->m_34`
//     (reached via `lea ecx,[esi+0x34]`). Modeled by the shared CSerialObjRef
//     (Chain @0x8c00, <Gruntz/SerialObjRef.h>).
// (Both bodies are pinned in src/Stub/Discovered.cpp.)

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
// CGameRegistry (g_gameReg @0x64556c), CTileGrid (+0x70), g_typeColl (CTypeKeyColl,
// tuning), g_resButeMgr (config strings: FadeTransparency/SafeFlashTime/AccelerateFlash/
// EntranceSafeTime). Both are DEFERRED to the final sweep: they are decompiler-gated
// mega-methods (MSVC /O2 stack-slot aliasing + a local CByteArray + the tile grid double-
// loops + switch tables), of the same shape as CGrunt::ArrivalReticleScan whose front is
// banked in GruntReticle.cpp. Kept as CUserLogic stubs here (RVA-anchored, so re-homing
// to a real CGrunt TU is a follow-up); reconstructing them needs the Ghidra decompiler C.
// @confidence: med
// @source: string-xref;vtable-slot
// @stub
RVA(0x0004dd50, 0x22c0)
void CUserLogic::LoadGruntTypeTable(i32, i32, i32, i32) {}

// @confidence: med
// @source: string-xref;vtable-slot
// @stub
RVA(0x0005d210, 0x1443)
void CUserLogic::LoadGruntTuningConstants(i32) {}

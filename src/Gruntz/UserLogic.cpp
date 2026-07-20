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
#include <Gruntz/Grunt.h> // complete CGrunt (its slot-3 XferName stub is homed here)
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
    return static_cast<LogicTypeId>(0);
}

void CUserLogic::XferName(char* name) {
    // ret-4 one-arg no-op (the base 0x00106e shape); leaves override to receive
    // their serialized type name.
}
// The real base body (0x8b70, reached via the slot's ILT thunk 0x246e) is a bare
// `ret 4` - an empty do-nothing hook taking the activation id. Anchor only.
void CUserLogic::FireActivation(i32) {}
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
// @rva-symbol: ?SerializeMove@CUserBase@@UAEHPAVCFileMemBase@@HHH@Z 0x000087d0
// @rva-symbol: ?GetTypeTag@CUserBase@@UAE?AW4LogicTypeId@@XZ 0x000087f0
// @rva-symbol: ?XferName@CUserLogic@@UAEXPAD@Z 0x00008b50
// @rva-symbol: ?FireActivation@CUserLogic@@UAEXH@Z 0x00008b70
// @rva-symbol: ?Activate@CUserLogic@@UAEHXZ 0x000088d0
// @rva-symbol: ?UserLogicVfunc5@CUserLogic@@UAEHXZ 0x000088f0
// @rva-symbol: ?UserLogicVfunc6@CUserLogic@@UAEHXZ 0x00008910
// @rva-symbol: ?StepAttackFire@CUserLogic@@UAEHXZ 0x00008930
// @rva-symbol: ?UserLogicVfunc8@CUserLogic@@UAEHXZ 0x00008950
// @rva-symbol: ?UserLogicVfunc9@CUserLogic@@UAEHXZ 0x00008970
// @rva-symbol: ?UserLogicVfuncA@CUserLogic@@UAEHXZ 0x00008990
// @rva-symbol: ?UserLogicVfuncB@CUserLogic@@UAEHXZ 0x000089b0
// @rva-symbol: ?UserLogicVfuncC@CUserLogic@@UAEHXZ 0x000089d0
// @rva-symbol: ?UserLogicVfuncD@CUserLogic@@UAEHXZ 0x000089f0

// ---------------------------------------------------------------------------
// CSecretTeleporterTrigger virtual support. Two engine externs the Serialize
// override (0x010a10) chains; both __thiscall ret 0x10 (4 args), modeled NO-body
// so the calls reloc-mask:
//   * CUserLogic::SerializeMove (0x16e7f0) - run on `this`.
//   * the +0x34 serializable sub-object's chain (0x8c00) - run on `&this->m_34`
//     (reached via `lea ecx,[esi+0x34]`). Modeled by the shared CSerialObjRef
//     (Chain @0x8c00, <Gruntz/SerialArchive.h>).
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
// CGameRegistry (g_gameReg @0x64556c), CTileGrid (+0x70), g_typeColl (zDArray,
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

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

// (g_buteTree @0x6bf620 - the engine bute store the tile-logic tails query for
// their "A" node - moved out with the tile-logic leaf ctors it served; no longer
// referenced by any CUserLogic-own method.)

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
// ===========================================================================
// Class declarations (one vftable each; some have both ctor shapes).
// ===========================================================================
// CSecretLevelTrigger is the canonical <Gruntz/SecretLevelTrigger.h> class (its
// ctor/dtor bodies stay here; the class shape is shared with SecretLevelTrigger.cpp).

// CTileTrigger is declared in <Gruntz/UserLogic.h>.

// The CTileTrigger family (base + the three RTTI-named leaves CTileSecretTrigger/
// CGiantRock/CCoveredPowerup + the g_tileTriggerActReg/g_tileSecretTriggerActReg
// registries) re-homed to src/Gruntz/TileTrigger.{h,cpp}; the local leaf views are
// dissolved onto the canonical <Gruntz/TileTrigger.h>. (The tile-logic pumps below
// still `new` CTileTrigger + its leaves via that header.)

// CGruntHealthSprite (no-arg ctor 0x11ef0 + 1-arg ctor 0x7eb00 + dtor anchor) re-homed
// to src/Gruntz/GruntHealthSprite.cpp; the local view is dissolved onto the canonical
// <Gruntz/GruntHealthSprite.h> class.

// CVoiceTrigger is the canonical <Gruntz/VoiceTrigger.h> class (its no-arg ctor +
// GetTypeTag bodies stay here; the class shape is shared with VoiceTrigger.cpp).

// CTeleporter (ctor 0x041020 + GetTypeTag 0x10d80 + dtor anchor) re-homed to
// src/Gruntz/Teleporter.cpp; the local view is dissolved onto the canonical
// <Gruntz/Teleporter.h> class (which now owns the inline GetTypeTag + the
// EnterField1/EnterField2 this-methods).

// CSecretTeleporterTrigger (class <Gruntz/SecretTeleporterTrigger.h> + all its
// out-of-line bodies) re-homed to src/Gruntz/SecretTeleporterTrigger.cpp.

// CWarpStonePad comes from <Gruntz/WarpStonePad.h> (folded; ctor 0x10d650 defined below).

// CTileTriggerSwitch (ctor 0x10dc40 + InitActReg/FireActivation/RegisterActs +
// SerializeMove 0x11050 + ~dtor 0x110f0 + the g_tileTriggerSwitchActReg registry)
// re-homed to src/Gruntz/TileTriggerSwitch.cpp; the local view is dissolved onto the
// canonical <Gruntz/TileTriggerSwitch.h>. (The TileTriggerSwitchStep pump below still
// `new`s CTileTriggerSwitch via that header.)

// CTileTriggerTransition (vptr 0x5e7db4) + its leaf methods and state pump now
// live in src/Gruntz/TileTriggerTransition.cpp.

// CToobSpikez comes from <Gruntz/ToobSpikez.h> (folded; ctor 0x1145c0 defined below).

// CParticlez comes from <Gruntz/Particlez.h> (folded; ctor 0x046ad0 + GetTypeTag 0x012cd0 below).

// CAniCycle comes from <Gruntz/AniCycle.h> (folded; ctor 0x0aad20 defined below).

// CSingleAnimation comes from <Gruntz/SingleAnimation.h> (folded; ctor 0x0ae7f0 defined below).

// The CGruntSprite-family leaves (CGruntSelectedSprite 0x07e3e0 / CGruntToySprite
// 0x07f350 / CGruntPowerupSprite 0x07fdb0) re-homed to their canonical per-class TUs
// (GruntSelectedSprite.cpp / GruntToySprite.cpp / GruntPowerupSprite.cpp); the local
// views are dissolved onto their canonical headers.

// ---------------------------------------------------------------------------
// The eyecandy / simple-animation leaves (1-arg ctors). They share a common
// z-clamp tail: poll the +0x198 layer's bounds against g_buteMgr's
// World/BigActHeight, then toggle the +0x7c sub-object's flag bits. m_40 caches
// the geometry token where a tail reuses it.
// ---------------------------------------------------------------------------
// CSingleFrameMessage comes from <Gruntz/SingleFrameMessage.h> (folded; ctor 0x0ab310 defined below).

// CSimpleAnimation comes from <Gruntz/SimpleAnimation.h> (folded; ctor 0x0ab940 defined below).

// CFrontCandy comes from <Gruntz/FrontCandy.h> (folded; ctor 0x0abfa0 defined below).

// CBehindCandy comes from <Gruntz/BehindCandy.h> (folded; ctor 0x0ac3f0 defined below).

// CEyeCandy comes from <Gruntz/EyeCandy.h> (folded; ctor 0x0ac620 defined below).

// CFrontCandyAni comes from the canonical <Gruntz/FrontCandyAni.h> (unified: the
// genuine ctor 0x0acf40 view + the acts facet). The ctor 0x0acf40 + RVA-less vtable-
// anchor dtor are defined below; the slot-1 Serialize (0xfdf0) + RVA'd dtor (0xfe90)
// live in FrontCandyAni.cpp with the rest of the class band.

// CBehindCandyAni comes from <Gruntz/BehindCandyAni.h> (folded; ctor 0x0ad540 below).

// CMenuSparkle comes from <Gruntz/MenuSparkle.h> (folded; ctor 0x0adbe0 defined below).

// CPathHazard (no-arg ctor 0x13170 + dtor anchor) re-homed to src/Gruntz/PathHazard.cpp;
// the local view is dissolved onto the canonical <Gruntz/PathHazard.h> class.

// The CSecretTeleporterTrigger band (Serialize 0x10a10 / ~dtor 0x10ab0 / ctor
// 0x41e90 / InitActReg / FireActivation / RegisterActs / SpawnTeleporter) + its
// per-coordinate registry (g_actColl @0x644688 + ActLookup) and the shared
// activation-name registry (g_nameReg @0x6bf650 + ActNameLookup) re-homed to
// src/Gruntz/SecretTeleporterTrigger.cpp. (g_frontCandyActReg @0x646060 was the
// mis-attributed CEyeCandyAni registry - it re-homed to src/Gruntz/EyeCandyAni.cpp
// as g_eyeCandyActReg; 0x646060's DATA symbol is pinned in
// src/Gruntz/LogicDispatchInit.cpp as g_eyeCandyDispatch.)

// CSecretLevelTrigger (no-arg ctor 0x10b20 + 1-arg ctor 0x424b0 + dtor anchor)
// re-homed to src/Gruntz/SecretLevelTrigger.cpp.

// The CTileTrigger family band (no-arg ctor 0x11160 / SerializeMove 0x111f0 /
// ~CTileTrigger COMDAT 0x11290 / the three leaf dtors 0x11540/0x11600/0x116c0 /
// 1-arg ctor 0x10e220 / Init/Fire/Register + the CTileSecretTrigger band /
// leaf ctors 0x10fa60/90/c0) re-homed to src/Gruntz/TileTrigger.cpp.

// CVoiceTrigger (no-arg ctor 0x13470 + GetTypeTag 0x133b0 + dtor anchor) re-homed to
// src/Gruntz/VoiceTrigger.cpp (joins its dtor 0x135a0 / Serialize 0x134e0 band).

// CUserLogic::GetScreenPos (0x00029a50) is now an inline member in the header.

// CUserLogic::IsAtSavedScreenPos (0x00029a80) is now an inline member in the header.

// CTeleporter ctor 0x041020 re-homed to src/Gruntz/Teleporter.cpp.

// The CSecretTeleporterTrigger band (ctor 0x41e90 + InitActReg/FireActivation/
// RegisterActs + SpawnTeleporter 0x42b80) re-homed to
// src/Gruntz/SecretTeleporterTrigger.cpp (joins its Serialize 0x10a10 / ~dtor
// 0x10ab0 band). CSecretLevelTrigger 1-arg ctor 0x0424b0 re-homed to
// src/Gruntz/SecretLevelTrigger.cpp.

// CParticlez (ctor 0x046ad0 + ~CParticlez anchor) re-homed to src/Gruntz/Particlez.cpp
// (the ctor anchors GetTypeTag @0x12cd0 + the ??_7CParticlez vtable there).

// CGruntSelectedSprite (ctor 0x07e3e0 + dtor anchor) re-homed to
// src/Gruntz/GruntSelectedSprite.cpp.
// CGruntHealthSprite 1-arg ctor 0x07eb00 re-homed to src/Gruntz/GruntHealthSprite.cpp.
// CGruntToySprite (ctor 0x07f350 + dtor anchor) re-homed to src/Gruntz/GruntToySprite.cpp.
// CGruntPowerupSprite (ctor 0x07fdb0 + dtor anchor) re-homed to
// src/Gruntz/GruntPowerupSprite.cpp.

// CAniCycle (ctor 0x0aad20 + ~CAniCycle anchor) re-homed to src/Gruntz/AniCycle.cpp
// (the ctor anchors GetTypeTag @0xf450 + the ??_7CAniCycle vtable there).

// CSingleFrameMessage (ctor 0x0ab310 + ~CSingleFrameMessage anchor) re-homed to
// src/Gruntz/SingleFrameMessage.cpp.

// CSimpleAnimation (ctor 0x0ab940 + ~CSimpleAnimation anchor) re-homed to
// src/Gruntz/SimpleAnimation.cpp.

// CFrontCandy (ctor 0x0abfa0 + ~CFrontCandy anchor) re-homed to
// src/Gruntz/FrontCandyAni.cpp (joins its Serialize 0xfa60 / dtor 0xfb00 band; the
// ctor anchors GetTypeTag @0xfa40 + the ??_7CFrontCandy vtable there).

// CBehindCandy (ctor 0x0ac3f0 + ~CBehindCandy anchor) re-homed to
// src/Gruntz/BehindCandy.cpp (the ctor anchors GetTypeTag @0xfb70 + the
// ??_7CBehindCandy vtable there).

// CEyeCandy (ctor 0x0ac620 + ~CEyeCandy anchor) re-homed to src/Gruntz/EyeCandy.cpp
// (the ctor anchors GetTypeTag @0xfca0 + the ??_7CEyeCandy vtable in that TU).

// --- CFrontCandyAni::RegisterActs (0x0acd10) + AdvanceAnim (0x0acf10) re-homed ---
// These were a mis-attribution of CEyeCandyAni's acts (registry 0x646060) and now
// live in src/Gruntz/EyeCandyAni.cpp, killing the ?RegisterActs@CFrontCandyAni
// dup-RVA (they emit as ?RegisterActs@CEyeCandyAni / ?AdvanceAnim@CEyeCandyAni).

// CFrontCandyAni (ctor 0x0acf40 + ~CFrontCandyAni anchor) re-homed to
// src/Gruntz/FrontCandyAni.cpp.

// CBehindCandyAni (ctor 0x0ad540 + ~CBehindCandyAni anchor) re-homed to
// src/Gruntz/BehindCandyAni.cpp (the ctor anchors GetTypeTag @0x10030 + the
// ??_7CBehindCandyAni vtable there).

// CMenuSparkle (ctor 0x0adbe0 + ~dtor 0x101b0 + AdvanceAnim 0xae2a0) re-homed to
// src/Gruntz/MenuSparkle.cpp (the canonical CMenuSparkle view; the slot-1 SerializeMove
// 0xae1c0 stays under the Grunt.h serialize view in MenuSparkleSerial.cpp).

// CSingleAnimation (ctor 0x0ae7f0 + InitActReg/RunAct/RegisterActs/AdvanceAnim +
// ~anchor + the g_singleAnimActReg registry) re-homed to src/Gruntz/SingleAnimation.cpp.

// The tile-logic worker-pump family (0x10cb10..0x10d510, TileTriggerStep/
// TileTriggerSwitchStep/TileSecretTriggerStep/GiantRockStep/CoveredPowerupStep/
// WarpStonePadStep + the TILE_LOGIC_WORKER_PUMP macro) re-homed to
// src/Gruntz/TileLogicPump.cpp (it `new`s the tile-logic leaves via their headers).

// CWarpStonePad (ctor 0x10d650 + InitActReg/FireWarp/RegisterActs + SerializeMove
// 0x10f20 + ~dtor 0x10fc0 + GetTypeTag 0x10f00 + the g_warpStonePadActReg registry)
// re-homed to src/Gruntz/WarpStonePad.cpp. (The WarpStonePadStep pump above still
// `new`s CWarpStonePad via <Gruntz/WarpStonePad.h>.)

// CTileTriggerSwitch band re-homed to src/Gruntz/TileTriggerSwitch.cpp (see the note
// at its former class-view site above).

// The CTileTrigger family band (1-arg ctor 0x10e220 + Init/Fire/RegisterActs +
// SerializeMove 0x111f0 + the CTileSecretTrigger Init/Fire/RegisterActs band + the
// three leaf 1-arg ctors 0x10fa60/90/c0) re-homed to src/Gruntz/TileTrigger.cpp.

// CToobSpikez (ctor 0x1145c0 + ~CToobSpikez anchor) re-homed to
// src/Gruntz/ToobSpikez.cpp (the ctor anchors GetTypeTag @0x12ba0 + the
// ??_7CToobSpikez vtable there).

// @confidence: low
// @source: winapi:CopyRect
// @stub
RVA(0x0004d800, 0x423)
i32 CUserLogic::winapi_04d800_CopyRect(i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32) {
    return 0;
}

// winapi_064540_PostMessageA @0x64540 was re-homed to GruntEntranceArrival.cpp
// (wave3-I): its retail body is text-contained in the 0x616e0 obj.

// winapi_0ee800_IntersectRect_PtInRect: XREF-recovered as CGrunt::ArrivalReticleScan
// (its `this` extends to +0x3f0 and the caller 0x5d210 is CGrunt vtable slot 3) and
// re-homed to src/Gruntz/GruntReticle.cpp as a real CGrunt method.

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

// The per-frame grunt "decay/wand" AI (CGruntBehaviorLeaf: LoadGruntDecayConfig
// 0x612a0, LoadGruntDecayConfig2 0x61570, LoadWandGruntItemConfig 0x65a60) re-homed
// to src/Gruntz/GruntBehaviorLeaf.{h,cpp} (the placeholder-identity leaf + its
// CDecayMgr/CDecayAnim views).

// The one-shot "begin action" driver (ActionBeginHost::Begin @0x0d7220) re-homed to
// src/Gruntz/ActionBeginHost.cpp (an @identity-TODO host: its concrete class is gated
// on reconstructing this TU's LoadGruntTypeTable stub, which drives it).

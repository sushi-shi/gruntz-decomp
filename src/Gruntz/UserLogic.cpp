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
#include <Gruntz/TriggerMgr.h>
#include <Wap32/ZVec.h>
#include <Wap32/ZDArrayDerived.h>
#include <Bute/ButeTree.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/GruntSpawnConfig.h>
#include <Image/ImageSet.h>
#include <Mfc.h>             // RECT / CopyRect (CSingleFrameMessage centers in a bounds rect)
#include <Gruntz/ActReg.h>   // shared CActColl/CVariantSlot/CActReg activation-registry archetype
#include <Gruntz/WarpStonePad.h>       // the canonical CWarpStonePad class (ctor defined below)
#include <Gruntz/TeleSpriteFactory.h>  // shared teleporter HUD-sprite factory
#include <Gruntz/Trigger.h>            // shared point-probe result object
#include <Gruntz/SecretLevelTrigger.h> // canonical CSecretLevelTrigger (ctors defined below)
#include <Gruntz/VoiceTrigger.h>       // canonical CVoiceTrigger (no-arg ctor + GetTypeTag below)
#include <Gruntz/Viewport.h>           // shared world->screen transform
#include <Gruntz/SerialObjRef.h>       // the shared serialized-object-reference (Chain @0x8c00)
#include <Bute/SymTab.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/TileTriggerTransition.h> // CTileTransitionController/State worker-pump view
#include <Gruntz/TileTriggerSwitch.h>      // the canonical CTileTriggerSwitch (state-pump new-site)
#include <Gruntz/UserLogic.h>
#include <Gruntz/WwdGameReg.h> // the canonical WwdGameReg singleton (g_gameReg)
#include <rva.h>
#include <Globals.h>

// ---------------------------------------------------------------------------
// CButeTree (declared in <Bute/ButeMgr.h>, pulled via UserLogic.h) - the engine
// bute store the tile-logic tails query for their "A" node. g_buteTree
// (0x6bf620) is the global instance; Find (0x16d190) is matched in
// src/Stub/CButeTree.cpp. Declared extern only so `g_buteTree.Find("A")`
// reloc-masks (the Stub TU owns the DATA label).
// ---------------------------------------------------------------------------
DATA(0x002bf620)
extern CButeTree g_buteTree;

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
i32 CUserLogic::UserLogicVfunc3() {
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
i32 CUserLogic::UserLogicVfunc7() {
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

// The three CTileTrigger leaves (1-arg ctors, RTTI-named). Each adds no data
// members; the ctor chains CTileTrigger(obj) (out-of-line call) and the leaf vptr
// auto-stamps; the empty dtor folds the bare CUserLogic teardown. Their ctors
// were previously stubbed (manual-vptr) in src/Stub/{CTileSecretTrigger,CGiantRock,
// CCoveredPowerup}.cpp; modeled polymorphically here so the /GX EH-frame dtor folds
// (a manual-vptr model is frameless - see docs/patterns/eh-dtor-needs-base-subobject.md).
SIZE_UNKNOWN(CTileSecretTrigger);
VTBL(CTileSecretTrigger, 0x001e7e64); // vtable_names -> code (RTTI game class)
class CTileSecretTrigger : public CTileTrigger {
public:
    virtual LogicTypeId GetTypeTag() OVERRIDE; // slot 2
    CTileSecretTrigger(CGameObject* obj);      // 0x10fa60
    virtual ~CTileSecretTrigger() OVERRIDE;
    static void InitActReg(); // 0x10f160 (construct g_tileSecretTriggerActReg over [2000,2010])
    void FireActivation(i32 coord); // 0x10f1e0 (vtable slot 4 body: per-coord PMF dispatch)
    static void RegisterActs();     // 0x10f340 (binds "A"/"B" handlers)
    i32 Act_10f6a0();               // 0x10f6a0 ("A" handler)
    i32 Act_10f970();               // 0x10f970 ("B" handler)
};

SIZE_UNKNOWN(CGiantRock);
VTBL(CGiantRock, 0x001e7d5c); // vtable_names -> code (RTTI game class)
class CGiantRock : public CTileTrigger {
public:
    virtual LogicTypeId GetTypeTag() OVERRIDE; // slot 2
    CGiantRock(CGameObject* obj);              // 0x10fa90
    virtual ~CGiantRock() OVERRIDE;
};

SIZE_UNKNOWN(CCoveredPowerup);
VTBL(CCoveredPowerup, 0x001e7e0c); // vtable_names -> code (RTTI game class)
class CCoveredPowerup : public CTileTrigger {
public:
    virtual LogicTypeId GetTypeTag() OVERRIDE; // slot 2
    CCoveredPowerup(CGameObject* obj);         // 0x10fac0
    virtual ~CCoveredPowerup() OVERRIDE;
};

// CGruntHealthSprite (no-arg ctor 0x11ef0 + 1-arg ctor 0x7eb00 + dtor anchor) re-homed
// to src/Gruntz/GruntHealthSprite.cpp; the local view is dissolved onto the canonical
// <Gruntz/GruntHealthSprite.h> class.

// CVoiceTrigger is the canonical <Gruntz/VoiceTrigger.h> class (its no-arg ctor +
// GetTypeTag bodies stay here; the class shape is shared with VoiceTrigger.cpp).

// CTeleporter (ctor 0x041020 + GetTypeTag 0x10d80 + dtor anchor) re-homed to
// src/Gruntz/Teleporter.cpp; the local view is dissolved onto the canonical
// <Gruntz/Teleporter.h> class (which now owns the inline GetTypeTag + the
// EnterField1/EnterField2 this-methods).

// CSecretTeleporterTrigger is the canonical <Gruntz/SecretTeleporterTrigger.h>
// class (extracted so the anim-worker dispatch handler can `new` it); its
// out-of-line bodies (ctor/Serialize/FireActivation/SpawnTeleporter) stay here.
#include <Gruntz/SecretTeleporterTrigger.h>

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

// The global game registry several tails poll for level flags (WwdGameReg, the
// same symbol wwdfile labels at RVA 0x24556c; only the fields these tails read
// are modeled). Declared extern only - wwdfile owns the DATA label.
SIZE_UNKNOWN(WwdGameRegAux);
struct WwdGameRegAux {
    char m_pad00[0x3c];
    i32 m_3c; // +0x3c
};

// The on-screen-cue receiver (g_gameReg->m_cueSink). The teleporter spawn fires a
// 6-arg cue (CueA, ret 0x18, via the 0x39f4 thunk). External/no-body
// (reloc-masked).

// The point-probe sink (g_gameReg->m_68): given screen x/y, fills two out-ints
// and returns the trigger object hit (or 0). 5-arg __thiscall ret 0x14, via the
// 0x35f3 thunk (-> 0x75af0). External/no-body (reloc-masked).
struct CTrigger; // the trigger object the probe returns / the cue receives

// The viewport rect base reached as g_gameReg->m_world->m_24->m_5c + 0x40; the
// on-screen test reads its left/top/right/bottom (m_0/m_4/m_8/m_c).
SIZE_UNKNOWN(CViewRect);
struct CViewRect {
    i32 m_left;   // +0x00
    i32 m_top;    // +0x04
    i32 m_right;  // +0x08
    i32 m_bottom; // +0x0c
};
// CViewport (world->screen transform) is the shared <Gruntz/Viewport.h> class;
// here only the +0x5c visible-rect base pointer is read.

// The HUD sprite object the teleporter spawn produces / reads its template from
// (the trigger's m_10) is the shared CGameObject (the same engine object as in
// CTeleporter.cpp) - the spawn copies its tile/teleport-link fields directly (+0x114/
// +0x118/+0x11c/+0x120/+0x124/+0x128/+0x164/+0x168, and the +0x7c aux's +0xbc link id).

// The HUD sprite factory the spawn calls (g_gameReg->m_world->m_8->CreateSprite) is the
// shared <Gruntz/TeleSpriteFactory.h> class; its result is cast to CGameObject*.

// CTrigger (the object the probe returns; its m_10 is the HUD sprite read for the
// on-screen cue's coordinates) is the shared <Gruntz/Trigger.h> class.
SIZE_UNKNOWN(CTeleResHolder);
struct CTeleResHolder { // the +0x30 resource/sprite-factory holder
    char m_pad0[0x8];
    CTeleSpriteFactory* m_8; // +0x08
    char m_pad0c[0x24 - 0xc];
    CViewport* m_24; // +0x24  viewport (visible-bounds source)
};

// The game registry singleton (canonical <Gruntz/WwdGameReg.h>). These teleporter
// tails reach its facet: m_world downcast to CTeleResHolder*, m_cueSink to
// CTeleCueSink*, m_68 to CTriggerProbe*, m_7c to WwdGameRegAux*; GetMessageBounds
// returns the on-screen message-bounds RECT.
extern WwdGameReg* g_gameReg;

// ---------------------------------------------------------------------------
// The per-coordinate activation registry FireActivation (0x042150) dispatches
// through. A coordinate maps to an Entry* either directly (when within the
// fast [g_actLo,g_actHi] range) via g_actBase + (coord-g_actLo)*g_actStride, or
// by a slow lookup in g_actColl (0x16da80, __thiscall ret 8), which on miss
// rebuilds the table (g_actAlloc 0x16d990 -> g_actCache, g_actColl2 insert
// 0x16d850 __thiscall ret 0xc) and yields g_actCur. The entry's first dword is a
// fn-ptr table; a nonzero entry's handler is called __thiscall on `this`.
// All globals are unnamed BSS (DATA-pinned here so the loads reloc-mask); the
// collection methods are external/no-body.
// CActColl / CVariantSlot / GetRetAddr + g_actCache (0x6bf464) / g_retAddrBreadcrumb
// (0x6bf428) are the shared coordinate-registry collection primitives from
// <Gruntz/ActReg.h>. g_actColl (0x644688) is this TU's own collection singleton.
struct CActEntry; // an entry: first dword is the registered handler vtable

DATA(0x00244688)
extern CActColl g_actColl;

// The shared per-leaf activation-coordinate registry singleton each CUserLogic
// leaf's RegisterActs binds its id->handler entry in - same [2000,2010] range
// shape as g_actColl but a distinct per-class instance. CLeafActReg is the shared
// <Gruntz/ActReg.h> CActReg archetype (was a per-file duplicate of its layout +
// ResolveEntry); it keeps its own placeholder name so the DATA-pinned globals below
// are unchanged.
SIZE_UNKNOWN(CLeafActReg);
struct CLeafActReg : public CActReg {};
// (g_frontCandyActReg @0x646060 was the mis-attributed CEyeCandyAni registry - it
// re-homed to src/Gruntz/EyeCandyAni.cpp as g_eyeCandyActReg; 0x646060's DATA symbol
// is pinned in src/Gruntz/LogicDispatchInit.cpp as g_logicDispatch_646060.)
DATA(0x0024e810)
extern CLeafActReg g_tileTriggerActReg; // 0x64e810
DATA(0x0024e7e8)
extern CLeafActReg g_tileSecretTriggerActReg; // 0x64e7e8

// Each leaf's handler entry: its first dword receives the per-frame handler PMF
// (AdvanceAnim, a 4-byte code ptr on the single-inheritance class). (CFrontCandyAni's
// entry re-homed to src/Gruntz/EyeCandyAni.cpp with its RegisterActs.)
typedef i32 (CTileTrigger::*TileTriggerHandler)();
SIZE_UNKNOWN(CTileTriggerActEntry);
struct CTileTriggerActEntry {
    TileTriggerHandler m_fn;
};
typedef i32 (CTileSecretTrigger::*TileSecretTriggerHandler)();
SIZE_UNKNOWN(CTileSecretTriggerActEntry);
struct CTileSecretTriggerActEntry {
    TileSecretTriggerHandler m_fn;
};

// The Entry the registry yields: its first dword is the handler fn-ptr, a
// __thiscall called with the trigger as `this`.
class CSecretTeleporterTrigger;
// The entry's first dword is a pointer-to-member-function of the trigger class
// (single inheritance -> a 4-byte code pointer); FireActivation invokes it on
// `this`, emitting `mov ecx,this; call [entry]`.
typedef void (CSecretTeleporterTrigger::*ActHandler)();
SIZE_UNKNOWN(CActEntry);
struct CActEntry {
    ActHandler m_fn; // [entry]
};

// The inlined coordinate->Entry* lookup FireActivation folds in twice.
static inline CActEntry* ActLookup(i32 coord) {
    g_actScratch = 0;
    if (coord >= g_actLo && coord <= g_actHi) {
        return (CActEntry*)(g_actBase + (coord - g_actLo) * g_actStride);
    }
    if ((i32)((_zvec*)&g_actColl)->GrowTo(coord, 0)) {
        return (CActEntry*)(g_actBase + (coord - g_actLo) * g_actStride);
    }
    void* item = g_actCache;
    g_retAddrBreadcrumb = GetRetAddr();
    g_actColl2->Set(&g_actColl, (i32)item, 0xc);
    return g_actCur;
}

// ---------------------------------------------------------------------------
// The shared activation-NAME registry RegisterActs interns the key "A" through
// (@0x6bf650; same range/cache shape as g_actColl). g_buteTree doubles as the
// name->id map (Find returns the id, 0 == absent; Insert maps a new key->id);
// g_nextActId (0x61aea8) is the running id counter; s_actKeyA (0x60a454) is the
// "A" key. The id->name-slot resolve reuses the shared Find/GetRetAddr/Insert +
// g_actCache/g_retAddrBreadcrumb collection methods already declared above.
// ---------------------------------------------------------------------------
DATA(0x0021aea8)
extern i32 g_nextActId;
DATA(0x0020a454)
extern char s_actKeyA[];
DATA(0x0020d1bc)
extern char s_actKeyB[]; // "B"
DATA(0x002bf650)
extern CActColl g_nameReg; // 0x6bf650
DATA(0x002bf654)
extern CVariantSlot* g_nameReg2; // 0x6bf654
DATA(0x002bf658)
extern i32 g_nameRegLo;
DATA(0x002bf65c)
extern i32 g_nameRegHi;
DATA(0x002bf660)
extern char* g_nameRegBase;
DATA(0x002bf668)
extern i32 g_nameRegStride;
DATA(0x002bf664)
extern char* g_nameRegCur; // slow-path result slot
DATA(0x002bf66c)
extern void** g_nameRegCurList; // the slot's CString list base
DATA(0x002bf670)
extern i32 g_nameRegScratch; // zeroed first; doubles as the list count

// The CString in the resolved name slot: ~CString (0x1b9b93) frees the old list,
// operator= (0x1b9e74) assigns the new key. Modeled so the calls reloc-mask.
#include <Gruntz/ActName.h> // CActName (shared)
#include <Wap32/ZVec.h>
#include <Wap32/ZDArrayDerived.h>

// The id->name-slot resolve (the fast range path + the slow Find/GetRetAddr/Insert
// rebuild). Folded inline by RegisterActs once, in the new-id branch.
static inline char* ActNameLookup(i32 id) {
    g_nameRegScratch = 0;
    if (id >= g_nameRegLo && id <= g_nameRegHi) {
        return g_nameRegBase + (id - g_nameRegLo) * g_nameRegStride;
    }
    if ((i32)((_zvec*)&g_nameReg)->GrowTo(id, 0)) {
        return g_nameRegBase + (id - g_nameRegLo) * g_nameRegStride;
    }
    void* item = g_actCache;
    g_retAddrBreadcrumb = GetRetAddr();
    g_nameReg2->Set(&g_nameReg, (i32)item, 0xc);
    return g_nameRegCur;
}

// The activation-registry entry for SpawnTeleporter (an i32-returning handler PMF
// on the complete single-inheritance class).
typedef i32 (CSecretTeleporterTrigger::*SpawnHandler)();
SIZE_UNKNOWN(CTelActEntry);
struct CTelActEntry {
    SpawnHandler m_fn;
};

// ===========================================================================
// Definitions in ascending-RVA order.
// ===========================================================================

// --- CSecretTeleporterTrigger::Serialize (0x010a10), vtable slot 1 ---
// Chains the shared serialize helper on `this`, and (only on success) the +0x34
// serializable sub-object's chain; normalizes the result to a strict bool.
RVA(0x00010a10, 0x47)
i32 CSecretTeleporterTrigger::Serialize(i32 a, i32 b, i32 c, i32 d) {
    if (!SerializeChain(a, b, c, d)) {
        return 0;
    }
    return SerialRef34()->Chain((CSerialArchive*)a, b, c, (CSerialObj*)d) != 0;
}

// --- CSecretTeleporterTrigger::~CSecretTeleporterTrigger (0x010ab0) ---
// The leaf adds no members, so its dtor folds the bare CUserLogic teardown:
// store the CUserLogic vptr, inline-destruct the +0x18 link (the embedded
// ~EngStr call), store the CUserBase vptr. The destructible link forces the /GX
// EH frame. The fold requires ~CUserBase/~CUserLogic/~CUserBaseLink to be inline
// (see UserLogic.h); the empty leaf body below is enough for cl to emit it.
RVA(0x00010ab0, 0x44)
CSecretTeleporterTrigger::~CSecretTeleporterTrigger() {}

// CSecretLevelTrigger (no-arg ctor 0x10b20 + 1-arg ctor 0x424b0 + dtor anchor)
// re-homed to src/Gruntz/SecretLevelTrigger.cpp.

RVA(0x00011160, 0x4b)
CTileTrigger::CTileTrigger() {}

// --- CTileTrigger / leaf destructors (0x011290 / 0x011540 / 0x011600 / 0x0116c0) ---
// All four are the SAME folded CUserLogic teardown (store CUserLogic vptr,
// inline-destruct the +0x18 link via ~EngStr 0x16d2a0, store CUserBase vptr; the
// destructible link forces the /GX EH frame; leaf vptr store dead-eliminated).
// ~CTileTrigger is inline (header) so it folds into the three leaf dtors instead
// of being called. MSVC still emits one out-of-line COMDAT copy of ~CTileTrigger
// (called by its scalar-deleting dtor); it lands at 0x011290. An inline-defined
// dtor can't hang an RVA() (the attribute would also tag the synthesized ??_G ->
// duplicate-RVA), so it is pinned by mangled name here:
// @rva-symbol: ??1CTileTrigger@@UAE@XZ 0x00011290 0x44
RVA(0x00011540, 0x44)
CTileSecretTrigger::~CTileSecretTrigger() {}
RVA(0x00011600, 0x44)
CGiantRock::~CGiantRock() {}
RVA(0x000116c0, 0x44)
CCoveredPowerup::~CCoveredPowerup() {}

// CVoiceTrigger (no-arg ctor 0x13470 + GetTypeTag 0x133b0 + dtor anchor) re-homed to
// src/Gruntz/VoiceTrigger.cpp (joins its dtor 0x135a0 / Serialize 0x134e0 band).

// CUserLogic::GetScreenPos (0x00029a50) is now an inline member in the header.

// CUserLogic::IsAtSavedScreenPos (0x00029a80) is now an inline member in the header.

// CTeleporter ctor 0x041020 re-homed to src/Gruntz/Teleporter.cpp.

// --- CSecretTeleporterTrigger (0x041e90), vptr 0x5e7564 ---
RVA(0x00041e90, 0x1ac)
CSecretTeleporterTrigger::CSecretTeleporterTrigger(CGameObject* obj) : CUserLogic(obj) {
    TILE_LOGIC_SEED(obj);
    if (g_gameReg->m_isEasyMode == 0 && g_gameReg->m_134 == 1) {
        m_38->m_flags |= 0x10000;
    } else {
        m_object->m_screenX = (m_object->m_screenX & ~0x1f) + 0x10;
        m_object->m_screenY = (m_object->m_screenY & ~0x1f) + 0x10;
        if (m_object->m_latchedAnimId != 0) {
            m_object->m_latchedAnimId = 0;
            m_object->m_flags |= 0x20000;
        }
        m_38->m_flags |= 2;
        m_38->m_stateFlags |= 1;
        m_prevAnimSetNode = m_objAux->m_1c;
        m_objAux->m_1c = g_buteTree.Find("A");
        ((WwdGameRegAux*)g_gameReg->m_7c)->m_3c++;
    }
}

// --- CSecretTeleporterTrigger::InitActReg (0x0420d0) ---
// Construct the class's activation-coordinate registry singleton (g_actColl
// @0x644688) over the fixed range [2000, 2010] via the shared registry ctor
// (0x408710). Free init thunk; reloc-masked.
RVA(0x000420d0, 0x15)
void CSecretTeleporterTrigger::InitActReg() {
    ((CZDArrayDerived*)&g_actColl)->Construct(2000, 2010);
}

// --- CSecretTeleporterTrigger::FireActivation (0x042150), vtable slot 4 ---
// Look the activation coordinate up in the per-coordinate registry; if the entry
// has a registered handler, look it up again and dispatch it __thiscall on this.
RVA(0x00042150, 0x102)
void CSecretTeleporterTrigger::FireActivation(i32 coord) {
    CActEntry* e = ActLookup(coord);
    if (e->m_fn != 0) {
        CActEntry* e2 = ActLookup(coord);
        (this->*(e2->m_fn))();
    }
}

// --- CSecretTeleporterTrigger::RegisterActs (0x0422b0) ---
// Bind the per-point handler (SpawnTeleporter @0x042b80) to the activation key
// "A" via the shared name registry, then bind id->entry in the class's own
// coordinate registry (g_actColl). The SAME archetype as
// CSecretLevelTrigger::RegisterActs.
//
// @early-stop
// register-pinning wall (docs/patterns/zero-register-pinning.md +
// test-old-value-decrement-loop-while-postdec.md, topic:wall topic:regalloc): logic
// byte-faithful (every call/immediate/branch/offset + the `mov [entry],offset
// SpawnTeleporter` handler store match retail); residual is the slot-vs-id
// callee-saved register choice cascading into the free-loop count. Deferred.
RVA(0x000422b0, 0x18d)
void CSecretTeleporterTrigger::RegisterActs() {
    i32 id = (i32)g_buteTree.Find(s_actKeyA);
    if (id == 0) {
        id = g_nextActId;
        g_buteTree.Insert(s_actKeyA, (void*)id);
        char* slot = ActNameLookup(id);
        i32 n = g_nameRegScratch;
        void** list = g_nameRegCurList;
        while (n-- != 0) {
            if (list != 0) {
                ((CString*)list)->CString::~CString();
            }
            list++;
        }
        ((CString*)slot)->operator=(s_actKeyA);
        g_nextActId++;
    }
    ((CTelActEntry*)ActLookup(id))->m_fn = &CSecretTeleporterTrigger::SpawnTeleporter;
}

// CSecretLevelTrigger 1-arg ctor 0x0424b0 re-homed to src/Gruntz/SecretLevelTrigger.cpp.

// --- CSecretTeleporterTrigger::SpawnTeleporter (0x042b80) ---
// The registered point-activation callback: probe the trigger's screen point for
// a hit grunt; if hit, spawn the "Teleporter" HUD sprite at the (tile<<5)+0x10
// position, clone the trigger's teleport-link/tile fields into it, and (when the
// hit grunt is on-screen) fire the 6-arg cue. Always closes by marking the
// trigger sub-object hidden (m_38->m_08 |= 0x10000).
RVA(0x00042b80, 0x153)
i32 CSecretTeleporterTrigger::SpawnTeleporter() {
    i32 loc0, loc4;
    CGameObject* o = m_object;
    CTrigger* hit = (CTrigger*)((CTriggerMgr*)g_gameReg->m_68)
                        ->HitTestCell(o->m_screenX, o->m_screenY, &loc0, &loc4, 1);
    if (hit) {
        o = m_object;
        CTeleSpriteFactory* fac = ((CTeleResHolder*)g_gameReg->m_world)->m_8;
        CGameObject* spr = (CGameObject*)fac->CreateSprite(
            0,
            (o->m_114 << 5) + 0x10,
            (o->m_118 << 5) + 0x10,
            0,
            "Teleporter",
            0x40003
        );
        if (spr) {
            spr->m_124 = 2;
            spr->m_7c->m_bc = m_object->m_7c->m_bc;
            spr->m_164 = m_object->m_164;
            spr->m_168 = m_object->m_168;
            spr->m_11c = m_object->m_11c;
            spr->m_120 = m_object->m_120;
            spr->m_114 = m_object->m_114;
            spr->m_118 = m_object->m_118;
            spr->m_placeMode = 0;
            CGameObject* eo = hit->m_10;
            WwdGameReg* g = g_gameReg;
            i32 ey = eo->m_screenY;
            i32 ex = eo->m_screenX;
            CViewRect* rc = (CViewRect*)(((CTeleResHolder*)g->m_world)->m_24->m_5c + 0x40);
            if (ex < rc->m_right && ex >= rc->m_left && ey < rc->m_bottom && ey >= rc->m_top) {
                ((CGruntSpawnConfig*)g->m_cueSink)
                    ->SpawnVoiceDriver((i32)hit, 0x3fc, -1, 0, -1, -1);
            }
        }
        m_38->m_flags |= 0x10000;
    }
    return 0;
}

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

// ---------------------------------------------------------------------------
// The tile-logic worker-pump family (0x10cb10..0x10d510). Each is a free __cdecl
// /GX pump byte-identical to StepController @0x10d150 (src/Gruntz/TileTrigger
// Transition.cpp) bar the leaf TYPE `new`d on state 0: read the controller at
// obj->m_7c, dispatch on its state id, build the leaf on state 0 and dispatch to
// the state object's vtable slots otherwise. Shared via a macro; only the leaf +
// its `new`-size differ. In ascending retail-RVA order.
// ---------------------------------------------------------------------------
#define TILE_LOGIC_WORKER_PUMP(LEAF)                                                               \
    CTileTransitionController* ctl = (CTileTransitionController*)obj->m_7c;                        \
    switch (ctl->m_stateId) {                                                                      \
        case 0: {                                                                                  \
            ctl->m_stateId = 0x3e8;                                                                \
            LEAF* t = new LEAF(obj);                                                               \
            ((CTileTransitionState*)t)->Activate();                                                \
            ctl->m_state = (CTileTransitionState*)t;                                               \
            break;                                                                                 \
        }                                                                                          \
        case 0x1d:                                                                                 \
            ctl->m_state->Vfunc2C();                                                               \
            break;                                                                                 \
        case 0x1e:                                                                                 \
            ctl->m_state->Vfunc28();                                                               \
            break;                                                                                 \
        case 0x50:                                                                                 \
            ctl->m_state->Vfunc38();                                                               \
            break;                                                                                 \
        case 0x51:                                                                                 \
            ctl->m_state->Vfunc34();                                                               \
            break;                                                                                 \
        case 0x52:                                                                                 \
            ctl->m_state->Vfunc30();                                                               \
            break;                                                                                 \
        case 0x53:                                                                                 \
            ctl->m_state->Vfunc3C();                                                               \
            break;                                                                                 \
        case 0x3e8:                                                                                \
            break;                                                                                 \
        default:                                                                                   \
            TileTransitionDefaultStep(ctl->m_state);                                               \
            break;                                                                                 \
    }                                                                                              \
    return 1;

RVA(0x0010cb10, 0xf1)
i32 TileTriggerStep(CGameObject* obj){TILE_LOGIC_WORKER_PUMP(CTileTrigger)}

RVA(0x0010cc50, 0xf1)
i32 TileTriggerSwitchStep(CGameObject* obj){TILE_LOGIC_WORKER_PUMP(CTileTriggerSwitch)}

RVA(0x0010cd90, 0xf1)
i32 TileSecretTriggerStep(CGameObject* obj){TILE_LOGIC_WORKER_PUMP(CTileSecretTrigger)}

RVA(0x0010ced0, 0xf1)
i32 GiantRockStep(CGameObject* obj){TILE_LOGIC_WORKER_PUMP(CGiantRock)}

RVA(0x0010d010, 0xf1)
i32 CoveredPowerupStep(CGameObject* obj){TILE_LOGIC_WORKER_PUMP(CCoveredPowerup)}

RVA(0x0010d510, 0xf1)
i32 WarpStonePadStep(CGameObject* obj){TILE_LOGIC_WORKER_PUMP(CWarpStonePad)}

// CWarpStonePad (ctor 0x10d650 + InitActReg/FireWarp/RegisterActs + SerializeMove
// 0x10f20 + ~dtor 0x10fc0 + GetTypeTag 0x10f00 + the g_warpStonePadActReg registry)
// re-homed to src/Gruntz/WarpStonePad.cpp. (The WarpStonePadStep pump above still
// `new`s CWarpStonePad via <Gruntz/WarpStonePad.h>.)

// CTileTriggerSwitch band re-homed to src/Gruntz/TileTriggerSwitch.cpp (see the note
// at its former class-view site above).

// --- CTileTrigger 1-arg (0x10e220), vptr 0x5e7f14 ---
RVA(0x0010e220, 0x17d)
CTileTrigger::CTileTrigger(CGameObject* obj) : CUserLogic(obj) {
    TILE_LOGIC_SEED(obj);
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    m_38->m_flags |= 2;
    m_38->m_flags |= 1;
    m_38->m_stateFlags |= 1;
    m_object->m_164 = m_object->m_screenX >> 5;
    m_object->m_168 = m_object->m_screenY >> 5;
    m_object->m_04 = (m_object->m_164 << 8) + m_object->m_168;
}

// --- CTileTrigger::InitActReg (0x10e420) ---
// Construct the class's activation-coordinate registry singleton
// (g_tileTriggerActReg @0x64e810) over the fixed range [2000, 2010] via the
// shared registry ctor (0x408710). Free init thunk; reloc-masked.
RVA(0x0010e420, 0x15)
void CTileTrigger::InitActReg() {
    ((CZDArrayDerived*)&g_tileTriggerActReg)->Construct(2000, 2010);
}

// --- CTileTrigger::FireActivation (0x10e4a0), vtable slot 4 ---
// Look the activation coordinate up in the class registry (g_tileTriggerActReg); if the
// resolved entry carries a registered handler PMF, resolve it again and dispatch it
// __thiscall on `this`. Same archetype as CWarpStonePad::FireWarp.
RVA(0x0010e4a0, 0x102)
void CTileTrigger::FireActivation(i32 coord) {
    CTileTriggerActEntry* e = (CTileTriggerActEntry*)g_tileTriggerActReg.ResolveEntry(coord);
    if (e->m_fn != 0) {
        CTileTriggerActEntry* e2 = (CTileTriggerActEntry*)g_tileTriggerActReg.ResolveEntry(coord);
        (this->*(e2->m_fn))();
    }
}

// --- CTileTrigger::RegisterActs (0x10e600) ---
// Bind the per-frame handler (AdvanceAnim @0x10ee00) to the activation key "A"
// via the shared name registry + the class's coordinate registry
// (g_tileTriggerActReg). SAME archetype as CSecretTeleporterTrigger::RegisterActs.
//
// @early-stop
// register-pinning wall (docs/patterns/zero-register-pinning.md +
// test-old-value-decrement-loop-while-postdec.md, topic:wall topic:regalloc): logic
// byte-faithful (every call/immediate/branch/offset + the handler store match
// retail); residual is the slot-vs-id callee-saved register choice cascading into
// the free-loop count materialization. Deferred.
RVA(0x0010e600, 0x18d)
void CTileTrigger::RegisterActs() {
    i32 id = (i32)g_buteTree.Find(s_actKeyA);
    if (id == 0) {
        id = g_nextActId;
        g_buteTree.Insert(s_actKeyA, (void*)id);
        char* slot = ActNameLookup(id);
        i32 n = g_nameRegScratch;
        void** list = g_nameRegCurList;
        while (n-- != 0) {
            if (list != 0) {
                ((CString*)list)->CString::~CString();
            }
            list++;
        }
        ((CString*)slot)->operator=(s_actKeyA);
        g_nextActId++;
    }
    ((CTileTriggerActEntry*)g_tileTriggerActReg.ResolveEntry(id))->m_fn =
        &CTileTrigger::AdvanceAnim;
}

// --- CTileTrigger::SerializeMove (0x111f0), vtable slot 1 ---
// Base impl shared (inherited) by CGiantRock/CCoveredPowerup/CTileSecretTrigger
// (their slot-1 vtable entries all point here - no leaf override).
RVA(0x000111f0, 0x47)
i32 CTileTrigger::SerializeMove(CGruntArchive* ar, i32 mode, i32 a3, i32 a4) {
    if (!SerializeChain((i32)ar, mode, a3, a4)) {
        return 0;
    }
    return SerialRef34()->Chain((CSerialArchive*)ar, mode, a3, (CSerialObj*)a4) != 0;
}

// --- CTileSecretTrigger::InitActReg (0x10f160) ---
// Construct the class's activation-coordinate registry singleton
// (g_tileSecretTriggerActReg @0x64e7e8) over the fixed range [2000, 2010] via the
// shared registry ctor (0x408710). Free init thunk; reloc-masked.
RVA(0x0010f160, 0x15)
void CTileSecretTrigger::InitActReg() {
    ((CZDArrayDerived*)&g_tileSecretTriggerActReg)->Construct(2000, 2010);
}

// --- CTileSecretTrigger::FireActivation (0x10f1e0), vtable slot 4 ---
// Look the activation coordinate up in the class registry (g_tileSecretTriggerActReg);
// if the resolved entry carries a registered handler PMF, resolve it again and dispatch
// it __thiscall on `this`. Same archetype as CWarpStonePad::FireWarp.
RVA(0x0010f1e0, 0x102)
void CTileSecretTrigger::FireActivation(i32 coord) {
    CTileSecretTriggerActEntry* e =
        (CTileSecretTriggerActEntry*)g_tileSecretTriggerActReg.ResolveEntry(coord);
    if (e->m_fn != 0) {
        CTileSecretTriggerActEntry* e2 =
            (CTileSecretTriggerActEntry*)g_tileSecretTriggerActReg.ResolveEntry(coord);
        (this->*(e2->m_fn))();
    }
}

// --- CTileSecretTrigger::RegisterActs (0x10f340) ---
// Intern "A" and "B" and bind each to its per-frame handler (0x10f6a0 / 0x10f970)
// in the class's coordinate registry (g_tileSecretTriggerActReg). Two back-to-back
// single-key registrations; the SAME archetype as CTileTrigger::RegisterActs done
// twice.
//
// @early-stop
// register-pinning wall (docs/patterns/zero-register-pinning.md +
// test-old-value-decrement-loop-while-postdec.md, topic:wall topic:regalloc): logic
// byte-faithful (both intern/name-resolve blocks + the OWN-registry resolves + the
// `mov [entry],offset handler` stores match retail); residual is the slot-vs-id
// callee-saved register choice cascading into the free-loop counts. Deferred.
RVA(0x0010f340, 0x2ac)
void CTileSecretTrigger::RegisterActs() {
    i32 id = (i32)g_buteTree.Find(s_actKeyA);
    if (id == 0) {
        id = g_nextActId;
        g_buteTree.Insert(s_actKeyA, (void*)id);
        char* slot = ActNameLookup(id);
        i32 n = g_nameRegScratch;
        void** list = g_nameRegCurList;
        while (n-- != 0) {
            if (list != 0) {
                ((CString*)list)->CString::~CString();
            }
            list++;
        }
        ((CString*)slot)->operator=(s_actKeyA);
        g_nextActId++;
    }
    ((CTileSecretTriggerActEntry*)g_tileSecretTriggerActReg.ResolveEntry(id))->m_fn =
        &CTileSecretTrigger::Act_10f6a0;

    i32 id2 = (i32)g_buteTree.Find(s_actKeyB);
    if (id2 == 0) {
        id2 = g_nextActId;
        g_buteTree.Insert(s_actKeyB, (void*)id2);
        char* slot = ActNameLookup(id2);
        i32 n = g_nameRegScratch;
        void** list = g_nameRegCurList;
        while (n-- != 0) {
            if (list != 0) {
                ((CString*)list)->CString::~CString();
            }
            list++;
        }
        ((CString*)slot)->operator=(s_actKeyB);
        g_nextActId++;
    }
    ((CTileSecretTriggerActEntry*)g_tileSecretTriggerActReg.ResolveEntry(id2))->m_fn =
        &CTileSecretTrigger::Act_10f970;
}

// --- The three CTileTrigger leaves' 1-arg ctors (0x10fa60/90/c0) ---
// Each just chains CTileTrigger(obj) (out-of-line call) then the leaf vptr
// auto-stamps. vptrs: CTileSecretTrigger 0x5e7e64, CGiantRock 0x5e7d5c,
// CCoveredPowerup 0x5e7e0c.
RVA(0x0010fa60, 0x19)
CTileSecretTrigger::CTileSecretTrigger(CGameObject* obj) : CTileTrigger(obj) {}
RVA(0x0010fa90, 0x19)
CGiantRock::CGiantRock(CGameObject* obj) : CTileTrigger(obj) {}
RVA(0x0010fac0, 0x19)
CCoveredPowerup::CCoveredPowerup(CGameObject* obj) : CTileTrigger(obj) {}

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

// winapi_064540 (0x64540): a per-frame "warp-to-level" trigger on a large grunt-logic
// leaf (the `this` extends CUserLogic out to +0x360; the leaf isn't modeled, so its
// fields are reached through a TU-local offset view). Poke the +0x1a0 arrival sub-object;
// if it has arrived (m_28) and isn't busy (m_20), and this is warp-mode 0xc, format the
// destination "WORLDZ\LEVEL%i" key and (if that level exists) PostMessage a WM_COMMAND
// (0x807f) to the mgr's top-level window; then, unless suppressed (m_36c), fire the
// arrival anim (m_260->Anim2a72), and latch the object dirty (m_154->m_8 |= 0x10000).
// The CString + Format + the sub-object/anim/level-lookup callees all reloc-mask.
SIZE_UNKNOWN(CWarpM154);
struct CWarpM154 {
    char m_pad00[0x8];
    i32 m_8; // +0x08 dirty flags
    char m_pad0c[0x1a0 - 0xc];
    CAniAdvanceCursor m_1a0; // +0x1a0
};
SIZE_UNKNOWN(CWarpLevelReg);
struct CWarpLevelReg {
    char m_pad00[0x1c];
    i32 m_baseLevel; // +0x1c base level number
    char m_pad20[0x28 - 0x20];
    CSymTab* m_28; // +0x28
};
SIZE_UNKNOWN(CWarpMgrWnd);
struct CWarpMgrWnd {
    char m_pad00[0x4];
    void* m_4; // +0x04 top-level HWND
};
SIZE_UNKNOWN(CWarpMgr);
struct CWarpMgr {
    char m_pad00[0x4];
    CWarpMgrWnd* m_4; // +0x04
    char m_pad08[0x2c - 0x8];
    CWarpLevelReg* m_curState; // +0x2c
};
SIZE_UNKNOWN(CWarpLeaf);
struct CWarpLeaf { // offset view of the grunt-logic leaf `this`
    char m_pad000[0x154];
    CWarpM154* m_drawState; // +0x154
    char m_pad158[0x1ec - 0x158];
    i32 m_animArg0; // +0x1ec anim arg 0
    i32 m_animArg1; // +0x1f0 anim arg 1
    char m_pad1f4[0x260 - 0x1f4];
    CTriggerMgr* m_animObj; // +0x260
    char m_pad264[0x360 - 0x264];
    i32 m_warpMode; // +0x360 warp mode
    char m_pad364[0x36c - 0x364];
    i32 m_animSuppress; // +0x36c anim-suppress gate
};
// The frame-clock snapshot fed to the arrival poke (ds:0x6bf3bc).
extern "C" i32 g_6bf3bc;
// The mgr singleton (same 0x64556c datum) + the WM_COMMAND PostMessageA IAT slot.
DATA(0x0024556c)
extern "C" CWarpMgr* g_mgrSettings;
typedef i32(WINAPI* WarpPostFn)(void* hwnd, unsigned msg, unsigned wp, i32 lp);
DATA(0x002c44c8)
extern WarpPostFn g_pPostMessageA;
// @early-stop
// 86.4%: logic byte-faithful. Residual is the leaf's offset-view register scheduling
// (the m_drawState reloads) + the /GX CString unwind state ordering; not source-steerable.
RVA(0x00064540, 0x11c)
i32 CUserLogic::winapi_064540_PostMessageA() {
    CWarpLeaf* self = (CWarpLeaf*)this;
    self->m_drawState->m_1a0.Advance_15c360(g_6bf3bc);
    CAniAdvanceCursor* sub = &self->m_drawState->m_1a0;
    if (sub->m_28 == 0) {
        return 0;
    }
    if (sub->m_20 != 0) {
        return 0;
    }
    if (self->m_warpMode == 0xc) {
        CWarpLevelReg* reg = g_mgrSettings->m_curState;
        i32 lvl = reg->m_baseLevel + 0x64;
        CString s;
        s.Format("WORLDZ\\LEVEL%i", lvl);
        if (reg->m_28->ResolveQualified((LPCTSTR)s, (void*)0x575744)) {
            g_pPostMessageA(g_mgrSettings->m_4->m_4, 0x111, 0x807f, lvl);
        }
    }
    if (self->m_animSuppress == 0) {
        self->m_animObj->NotifyCell(self->m_animArg0, self->m_animArg1, 1);
    }
    self->m_drawState->m_8 |= 0x10000;
    return 0;
}

// @confidence: low
// @source: winapi:IntersectRect;PtInRect
// @stub
RVA(0x000ee800, 0x971)
i32 CUserLogic::winapi_0ee800_IntersectRect_PtInRect() {
    return 0;
}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x0004dd50, 0x22c0)
void CUserLogic::LoadGruntTypeTable(i32, i32, i32, i32) {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x0005d210, 0x1443)
void CUserLogic::LoadGruntTuningConstants(i32) {}

// The per-frame grunt "decay/wand" AI (CGruntBehaviorLeaf: LoadGruntDecayConfig
// 0x612a0, LoadGruntDecayConfig2 0x61570, LoadWandGruntItemConfig 0x65a60) re-homed
// to src/Gruntz/GruntBehaviorLeaf.{h,cpp} (the placeholder-identity leaf + its
// CDecayMgr/CDecayAnim views).

// ---------------------------------------------------------------------------
// The one-shot "begin action" (0x0d7220), re-homed from the ApiCaller stubs:
// CUserLogic::LoadGruntTypeTable (0x04dd50) drives it on a game-object action
// state (fields at +0x40c..+0x510). Guarded by m_500: if already begun, bail;
// otherwise let the +0x410 sub accept the arg (0x1bedde), stash it, arm the
// state, post WM_COMMAND 0x816e to the top window, and set bit 0 on the +0x4e4
// peer. A placeholder host whose concrete class is not yet recovered; offsets +
// code bytes load-bearing.
// The base CGameMgr window-holder chain g_gameReg->m_gameWnd->m_hwnd that
// WwdGameReg pads over (+0x04); reached via a minimal cast.
struct ActionRegWnd {
    char m_pad0[4];
    HWND m_hwnd; // +0x04
};
struct ActionRegWndHolder {
    char m_pad0[4];
    ActionRegWnd* m_gameWnd; // +0x04
};
struct ActionAcceptSub {
    i32 Accept(i32 arg); // thiscall, RVA 0x1bedde (on this+0x410)
};
struct ActionPeer {
    char m_pad0[0x40];
    i32 m_40; // +0x40
};
struct ActionBeginHost {
    char m_pad0[0x40c];
    i32 m_40c;             // +0x40c
    ActionAcceptSub m_410; // +0x410 (empty view, 1 byte)
    char m_pad411[0x4e4 - 0x411];
    ActionPeer* m_4e4; // +0x4e4
    char m_pad4e8[0x500 - 0x4e8];
    i32 m_500; // +0x500
    char m_pad504[0x510 - 0x504];
    i32 m_510; // +0x510
    i32 Begin(i32 arg);
};
SIZE_UNKNOWN(ActionRegWnd);
SIZE_UNKNOWN(ActionRegWndHolder);
SIZE_UNKNOWN(ActionAcceptSub);
SIZE_UNKNOWN(ActionPeer);
SIZE_UNKNOWN(ActionBeginHost);
RVA(0x000d7220, 0x7b)
i32 ActionBeginHost::Begin(i32 arg) {
    if (m_500) {
        return 0;
    }
    if (!m_410.Accept(arg)) {
        return 0;
    }
    m_40c = arg;
    m_510 = 2;
    m_500 = 1;
    PostMessageA(((ActionRegWndHolder*)g_gameReg)->m_gameWnd->m_hwnd, 0x111, 0x816e, 0);
    if (m_4e4) {
        m_4e4->m_40 |= 1;
    }
    return 1;
}

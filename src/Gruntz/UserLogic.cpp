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
#include <Gruntz/AniCycle.h> // the canonical CAniCycle class (ctor defined below)
#include <Gruntz/BehindCandy.h>        // the canonical CBehindCandy class (ctor defined below)
#include <Gruntz/BehindCandyAni.h>     // the canonical CBehindCandyAni class (ctor defined below)
#include <Gruntz/EyeCandy.h>           // the canonical CEyeCandy class (ctor defined below)
#include <Gruntz/Particlez.h>          // the canonical CParticlez class (ctor defined below)
#include <Gruntz/SimpleAnimation.h>    // the canonical CSimpleAnimation class (ctor defined below)
#include <Gruntz/SingleAnimation.h>    // the canonical CSingleAnimation class (ctor defined below)
#include <Gruntz/SingleFrameMessage.h> // the canonical CSingleFrameMessage class (ctor defined below)
#include <Gruntz/TeleSpriteFactory.h>  // shared teleporter HUD-sprite factory
#include <Gruntz/ToobSpikez.h>         // the canonical CToobSpikez class (ctor defined below)
#include <Gruntz/Trigger.h>            // shared point-probe result object
#include <Gruntz/SecretLevelTrigger.h> // canonical CSecretLevelTrigger (ctors defined below)
#include <Gruntz/VoiceTrigger.h>       // canonical CVoiceTrigger (no-arg ctor + GetTypeTag below)
#include <Gruntz/Viewport.h>           // shared world->screen transform
#include <Gruntz/SerialObjRef.h>       // the shared serialized-object-reference (Chain @0x8c00)
#include <Bute/SymTab.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/UserLogic.h>
#include <Gruntz/WwdGameReg.h> // the canonical WwdGameReg singleton (g_gameReg)
#include <rva.h>
#include <Globals.h>

#include <stdlib.h> // rand (0x11fee0; CMenuSparkle timer seed)

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
    static void InitActReg();   // 0x10f160 (construct g_tileSecretTriggerActReg over [2000,2010])
    static void RegisterActs(); // 0x10f340 (binds "A"/"B" handlers)
    i32 Act_10f6a0();           // 0x10f6a0 ("A" handler)
    i32 Act_10f970();           // 0x10f970 ("B" handler)
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

class CGruntHealthSprite : public CUserLogic {
    virtual i32 Vslot16();                     // slot 16 (new)
    virtual LogicTypeId GetTypeTag() OVERRIDE; // slot 2
public:
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    virtual i32 UserLogicVfunc2() OVERRIDE;                            // slot 4
    TILE_LOGIC_TAIL
public:
    CGruntHealthSprite();
    CGruntHealthSprite(CGameObject* obj); // 0x07eb00 (1-arg)
    virtual ~CGruntHealthSprite() OVERRIDE;
    char m_pad40[0x5c - 0x40]; // CUserLogic ends +0x40; anchors at +0x5c
    i32 m_5c;                  // +0x5c
    i32 m_60;                  // +0x60
};
// VTBL(CGruntHealthSprite, 0x1e7ba4) is bound canonically in <Gruntz/GruntHealthSprite.h>
// (scanned tree-wide); this TU keeps a local class view for the ctor/dtor bodies below.

// CVoiceTrigger is the canonical <Gruntz/VoiceTrigger.h> class (its no-arg ctor +
// GetTypeTag bodies stay here; the class shape is shared with VoiceTrigger.cpp).

class CTeleporter : public CUserLogic {
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    virtual LogicTypeId GetTypeTag() OVERRIDE;                         // slot 2
    virtual i32 UserLogicVfunc2() OVERRIDE;                            // slot 4
public:
    TILE_LOGIC_TAIL
public:
    CTeleporter(CGameObject* obj); // 0x041020
    virtual ~CTeleporter() OVERRIDE;
    char m_pad40[0x58 - 0x40];  // CUserLogic ends +0x40; CTeleporter fields at +0x58
    i32 m_58, m_5c, m_60, m_64; // +0x58..+0x67
    void EnterField1();         // 0x1771 (this-method, no body)
    void EnterField2();         // 0x27d9 (this-method, no body)
};

SIZE_UNKNOWN(CSecretTeleporterTrigger);
class CSecretTeleporterTrigger : public CUserLogic {
public:
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    virtual LogicTypeId GetTypeTag() OVERRIDE;                         // slot 2
    virtual i32 UserLogicVfunc2() OVERRIDE;                            // slot 4
    TILE_LOGIC_TAIL
public:
    CSecretTeleporterTrigger(CGameObject* obj); // 0x041e90
    virtual ~CSecretTeleporterTrigger() OVERRIDE;
    // Construct the class's activation-coordinate registry (g_actColl @0x644688)
    // over the fixed [2000,2010] range; a free init thunk, reloc-masked.
    static void InitActReg(); // 0x0420d0
    // Bind SpawnTeleporter to the activation key "A" via the shared name registry
    // (the same archetype as CSecretLevelTrigger::RegisterActs).
    static void RegisterActs(); // 0x0422b0
    // The two overridden CUserLogic virtuals reconstructed below.
    i32 Serialize(i32 a, i32 b, i32 c, i32 d); // 0x010a10 (vtable slot 1)
    void FireActivation(i32 coord);            // 0x042150 (vtable slot 4)
    // The registered point-activation callback 0x042b80 stamped into the
    // coordinate registry by FireActivation. __thiscall, no args, returns int.
    i32 SpawnTeleporter(); // 0x042b80
};
VTBL(CSecretTeleporterTrigger, 0x1e7564);

SIZE_UNKNOWN(CWarpStonePad);
VTBL(CWarpStonePad, 0x001e71ac); // vtable_names -> code (RTTI game class)
class CWarpStonePad : public CUserLogic {
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    virtual LogicTypeId GetTypeTag() OVERRIDE;                         // slot 2
    virtual i32 UserLogicVfunc2() OVERRIDE;                            // slot 4
public:
    TILE_LOGIC_TAIL
public:
    CWarpStonePad(CGameObject* obj); // 0x10d650
    virtual ~CWarpStonePad() OVERRIDE;
    static void InitActReg();   // 0x10d840
    void FireWarp(i32 coord);   // 0x10d8c0 (vtable slot 4)
    static void RegisterActs(); // 0x10da20
    i32 AdvanceAnim();          // 0x10dc20
};

SIZE_UNKNOWN(CTileTriggerSwitch);
class CTileTriggerSwitch : public CUserLogic {
    virtual LogicTypeId GetTypeTag() OVERRIDE; // slot 2
public:
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    virtual i32 UserLogicVfunc2() OVERRIDE;                            // slot 4
    TILE_LOGIC_TAIL
public:
    CTileTriggerSwitch(CGameObject* obj); // 0x10dc40
    virtual ~CTileTriggerSwitch() OVERRIDE;
    static void InitActReg();   // 0x10de20
    static void RegisterActs(); // 0x10e000
    i32 AdvanceAnim();          // 0x10e200
};
VTBL(CTileTriggerSwitch, 0x1e7f6c);

// CTileTriggerTransition (vptr 0x5e7db4) + its leaf methods and state pump now
// live in src/Gruntz/TileTriggerTransition.cpp.

// CToobSpikez comes from <Gruntz/ToobSpikez.h> (folded; ctor 0x1145c0 defined below).

// CParticlez comes from <Gruntz/Particlez.h> (folded; ctor 0x046ad0 + GetTypeTag 0x012cd0 below).

// CAniCycle comes from <Gruntz/AniCycle.h> (folded; ctor 0x0aad20 defined below).

// CSingleAnimation comes from <Gruntz/SingleAnimation.h> (folded; ctor 0x0ae7f0 defined below).

// ---------------------------------------------------------------------------
// The CGruntSprite-family leaves (1-arg ctors). Each folds the inline
// CUserLogic(obj) base, stamps its own vptr, then runs a per-class sprite tail
// (ApplyLookupSprite/ApplyName + ApplyLookupGeometry, the "A" bute seed, a pose
// id force, anchor fields). m_5c/m_60 (CGruntHealthSprite/CGruntToySprite) and
// m_40 (the geometry token cache, the rest) are leaf fields.
// ---------------------------------------------------------------------------
class CGruntSelectedSprite : public CUserLogic {
    virtual i32 UserLogicVfunc2() OVERRIDE; // slot 4
public:
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    virtual LogicTypeId GetTypeTag() OVERRIDE;                         // slot 2
    TILE_LOGIC_TAIL
public:
    CGruntSelectedSprite(CGameObject* obj); // 0x07e3e0
    virtual ~CGruntSelectedSprite() OVERRIDE;
    i32 m_40; // +0x40
};
VTBL(CGruntSelectedSprite, 0x1e7bfc);

class CGruntToySprite : public CUserLogic {
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    virtual LogicTypeId GetTypeTag() OVERRIDE;                         // slot 2
    virtual i32 UserLogicVfunc2() OVERRIDE;                            // slot 4
public:
    TILE_LOGIC_TAIL
public:
    CGruntToySprite(CGameObject* obj); // 0x07f350
    virtual ~CGruntToySprite() OVERRIDE;
    char m_pad40[0x5c - 0x40];
    i32 m_5c; // +0x5c
};

class CGruntPowerupSprite : public CUserLogic {
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    virtual LogicTypeId GetTypeTag() OVERRIDE;                         // slot 2
public:
    virtual i32 UserLogicVfunc2() OVERRIDE; // slot 4
    TILE_LOGIC_TAIL
public:
    CGruntPowerupSprite(CGameObject* obj); // 0x07fdb0
    virtual ~CGruntPowerupSprite() OVERRIDE;
    i32 m_40; // +0x40
};
VTBL(CGruntPowerupSprite, 0x1e76c4);

// ---------------------------------------------------------------------------
// The eyecandy / simple-animation leaves (1-arg ctors). They share a common
// z-clamp tail: poll the +0x198 layer's bounds against g_buteMgr's
// World/BigActHeight, then toggle the +0x7c sub-object's flag bits. m_40 caches
// the geometry token where a tail reuses it.
// ---------------------------------------------------------------------------
// CSingleFrameMessage comes from <Gruntz/SingleFrameMessage.h> (folded; ctor 0x0ab310 defined below).

// CSimpleAnimation comes from <Gruntz/SimpleAnimation.h> (folded; ctor 0x0ab940 defined below).

SIZE_UNKNOWN(CFrontCandy);
class CFrontCandy : public CUserLogic {
public:
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    virtual LogicTypeId GetTypeTag() OVERRIDE;                         // slot 2
    TILE_LOGIC_TAIL
public:
    CFrontCandy(CGameObject* obj); // 0x0abfa0
    virtual ~CFrontCandy() OVERRIDE;
};
VTBL(CFrontCandy, 0x1e84ec);

// CBehindCandy comes from <Gruntz/BehindCandy.h> (folded; ctor 0x0ac3f0 defined below).

// CEyeCandy comes from <Gruntz/EyeCandy.h> (folded; ctor 0x0ac620 defined below).

// This local is ONLY the genuine CFrontCandyAni ctor 0x0acf40 (vptr 0x5e83e4, Ghidra
// RTTI-confirmed) - a partial view, NOT foldable into <Gruntz/FrontCandyAni.h>. The
// RegisterActs 0x0acd10 (registry 0x646060) + AdvanceAnim 0x0acf10 that used to sit
// here were a MIS-ATTRIBUTION: layout order proves they belong to CEyeCandyAni (ctor
// 0xac870) - the candy cluster runs {ctor 0xac870, InitActReg 0xacb30 -> 0x646060,
// FireActivation, RegisterActs 0xacd10, AdvanceAnim 0xacf10} contiguously, all in
// CEyeCandyAni's run BEFORE this ctor. They have been re-homed to src/Gruntz/
// CEyeCandyAni.cpp (0x646060 is CEyeCandyAni's registry). The real CFrontCandyAni
// acts (registry 0x6460b0) stay in src/Gruntz/FrontCandyAni.cpp at 0x0ad310/0x0ad510,
// so ?RegisterActs@CFrontCandyAni@@SAXXZ is now emitted at ONE RVA (0x0ad310), not two.
class CFrontCandyAni : public CUserLogic {
public:
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    virtual LogicTypeId GetTypeTag() OVERRIDE;                         // slot 2
    virtual i32 UserLogicVfunc2() OVERRIDE;                            // slot 4
    TILE_LOGIC_TAIL
public:
    CFrontCandyAni(CGameObject* obj); // 0x0acf40
    virtual ~CFrontCandyAni() OVERRIDE;
    i32 m_40; // +0x40
};
VTBL(CFrontCandyAni, 0x1e83e4);

// CBehindCandyAni comes from <Gruntz/BehindCandyAni.h> (folded; ctor 0x0ad540 below).

class CMenuSparkle : public CUserLogic {
public:
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    virtual LogicTypeId GetTypeTag() OVERRIDE;                         // slot 2
    virtual i32 UserLogicVfunc2() OVERRIDE;                            // slot 4
    TILE_LOGIC_TAIL
public:
    CMenuSparkle(CGameObject* obj); // 0x0adbe0
    virtual ~CMenuSparkle() OVERRIDE;
    i32 m_40; // +0x40
};
VTBL(CMenuSparkle, 0x1e82dc);

// CPathHazard (0x13170, no-arg): same folded base schedule, then zeroes its own
// eight pointer fields at +0x108..+0x12c.
class CPathHazard : public CUserLogic {
public:
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    virtual LogicTypeId GetTypeTag() OVERRIDE;                         // slot 2
    virtual i32 UserLogicVfunc2() OVERRIDE;                            // slot 4
    TILE_LOGIC_TAIL
public:
    CPathHazard();
    virtual ~CPathHazard() OVERRIDE;
    char m_pad[0x108 - 0x40]; // pad CUserLogic (ends +0x40) .. +0x107
    void* m_108;
    void* m_10c;
    void* m_110;
    void* m_114;
    char m_pad118[0x120 - 0x118];
    void* m_120;
    void* m_124;
    void* m_128;
    void* m_12c;
};
VTBL(CPathHazard, 0x1e7394);

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
DATA(0x00245f70)
extern CLeafActReg g_singleAnimActReg; // 0x645f70
DATA(0x0024e6a0)
extern CLeafActReg g_warpStonePadActReg; // 0x64e6a0
DATA(0x0024e810)
extern CLeafActReg g_tileTriggerActReg; // 0x64e810
DATA(0x0024e798)
extern CLeafActReg g_tileTriggerSwitchActReg; // 0x64e798
DATA(0x0024e7e8)
extern CLeafActReg g_tileSecretTriggerActReg; // 0x64e7e8

// Each leaf's handler entry: its first dword receives the per-frame handler PMF
// (AdvanceAnim, a 4-byte code ptr on the single-inheritance class). (CFrontCandyAni's
// entry re-homed to src/Gruntz/EyeCandyAni.cpp with its RegisterActs.)
typedef i32 (CSingleAnimation::*SingleAnimHandler)();
SIZE_UNKNOWN(CSingleAnimActEntry);
struct CSingleAnimActEntry {
    SingleAnimHandler m_fn;
};
typedef i32 (CWarpStonePad::*WarpStonePadHandler)();
SIZE_UNKNOWN(CWarpStonePadActEntry);
struct CWarpStonePadActEntry {
    WarpStonePadHandler m_fn;
};
typedef i32 (CTileTrigger::*TileTriggerHandler)();
SIZE_UNKNOWN(CTileTriggerActEntry);
struct CTileTriggerActEntry {
    TileTriggerHandler m_fn;
};
typedef i32 (CTileTriggerSwitch::*TileTriggerSwitchHandler)();
SIZE_UNKNOWN(CTileTriggerSwitchActEntry);
struct CTileTriggerSwitchActEntry {
    TileTriggerSwitchHandler m_fn;
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
    return ((CSerialObjRef*)((char*)this + 0x34))->Chain((CSerialArchive*)a, b, c, (CSerialObj*)d)
           != 0;
}

// --- CSecretTeleporterTrigger::~CSecretTeleporterTrigger (0x010ab0) ---
// The leaf adds no members, so its dtor folds the bare CUserLogic teardown:
// store the CUserLogic vptr, inline-destruct the +0x18 link (the embedded
// ~EngStr call), store the CUserBase vptr. The destructible link forces the /GX
// EH frame. The fold requires ~CUserBase/~CUserLogic/~CUserBaseLink to be inline
// (see UserLogic.h); the empty leaf body below is enough for cl to emit it.
RVA(0x00010ab0, 0x44)
CSecretTeleporterTrigger::~CSecretTeleporterTrigger() {}

CSecretLevelTrigger::~CSecretLevelTrigger() {}
RVA(0x00010b20, 0x4b)
CSecretLevelTrigger::CSecretLevelTrigger() {}

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

CGruntHealthSprite::~CGruntHealthSprite() {}
RVA(0x00011ef0, 0x4b)
CGruntHealthSprite::CGruntHealthSprite() {}

CPathHazard::~CPathHazard() {}
RVA(0x00013170, 0x7b)
CPathHazard::CPathHazard() {
    m_108 = 0;
    m_110 = 0;
    m_10c = 0;
    m_114 = 0;
    m_120 = 0;
    m_128 = 0;
    m_124 = 0;
    m_12c = 0;
}

CVoiceTrigger::~CVoiceTrigger() {}
// CVoiceTrigger::GetTypeTag (0x000133b0) is now an inline member in the class header.
RVA(0x00013470, 0x4b)
CVoiceTrigger::CVoiceTrigger() {}

// --- CUserLogic::GetScreenPos (0x029a50) ---
// Copies the bound object's screen position into the out point.
RVA(0x00029a50, 0x15)
void CUserLogic::GetScreenPos(ScreenPoint* out) {
    CGameObject* o = m_object;
    i32 y = o->m_screenY;
    i32 x = o->m_screenX;
    out->x = x;
    out->y = y;
}

// --- CUserLogic::IsAtSavedScreenPos (0x029a80) ---
RVA(0x00029a80, 0x29)
i32 CUserLogic::IsAtSavedScreenPos() {
    CGameObject* o = m_object;
    i32 sx = *(i32*)((char*)this + 0x17c);
    if (o->m_screenX == sx && o->m_screenY == *(i32*)((char*)this + 0x180)) {
        return 1;
    }
    return 0;
}

// --- CTeleporter (0x041020), vptr 0x5e80cc ---
CTeleporter::~CTeleporter() {}
RVA(0x00041020, 0x170)
CTeleporter::CTeleporter(CGameObject* obj) : CUserLogic(obj) {
    TILE_LOGIC_SEED(obj);
    m_58 = 0;
    m_60 = 0;
    m_5c = 0;
    m_64 = 0;
    m_38->m_flags |= 0x2000002;
    if (m_object->m_latchedAnimId != 0x1869f) {
        m_object->m_latchedAnimId = 0x1869f;
        m_object->m_flags |= 0x20000;
    }
    m_object->m_screenX = (m_object->m_screenX & ~0x1f) + 0x10;
    m_object->m_screenY = (m_object->m_screenY & ~0x1f) + 0x10;
    EnterField1();
    EnterField2();
}

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

// --- CSecretLevelTrigger 1-arg (0x0424b0), vptr 0x5e8804 ---
RVA(0x000424b0, 0x1a0)
CSecretLevelTrigger::CSecretLevelTrigger(CGameObject* obj) : CUserLogic(obj) {
    TILE_LOGIC_SEED(obj);
    if (g_gameReg->m_134 == 1 && g_gameReg->m_130 == 0) {
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
    } else {
        m_38->m_flags |= 0x10000;
    }
}

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

// --- CParticlez (0x046ad0), vptr 0x5e7614 ---
CParticlez::~CParticlez() {}
// CParticlez::GetTypeTag (0x00012cd0) is now an inline member in the class header.
RVA(0x00046ad0, 0x15e)
CParticlez::CParticlez(CGameObject* obj) : CUserLogic(obj) {
    TILE_LOGIC_SEED(obj);
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    m_38->m_flags |= 0x2000002;
    if (m_object->m_latchedAnimId != 0xcf84f) {
        m_object->m_latchedAnimId = 0xcf84f;
        m_object->m_flags |= 0x20000;
    }
    m_object->m_38 = 0;
}

// --- CGruntSelectedSprite (0x07e3e0), vptr 0x5e7bfc ---
CGruntSelectedSprite::~CGruntSelectedSprite() {}
RVA(0x0007e3e0, 0x178)
CGruntSelectedSprite::CGruntSelectedSprite(CGameObject* obj) : CUserLogic(obj) {
    TILE_LOGIC_SEED(obj);
    m_38->ApplyName("GAME_GRUNTSELECTEDSPRITE");
    m_40 = m_38->m_geoId;
    m_38->ApplyLookupGeometry("GAME_GRUNTSELECTEDSPRITE", 0);
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    if (m_object->m_latchedAnimId != 0x14) {
        m_object->m_latchedAnimId = 0x14;
        m_object->m_flags |= 0x20000;
    }
}

// --- CGruntHealthSprite (0x07eb00), vptr 0x5e7ba4 ---
RVA(0x0007eb00, 0x170)
CGruntHealthSprite::CGruntHealthSprite(CGameObject* obj) : CUserLogic(obj) {
    TILE_LOGIC_SEED(obj);
    m_38->ApplyLookupSprite("GAME_GRUNTHEALTHSPRITE", 1);
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    m_5c = 0x64;
    if (m_object->m_latchedAnimId != 0xdbba0) {
        m_object->m_latchedAnimId = 0xdbba0;
        m_object->m_flags |= 0x20000;
    }
    m_60 = -0x19;
}

// --- CGruntToySprite (0x07f350), vptr 0x5e7b4c ---
CGruntToySprite::~CGruntToySprite() {}
RVA(0x0007f350, 0x16a)
CGruntToySprite::CGruntToySprite(CGameObject* obj) : CUserLogic(obj) {
    TILE_LOGIC_SEED(obj);
    m_38->ApplyLookupSprite("GAME_STATUSBAR_TABZ_STATZTAB_SMALL", 0);
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    m_38->m_stateFlags |= 1;
    if (m_object->m_latchedAnimId != 0xdbba0) {
        m_object->m_latchedAnimId = 0xdbba0;
        m_object->m_flags |= 0x20000;
    }
    m_5c = 0;
}

// --- CGruntPowerupSprite (0x07fdb0), vptr 0x5e76c4 ---
CGruntPowerupSprite::~CGruntPowerupSprite() {}
RVA(0x0007fdb0, 0x166)
CGruntPowerupSprite::CGruntPowerupSprite(CGameObject* obj) : CUserLogic(obj) {
    TILE_LOGIC_SEED(obj);
    m_38->ApplyName("GAME_LIGHTING_POWERUP");
    m_40 = m_38->m_geoId;
    m_38->ApplyLookupGeometry("GAME_CYCLE100", 0);
    if (m_object->m_latchedAnimId != 0x15) {
        m_object->m_latchedAnimId = 0x15;
        m_object->m_flags |= 0x20000;
    }
    m_38->m_stateFlags |= 1;
}

// --- CAniCycle (0x0aad20), vptr 0x5e86a4 ---
CAniCycle::~CAniCycle() {}
RVA(0x000aad20, 0x15c)
CAniCycle::CAniCycle(CGameObject* obj) : CUserLogic(obj) {
    TILE_LOGIC_SEED(obj);
    m_38->m_flags |= 1;
    if (m_38->m_geoId == 0) {
        m_40 = m_38->m_geoId;
        m_38->ApplyLookupGeometry("GAME_CYCLE100", 0);
    }
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
}

// --- CSingleFrameMessage (0x0ab310), vptr 0x5e864c ---
// The tail asks g_gameReg for the on-screen message-bounds RECT (0x2cb1 thunk),
// copies it into a local via the CopyRect Win32 import, then centers the object
// (m_object->m_5c/m_60) inside it. ApplyLookupSprite takes m_38->m_04 as its flag.
CSingleFrameMessage::~CSingleFrameMessage() {}
RVA(0x000ab310, 0x18d)
CSingleFrameMessage::CSingleFrameMessage(CGameObject* obj) : CUserLogic(obj) {
    TILE_LOGIC_SEED(obj);
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    m_object->ApplyLookupSprite("GAME_MESSAGEZ", m_38->m_04);
    RECT bounds;
    RECT r;
    CopyRect(&r, g_gameReg->GetMessageBounds(&bounds));
    m_object->m_screenX = r.left + (r.right - r.left) / 2;
    m_object->m_screenY = r.top + (r.bottom - r.top) / 2;
}

// --- CSimpleAnimation (0x0ab940), vptr 0x5e8544 ---
CSimpleAnimation::~CSimpleAnimation() {}
RVA(0x000ab940, 0x1b8)
CSimpleAnimation::CSimpleAnimation(CGameObject* obj) : CUserLogic(obj) {
    TILE_LOGIC_SEED(obj);
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    CGameObjLayer* aux = m_object->m_layer;
    if (aux != 0) {
        if (aux->m_zClampLo >= g_buteMgr.GetInt("World", "BigActHeight")
            || m_object->m_layer->m_zClampHi >= g_buteMgr.GetInt("World", "BigActHeight")) {
            if (m_object->m_7c != 0) {
                m_object->m_7c->m_08 &= ~6;
                m_object->m_7c->m_08 |= 1;
                m_38->m_flags &= ~0x1000002;
                m_38->m_flags |= 0x800000;
            }
        }
    }
}

// --- CFrontCandy (0x0abfa0), vptr 0x5e84ec ---
CFrontCandy::~CFrontCandy() {}
RVA(0x000abfa0, 0x1b6)
CFrontCandy::CFrontCandy(CGameObject* obj) : CUserLogic(obj) {
    TILE_LOGIC_SEED(obj);
    if (m_object->m_latchedAnimId != 0xf4240) {
        m_object->m_latchedAnimId = 0xf4240;
        m_object->m_flags |= 0x20000;
    }
    CGameObjLayer* aux = m_object->m_layer;
    if (aux != 0) {
        if (aux->m_zClampLo >= g_buteMgr.GetInt("World", "BigActHeight")
            || m_object->m_layer->m_zClampHi >= g_buteMgr.GetInt("World", "BigActHeight")) {
            if (m_object->m_7c != 0) {
                m_object->m_7c->m_08 &= ~6;
                m_object->m_7c->m_08 |= 1;
                m_38->m_flags &= ~0x1000002;
                m_38->m_flags |= 0x800000;
            }
        }
    }
}

// --- CBehindCandy (0x0ac3f0), vptr 0x5e8494 ---
CBehindCandy::~CBehindCandy() {}
RVA(0x000ac3f0, 0x1b1)
CBehindCandy::CBehindCandy(CGameObject* obj) : CUserLogic(obj) {
    TILE_LOGIC_SEED(obj);
    if (m_object->m_latchedAnimId != 0) {
        m_object->m_latchedAnimId = 0;
        m_object->m_flags |= 0x20000;
    }
    if (m_object->m_layer != 0) {
        if (m_object->m_layer->m_zClampLo >= g_buteMgr.GetInt("World", "BigActHeight")
            || m_object->m_layer->m_zClampHi >= g_buteMgr.GetInt("World", "BigActHeight")) {
            if (m_object->m_7c != 0) {
                m_object->m_7c->m_08 &= ~6;
                m_object->m_7c->m_08 |= 1;
                m_38->m_flags &= ~0x1000002;
                m_38->m_flags |= 0x800000;
            }
        }
    }
}

// --- CEyeCandy (0x0ac620), vptr 0x5e843c ---
CEyeCandy::~CEyeCandy() {}
RVA(0x000ac620, 0x1cf)
CEyeCandy::CEyeCandy(CGameObject* obj) : CUserLogic(obj) {
    TILE_LOGIC_SEED(obj);
    CGameObject* o = m_object;
    if (o->m_latchedAnimId == 0 && o->m_layer != 0) {
        i32 v = o->m_layer->m_1c + o->m_screenY + 0x186a0;
        if (o->m_latchedAnimId != v) {
            o->m_latchedAnimId = v;
            o->m_flags |= 0x20000;
        }
    }
    CGameObjLayer* aux = m_object->m_layer;
    if (aux != 0) {
        if (aux->m_zClampLo >= g_buteMgr.GetInt("World", "BigActHeight")
            || m_object->m_layer->m_zClampHi >= g_buteMgr.GetInt("World", "BigActHeight")) {
            if (m_object->m_7c != 0) {
                m_object->m_7c->m_08 &= ~6;
                m_object->m_7c->m_08 |= 1;
                m_38->m_flags &= ~0x1000002;
                m_38->m_flags |= 0x800000;
            }
        }
    }
}

// --- CFrontCandyAni::RegisterActs (0x0acd10) + AdvanceAnim (0x0acf10) re-homed ---
// These were a mis-attribution of CEyeCandyAni's acts (registry 0x646060) and now
// live in src/Gruntz/EyeCandyAni.cpp, killing the ?RegisterActs@CFrontCandyAni
// dup-RVA (they emit as ?RegisterActs@CEyeCandyAni / ?AdvanceAnim@CEyeCandyAni).

// --- CFrontCandyAni (0x0acf40), vptr 0x5e83e4 ---
CFrontCandyAni::~CFrontCandyAni() {}
RVA(0x000acf40, 0x16e)
CFrontCandyAni::CFrontCandyAni(CGameObject* obj) : CUserLogic(obj) {
    TILE_LOGIC_SEED(obj);
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    if (m_38->m_geoId == 0) {
        m_40 = m_38->m_geoId;
        m_38->ApplyLookupGeometry("GAME_CYCLE100", 0);
    }
    if (m_object->m_latchedAnimId != 0xf4240) {
        m_object->m_latchedAnimId = 0xf4240;
        m_object->m_flags |= 0x20000;
    }
}

// --- CBehindCandyAni (0x0ad540), vptr 0x5e838c ---
CBehindCandyAni::~CBehindCandyAni() {}
RVA(0x000ad540, 0x1f0)
CBehindCandyAni::CBehindCandyAni(CGameObject* obj) : CUserLogic(obj) {
    TILE_LOGIC_SEED(obj);
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    if (m_38->m_geoId == 0) {
        m_40 = m_38->m_geoId;
        m_38->ApplyLookupGeometry("GAME_CYCLE100", 0);
    }
    if (m_object->m_latchedAnimId != 0) {
        m_object->m_latchedAnimId = 0;
        m_object->m_flags |= 0x20000;
    }
    if (m_object->m_layer != 0) {
        if (m_object->m_layer->m_zClampLo >= g_buteMgr.GetInt("World", "BigActHeight")
            || m_object->m_layer->m_zClampHi >= g_buteMgr.GetInt("World", "BigActHeight")) {
            if (m_object->m_7c != 0) {
                m_object->m_7c->m_08 &= ~6;
                m_object->m_7c->m_08 |= 1;
                m_38->m_flags &= ~0x1000002;
                m_38->m_flags |= 0x800000;
            }
        }
    }
}

// --- CMenuSparkle (0x0adbe0), vptr 0x5e82dc ---
CMenuSparkle::~CMenuSparkle() {}
RVA(0x000adbe0, 0x178)
CMenuSparkle::CMenuSparkle(CGameObject* obj) : CUserLogic(obj) {
    TILE_LOGIC_SEED(obj);
    m_38->ApplyName("MENU_SPARKLE");
    m_40 = m_38->m_geoId;
    m_38->ApplyLookupGeometry("MENU_FORWARD100", 0);
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    m_objAux->m_130 = rand() % 0xfa1 + 0x3e8;
}

// --- CSingleAnimation (0x0ae7f0), vptr 0x5e745c ---
CSingleAnimation::~CSingleAnimation() {}
RVA(0x000ae7f0, 0x13d)
CSingleAnimation::CSingleAnimation(CGameObject* obj) : CUserLogic(obj) {
    TILE_LOGIC_SEED(obj);
    m_38->m_flags |= 2;
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
}

// --- CSingleAnimation::InitActReg (0x0ae9a0) ---
// Construct the class's per-coordinate activation registry singleton
// (g_singleAnimActReg @0x645f70) over the fixed range [2000, 2010] via the shared
// registry ctor (0x408710). Free init thunk; reloc-masked.
RVA(0x000ae9a0, 0x15)
void CSingleAnimation::InitActReg() {
    ((CZDArrayDerived*)&g_singleAnimActReg)->Construct(2000, 2010);
}

// --- CSingleAnimation::RegisterActs (0x0aeb80) ---
// Bind the per-frame handler (AdvanceAnim @0x0aed80) to the activation key "A"
// via the shared name registry + the class's coordinate registry
// (g_singleAnimActReg). SAME archetype as CSecretTeleporterTrigger::RegisterActs.
//
// @early-stop
// register-pinning wall (docs/patterns/zero-register-pinning.md +
// test-old-value-decrement-loop-while-postdec.md, topic:wall topic:regalloc): logic
// byte-faithful (every call/immediate/branch/offset + the handler store match
// retail); residual is the slot-vs-id callee-saved register choice cascading into
// the free-loop count materialization. Deferred.
RVA(0x000aeb80, 0x18d)
void CSingleAnimation::RegisterActs() {
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
    ((CSingleAnimActEntry*)g_singleAnimActReg.ResolveEntry(id))->m_fn =
        &CSingleAnimation::AdvanceAnim;
}

// --- CWarpStonePad (0x10d650), vptr 0x5e71ac ---
CWarpStonePad::~CWarpStonePad() {}
RVA(0x0010d650, 0x16c)
CWarpStonePad::CWarpStonePad(CGameObject* obj) : CUserLogic(obj) {
    TILE_LOGIC_SEED(obj);
    m_38->m_flags |= 2;
    m_38->m_flags |= 1;
    if (g_gameReg->m_134 == 1) {
        m_38->m_stateFlags |= 1;
        m_38->m_flags |= 0x10000;
    }
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
}

// --- CWarpStonePad::InitActReg (0x10d840) ---
// Construct the class's activation-coordinate registry singleton
// (g_warpStonePadActReg @0x64e6a0) over the fixed range [2000, 2010] via the
// shared registry ctor (0x408710). Free init thunk; reloc-masked.
RVA(0x0010d840, 0x15)
void CWarpStonePad::InitActReg() {
    ((CZDArrayDerived*)&g_warpStonePadActReg)->Construct(2000, 2010);
}

// --- CWarpStonePad::FireWarp (0x10d8c0), vtable slot 4 ---
// Look the activation coordinate up in the class's own registry singleton
// (g_warpStonePadActReg); if the resolved entry carries a registered handler,
// look it up again and dispatch it __thiscall on this. The SAME archetype as
// CSecretTeleporterTrigger::FireActivation, but driving the warp-pad registry.
RVA(0x0010d8c0, 0x102)
void CWarpStonePad::FireWarp(i32 coord) {
    CWarpStonePadActEntry* e = (CWarpStonePadActEntry*)g_warpStonePadActReg.ResolveEntry(coord);
    if (e->m_fn != 0) {
        CWarpStonePadActEntry* e2 =
            (CWarpStonePadActEntry*)g_warpStonePadActReg.ResolveEntry(coord);
        (this->*(e2->m_fn))();
    }
}

// --- CWarpStonePad::RegisterActs (0x10da20) ---
// Bind the per-frame handler (AdvanceAnim @0x10dc20) to the activation key "A"
// via the shared name registry + the class's coordinate registry
// (g_warpStonePadActReg). SAME archetype as CSecretTeleporterTrigger::RegisterActs.
//
// @early-stop
// register-pinning wall (docs/patterns/zero-register-pinning.md +
// test-old-value-decrement-loop-while-postdec.md, topic:wall topic:regalloc): logic
// byte-faithful (every call/immediate/branch/offset + the handler store match
// retail); residual is the slot-vs-id callee-saved register choice cascading into
// the free-loop count materialization. Deferred.
RVA(0x0010da20, 0x18d)
void CWarpStonePad::RegisterActs() {
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
    ((CWarpStonePadActEntry*)g_warpStonePadActReg.ResolveEntry(id))->m_fn =
        &CWarpStonePad::AdvanceAnim;
}

// --- CTileTriggerSwitch (0x10dc40), vptr 0x5e7f6c ---
// The most-derived dtor at 0x110f0 is the 0x44 folded CUserLogic teardown: the
// leaf vptr store (0x5e7f6c) is dead-eliminated by the immediately-inlined
// CUserLogic base dtor (store 0x5e705c, ~EngStr on +0x18, store 0x5e70b4).
RVA(0x000110f0, 0x44)
CTileTriggerSwitch::~CTileTriggerSwitch() {}
RVA(0x0010dc40, 0x154)
CTileTriggerSwitch::CTileTriggerSwitch(CGameObject* obj) : CUserLogic(obj) {
    TILE_LOGIC_SEED(obj);
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    m_38->m_flags |= 2;
    m_38->m_flags |= 1;
    m_38->m_stateFlags |= 1;
}

// --- CTileTriggerSwitch::InitActReg (0x10de20) ---
// Construct the class's activation-coordinate registry singleton
// (g_tileTriggerSwitchActReg @0x64e798) over the fixed range [2000, 2010] via
// the shared registry ctor (0x408710). Free init thunk; reloc-masked.
RVA(0x0010de20, 0x15)
void CTileTriggerSwitch::InitActReg() {
    ((CZDArrayDerived*)&g_tileTriggerSwitchActReg)->Construct(2000, 2010);
}

// --- CTileTriggerSwitch::RegisterActs (0x10e000) ---
// Bind the per-frame handler (AdvanceAnim @0x10e200) to the activation key "A"
// via the shared name registry + the class's coordinate registry
// (g_tileTriggerSwitchActReg). SAME archetype as CTileTrigger::RegisterActs.
//
// @early-stop
// register-pinning wall (docs/patterns/zero-register-pinning.md +
// test-old-value-decrement-loop-while-postdec.md, topic:wall topic:regalloc): logic
// byte-faithful (every call/immediate/branch/offset + the `mov [entry],offset
// AdvanceAnim` handler store match retail); residual is the slot-vs-id callee-saved
// register choice cascading into the free-loop count materialization. Deferred.
RVA(0x0010e000, 0x18d)
void CTileTriggerSwitch::RegisterActs() {
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
    ((CTileTriggerSwitchActEntry*)g_tileTriggerSwitchActReg.ResolveEntry(id))->m_fn =
        &CTileTriggerSwitch::AdvanceAnim;
}

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

// --- CTileSecretTrigger::InitActReg (0x10f160) ---
// Construct the class's activation-coordinate registry singleton
// (g_tileSecretTriggerActReg @0x64e7e8) over the fixed range [2000, 2010] via the
// shared registry ctor (0x408710). Free init thunk; reloc-masked.
RVA(0x0010f160, 0x15)
void CTileSecretTrigger::InitActReg() {
    ((CZDArrayDerived*)&g_tileSecretTriggerActReg)->Construct(2000, 2010);
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

// --- CToobSpikez (0x1145c0), vptr 0x5e7774 ---
CToobSpikez::~CToobSpikez() {}
RVA(0x001145c0, 0x18e)
CToobSpikez::CToobSpikez(CGameObject* obj) : CUserLogic(obj) {
    TILE_LOGIC_SEED(obj);
    m_40 = m_38->m_geoId;
    m_38->ApplyLookupGeometry("GAME_CYCLE100", 2);
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    m_38->m_flags |= 2;
    m_object->m_164 = m_object->m_screenX >> 5;
    m_object->m_168 = m_object->m_screenY >> 5;
    if (m_object->m_latchedAnimId != 0xc) {
        m_object->m_latchedAnimId = 0xc;
        m_object->m_flags |= 0x20000;
    }
}

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

// ---------------------------------------------------------------------------
// The per-frame grunt "decay/wand" AI (0x612a0 / 0x61570 / 0x65a60). These run on
// a grunt-behavior leaf that extends CUserLogic (base at +0) with a decay-timer +
// anim state (modeled below as CGruntBehaviorLeaf; placeholder name, only offsets +
// code bytes are load-bearing). The timer (m_830 start / m_838 duration, both
// hi=0) drives a 0..256 fixed-point fill bar on the bound object's draw command;
// m_object is the real inherited CGameObject. The bute/anim callees are reloc-masked
// __thiscall externs. g_645588 = running ms clock.
// ---------------------------------------------------------------------------
extern "C" u32 g_645588;   // running game clock (ms)
extern CButeMgr g_buteMgr; // 0x6453d8 - getters reloc-mask
extern char k_60bebc[];    // interned bute-node name "R"

SIZE_UNKNOWN(CDecayMgr);
struct CDecayMgr { // m_154 - the bound draw-state manager
    char m_pad00[0x8];
    i32 m_8; // +0x08 dirty flags
    char m_pad0c[0x40 - 0xc];
    i32 m_40; // +0x40 flags
    char m_pad44[0x194 - 0x44];
    CImageSet* m_194; // +0x194
    char m_pad198[0x1a0 - 0x198];
    CAniAdvanceCursor m_1a0; // +0x1a0
};
SIZE_UNKNOWN(CDecayAnim);
struct CDecayAnim {                                               // m_260 - anim/sprite controller
    void Anim2a72(i32 a, i32 b, i32 c);                           // 0x2a72
    void DrawAnimAt(i32 a, i32 b, i32 c, i32 d);                  // 0x1073
    void PlayAnimEx(i32 a, i32 b, i32 c, i32 d, i32 e, i32 f);    // 0x3003
    void SetAnim(i32 a, i32 b, i32 c, i32 d);                     // 0x2e96
    void PlayStateAnim(i32 a, i32 b, i32 c, i32 d, i32 e, i32 f); // 0x3945
};

SIZE_UNKNOWN(CGruntBehaviorLeaf);
class CGruntBehaviorLeaf : public CUserLogic {
public:
    TILE_LOGIC_TAIL
public:
    i32 LoadGruntDecayConfig();    // 0x612a0
    i32 LoadGruntDecayConfig2();   // 0x61570
    i32 LoadWandGruntItemConfig(); // 0x65a60
    // Leaf's own reloc-masked __thiscall helpers.
    void RefreshDecay();                     // 0x22de
    void SetDecayTarget(i32 a);              // 0x3c29
    void InitAnimState(i32 a, i32 b, i32 c); // 0x136b

    // Members beyond CUserLogic's 0x40 base.
    char m_pad40[0x154 - 0x40];
    CDecayMgr* m_drawState; // +0x154 bound draw-state manager
    char m_pad158[0x170 - 0x158];
    i32 m_gruntSubState; // +0x170 grunt sub-state
    char m_pad174[0x1c0 - 0x174];
    char* m_gruntTypeTag; // +0x1c0 grunt-type bute tag
    i32 m_1c4;            // +0x1c4
    char m_pad1c8[0x1e4 - 0x1c8];
    i32 m_downtimeLatch; // +0x1e4 latch flag
    char m_pad1e8[0x1ec - 0x1e8];
    i32 m_animArg0; // +0x1ec anim arg
    i32 m_animArg1; // +0x1f0 anim arg
    i32 m_animArg2; // +0x1f4 anim arg
    char m_pad1f8[0x258 - 0x1f8];
    i32 m_typeDisc; // +0x258 type discriminator (0x3b = no-downtime)
    char m_pad25c[0x260 - 0x25c];
    CDecayAnim* m_animCtrl; // +0x260 anim controller
    char m_pad264[0x360 - 0x264];
    i32 m_gruntMode; // +0x360 grunt mode
    char m_pad364[0x36c - 0x364];
    i32 m_animSuppress; // +0x36c anim-suppress gate
    char m_pad370[0x380 - 0x370];
    i32 m_380; // +0x380
    char m_pad384[0x3e4 - 0x384];
    i32 m_3e4;    // +0x3e4
    i32 m_3e8;    // +0x3e8
    i32 m_health; // +0x3ec health
    i32 m_3f0;    // +0x3f0
    char m_pad3f4[0x460 - 0x3f4];
    i32 m_460; // +0x460
    char m_pad464[0x830 - 0x464];
    i32 m_decayTimerLo;    // +0x830 timer start (lo)
    i32 m_decayTimerHi;    // +0x834 (hi, always 0)
    i32 m_decayDurationLo; // +0x838 timer duration (lo)
    i32 m_decayDurationHi; // +0x83c (hi, always 0)
    char m_pad840[0x860 - 0x840];
    i32 m_wandTimerLo;    // +0x860 wand timer start (lo)
    i32 m_wandTimerHi;    // +0x864 (hi)
    i32 m_wandDowntimeLo; // +0x868 wand downtime (lo)
    i32 m_wandDowntimeHi; // +0x86c (hi)
};

// LoadGruntDecayConfig (0x612a0): advance the arrival probe, drive the walk/idle
// anim by grunt mode, then (once arrived + not busy) latch the decay timer + fill.
// @early-stop
// 90.5%: logic byte-faithful. Residual is CSE/regalloc of the 64-bit timer delta -
// retail keeps g_645588 pinned in eax and re-does the m_decayTimerLo subtraction in the fill
// tail, while cl shares the whole `(i64)clock - m_decayTimerLo` delta, shifting the lo dword
// off eax and cascading register names + the epilogue merge. Not source-steerable.
RVA(0x000612a0, 0x23c)
i32 CGruntBehaviorLeaf::LoadGruntDecayConfig() {
    if (m_gruntMode == 0) {
        return 0;
    }
    if (m_drawState->m_1a0.Advance_15c360(g_6bf3bc) == 1) {
        if (m_gruntSubState == 1 && m_gruntMode != 5) {
            m_animCtrl->DrawAnimAt(m_object->m_screenX, m_object->m_screenY, 1, m_animArg0);
        } else {
            m_animCtrl->PlayAnimEx(
                m_object->m_screenX,
                m_object->m_screenY,
                m_animArg0,
                m_animArg2,
                m_gruntMode != 5,
                0x19
            );
        }
    }
    CAniAdvanceCursor* sub = &m_drawState->m_1a0;
    if (sub->m_28 == 0) {
        return 0;
    }
    if (sub->m_20 != 0) {
        return 0;
    }
    i32 mode = m_gruntMode;
    if (mode == 1 || mode == 2 || mode == 0xb || mode == 6) {
        m_prevAnimSetNode = m_objAux->m_1c;
        m_objAux->m_1c = g_buteTree.Find(k_60bebc);
        if (m_animSuppress == 0) {
            m_animCtrl->Anim2a72(m_animArg0, m_animArg1, 0);
        }
        i32 dt = (i32)g_buteMgr.GetDwordDef("Grunt", "DecayTime", 0xbb8);
        if (m_object->m_drawFillCmd == 0xb) {
            m_decayDurationLo = dt;
            m_decayTimerLo = (i32)g_645588 - m_object->m_fillFraction * dt / 256;
            m_decayDurationHi = 0;
        } else {
            m_decayDurationLo = dt;
            m_decayDurationHi = 0;
            m_decayTimerLo = (i32)g_645588;
        }
        m_decayTimerHi = 0;
        i64 e = (i64)(u32)g_645588 - *(i64*)&m_decayTimerLo;
        u32 elapsed = e < 0 ? 0 : (u32)e;
        i32 r = (i32)((double)elapsed * 256.0
                      / (double)g_buteMgr.GetDwordDef("Grunt", "DecayTime", 0xbb8));
        m_object->m_drawActive = 1;
        m_object->m_drawFillCmd = 0xb;
        m_object->m_fillFraction = r;
        return 0;
    }
    if (m_animSuppress == 0) {
        m_animCtrl->Anim2a72(m_animArg0, m_animArg1, 0);
    }
    m_drawState->m_8 |= 0x10000;
    return 0;
}

// LoadGruntDecayConfig2 (0x61570): if the timer has fully elapsed, fire the finish
// (flag + finish anim); else refresh the 0..256 fill fraction on the draw command.
// @early-stop
// ~77%: logic byte-faithful. Same CSE/regalloc wall as LoadGruntDecayConfig -
// retail loads g_645588 once into eax and recomputes the m_decayTimerLo subtraction in the
// fill branch (eax preserved -> lo stays in eax); cl CSEs the whole 64-bit delta and
// pins lo in ecx, cascading a register-name mismatch through the tail. Hoisting the
// clock into a local regressed it. Not source-steerable.
RVA(0x00061570, 0x11d)
i32 CGruntBehaviorLeaf::LoadGruntDecayConfig2() {
    if ((i64)(u32)g_645588 - *(i64*)&m_decayTimerLo >= *(i64*)&m_decayDurationLo) {
        m_drawState->m_40 |= 1;
        m_drawState->m_194->SetAllTypes(1);
        if (m_animSuppress == 0) {
            m_animCtrl->Anim2a72(m_animArg0, m_animArg1, 0);
        }
        m_drawState->m_8 |= 0x10000;
        return 0;
    }
    i64 e = (i64)(u32)g_645588 - *(i64*)&m_decayTimerLo;
    u32 elapsed = e < 0 ? 0 : (u32)e;
    i32 r =
        (i32)((double)elapsed * 256.0 / (double)g_buteMgr.GetDwordDef("Grunt", "DecayTime", 0xbb8));
    m_object->m_drawActive = 1;
    m_object->m_drawFillCmd = 0xb;
    m_object->m_fillFraction = r;
    return 0;
}

// LoadWandGruntItemConfig (0x65a60): per-frame wand-grunt item logic. Advance the
// arrival probe; on the peak phase (0x63) latch the item downtime timer, tick the
// wand health loss, and fire the depletion anim; every active frame run the wand
// projectile step; finally, once arrived + idle, clear the latch + run the reset.
// @early-stop
// ~95%: whole body byte-identical (incl. the branchless max(0,hp) sub/sets/dec/and
// idiom) except cl schedules the `if (m_1c4)` load a few slots earlier than retail
// (which interleaves it among the timer zero-stores). Pure scheduling; not steerable.
RVA(0x00065a60, 0x159)
i32 CGruntBehaviorLeaf::LoadWandGruntItemConfig() {
    i32 phase = m_drawState->m_1a0.Advance_15c360(g_6bf3bc);
    if (phase > 0) {
        if (phase == 0x63) {
            m_downtimeLatch = 1;
            u32 downtime = g_buteMgr.GetDword(m_gruntTypeTag, "ItemDowntime");
            if (m_typeDisc == 0x3b) {
                downtime = 0;
            }
            m_wandDowntimeLo = downtime;
            m_wandDowntimeHi = 0;
            m_wandTimerLo = g_645588;
            m_wandTimerHi = 0;
            m_460 = 0;
            m_3f0 = 0;
            if (m_1c4 != 0) {
                RefreshDecay();
            }
            if (m_gruntSubState == 0x13) {
                SetDecayTarget(m_380);
                i32 hp = m_health - g_buteMgr.GetIntDef("WANDGRUNT", "HealthLoss", 0x19);
                m_health = hp < 0 ? 0 : hp;
                if (m_health <= 0) {
                    m_animCtrl->SetAnim(m_animArg0, m_animArg1, 1, -1);
                }
            }
        }
        m_animCtrl->PlayStateAnim(m_animArg0, m_animArg1, m_3e4, m_3e8, m_gruntSubState, phase);
    }
    CAniAdvanceCursor* sub = &m_drawState->m_1a0;
    if (sub->m_28 != 0 && sub->m_20 == 0) {
        m_downtimeLatch = 0;
        InitAnimState(1, 0, 0);
    }
    return 0;
}

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

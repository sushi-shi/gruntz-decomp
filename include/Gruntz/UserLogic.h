// UserLogic.h - Gruntz game-object base hierarchy (C:\Proj\Gruntz).
//
// Reconstruction sufficient to byte-match the game-object constructors. Field
// names are placeholders; the OFFSETS and the inheritance chain are the
// load-bearing facts the matches prove.
//
// Hierarchy (recovered from RTTI ClassHierarchyDescriptors in GRUNTZ.EXE):
//     CUserBase                       vftable 0x5e70b4  (3 virtuals)
//       +-- CUserLogic : CUserBase    vftable 0x5e705c  (16 virtuals)
//             +-- CSecretLevelTrigger, CTileTrigger, CTeleporter, CWarpStonePad,
//                 CToobSpikez, ...    (the tile-logic game-object leaves)
//
// CUserBase is just a vptr; CUserLogic owns the whole data layout and the link
// sub-object at +0x18. There are TWO ctor shapes:
//
//  * the NO-ARG leaf ctor (75 B, e.g. CTileTrigger 0x011160): store CUserBase
//    vptr (0x5e70b4); construct the +0x18 link via 0x16d710 (it can throw -> the
//    /GX EH frame); store the leaf's own most-derived vptr. The intermediate
//    CUserLogic vptr store (0x5e705c) is dead-eliminated (nothing observes it).
//
//  * the 1-ARG ctor `(CGameObject*)` (e.g. CTileTrigger 0x10e220): the same base
//    prologue PLUS the full CUserLogic init the leaves share - seed the link's
//    name from the global empty string, lazily build the logic-type table once,
//    register the three built-in handlers, set the data fields. Here the
//    CUserLogic vptr store survives (the init runs after it). Each leaf adds its
//    own most-derived vptr store + a per-class tail. Modeled as an INLINE
//    CUserLogic(CGameObject*) ctor so MSVC folds it into every leaf (it only
//    folds a base ctor it can see inline).
#ifndef GRUNTZ_USERLOGIC_H
#define GRUNTZ_USERLOGIC_H

#include <rva.h>
#include <Gruntz/LogicTypeId.h>

// The destructible +0x18 link sub-object (CUserBaseLink, embedding a zBitVec name),
// shared with the CGrunt world so both embed the identical link.
#include <Gruntz/UserBaseLink.h>

// The REAL +0x1a0 tail member: the wide game object embeds the 0x3c-byte
// CAniAdvanceCursor at +0x1a0 (vptr @+0x1a0, ends at +0x1dc == SIZE(CGameObject)).
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/WwdGridIter.h> // WwdRegion - the embedded +0x9c region node

// ---------------------------------------------------------------------------
// CGameObject - the engine object the 1-arg ctors are handed (read into edi).
// Only the fields/methods those ctors touch are modeled; bodies live in engine
// TUs (modeled NO-body so the calls reloc-mask):
//   AddLogicHit/Attack/Bump = 0x150f50 / 0x151030 / 0x151110  (__thiscall, char*)
//   m_7c                    = a sub-object pointer copied into the trigger.
// ---------------------------------------------------------------------------
struct CGameObject;  // fwd (the worker's collide callback takes the object)
struct LeafCue;          // the +0x19c resolved leaf-scan cue (<Gruntz/LeafCue.h>)
class CDDrawSurfacePair; // slots 11-14 params (<DDrawMgr/DDrawSurfacePair.h>)
class CUserLogic; // fwd (AnimWorkerObj::m_logic is the object's bound logic leaf)
struct Coord;
typedef Coord GruntTilePos; // the {m_x,m_y} out-point == the one engine Coord (<Gruntz/CoordNode.h>)

// The lazily-built per-object worker held at CGameObject::m_88 / +0x90 (the same
// 0x17c-byte anim worker AnimWorkerHandlers.cpp models): foreign vtable
// 0x5efb80, the existing worker's slot-7 reuses it (vtbl[0x1c]), and the freshly-
// built one is fed CGameObject->m_10's payload through slot 9 (vtbl[0x24]).
// Modeled as a polymorphic class so both `mov eax,[w]; call [eax+N]` dispatches
// fall out; its vtable lives in another TU (the worker ctor stamps 0x5efb80 by
// address - g_animWorkerVtbl - so this TU never emits one).
// Polymorphic so the vtable-slot dispatches (`mov eax,[w]; call [eax+0x1c]` and
// `call [eax+0x24]`) fall out; the vtable itself is the foreign 0x5efb80 stamped
// by address in the ctor, so this class never emits one (the named virtuals only
// drive the dispatch shape - slot 7 @ +0x1c, slot 9 @ +0x24).
// [The former CAnimWorker 10-slot placeholder view IS the canonical AnimWorkerObj
// (<DDrawMgr/AnimWorkerObj.h>, ??_7 @0x1efb80): "Slot07 (+0x1c, reuse path)" is its
// Clear (slot 7, 0x151e70) and "Slot09 (+0x24)" its Vfunc24/Init (slot 9, 0x151e20).
// The m_collideNotify (+0x10) semantic name migrated onto the canonical.]
#include <DDrawMgr/AnimWorkerObj.h>

// The +0x198 cached frame IS the real CImage (<Image/CImage.h>): every writer of
// the slot stores a CSprite/CImageSet frame element (ApplyLookupSprite/ApplyName/
// Sub150c30/LightFx/KitchenSlime), and the ex-CGameObjLayer view's fields were
// CImage's at identical offsets - "z-clamp bounds" +0x10/+0x14 == m_width/m_height
// (the BigActHeight test), "half-extents" +0x18/+0x1c == m_anchorX/m_anchorY
// (width>>1/height>>1 - the +-7 wander box + z-sort consumers), "+0x20/+0x24
// screen offsets" == m_originX/m_originY (the CGruntVoice bubble placement).
class CDDrawWorker;           // the +0x194 cached sprite IS CDDrawWorker
typedef CDDrawWorker CSprite;   // (identical repeat of Sprite.h's typedef)
typedef CDDrawWorker CImageSet; // the +0x194 union's other role - the SAME class again
class CImage;

// (The former CLogicHandlerMap shell + the LogicMap() offset-hop accessor are
// worker cache map - m_0c (the CDDrawSurfaceMgr) -> m_workerCache (+0x14, the
// CDDrawWorkerCache) -> m_10 (its +0x10 CMapStringToOb; retail Lookup 0x1b8008,
// which the old `(CMapStringToPtr*)` cast at the AddLogic* sites mis-bound to
// the 0x1b8438 Ptr band). The AddLogic* bodies now read the typed path.)

// The wide game object IS the CGameObject family (<Wwd/WwdGameObjectFamily.h>):
// the former flat 0x1dc struct modeled here (17 declared slots + all data) was the
// A-kind layout smeared over the whole family; fields/methods now live at their
// real levels (CResolveNode +0x00..+0x67 / E +0x68..+0x18b / the A tail). The
// 0x1dc byte-proof (CreateSpriteImpl @0x159600 news 0x1dc; ReadPlaneObjects
// `operator new(0x1dc)` + engine ctor 0x15b390) backs SIZE(CWwdGameObjectA).
#include <Wwd/WwdGameObjectFamily.h>

// (The former CGameObjWorld / CGameObjChain / CGameObjNode walking views are
// CDDrawSurfaceMgr now typed at CGameObject::m_0c (its +0x08 m_objChain ==
// m_childGroup, +0x14 m_workerCache, +0x24 m_level == m_level), the
// chain IS CDDrawChildGroup (17-slot vtable 0x1efdc0; CObList @+0x10, head node
// @+0x14) and the node IS CDDrawGroupNode - both <DDrawMgr/DDrawChildGroup.h>.
// The walkers read the head as `(CDDrawGroupNode*)group->m_list.GetHeadPosition()`
// - the same `mov reg,[grp+0x14]` bytes.)

// canonical AnimWorkerObj (<DDrawMgr/AnimWorkerObj.h>, the 0x17c worker/logic
// record, vtable 0x1efb80): its "Init @+0x10 post-create driver" IS the worker's
// m_notify fire callback, and m_logic/m_08/m_1c/m_2c/m_30/m_bc/m_f0../m_130 are
// the same fields under the same offsets - the union of both views' knowledge
// now lives on the canonical.)

// The engine bute manager the eyecandy ctors query for "World"/"BigActHeight"
// (CButeMgr::GetInt 0x171af0). The class + its singleton g_buteMgr
// (?g_buteMgr@@3VCButeMgr@@A, RVA 0x2453d8) live in the bute TUs; declared
// extern only here so the `ecx=&g_buteMgr; call GetInt` shape reloc-masks
// against the already-matched symbols (BattlezMapConfig owns the DATA label).
#include <Bute/ButeMgr.h>
extern CButeMgr g_buteMgr;

// One-shot guard for the built-in tile-logic type registration (0x6bf674).
extern i32 g_logicTypesRegistered;

// BuildLogicTypeTable (0x8a40, via the 0x39c2 thunk): registers the three
// built-in logic types the first time any tile-logic object is built. It is a
// __thiscall member that IGNORES `this` (its impl reads the object as its explicit
// stack arg) - that is why the retail call carries `mov ecx,esi; push obj`. See
// thiscall-ignoring-this. Declared as a CUserLogic method below; the arg is the

// ---------------------------------------------------------------------------
// CUserBase - root of the game-object hierarchy: just a vptr (3 virtuals,
// vftable 0x5e70b4). Inline ctor so it folds into derived ctors.
// ---------------------------------------------------------------------------
// The slot-1 serialize archive IS the one engine stream - CSerialArchive ==
// CFileMemBase (<Gruntz/SerialArchive.h>: Read @slot 11 +0x2c, Write @slot 12
// the name stays as a typedef so every SerializeMove override keeps its spelling.
// (Do NOT fwd-declare `struct CGruntArchive` anywhere - an elaborated fwd decl
// silently out-ranks this typedef under MSVC5.)
class CFileMemBase;
typedef CFileMemBase CGruntArchive;
class CUserBase {
public:
    CUserBase() {}
    virtual ~CUserBase() {} // inline: folds into leaf dtors (final base vptr store)
    virtual i32 SerializeMove(CGruntArchive* ar, i32 mode, i32 a3, i32 a4); // slot 1
    virtual LogicTypeId
    GetTypeTag(); // slot 2 (per-class logic-type id)                                           // slot 2
};
SIZE_UNKNOWN(CUserBase);     // (was covered by the BoundaryMisc placeholder before its rename)
VTBL(CUserBase, 0x001e70b4); // ??_7CUserBase@@6B@ (the RTTI base vtable; catalog only,
                             // GRUNTZ_META no-op in the compile - was on the old
                             // BoundaryMisc placeholder, now on the real class)

// ---------------------------------------------------------------------------
// CUserLogic : CUserBase (vftable 0x5e705c, 16 slots; ALL 16 modeled here -
// CUserBase slots 0..2 + UserLogicVfunc1..D at slots 3..15). Owns the shared data
// layout + the +0x18 link sub-object. The full 16-slot model lets CUserLogic-derived
// leaves that add their OWN virtuals (CMovingLogic::Update @slot 16, CPathHazard's
// Tick..HitTest @slots 16..20) land them at the true retail offsets with no filler.
// The default ctor just constructs the link (used by the no-arg leaves). The
// inline 1-arg ctor folds the full shared init into each leaf's 1-arg ctor.
//
// TRUE SIZE = 0x30 (see the NOTE below). This "fat" view extends the class to
// 0x40 by ABSORBING the tile-logic leaves' common tail m_30/m_34/m_38/m_3c into
// the base - a byte-neutral modeling convenience (all tile-logic leaves set
// m_34/m_38/m_3c = obj/obj/obj->m_7c right after the folded base init, so folding
// the tail into the base ctor reproduces every leaf's bytes without spelling it
// out per leaf). The CGrunt world (<Gruntz/Grunt.h>) models the SAME class at its
// true 0x30 boundary, because CGrunt uses 0x30..0x3c for its OWN (different)
// fields. Both are correct expressions of one class - see the NOTE.
// ---------------------------------------------------------------------------
struct CAnimLookupNode; // Grunt.h view of the +0x14 aux (anim-set lookup)
class CUserLogic : public CUserBase {
public:
    CUserLogic() {}
    CUserLogic(CGameObject* obj);
    virtual ~CUserLogic() OVERRIDE {} // inline: folds into leaf dtors (link teardown + vptr stores)
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    virtual LogicTypeId GetTypeTag() OVERRIDE;                         // slot 2
    // slot 3 (+0x0c): the serialize name-in hook - ProjTypeXfer (0x16e4f0) resolves
    // the type name and dispatches it here virtually (`push name; call [vptr+0xc]`,
    // llvm-objdump-proven). Default 0x00106e: a ret-4 one-arg no-op.
    virtual void XferName(char* name);
    // slot 4 (+0x10): the activation dispatcher - SIGNATURE SETTLED (2026-07-15) from
    // retail: the base body (thunk 0x246e -> 0x8b70) is a bare `ret 4`, i.e. an empty
    // __thiscall taking ONE stack arg, and every leaf override reads that arg at
    // [esp+0xc] and `ret 4` too. Was a no-arg `UserLogicVfunc2()` placeholder, which
    // forced all 51 overriding leaves to park their real body beside it as a
    // non-virtual under 9 different invented names (FireActivation/RunAct/Dispatch/...).
    // The arg is the activation ID interned by the leaf's RegisterActs via
    // g_buteTree.Find(key) (see <Gruntz/ActNameRegistry.h>), NOT a coordinate.
    // Returns void: no exit in the base or in any override materializes eax (the 17
    // leaves that were spelled `i32` only ever "returned" the leftover entry pointer
    // cast to int - an RE artifact). Same shape as the sibling slot 5 FinalizeStep.
    //
    // NB the OVERRIDE macro is what polices this: MSVC5 expands it to NOTHING, so a
    // leaf whose spelling drifts from this signature still COMPILES under cl and
    // silently re-binds to a new non-virtual, leaving the vtable slot on the base.
    // Only the clang label pass (which expands OVERRIDE to `override`) rejects it.
    // If you change this signature, trust clang's error, not cl's silence.
    virtual void FireActivation(i32 id);
    // slot 5 (+0x14): the per-tick finalize step - SIGNATURE SETTLED (Fable A2,
    // 2026-07-14) from all three bodies: base 0x8b90 (`ret 4`, arg unused; body in
    // LogicTypeTable.cpp - fires the two m_04/m_08 deferred callbacks, resets
    // m_28), CMovingLogic's 0x13c70 override (QAEXH, 100% EXACT; + MovingSlot16
    // tail) and CGrunt's 0x5ecd0 (`ret 4`, NO exit materializes eax -> void).
    // Was the no-arg/i32 `UserLogicVfunc3` placeholder + a non-virtual
    // FinalizeStep twin of the same 0x8b90 body.
    virtual void FinalizeStep(i32 unused);
    virtual i32 Activate();
    virtual i32 UserLogicVfunc5();
    virtual i32 UserLogicVfunc6();
    // slot 9: a return-0 default; the one known override is CGrunt's per-frame
    // attack-fire step (@0x61cb0), which names the slot.
    virtual i32 StepAttackFire();
    virtual i32 UserLogicVfunc8();
    virtual i32 UserLogicVfunc9();
    virtual i32 UserLogicVfuncA();
    virtual i32 UserLogicVfuncB();
    virtual i32 UserLogicVfuncC(); // slot 14 (retail impl 0x001730)
    virtual i32 UserLogicVfuncD(); // slot 15 (retail impl 0x003607)

    // (The shared serialize chain at 0x16e7f0 is the `SerializeMove` slot-1 virtual
    // declared above - it is THIS class's own override, defined in MovingLogic.cpp.
    // The `SerializeChain` non-virtual twin that used to be declared here, and the
    // fake CMovingLogicBase the leaves cast to, were both dissolved onto it.)

    // (The former SerialRef34() +0x34 facet hop is gone: the "+0x34 CSerialObjRef"
    // IS the CWapX second base below - leaves call the inherited Chain directly.)

    // Copies the bound object's screen position into the out point (m_object->m_5c
    // = x, m_object->m_60 = y). 0x29a50, __thiscall ret 4. The out-point is the
    // shared {m_x,m_y} GruntTilePos (CGrunt reaches this same method inherited).
    typedef GruntTilePos ScreenPoint;
    void GetScreenPos(ScreenPoint* out); // 0x29a50 (out-of-line in BattlezMapConfig.cpp)

    // True when the bound object's current screen pos (m_object->m_5c/m_60) still
    // equals the saved pos at this+0x17c/+0x180 (leaf-class fields beyond
    // CUserLogic's 0x40 - read via offset since the leaf isn't modeled). 0x29a80.
    i32 IsAtSavedScreenPos(); // 0x29a80 (out-of-line in BattlezMapConfig.cpp)

    // Inline one-shot wrapper: registers the built-in logic types the first time
    // any tile-logic object is built. Inlined into the 1-arg ctor; its `this`
    // setup is why the retail call carries the dead `mov ecx,esi`.
    void RegisterLogicTypesOnce();
    void BuildLogicTypeTable(CGameObject* obj); // 0x8a40 (ignores this)

    // __thiscall stub methods re-homed from src/Stub/ApiCallers.cpp; bodies in
    // src/Gruntz/UserLogic.cpp.
    i32 winapi_04d800_CopyRect(i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32);
    // (winapi_064540_PostMessageA was XREF-recovered as CGrunt::StepWarpExit - the
    //  anim-code "C" act handler; body in src/Gruntz/GruntEntranceArrival.cpp.)
    // (winapi_0ee800_IntersectRect_PtInRect was XREF-recovered as CGrunt::ArrivalReticleScan
    //  and re-homed to src/Gruntz/GruntReticle.cpp as a real CGrunt method.)
    // (LoadGruntTypeTable @0x4dd50 is really CGrunt's - SYMBOL-exported in UserLogic.cpp
    //  under ?LoadGruntTypeTable@CGrunt@@; declared on CGrunt in <Gruntz/Grunt.h>, not here.)
    void LoadGruntTuningConstants(i32);
    // Leaf placement/arm entrypoints reached through the bound-logic base pointer
    // (CTriggerMgr::SpawnGrunt / ResetGroup on the created sprite's AnimWorkerObj::m_logic):
    // Place @0x4c1c4 (the grunt/puddle placement driver), Arm @0x4e517 (the target-cursor
    // lighting/config arm). Reloc-masked leaf bodies.
    // (FinalizeStep - 0x8b90, body in LogicTypeTable.cpp - is now the slot-5
    // virtual above; retail's slot holds its ILT thunk 0x3913, which
    // reloc_fidelity thunk-resolves onto the body.)
    i32 Place(i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32); // 0x4c1c4
    void Arm(const char* lighting, const char* cursor, i32 kind, i32 one); // 0x4e517

    i32 m_04;          // +0x04
    i32 m_08;          // +0x08
    CGameObject* m_0c; // +0x0c
    union { // +0x10  bound game object (== m_38; one type, two historical spellings).
            // The bound obj IS the created A-kind sprite (every binding site hands a
            // CreateSprite/ReadPlaneObjects product; leaves read its m_1a0/frame cache).
        CWwdGameObjectA* m_object;
        CWwdGameObjectA* m_10;
    };
    union { // +0x14  aux sub-object (obj->m_7c); CGrunt views it as CAnimLookupNode*
        AnimWorkerObj* m_objAux;
        CAnimLookupNode* m_14;
    };
    CUserBaseLink m_link; // +0x18..+0x27 (ctor 0x16d710, can throw)
    i32 m_28;             // +0x28
    i32 m_2c;             // +0x2c  (base ctor 0x58cd0's highest write: `mov [esi+0x2c],2`)
    // +0x30 opaque anim-set node handle (m_objAux->m_1c), saved before a re-latch.
    // OWNED BY THE BASE (2026-07-17, SM1): CUserLogic's OWN slot-1 virtual
    // SerializeMove (0x16e7f0, below) read/writes this+0x30 (`lea ecx,[edi+0x30]`
    // in both the mode-4 write and mode-7 read arms, streamed right after m_28/m_2c)
    // - a base virtual cannot touch a derived field, so +0x30 is CUserLogic's.
    // Was declared THREE times in the derived worlds (TILE_LOGIC_TAIL, CTileLogic,
    // CGrunt) under this same name/offset/role: one inherited field, modeled thrice.
    void* m_prevAnimSetNode; // +0x30
};
SIZE(CUserLogic, 0x34);       // base size 0x34 (see the NOTE). The tile-logic leaves'
VTBL(CUserLogic, 0x001e705c); // vtable_names -> code (RTTI game class)
                              // 0x34..0x54 band is the CWapX SECOND BASE (below).
// NOTE - the ONE TRUE CUserLogic size is 0x34, NOT 0x30 and NOT 0x40. Evidence (retail):
//   * INDEPENDENT CORROBORATION (MI1, 2026-07-17): every tile-logic leaf's RTTI
//     ClassHierarchyDescriptor places its CWapX second base at PMD.mdisp +0x34 (65
//     CHDs; e.g. CWarlord's @VA 0x5f3818). MSVC lays base subobjects consecutively,
//     so mdisp IS the compiler's own statement that sizeof(CUserLogic)==0x34 - a
//     second, structural proof of the same boundary the SerializeMove body proves
//     below. Two unrelated lines of retail evidence agree.
//   * CUserLogic's OWN slot-1 virtual is SerializeMove @0x16e7f0 (vtable 0x1e705c
//     slot 1, tagged `override` of CUserBase's slot-1 by vtable_hierarchy/RTTI; the
//     sibling CGruntVoice : CUserLogic tags the same slot `inherited`, so CUserLogic
//     is the class that DEFINES it). That body read/writes this+0x04..+0x30
//     INCLUSIVE - `lea ecx,[edi+0x30]` feeds the archive Read/Write right after
//     m_28/m_2c. -> the base object extends through 0x30, i.e. size 0x34.
//   * SUPERSEDED (was: "the base ctor 0x58cd0 only writes through m_2c, so the base
//     ends at 0x30"). A ctor need not initialize every field: +0x30 is a lookup
//     cache the leaves seed on demand (`m_prevAnimSetNode = m_objAux->m_1c`), so
//     its absence from the ctor proves nothing about the boundary.
//   * SUPERSEDED (was: "CGrunt places its OWN members at 0x30, proving the base is
//     0x30"). CGrunt::SerializeMove @0x53b80 CALLS 0x16e7f0 (`sema xref 0x16e7f0`),
//     so the base chain writes CGrunt's +0x30 - it cannot be CGrunt's own field.
//     CGrunt's own members begin at +0x34.
//   * The ??_7CUserLogic@@6B@ vftable is 16 slots (0x40 bytes); ??_7CUserBase is
//     3 slots (config/vtable_names.csv).
//   * Corroboration (matcher-2, 0x9b8b0 - a tile-logic leaf ctor, vptr 0x5e801c): the
//     retail fold is base-init-through-m_2c (m_04/m_08=0, m_28=0x3e9, m_2c=2) THEN the
//     tail `mov [esi+0x34],edi; mov [esi+0x38],edi; mov eax,[edi+0x7c]; mov [esi+0x3c],
//     eax` THEN the leaf vptr `mov [esi],0x5e801c` - i.e. m_34/m_38/m_3c are set AFTER
//     the base ends and BEFORE the leaf vptr, exactly what CTileLogic(obj) below emits.
//
// REPARENT HISTORY: matcher-2 (2026-07-05) first split the fat 0x40 view into a base +
// a CTileLogic intermediate carrying a 0x30..0x40 "tail". SM1 (2026-07-17) proved the
// base's true boundary is 0x34 (m_prevAnimSetNode is CUserLogic's). MI1 (2026-07-17)
// finished it from RTTI: there is NO CTileLogic in retail - the "tail" (+ each leaf's
// m_pad40) was the CWapX SECOND BASE at +0x34 spelled as padding, and every tile-logic
// leaf is `class CLeaf : public CUserLogic, public CWapX`. The intermediate and the
// TILE_LOGIC_TAIL/TILE_LOGIC_SEED macros are deleted.
//
// RIDER EVIDENCE (matcher-2) - the parallel CUserBase/CUserLogic in <Gruntz/Grunt.h>:
// ~1119/1130 is still a separate ODR view. Member diff (offsets identical; owner/type/
// name differ):
//   * Field-owner boundary: canonical CUserBase is EMPTY (just the vptr); every
//     +0x04..+0x2c field lives on CUserLogic (base ctor 0x58cd0 writes them). Grunt.h
//     puts +0x04..+0x14 on CUserBase - a modeling choice (CUserBase's ctor writes
//     nothing). Canonical boundary is the correct one.
//   * +0x10: canonical m_object (CGameObject*) == Grunt.h m_10 (CGruntHud*, a redundant
//     partial CGameObject view -> delete on merge).
//   * +0x14: canonical m_objAux (AnimWorkerObj*, == obj->m_7c) == Grunt.h m_14
//     (CAnimLookupNode*, a partial AnimWorkerObj view).
//   * Vtable: 16 slots, same order. Grunt.h's SerializeMove(1)/UserBaseVfunc2(2)/
//     Activate(6)/UserLogicVfunc9(11) are CGrunt-SPECIFIC OVERRIDE names; the base
//     slot names stay generic (UserBaseVfunc*/UserLogicVfunc*).
//
// PER-SLOT SIGNATURE TABLE (stage 4, disasm evidence). The dispatch call sites
// (Anim/Logic/InGameWorkerHandlers, StateDispatch 0x9b...) DISCARD the returns, so the
// CALLER bytes are i32/void-neutral; the IMPL decides. i32-return is proven/corroborated:
//   slot 1  Serialize   base 0x16e7f0 -> `mov eax,1; ret 0x10`            = i32 (proven)
//   slots 6/10/11/12/13/14/15  consumed as i32 by three independent dispatchers
//     (Logic/AnimWorkerHandlers + StateDispatch's vtbl 0x18/0x28/0x2c/0x30/0x34/0x38/
//      0x3c) -> i32 (corroborated). Canonical models ALL 16 as `i32 f()` and matches.
//   Grunt.h models the SAME slots `void f()` and ALSO matches (its impls that don't set
//     eax pair with a void prototype). Retail base is i32; a void override of an i32
//     virtual is a C++ error, so a merged CGrunt must switch its `void` overrides to
//     `i32` - which ADDS a `mov eax` to any CGrunt impl that does NOT already set eax,
//     breaking those exacts. => signature reconciliation is NOT free; it breaks whichever
//     world adopts the other's prototype.
//
// CGrunt ODR MERGE - STILL BLOCKED after the reparent. Of the four blockers:
//   (1) SIZE      : RESOLVED - canonical CUserLogic is now the true 0x30; CGrunt's own
//                   0x30/0x38/0x40 members no longer collide with a fat base.
//   (2) +0x38 TYPE: RESOLVED - m_38 is no longer a CUserLogic field (it moved to
//                   CTileLogic); CGrunt places its OWN m_38 (CGruntAnimState*) with no
//                   base clash.
//   (3) CTOR MODEL: BLOCKS. tile-logic leaves NEED an INLINE CUserLogic(obj) (they fold
//                   the full AddLogic* init, e.g. 0x9b8b0); CGrunt NEEDS it OUT-OF-LINE
//                   (it CALLs the standalone 0x58cd0 via ILT 0x3828). One shared header
//                   cannot be both inline and out-of-line -> whichever choice breaks the
//                   other world. Unresolvable by reparenting.
//   (4) SIGNATURE : BLOCKS (see the table): i32 base breaks CGrunt's void impls; void
//                   base breaks the canonical i32 world.
// => merge deferred (stage 5 NOT executed): blockers (3)+(4) each force a choice that
//    breaks one world, exactly the STOP-and-report condition. The two defs remain a
//    documented ODR dual-model that never coexists in a TU. Evidence banked; do not
//    re-derive. (Also deferred: fold StateDispatch.cpp's local 16-slot CUserLogic view
//    onto this class - safe once merge (3)/(4) are cracked.)

// Shared true-0x30 base init the leaves fold in. Inline so MSVC inlines it; stores
// the CUserLogic vptr, then inits fields through m_2c (exactly retail 0x58cd0, whose
// highest write is `mov [esi+0x2c],2`). Defined here (not the .cpp) because only an
// inline base ctor is folded into the derived ctors. The tile-logic leaves' 0x30-0x3c
// tail is seeded by CTileLogic(obj) below (retail: each leaf sets m_34/m_38/m_3c right
// after this folded base init, e.g. CTeleporter @0x410f9).
// A TU that needs retail's OUT-OF-LINE `call ??0CUserLogic` (0x58cd0) instead of the
// folded inline (the SBI_ITEM_OWN_CTOR device pattern) #defines USERLOGIC_OOL_CTOR
// before including this header; the one real body stays in UserLogicCtorEmit.cpp.
#ifndef USERLOGIC_OOL_CTOR
inline CUserLogic::CUserLogic(CGameObject* obj) {
    m_0c = obj;
    m_object = static_cast<CWwdGameObjectA*>(obj); // the bound obj IS the A-kind sprite
    m_objAux = obj->m_7c;
    {
        zBitVec tmp(g_emptyString, 0);
        m_link.m_str = tmp;
    }
    RegisterLogicTypesOnce();
    m_object->AddLogicHit("LogicHit");
    m_object->AddLogicAttack("LogicAttack");
    m_object->AddLogicBump("LogicBump");
    m_04 = 0;
    m_08 = 0;
    m_28 = 0x3e9;
    m_2c = 2;
}
#endif // USERLOGIC_OOL_CTOR

inline void CUserLogic::RegisterLogicTypesOnce() {
    if (!g_logicTypesRegistered) {
        BuildLogicTypeTable(m_0c);
        g_logicTypesRegistered = 1;
    }
}

// ---------------------------------------------------------------------------
// HISTORY (2026-07-17): the shared "+0x30 tail" documented here for months was CWapX
// below, spelled as padding. Three shapes modelled it in turn - a fat CTileLogic : CUserLogic
// intermediate (SIZE 0x40), the TILE_LOGIC_TAIL/TILE_LOGIC_SEED member-injection macros, and
// per-leaf m_pad40 - all three now deleted in CWapX's favour.
//
// Why they held for so long: none was refuted by a build. A test of macro-vs-CTileLogic
// picked the macro (leaf ctor 96.87 vs 96.17) and that was written down as "TILE_LOGIC_TAIL
// is RTTI-faithful, don't fix it to inheritance" - but the ballot had two names on it and
// the real base was on neither. Rejecting one alternative never elects the incumbent.
// The RTTI claim was also read off each leaf's TYPE DESCRIPTOR (COMDAT-folded to
// .?AVCUserLogic@@), which is structurally blind to a second base: multiple inheritance
// lives in the CHD's base-class array, unread until the walk below.
// ---------------------------------------------------------------------------
// CWapX - the REAL SECOND BASE of every tile-logic leaf (RTTI .?AVCWapX@@, TD @VA
// 0x60a3e0). Non-polymorphic (no vtable/COL anywhere in retail). This class is what
// the TILE_LOGIC_TAIL macro + the CTileLogic intermediate + each leaf's m_pad40
// were SPELLING AS PADDING - all three are deleted in its favour (2026-07-17).
//
// PROVEN from retail's own tables (a raw-bytes RTTI walk over GRUNTZ.EXE):
//   * 68 ClassHierarchyDescriptors carry it - every one attributes=1 (MULTIPLE
//     INHERITANCE). 65 tile-logic leaves list it at PMD.mdisp +0x34 as a DIRECT
//     base beside CUserLogic (e.g. CWarlord CHD @VA 0x5f3818:
//     CWarlord+0 | CUserLogic+0 | CUserBase+0 | CWapX+0x34 - while CUserLogic's own
//     CHD @0x5f1fd8 is attributes=0 / 2 bases, so CWapX does NOT arrive through it);
//     CGrunt/CProjectile list it at +0x150 (a DIRECT base of each, past the 0x150
//     CMovingLogic spine); CBoomerang INHERITS it through CProjectile and must not
//     declare it (decoded from the CHD numContainedBases nesting - see below).
//
// THREE DISPLACEMENTS, ONE CLASS (MI1, 2026-07-17). Decoding each CHD's
// numContainedBases nesting (the base array is a pre-order flattening, so a direct
// base is found by skipping each entry's whole subtree) gives the DIRECT declarers:
//   CTileTrigger & the 60-odd tile leaves : CUserLogic(0x34) + CWapX@0x34
//   CGrunt                                : CMovingLogic(0x150) + CWapX@0x150
//   CProjectile                           : CMovingLogic(0x150) + CWapX@0x150
//   CBoomerang                            : CProjectile only  -> INHERITS CWapX
//   CCoveredPowerup/CGiantRock/...        : CTileTrigger only -> INHERIT CWapX
// The +0x150 world spelled this base FLAT as own members and reached the same five
// fields by three independent recoveries - CGrunt's m_150/m_154/m_158/
// m_prevEntranceDesc/pad160 and CProjectile's m_150/m_sprite/m_158/m_savedFrameGeo/
// m_pad160 are m_34/m_38/m_3c/m_value/m_blob. The BYTES agree: CGrunt's ctor @0x47a10
// emits `mov [esi+0x150],ebp; mov [esi+0x154],ebp; mov ecx,[ebp+0x7c];
// mov [esi+0x158],ecx` - the CWapX(obj) three-store seed, at +0x150 instead of +0x34.
//   * ~CWarlord's FuncInfo @VA 0x5f8298 (magic 0x19930520) has maxState=3; state 1's
//     funclet @0x1d8578 is `p = this ? this+0x34 : 0; ~T(p)` - the null-check
//     this-adjust cl emits ONLY for a non-primary base - and it tail-jmps (via ILT
//     0x1b04) to 0x8be0: a 1-byte `ret` = the out-of-line COMDAT of an EMPTY INLINE
//     dtor (the main dtor body 0x107f0 carries NO call to it - cl inlined it away -
//     so the definition was visible: `~CWapX() {}` here).
//   * SIZE 0x20, thrice-corroborated: CWarlord's first own member (the m_54 CString
//     its dtor destroys) sits at 0x34+0x20=0x54; the CTileTrigger state pumps
//     `operator new(0x54)` = 0x34 (CUserLogic) + 0x20 (CWapX) with no leaf members;
//     CPulseHighlight's own fields likewise start at +0x54.
//   * mdisp +0x34 independently corroborates SIZE(CUserLogic, 0x34) above (bases are
//     laid consecutively, both 4-aligned, no inter-base padding) - the same boundary
//     SM1 proved from CUserLogic::SerializeMove's own `lea ecx,[edi+0x30]`.
//
// WHAT IT IS: the serialized-object-reference mixin (the former CSerialObjRef view,
// dissolved onto it). Chain (0x8c00, __thiscall ret 0x10) is run by each leaf's
// SerializeMove override to persist WHICH registry object this logic points at,
// keyed by that object's registry NAME: mode 7 = READ (read an 0x80-byte key name
// + the 0x10-byte blob, resolve the name through obj->m_7c->m_0c->m_animRegistry's
// CMapStringToPtr m_10 into m_value); mode 4 = WRITE (re-derive the value's name via
// KeyOfValue_152d30, write it back + the blob). Chain's body writes this-rel
// +0x00/+0x04/+0x08/+0x0c and streams +0x10..0x20 - the whole 0x20 layout.
//
// CONSTRUCTION - the ctor is very likely TRIVIAL; the derived ctors assign these
// fields THEMSELVES (MI1, 2026-07-17). The +0x34 leaves write [esi+0x34]=obj,
// [esi+0x38]=obj, [esi+0x3c]=obj->m_7c right after the folded CUserLogic init, which
// an init-list `CWapX(obj)` reproduces exactly - so the leaves below use that spelling
// and are byte-matched. But CProjectile::CProjectile(owner) @0xdec60 emits the SAME
// three stores at +0x150 AFTER four unrelated body statements (m_148=0, m_14c=0,
// m_object->m_moveMode=7, Fn16ea90()), which a base ctor CANNOT do - it must run before
// the body. The only shape consistent with BOTH is a trivial CWapX ctor + per-derived
// body assignment, with the leaves' stores merely happening to come first. The
// init-list spelling is kept where it is byte-identical (the leaves) and NOT used where
// the order forbids it (CProjectile); the 1-arg ctor below is therefore a spelling
// convenience, not evidence of a seeding base ctor. Do not "unify" the two by moving
// CProjectile's stores into an init list - that is the disagreement, not a defect.
// The no-arg leaf ctors write nothing here (e.g. CTileTrigger @0x11160).
//
// Field names keep the tile-leaf flat-offset spellings every leaf already uses
// (CWapX-relative +0x00/+0x04/+0x08; the CGrunt world sees them at +0x150).
// ---------------------------------------------------------------------------
#include <Gruntz/SerialArchive.h> // CSerialArchive == CFileMemBase (typedef; NEVER fwd-declare it)
class CWapX {
public:
    CWapX() {}
    CWapX(CGameObject* obj) {
        m_34 = obj;
        m_38 = static_cast<CWwdGameObjectA*>(obj); // the bound obj IS the created A-kind sprite
        m_3c = obj->m_7c;
    }
    ~CWapX() {} // EMPTY INLINE (see the 0x8be0 evidence above); out-of-line COMDAT
                // pinned by @rva-symbol in ActionArea.cpp
    // Serialize the referenced object by its registry key name (read/write per mode).
    i32 Chain(CSerialArchive* arc, i32 mode, i32 unused, CGameObject* obj); // 0x8c00

    // Field names keep the tile-leaf +0x34 spellings (this class is reached at THREE
    // displacements - see the note above - so no one spelling can be offset-accurate).
    // The union of what the three worlds independently recovered about each slot:
    CGameObject* m_34;      // +0x00 (leaf +0x34, moving +0x150)  the referenced object
    CWwdGameObjectA* m_38;  // +0x04 (leaf +0x38, moving +0x154)  == m_34: the bound/render
                         //   CREATED SPRITE (the A kind - every binding site hands a
                         //   CreateSprite product; leaves read its m_1a0 cursor/frame cache)
                         //   object (leaves read m_38->m_flags; the projectile world
                         //   called this m_sprite, "primary sprite/render object")
    AnimWorkerObj* m_3c; // +0x08 (leaf +0x3c, moving +0x158)  obj->m_7c - the bound
                         //   object's worker record (CGrunt called this m_158 and
                         //   proved it the same way: ctor tail `= obj->m_7c`)
    // +0x0c (leaf +0x40, moving +0x15c)  the resolved registry value - the anim
    // registry's values are CAniElement (: CObject) entries; every world's ctors
    // snapshot the bound object's active descriptor here via the SAME expression
    // (`m_value = m_38->m_1a0.m_14`): the ex per-leaf `CAniElement* m_40`, CGrunt's
    // `m_prevEntranceDesc` and CProjectile's `m_savedFrameGeo` were all this field.
    // Chain re-resolves it by name on READ / KeyOfValue's it on WRITE (CObject* upcast).
    class CAniElement* m_value;
    char m_blob[0x10]; // +0x10..0x20 (leaf +0x44..0x54, moving +0x160..0x170)  the
                       // serialized blob (every world had it as a 0x10 pad)
};
SIZE(CWapX, 0x20);

// (The former CTileLogic::CTileLogic(obj) tail-seeding ctor is gone: those three
// stores are the CWapX(obj) base ctor above, folded from each leaf's init list.)

// ---------------------------------------------------------------------------
// CTileTrigger : CTileLogic (vftable 0x5e7f14). Adds no data members. Three
// trace-discovered leaf classes derive from it (CTileSecretTrigger /
// CGiantRock / CCoveredPowerup, each in its own src/Stub/ TU) - shared here so
// they get a single class definition. Ctors/dtor are out-of-line in
// src/Gruntz/UserLogic.cpp (no-arg 0x011160, 1-arg 0x10e220, dtor 0x011290).
// ---------------------------------------------------------------------------
SIZE(CTileTrigger, 0x54);
class CTileTrigger : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    virtual LogicTypeId GetTypeTag() OVERRIDE;                         // slot 2
public:
    CTileTrigger();                 // 0x011160 (no-arg)
    CTileTrigger(CGameObject* obj); // 0x10e220 (1-arg)
    static void InitActReg();       // 0x10e420
    virtual void FireActivation(i32 id)
        OVERRIDE;               // 0x10e4a0 (vtable slot 4 body: per-coord PMF dispatch)
    static void RegisterActs(); // 0x10e600
    i32 AdvanceAnim();          // 0x10ee00
    // No data of its own: SIZE 0x54 = 0x34 (CUserLogic) + 0x20 (CWapX) EXACTLY, and
    // the three sub-leaves (CTileSecretTrigger/CGiantRock/CCoveredPowerup) add none
    // either - the state pumps' `new CTileTrigger`/`new CTileSecretTrigger`/... all
    // push 0x54. (The old `char m_pad40[0x54-0x40]` was the CWapX m_value+m_blob.)
    //
    // NO user-declared dtor: retail 0x011290 is the COMPILER-GENERATED one (implicit
    // elides the leaf-vptr restamp a user `{}` would emit now that the CWapX base EH
    // state blocks the old dead-store elision; eh-dtor-vptr-restamp CAUSE B). It stays
    // implicitly-inline, so the three sub-leaf dtors (0x11540/0x11600/0x116c0) still
    // FOLD it; the out-of-line COMDAT (called by ??_G) is labeled via the @rva-symbol
    // pin in src/Gruntz/UserLogic.cpp.
};
VTBL(CTileTrigger, 0x1e7f14);

// --- vtable catalog (reduced-view classes share their base vtable rva) ---

// --- vtable catalog (view/base classes bound to their unit vtable rva) ---

#endif // GRUNTZ_USERLOGIC_H

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

// ---------------------------------------------------------------------------
// CGameObject - the engine object the 1-arg ctors are handed (read into edi).
// Only the fields/methods those ctors touch are modeled; bodies live in engine
// TUs (modeled NO-body so the calls reloc-mask):
//   AddLogicHit/Attack/Bump = 0x150f50 / 0x151030 / 0x151110  (__thiscall, char*)
//   m_7c                    = a sub-object pointer copied into the trigger.
// ---------------------------------------------------------------------------
struct CGameObject;  // fwd (the worker's collide callback takes the object)
class CUserLogic;    // fwd (AnimWorkerObj::m_logic is the object's bound logic leaf)
class CSerialObjRef; // fwd (the +0x34 serialize-ref facet; <Gruntz/SerialObjRef.h>)
struct GruntTilePos; // fwd (the {m_x,m_y} screen-pos out-point; <Gruntz/Grunt.h>)

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

// The +0x198 layer descriptor several eyecandy ctors poll for z-clamping (its
// +0x10/+0x14 bounds + +0x1c base offset). Only the touched offsets are modeled.
SIZE_UNKNOWN(CGameObjLayer);
struct CGameObjLayer {
    char m_pad00[0x10];
    i32 m_zClampLo; // +0x10  z-clamp bound (eyecandy)
    i32 m_zClampHi; // +0x14  z-clamp bound (eyecandy)
    // +0x18/+0x1c are the object's HALF-EXTENTS, not a base origin (the old
    // m_baseX/"layer base X" name was a misreading; renamed via sema rename).
    // Three independent consumers agree, all building a box SYMMETRICALLY around the
    // object's screen position:
    //   PathHazard::0x298    rect = (sx - m_halfWidth + 7, sy - m_halfHeight + 7,
    //                                sx + m_halfWidth - 7, sy + m_halfHeight - 7)
    //   CObjectDropper::Update  the identical +-7 wander box
    //   TileLogicPump/FrontCandyAni  z-sort key = sy + m_halfHeight (the bottom/feet edge)
    i32 m_halfWidth;  // +0x18  half-width  (screen px)
    i32 m_halfHeight; // +0x1c  half-height (screen px)
    i32 m_20;         // +0x20  layer screen-offset X (CGruntVoice::Update bubble placement)
    i32 m_24;         // +0x24  layer screen-offset Y (CGruntVoice::Update bubble placement)
};

// (The former CLogicHandlerMap shell + the LogicMap() offset-hop accessor are
// worker cache map - m_0c (the CDDrawSurfaceMgr) -> m_workerCache (+0x14, the
// CDDrawWorkerCache) -> m_10 (its +0x10 CMapStringToOb; retail Lookup 0x1b8008,
// which the old `(CMapStringToPtr*)` cast at the AddLogic* sites mis-bound to
// the 0x1b8438 Ptr band). The AddLogic* bodies now read the typed path.)

// Exact size 0x1dc, byte-proven from TWO new-sites: CDDrawChildGroup::CreateSpriteImpl
// (@0x159600) news 0x1dc for every created instance, and WwdFile's ReadPlaneObjects
// manually `operator new(0x1dc)`s + runs the same engine ctor (0x15b390).
SIZE(CGameObject, 0x1dc);
// NO VTBL: this struct declares no virtual at all, so it has no vtable datum and cl
// emits no ??_7CGameObject anywhere (llvm-nm over every base obj). The old
// RELOC_VTBL(CGameObject, 0x001efb80) was pure noise - 0x1efb80 is ??_7AnimWorkerObj
// (VTBL'd in <DDrawMgr/AnimWorkerObj.h>), one of the THREE vtables the engine ctor
// 0x15b390 stamps (0x5efbc0 WwdBResolve / 0x5f0020 CWwdGameObjectE / 0x5efb80
// AnimWorkerObj), i.e. an EMBEDDED sub-object's vtable, never this class's.
class CAniElement; // ApplyGeometryDirect's geometry source (<Gruntz/AniElement.h>)
struct CGameObject {
    void Construct(void* owner, i32 id, i32 z); // 0x15b390  the engine ctor (base subobject)
    void AddLogicHit(char* key);                // 0x150f50
    void AddLogicAttack(char* key);             // 0x151030
    void AddLogicBump(char* key);               // 0x151110
    // The created game-sprite's frame-cache + geometry methods (bodies in
    // SpriteResource.cpp). They reinterpret the role-union fields m_0c/m_194/m_198(m_layer)
    // /m_19c as the resource holder / cached sprite / frame ptr / frame number (see the
    // field comments) - authentic union access, cast in the bodies.
    void ApplyLookupSprite(const char* key, i32 flag);  // 0x1504d0 (frame-cache, 2-arg)
    void ApplyName(const char* name);                   // 0x150540 (first-frame cache)
    i32 ApplyLookupGeometry(const char* key, i32 flag); // 0x1505b0
    i32 LookupAnimSprite(const char* name);             // 0x150610  (anim-set cache)
    void ApplyGeometryDirect(CAniElement* srcSprite, i32 applyDefault); // 0x58b60 (the
    // geometry source IS the resolved CAniElement - the body feeds it straight to
    // the +0x1a0 cursor Setup)
    i32 EnsureWorker80(CGameObject* src); // 0x150eb0  (lazy worker @ +0x80, dispatch)
    i32
    EnsureWorker88(CGameObject* src); // 0x150f90  (lazy worker @ +0x88, dispatch; returns Slot09)
    i32
    EnsureWorker90(CGameObject* src); // 0x151070  (lazy worker @ +0x90, dispatch; returns Slot09)

    // vptr @ +0x00 (declared-only slots; nothing constructs a bare CGameObject, so
    // no vtable is ever emitted from source - every dispatch reloc-masks). The real
    // object's table is the CWwdGameObjectE family's (0x5f0020): slots 0-4 are MFC
    // CObject's, 5/6 the CWapObj IsLoaded/IsReady pair - named after the canonicals
    // (<Wwd/WwdGameObjectFamily.h>), NOT derived: this view keeps its own slot 1
    // `Delete(i32)` spelling because retail's ReadPlaneObjects delete-sites carry NO
    // null guard (`push 1; call [edx+4]` bare), which plain `delete` under MSVC5
    // would add (RemoveAndDelete_159db0 shows the guarded form). [10] Load = the
    // record-load virtual (ReadPlaneObjects pushes 4 args + checks the int return);
    // [11] Draw: CGameLevel::VisitVisible dispatches it (+0x2c) per object.
    virtual void GetRuntimeClass();               // [0]  +0x00  CObject slot (0x1bef01)
    virtual void* Delete(i32 flag);               // [1]  +0x04  scalar-deleting dtor
    virtual void Serialize();                     // [2]  +0x08  CObject slot (0x0028ec)
    virtual void AssertValid();                   // [3]  +0x0c  CObject slot (0x00106e)
    virtual void Dump();                          // [4]  +0x10  CObject slot (0x004034)
    virtual i32 IsLoaded();                       // [5]  +0x14  0x15b370 (worker-gate)
    virtual i32 IsReady();                        // [6]  +0x18  0x001c08 (CWapObj default)
    virtual void ReleaseSubs();                   // [7]  +0x1c  0x15b5d0
    virtual i32 GetTypeId();                      // [8]  +0x20  (per-kind type tag;
                                                  //       CTriggerMgr::Load checks ==5)
    virtual i32 SetPosition(i32 x, i32 y);        // [9]  +0x24  0x164790 (pos + draw reseed)
    virtual i32 Load(i32 a, i32 b, i32 c, i32 d); // [10] +0x28  record-load virtual
    virtual void Draw(void* arg);                 // [11] +0x2c  per-object draw hook (VisitVisible)

    i32 m_04;    // +0x04
    i32 m_flags; // +0x08  bit4 = riding m_carrier; bit8 (0x100) = collision-active;
                 //        bit10 (0x400) = pass soft-block tiles; 0x20000 = z-key dirty;
                 //        0x400000 = special-tile latch (probe kind 4)
    // +0x0c  the owning world/display root: the ONE CDDrawSurfaceMgr
    // (<DDrawMgr/DDrawSurfaceMgr.h>). Every former view of this slot's pointee -
    // CGameObjWorld (+0x08 obj-chain == m_childGroup / +0x14 worker-cache ==
    // m_workerCache / +0x24 level == m_level), CEntranceResMgr (+0x2c ==
    // m_animRegistry), CDDrawSurfaceMgr (+0x04/+0x08/+0x10/+0x28/+0x2c), LfxMapHolder (+0x10 spec
    // store == m_imageRegistry / +0x2c effect store == m_animRegistry) - reads the same
    // object; the retail map-Lookup bands corroborate per-slot (0x1504d0 ->
    // 0x1b8008 via +0x10, 0x150610/0x1505b0 -> 0x1b8438 via +0x28/+0x2c).
    class CDDrawSurfaceMgr* m_0c;
    i32 m_10; // +0x10  (worker getters pass src->m_10 through slot 9)
    char m_pad14[0x38 - 0x14];
    i32 m_38; // +0x38
    char m_pad3c[0x40 - 0x3c];
    i32 m_stateFlags; // +0x40  bit0 = visible/active (set by the icon/glitter/booty
                      //        creators; cleared to hide - IconLoaders/GameMode/BzState)
    char m_pad44[0x4c - 0x44];
    i32 m_drawFillArg;   // +0x4c
    i32 m_drawFillCmd;   // +0x50  draw-fill command type (0xb = decay fill-bar)
    i32 m_fillFraction;  // +0x54  fill fraction (0..256)
    i32 m_drawActive;    // +0x58  dirty/active flag
    i32 m_screenX;       // +0x5c  screen x
    i32 m_screenY;       // +0x60  screen y
    i32 m_64;            // +0x64  captured config triple (checkpoint state slots 12..14)
    i32 m_68;            // +0x68
    i32 m_6c;            // +0x6c
    i32 m_70;            // +0x70  (WwdFile record clipRect bottom)
    i32 m_latchedAnimId; // +0x74
    char m_pad78[0x7c - 0x78];
    AnimWorkerObj* m_7c; // +0x7c  the owned 0x17c worker/logic record (its m_notify
                         //        is the post-create init driver the creators run)
    AnimWorkerObj* m_80; // +0x80  lazily-built worker (EnsureWorker80)
    char m_pad84[0x88 - 0x84];
    AnimWorkerObj* m_88; // +0x88  lazily-built worker (EnsureWorker88)
    char m_pad8c[0x90 - 0x8c];
    AnimWorkerObj* m_collideWorker; // +0x90  lazily-built worker (EnsureWorker90); its
                                    //        m_collideNotify is fired by BroadPhase
    CGameObject* m_hitOther;        // +0x94  the other party of the pending collision
                                    //        (stored just before m_collideNotify fires)
    CGameObject* m_carrier;         // +0x98  latched carrier (a category-0x80 platform
                                    //        object; StepAxisAlt stores it + sets flags
                                    //        bit4; CMovingLogic::Update then advances
                                    //        m_screenX/Y by the carrier's m_deltaX/Y)
    char m_pad9c[0xe4 - 0x9c];
    // +0xe4  movement-resolution mode (CGameLevel::DispatchMove kinds 1..8):
    // 7 = direct set (no tile collision; CProjectile seeds it), 1/2/5 -> handler A,
    // 3 -> B, 4 -> C, 8 -> B/C by direction, 6 -> D (two-probe recovery); the
    // handlers transition 1 <-> 4 <-> 6 as moves land/fall/block.
    i32 m_moveMode;     // +0xe4
    u32 m_collCategory; // +0xe8  collision category bits (0x80 = carrier/platform;
                        //        BroadPhase tests other->m_collCategory & t->m_collMask)
    i32 m_ec;           // +0xec  (WwdFile record scatter target)
    i32 m_f0;           // +0xf0  (the entrance-sprite ctor seeds 1)
    u32 m_collMask;     // +0xf4  which categories this object collides with
    i32 m_strideX;      // +0xf8  tile-probe stride X (the move steppers' scan step)
    i32 m_strideY;      // +0xfc  tile-probe stride Y
    char m_pad100[0x114 - 0x100];
    i32 m_114;       // +0x114  (teleporter spawn: source-tile coordinate mirror)
    i32 m_118;       // +0x118  CSpotLight ctor: pi/0 mode gate
    i32 m_11c;       // +0x11c  CSpotLight ctor: settings-table index
    i32 m_120;       // +0x120  CSpotLight ctor: SpotLightTime override
    i32 m_124;       // +0x124  sprite-selector row key (leaf ctors pass it to ApplyLookupSprite)
    i32 m_placeMode; // +0x128  visibility/place mode (1 or 2; the on-screen gate discriminator)
    i32 m_12c;       // +0x12c  CSpotLight ctor: m_58 scale gate
    i32 m_130;       // +0x130  (CUFO ctor: seeds the spotlight's m_120)
    // +0x134..+0x140  signed per-side collision extents around (m_screenX, m_screenY):
    // left/top/right/bottom. Trigger ctors store TILE spans (world box = pos +/-
    // extent<<5 +/- 7); the movement steppers read them as PIXEL offsets (L stored
    // negative). 0x80000000 = unset (BroadPhase skips the object).
    i32 m_extentL; // +0x134
    i32 m_extentT; // +0x138
    i32 m_extentR; // +0x13c
    i32 m_extentB; // +0x140  the feet line (WalkColumnDown ground-snaps from it)
    // +0x144..+0x150  the derived activation/stand box L/T/R/B (world-space in the
    // trigger initializers: VoiceTrigger InitActReg; the platform-carry fit tests
    // read L/T/R relative to the carrier's position - basis differs per family).
    // CSpotLight ctor zeros all four.
    i32 m_areaL; // +0x144
    i32 m_areaT; // +0x148  a platform's stand surface row (AltStepValidate/HoldMove)
    i32 m_areaR; // +0x14c
    i32 m_areaB; // +0x150
    i32 m_154;   // +0x154  captured config block (checkpoint state slots 8..11)
    i32 m_158;   // +0x158
    i32 m_15c;   // +0x15c
    i32 m_160;   // +0x160
    i32 m_164;   // +0x164
    i32 m_168;   // +0x168
    char m_pad16c[0x174 - 0x16c];
    // +0x174/+0x178  per-frame movement deltas. CMovingLogic::Update advances a
    // riding object by its carrier's deltas; AltStepValidate widens the stand-
    // acceptance ceiling by a NEGATIVE (upward) m_deltaY so a rising platform
    // still catches its rider.
    i32 m_deltaX; // +0x174
    i32 m_deltaY; // +0x178
    char m_pad17c[0x188 - 0x17c];
    i32 m_188; // +0x188  object id (warlord battle-event id / game-object archive-cue id)
    i32 m_18c; // +0x18c  (WwdFile stamp: -1; CWwdGameObject low byte = dot color / setup flag)
    union { // +0x190  role-union
        i32 m_190;           // (WwdFile stamp: -1; sprite role: the cached frame number)
        i32 m_resolvedLayer; // grunt-indicator role: the layer index the glyph resolved from
    };
    union {    // +0x194  role-union: a WwdFile-loaded object keeps its source-def
               //         record; a CreateSprite'd object caches the looked-up sprite
               //         (ApplyName/ApplyLookupSprite) / its CImageSet (ActionArea's
               //         pulse ramp SetAllTypes/SetAllField18 @0x152480/0x1524d0).
        char* m_194;                 // source-def record (class-name string at +0x24)
        struct CSprite* m_sprite;    // cached sprite (frame-cache role)
        class CImageSet* m_imageSet; // cached image set (color/brightness role)
        // grunt-indicator role: the layer-clamp/glyph table the Grunt*Sprite updaters
        // walk (m_layerLo/m_layerHi bounds + m_layerTable). Was the CGruntRenderable
        // view's own +0x194 member before that view was folded onto this class.
        struct CGruntLayerHolder* m_layerHolder;
    };
    union {                     // +0x198  role-union
        CGameObjLayer* m_layer; // (sprite role: the cached frame ptr)
        i32 m_mappedLayer;      // grunt-indicator role: the glyph the layer mapped to
    };
    i32 m_19c;              // +0x19c  (WwdFile stamp: 0)
    // +0x1a0..+0x1db: the embedded CAniAdvanceCursor (one real 0x3c member; vptr
    // @+0x1a0, end +0x1dc == SIZE(CGameObject)). The former per-leaf sink views
    // (WwdAnimSub / CAnimSink / CTeleAnimSink / CWarlordAnimSub / CGruntPuddleSink /
    // ...) and the tail fields m_geoId(+0x1b4)/m_1c0(+0x1c0)/m_1c8(+0x1c8) were all
    // duplicate names for its interior: m_geoId == m_1a0.m_14 (the active
    // CAniElement* descriptor), m_1c0 == m_1a0.m_20 (per-frame timer / idle gate),
    // m_1c8 == m_1a0.m_28 (paused-done / active gate). ~CWwdGameObjectA/B stamp
    // ??_7CAniAdvanceCursor at +0x1a0 - the vtable proof of the embed.
    CAniAdvanceCursor m_1a0; // +0x1a0  the anim-advance / geometry cursor
};

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
    virtual i32 UserLogicVfunc1();
    virtual i32 UserLogicVfunc2();
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

    // The shared serialize-chain helper at 0x16e7f0 IS CMovingLogicBase::Serialize
    // (bound in movinglogic); leaf overrides now drive it via ((CMovingLogicBase*)this)
    // ->Serialize(...) [<Gruntz/MovingLogicBase.h>]. Decl kept (unused) to avoid a
    // cross-TU codegen ripple its removal triggers in CUserLogic-including TUs.
    i32 SerializeChain(i32 a, i32 b, i32 c, i32 d); // 0x16e7f0 (superseded; call-free)

    // The serialize-object-reference facet embedded at +0x34 of every tile-logic
    // leaf: a CSerialObjRef (its m_00/m_04/m_08 overlay the tail m_34/m_38/m_3c)
    // whose Chain (0x8c00) the leaf Serialize overrides drive to persist the
    // referenced registry object by name. Centralizes the +0x34 facet access so
    // the leaves don't each `(CSerialObjRef*)((char*)this + 0x34)`. <Gruntz/SerialObjRef.h>.
    CSerialObjRef* SerialRef34() {
        return (CSerialObjRef*)((char*)this + 0x34);
    }

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
    union {            // +0x10  bound game object (== m_38; one type, two historical spellings)
        CGameObject* m_object;
        CGameObject* m_10;
    };
    union { // +0x14  aux sub-object (obj->m_7c); CGrunt views it as CAnimLookupNode*
        AnimWorkerObj* m_objAux;
        CAnimLookupNode* m_14;
    };
    CUserBaseLink m_link; // +0x18..+0x27 (ctor 0x16d710, can throw)
    i32 m_28;             // +0x28
    i32 m_2c;             // +0x2c  (base ctor 0x58cd0's highest write: `mov [esi+0x2c],2`)
};
SIZE(CUserLogic, 0x30);       // TRUE base size: 0x30 (see the NOTE). The tile-logic leaves'
VTBL(CUserLogic, 0x001e705c); // vtable_names -> code (RTTI game class)
                              // 0x30..0x3c tail lives on CTileLogic (below).
// NOTE - the ONE TRUE CUserLogic size is 0x30, NOT 0x40. Evidence (retail):
//   * The base ctor CUserLogic(CGameObject*) @0x58cd0 initializes fields only
//     through m_2c (the highest write is `mov [esi+0x2c],2`), then returns. It
//     never writes 0x30..0x3c. -> the base object ends at 0x30.
//   * CGrunt : CUserLogic (<Gruntz/Grunt.h>) places its OWN, byte-exact-matched
//     members at 0x30 (m_prevAnimSetNode), 0x38 (anim player), 0x40 (activeAnim)
//     - proving the base it inherits is exactly 0x30.
//   * The ??_7CUserLogic@@6B@ vftable is 16 slots (0x40 bytes); ??_7CUserBase is
//     3 slots (config/vtable_names.csv).
//   * Corroboration (matcher-2, 0x9b8b0 - a tile-logic leaf ctor, vptr 0x5e801c): the
//     retail fold is base-init-through-m_2c (m_04/m_08=0, m_28=0x3e9, m_2c=2) THEN the
//     tail `mov [esi+0x34],edi; mov [esi+0x38],edi; mov eax,[edi+0x7c]; mov [esi+0x3c],
//     eax` THEN the leaf vptr `mov [esi],0x5e801c` - i.e. m_34/m_38/m_3c are set AFTER
//     the base ends and BEFORE the leaf vptr, exactly what CTileLogic(obj) below emits.
//
// REPARENT DONE (matcher-2, 2026-07-05): the CTileLogic intermediate now exists and the
// tile-logic leaves (~90: EyeCandy/Teleporter/Projectile/SpotLight/TileTrigger family/
// CMovingLogic/...) derive from it; CUserLogic is slimmed to its TRUE 0x30 and the tail
// lives on CTileLogic. Byte-neutral (full sweep 1847 exacts unchanged). This RESOLVES two
// of the four CGrunt-ODR-merge blockers below.
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
inline CUserLogic::CUserLogic(CGameObject* obj) {
    m_0c = obj;
    m_object = obj;
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

inline void CUserLogic::RegisterLogicTypesOnce() {
    if (!g_logicTypesRegistered) {
        BuildLogicTypeTable(m_0c);
        g_logicTypesRegistered = 1;
    }
}

// ---------------------------------------------------------------------------
// CTileLogic : CUserLogic - the fat tile-logic intermediate (SIZE 0x40). The true
// CUserLogic base ends at 0x30 (base ctor 0x58cd0 writes only through m_2c); the
// tile-logic game-object leaves (EyeCandy, Teleporter, Projectile, SpotLight, the
// TileTrigger family, ...) share a common 0x10-byte tail m_30/m_34/m_38/m_3c that
// their 1-arg ctors seed to obj/obj/obj->m_7c right after the folded base init. This
// intermediate carries that tail + the tail-setting ctor so the leaves fold both the
// base and the tail as one flat sequence, exactly as retail emits them.
//
// Adds NO new/overridden virtual (its vtable == CUserLogic's 16 slots), so MSVC emits
// no distinct ??_7CTileLogic and no extra vptr stamp in leaf ctors/dtors - byte-
// identical to the old fat-CUserLogic view (verified: guardpoint holds its exact
// dtor/Serialize; whole-sweep exacts unchanged).
//
// Naming basis: no RTTI / leaked-path / string evidence names this intermediate (it
// is COMDAT-folded into every leaf and never instantiated standalone with its own
// vftable). "CTileLogic" is a descriptive placeholder for the tile-logic-leaf common
// tail owner; kept generic like the m_<hex> field placeholders. Only the 0x30-vs-0x40
// boundary + the tail offsets are load-bearing.
// ---------------------------------------------------------------------------
// The tile-logic leaves' shared +0x30 tail, inlined per-leaf so each derives
// CUserLogic DIRECTLY (matching RTTI - CTileLogic is not a retail class). Declare
// FIRST in the leaf body so it lands at +0x30. Seed with TILE_LOGIC_SEED(obj).
#define TILE_LOGIC_TAIL                                                                            \
    void* m_prevAnimSetNode;                                                                       \
    CGameObject* m_34;                                                                             \
    CGameObject* m_38;                                                                             \
    AnimWorkerObj* m_3c;
#define TILE_LOGIC_SEED(obj)                                                                       \
    m_34 = (obj);                                                                                  \
    m_38 = (obj);                                                                                  \
    m_3c = (obj)->m_7c;

class CTileLogic : public CUserLogic {
public:
    CTileLogic() {}
    CTileLogic(CGameObject* obj);

    void*
        m_prevAnimSetNode; // +0x30  saved prior aux lookup node (m_objAux->m_1c) before installing "A"
    CGameObject* m_34;     // +0x34
    CGameObject* m_38;     // +0x38  (== the bound object; leaves read m_38->m_flags etc.)
    AnimWorkerObj* m_3c; // +0x3c
};
SIZE(CTileLogic, 0x40);

// The tail-setting 1-arg ctor: chains the true-0x30 CUserLogic(obj) base init, then
// seeds the leaf-shared tail. Inline so MSVC folds it (base + tail) into each leaf's
// 1-arg ctor as one flat sequence.
inline CTileLogic::CTileLogic(CGameObject* obj) : CUserLogic(obj) {
    m_34 = obj;
    m_38 = obj;
    m_3c = obj->m_7c;
}

// ---------------------------------------------------------------------------
// CTileTrigger : CTileLogic (vftable 0x5e7f14). Adds no data members. Three
// trace-discovered leaf classes derive from it (CTileSecretTrigger /
// CGiantRock / CCoveredPowerup, each in its own src/Stub/ TU) - shared here so
// they get a single class definition. Ctors/dtor are out-of-line in
// src/Gruntz/UserLogic.cpp (no-arg 0x011160, 1-arg 0x10e220, dtor 0x011290).
// ---------------------------------------------------------------------------
SIZE(CTileTrigger, 0x54);
class CTileTrigger : public CUserLogic {
public:
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    virtual LogicTypeId GetTypeTag() OVERRIDE;                         // slot 2
    virtual i32 UserLogicVfunc2() OVERRIDE;                            // slot 4
    TILE_LOGIC_TAIL
public:
    CTileTrigger();                 // 0x011160 (no-arg)
    CTileTrigger(CGameObject* obj); // 0x10e220 (1-arg)
    static void InitActReg();       // 0x10e420
    void FireActivation(i32 coord); // 0x10e4a0 (vtable slot 4 body: per-coord PMF dispatch)
    static void RegisterActs();     // 0x10e600
    i32 AdvanceAnim();              // 0x10ee00
    // Leaf tail: TILE_LOGIC_TAIL ends at +0x40; the three leaves (CTileSecretTrigger/
    // CGiantRock/CCoveredPowerup) add no data. Size 0x54 proven from the state pumps'
    // `new CTileTrigger`/`new CTileSecretTrigger`/... = operator new(0x54).
    char m_pad40[0x54 - 0x40]; // +0x40
    // Inline & trivial so it folds into the three leaf dtors (0x11540/0x11600/
    // 0x116c0) rather than being called. MSVC still emits one out-of-line COMDAT
    // copy (called by CTileTrigger's scalar-deleting dtor); it lands at 0x011290
    // and is labeled via the @rva-symbol pin in src/Gruntz/UserLogic.cpp (an
    // inline-defined fn can't hang an RVA() without also tagging the synthesized
    // ??_G - see the duplicate-RVA guard).
    virtual ~CTileTrigger() OVERRIDE {}
};
VTBL(CTileTrigger, 0x1e7f14);

// --- vtable catalog (reduced-view classes share their base vtable rva) ---

// --- vtable catalog (view/base classes bound to their unit vtable rva) ---

#endif // GRUNTZ_USERLOGIC_H

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

// EngStr + the destructible +0x18 link sub-object (CUserBaseLink), shared with
// the CGrunt world so both embed the identical link (see Gruntz/EngStr.h).
#include <Gruntz/EngStr.h>

// ---------------------------------------------------------------------------
// CGameObject - the engine object the 1-arg ctors are handed (read into edi).
// Only the fields/methods those ctors touch are modeled; bodies live in engine
// TUs (modeled NO-body so the calls reloc-mask):
//   AddLogicHit/Attack/Bump = 0x150f50 / 0x151030 / 0x151110  (__thiscall, char*)
//   m_7c                    = a sub-object pointer copied into the trigger.
// ---------------------------------------------------------------------------
struct CGameObjAux; // the sub-object reached through CGameObject::m_7c
struct CGameObject; // fwd (CAnimWorker's collide callback takes the object)

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
SIZE_UNKNOWN(CAnimWorker);
class CAnimWorker {
public:
    virtual void Slot00();              // +0x00
    virtual void Slot01();              // +0x04
    virtual void Slot02();              // +0x08
    virtual void Slot03();              // +0x0c
    virtual void Slot04();              // +0x10
    virtual void Slot05();              // +0x14
    virtual void Slot06();              // +0x18
    virtual i32 Slot07();               // +0x1c  reuse path
    virtual void Slot08();              // +0x20
    virtual i32 Slot09(i32 ctx, i32 z); // +0x24  fed CGameObject->m_10

    i32 m_04; // +0x04
    i32 m_08; // +0x08
    i32 m_0c; // +0x0c
    // +0x10  collision-notify callback: CGameLevel::BroadPhase calls
    // `obj->m_collideWorker->m_collideNotify(obj)` (a raw fn-ptr load off the
    // worker, NOT a vtable dispatch - one indirection in the retail bytes) when a
    // candidate move would overlap another object; zero-stamped at worker build
    // (UserBaseLink) = "no callback".
    i32 (*m_collideNotify)(CGameObject* obj); // +0x10
    i32 m_14;                                 // +0x14
    i32 m_18;                                 // +0x18
    i32 m_1c;                                 // +0x1c
    char m_pad20[0x170 - 0x20];
    i32 m_170;
    i32 m_174;
    i32 m_178;
};

// The foreign worker vftable (0x5efb80); the worker ctor stamps it by address so
// the DIR32 vptr store reloc-masks. Owned by another TU.
extern void* g_animWorkerVtbl;

// The +0x198 layer descriptor several eyecandy ctors poll for z-clamping (its
// +0x10/+0x14 bounds + +0x1c base offset). Only the touched offsets are modeled.
SIZE_UNKNOWN(CGameObjLayer);
struct CGameObjLayer {
    char m_pad00[0x10];
    i32 m_zClampLo; // +0x10  z-clamp bound (eyecandy)
    i32 m_zClampHi; // +0x14  z-clamp bound (eyecandy)
    i32 m_baseX;    // +0x18  layer base X (path/dropper screen-rect origin)
    i32 m_1c;       // +0x1c  layer base Y / base offset
};

SIZE_UNKNOWN(CGameObject);
struct CGameObject {
    void AddLogicHit(char* key);                        // 0x150f50
    void AddLogicAttack(char* key);                     // 0x151030
    void AddLogicBump(char* key);                       // 0x151110
    void ApplyLookupSprite(const char* key, i32 flag);  // 0x1504d0
    void ApplyName(const char* name);                   // 0x150540
    i32 ApplyLookupGeometry(const char* key, i32 flag); // 0x1505b0
    i32 EnsureWorker80(CGameObject* src);               // 0x150eb0  (lazy worker @ +0x80, dispatch)
    void EnsureWorker88(CGameObject* src);              // 0x150f90  (lazy worker @ +0x88, dispatch)
    void EnsureWorker90(CGameObject* src);              // 0x151070  (lazy worker @ +0x90, dispatch)

    // vptr @ +0x00 (declared-only slots; nothing constructs a bare CGameObject, so
    // no vtable is ever emitted from source - every dispatch reloc-masks). Slot
    // roles are unrecovered except [11] Draw: CGameLevel::VisitVisible dispatches
    // it (+0x2c) per object during the between-planes render walk, passing the
    // render visitor. [10] (+0x28) is the sibling hook the object-chain owner
    // also carries - unnamed here (no direct evidence on the object).
    virtual void v00();           // [0]  +0x00
    virtual void v04();           // [1]  +0x04
    virtual void v08();           // [2]  +0x08
    virtual void v0c();           // [3]  +0x0c
    virtual void v10();           // [4]  +0x10
    virtual void v14();           // [5]  +0x14
    virtual void v18();           // [6]  +0x18
    virtual void v1c();           // [7]  +0x1c
    virtual void v20();           // [8]  +0x20
    virtual void v24();           // [9]  +0x24
    virtual void v28(void* arg);  // [10] +0x28
    virtual void Draw(void* arg); // [11] +0x2c  per-object draw hook (VisitVisible)

    i32 m_04;    // +0x04
    i32 m_flags; // +0x08  bit4 = riding m_carrier; bit8 (0x100) = collision-active;
                 //        bit10 (0x400) = pass soft-block tiles; 0x20000 = z-key dirty;
                 //        0x400000 = special-tile latch (probe kind 4)
    i32 m_0c;    // +0x0c  owning world/context (CGameObjWorld view; generically-
                 //        typed i32 across the family, cast at the deref sites)
    i32 m_10;    // +0x10  (worker getters pass src->m_10 through slot 9)
    char m_pad14[0x38 - 0x14];
    i32 m_38; // +0x38
    char m_pad3c[0x40 - 0x3c];
    i32 m_stateFlags; // +0x40
    char m_pad44[0x4c - 0x44];
    i32 m_drawFillArg;  // +0x4c
    i32 m_drawFillCmd;  // +0x50  draw-fill command type (0xb = decay fill-bar)
    i32 m_fillFraction; // +0x54  fill fraction (0..256)
    i32 m_drawActive;   // +0x58  dirty/active flag
    i32 m_screenX;      // +0x5c  screen x
    i32 m_screenY;      // +0x60  screen y
    i32 m_64;           // +0x64  captured config triple (checkpoint state slots 12..14)
    i32 m_68;           // +0x68
    i32 m_6c;           // +0x6c
    char m_pad70[0x74 - 0x70];
    i32 m_latchedAnimId; // +0x74
    char m_pad78[0x7c - 0x78];
    CGameObjAux* m_7c; // +0x7c
    CAnimWorker* m_80; // +0x80  lazily-built worker (EnsureWorker80)
    char m_pad84[0x88 - 0x84];
    CAnimWorker* m_88; // +0x88  lazily-built worker (EnsureWorker88)
    char m_pad8c[0x90 - 0x8c];
    CAnimWorker* m_collideWorker; // +0x90  lazily-built worker (EnsureWorker90); its
                                  //        m_collideNotify is fired by BroadPhase
    CGameObject* m_hitOther;      // +0x94  the other party of the pending collision
                                  //        (stored just before m_collideNotify fires)
    CGameObject* m_carrier;       // +0x98  latched carrier (a category-0x80 platform
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
    char m_padec[0xf4 - 0xec];
    u32 m_collMask; // +0xf4  which categories this object collides with
    i32 m_strideX;  // +0xf8  tile-probe stride X (the move steppers' scan step)
    i32 m_strideY;  // +0xfc  tile-probe stride Y
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
    char m_pad18c[0x194 - 0x18c];
    char* m_194;            // +0x194  object source-def record (its class-name string is at +0x24)
    CGameObjLayer* m_layer; // +0x198
    // +0x1a0: the most-derived game object embeds a per-CLASS anim sub-object here
    // (WwdAnimSub / CPathSubMgr / CAnimSink / CTeleAnimSink / CWarlordAnimSub /
    // DropperAnim / CWormGeoSub / CGruntPuddleSink / ...). Its concrete type is
    // determined by the leaf class, so the base CGameObject* view legitimately can
    // only reach it by address: `((LeafSub*)((char*)m_38 + 0x1a0))->...`. The sink's
    // +0x20/+0x28 fields surface below as m_1c0/m_1c8. Kept as documented authentic
    // raw-offset access (a single base field can't type a per-leaf embedded object).
    char m_pad19c[0x1b4 - 0x19c];
    i32 m_geoId; // +0x1b4
    char m_pad1b8[0x1c0 - 0x1b8];
    i32 m_1c0; // +0x1c0  the +0x1a0 anim sub-mgr's idle flag  (sink+0x20)
    char m_pad1c4[0x1c8 - 0x1c4];
    i32 m_1c8; // +0x1c8  the +0x1a0 anim sub-mgr's active flag (sink+0x28)
};

// ---------------------------------------------------------------------------
// The game-object WORLD chain (the level's / each object's m_0c owner context).
// Real engine classes, UNMATCHED (candidates: the CWwdObjMgr/map-mgr family);
// modeled as typed shells so CGameLevel's movement/render walks and
// CMovingLogic's level hop are cast-free. Layout proven by the matched walkers:
//   owner(+0x0c) -> +0x08 = the object chain; owner -> +0x24 = the CGameLevel
//   (CMovingLogic::Update, WorldLevelPath); chain +0x14 = head node; node
//   {+0x00 next, +0x08 object}.
// ---------------------------------------------------------------------------
SIZE_UNKNOWN(CGameObjNode);
struct CGameObjNode {
    CGameObjNode* next; // +0x00
    char m_pad04[0x04]; // +0x04
    CGameObject* obj;   // +0x08
};

// The chain owner is polymorphic; slot +0x28 is the hook CGameLevel::VisitVisible
// dispatches between the plane syncs on the non-origin-fixed path. The head node
// hangs off a +0x10 sub-record (the engine lea's its ADDRESS and null-checks it
// before loading the head - the sub-struct keeps that byte shape).
SIZE_UNKNOWN(CGameObjChain);
struct CGameObjChain {
    virtual void v00();           // [0]  +0x00
    virtual void v04();           // [1]  +0x04
    virtual void v08();           // [2]  +0x08
    virtual void v0c();           // [3]  +0x0c
    virtual void v10();           // [4]  +0x10
    virtual void v14();           // [5]  +0x14
    virtual void v18();           // [6]  +0x18
    virtual void v1c();           // [7]  +0x1c
    virtual void v20();           // [8]  +0x20
    virtual void v24();           // [9]  +0x24
    virtual void Hook(void* arg); // [10] +0x28  render-walk hook (VisitVisible)

    char m_pad04[0x10 - 0x04];
    struct List {
        char m_pad00[0x04];
        CGameObjNode* head; // +0x04  (i.e. chain +0x14)
    } m_list;               // +0x10
};

class CGameLevel; // fwd (the world owns the level; full class in <Gruntz/GameLevel.h>)
SIZE_UNKNOWN(CGameObjWorld);
struct CGameObjWorld {
    char m_pad00[0x08];
    CGameObjChain* m_objChain; // +0x08  the live object chain (BroadPhase/StepAxisAlt)
    char m_pad0c[0x24 - 0x0c];
    CGameLevel* m_level; // +0x24  the loaded level (CMovingLogic / WorldLevelPath hop)
};

// The +0x7c sub-object: its +0x08 flags, +0x1c bute-node and +0x130 timer are
// touched by the eyecandy/sparkle ctors.
SIZE_UNKNOWN(CGameObjAux);
struct CGameObjAux {
    char m_pad00[0x08];
    i32 m_08; // +0x08
    char m_pad0c[0x1c - 0x0c];
    void* m_1c; // +0x1c
    char m_pad20[0xbc - 0x20];
    i32 m_bc; // +0xbc  per-tile time (teleporter reads the bound object's clock here)
    char m_padc0[0x130 - 0xc0];
    i32 m_130; // +0x130
};

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
// __thiscall member that IGNORES `this` (its impl reads the ctx as its explicit
// stack arg) - that is why the retail call carries `mov ecx,esi; push ctx`. See
// thiscall-ignoring-this. Declared as a CUserLogic method below.
struct CLogicTypeBuilder;

// ---------------------------------------------------------------------------
// CUserBase - root of the game-object hierarchy: just a vptr (3 virtuals,
// vftable 0x5e70b4). Inline ctor so it folds into derived ctors.
// ---------------------------------------------------------------------------
class CUserBase {
public:
    CUserBase() {}
    virtual ~CUserBase() {}       // inline: folds into leaf dtors (final base vptr store)
    virtual i32 UserBaseVfunc1(); // slot 1
    virtual i32 UserBaseVfunc2(); // slot 2
};

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
class CUserLogic : public CUserBase {
public:
    CUserLogic() {}
    CUserLogic(CGameObject* obj);
    virtual ~CUserLogic() OVERRIDE {} // inline: folds into leaf dtors (link teardown + vptr stores)
    virtual i32 UserLogicVfunc1();
    virtual i32 UserLogicVfunc2();
    virtual i32 UserLogicVfunc3();
    virtual i32 UserLogicVfunc4();
    virtual i32 UserLogicVfunc5();
    virtual i32 UserLogicVfunc6();
    virtual i32 UserLogicVfunc7();
    virtual i32 UserLogicVfunc8();
    virtual i32 UserLogicVfunc9();
    virtual i32 UserLogicVfuncA();
    virtual i32 UserLogicVfuncB();
    virtual i32 UserLogicVfuncC(); // slot 14 (retail impl 0x001730)
    virtual i32 UserLogicVfuncD(); // slot 15 (retail impl 0x003607)

    // The shared serialize-chain helper (0x16e7f0, __thiscall ret 0x10). Run on
    // `this` by the leaf Serialize overrides. External/no-body (reloc-masked;
    // pinned in src/Stub/Discovered.cpp).
    i32 SerializeChain(i32 a, i32 b, i32 c, i32 d); // 0x16e7f0

    // Copies the bound object's screen position into the out point (m_object->m_5c
    // = x, m_object->m_60 = y). 0x29a50, __thiscall ret 4.
    struct ScreenPoint {
        i32 x;
        i32 y;
    };
    void GetScreenPos(ScreenPoint* out); // 0x29a50

    // True when the bound object's current screen pos (m_object->m_5c/m_60) still
    // equals the saved pos at this+0x17c/+0x180 (leaf-class fields beyond
    // CUserLogic's 0x40 - read via offset since the leaf isn't modeled). 0x29a80.
    i32 IsAtSavedScreenPos(); // 0x29a80

    // Inline one-shot wrapper: registers the built-in logic types the first time
    // any tile-logic object is built. Inlined into the 1-arg ctor; its `this`
    // setup is why the retail call carries the dead `mov ecx,esi`.
    void RegisterLogicTypesOnce();
    void BuildLogicTypeTable(CLogicTypeBuilder* ctx); // 0x8a40 (ignores this)

    // __thiscall stub methods re-homed from src/Stub/ApiCallers.cpp; bodies in
    // src/Gruntz/UserLogic.cpp.
    i32 winapi_04d800_CopyRect(i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32);
    i32 winapi_064540_PostMessageA();
    i32 winapi_0ee800_IntersectRect_PtInRect();
    void LoadGruntTypeTable(i32, i32, i32, i32);
    void LoadGruntTuningConstants(i32);

    i32 m_04;          // +0x04
    i32 m_08;          // +0x08
    CGameObject* m_0c; // +0x0c
    CGameObject*
        m_object; // +0x10  bound game object (primary handle: position/logic/draw; == m_38)
    CGameObjAux* m_objAux; // +0x14  the object's +0x7c aux sub-object (obj->m_7c)
    CUserBaseLink m_link;  // +0x18..+0x27 (ctor 0x16d710, can throw)
    i32 m_28;              // +0x28
    i32 m_2c;              // +0x2c
    void*
        m_prevAnimSetNode; // +0x30  saved prior aux lookup node (m_objAux->m_1c) before installing "A"
    CGameObject* m_34;     // +0x34
    CGameObject* m_38;     // +0x38
    CGameObjAux* m_3c;     // +0x3c
};
SIZE(
    CUserLogic,
    0x40
); // this header's fat-view size (tail absorbed); see the NOTE re the true 0x30
// NOTE - the ONE TRUE CUserLogic size is 0x30, NOT 0x40. Evidence (retail):
//   * The base ctor CUserLogic(CGameObject*) @0x58cd0 initializes fields only
//     through m_2c (the highest write is `mov [esi+0x2c],2`), then returns. It
//     never writes 0x30..0x3c. -> the base object ends at 0x30.
//   * CGrunt : CUserLogic (<Gruntz/Grunt.h>) places its OWN, byte-exact-matched
//     members at 0x30 (m_prevAnimSetNode), 0x38 (anim player), 0x40 (activeAnim)
//     - proving the base it inherits is exactly 0x30.
//   * The ??_7CUserLogic@@6B@ vftable is 16 slots (0x40 bytes); ??_7CUserBase is
//     3 slots (config/vtable_names.csv).
// m_prevAnimSetNode/m_34/m_38/m_3c above are NOT base fields: they are the tile-logic leaves'
// common tail (each LEAF's 1-arg ctor sets m_34/m_38/m_3c itself; the standalone
// base ctor 0x58cd0 does not). This header folds them into the base purely so the
// shared inline ctor can spell them once; the layout is byte-identical to a true
// 0x30 base + a 0x10 leaf tail. A thin leaf new's 0x54 = 0x30 base + 0x24 leaf
// (0x10 tail + 0x14 own) - either boundary label gives the same offsets/size.
// UNIFYING the two class names into one 0x30 def would need a CTileLogic
// intermediate reparenting ~50 leaves + ~30 tuned 1-arg ctors (inline-depth /
// macro-controlled tail, UserLogicCtorEmit.cpp) - a byte-neutral change with real
// de-tuning risk and NO correctness gain (both views already encode the true
// 0x30). Kept as a documented byte-compatible dual-model; the two never coexist
// in a TU (the CGrunt-HUD sprites that bridge them include only Grunt.h).

// Shared 1-arg init the leaves fold in. Inline so MSVC inlines it; stores the
// CUserLogic vptr, then the full init. Defined here (not the .cpp) because only
// an inline base ctor is folded into the derived ctors.
inline CUserLogic::CUserLogic(CGameObject* obj) {
    m_0c = obj;
    m_object = obj;
    m_objAux = obj->m_7c;
    {
        EngStr tmp(g_emptyString, 0);
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
#ifndef USERLOGIC_STANDALONE_CTOR
    // The REAL out-of-line base ctor (retail 0x58cd0) does NOT set these three -
    // each game-object LEAF sets m_34/m_38/m_3c itself, right after the folded
    // base init (e.g. CTeleporter @0x410f9). They live in the base inline here so
    // the folded-into-a-leaf copies reproduce the leaf's bytes without every leaf
    // ctor having to spell them out. src/Gruntz/UserLogicCtorEmit.cpp defines
    // USERLOGIC_STANDALONE_CTOR to drop them for the byte-exact standalone COMDAT;
    // every other TU (all leaves) compiles this branch unchanged -> matching-neutral.
    m_34 = obj;
    m_38 = obj;
    m_3c = obj->m_7c;
#endif
}

inline void CUserLogic::RegisterLogicTypesOnce() {
    if (!g_logicTypesRegistered) {
        BuildLogicTypeTable((CLogicTypeBuilder*)m_0c);
        g_logicTypesRegistered = 1;
    }
}

// ---------------------------------------------------------------------------
// CTileTrigger : CUserLogic (vftable 0x5e7f14). Adds no data members. Three
// trace-discovered leaf classes derive from it (CTileSecretTrigger /
// CGiantRock / CCoveredPowerup, each in its own src/Stub/ TU) - shared here so
// they get a single class definition. Ctors/dtor are out-of-line in
// src/Gruntz/UserLogic.cpp (no-arg 0x011160, 1-arg 0x10e220, dtor 0x011290).
// ---------------------------------------------------------------------------
SIZE_UNKNOWN(CTileTrigger);
class CTileTrigger : public CUserLogic {
public:
    CTileTrigger();                 // 0x011160 (no-arg)
    CTileTrigger(CGameObject* obj); // 0x10e220 (1-arg)
    static void InitActReg();       // 0x10e420
    static void RegisterActs();     // 0x10e600
    i32 AdvanceAnim();              // 0x10ee00
    // Inline & trivial so it folds into the three leaf dtors (0x11540/0x11600/
    // 0x116c0) rather than being called. MSVC still emits one out-of-line COMDAT
    // copy (called by CTileTrigger's scalar-deleting dtor); it lands at 0x011290
    // and is labeled via the @rva-symbol pin in src/Gruntz/UserLogic.cpp (an
    // inline-defined fn can't hang an RVA() without also tagging the synthesized
    // ??_G - see the duplicate-RVA guard).
    virtual ~CTileTrigger() OVERRIDE {}
};

#endif // GRUNTZ_USERLOGIC_H

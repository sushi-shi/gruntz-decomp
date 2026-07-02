// UserLogic.h - Gruntz game-object base hierarchy (C:\Proj\Gruntz).
//
// Reconstruction sufficient to byte-match the game-object constructors. Field
// names are placeholders; the OFFSETS and the inheritance chain are the
// load-bearing facts the matches prove.
//
// Hierarchy (recovered from RTTI ClassHierarchyDescriptors in GRUNTZ.EXE):
//     CUserBase                       vftable 0x5e70b4  (3 virtuals)
//       +-- CUserLogic : CUserBase    vftable 0x5e705c  (12 virtuals)
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
    i32 m_10; // +0x10
    i32 m_14; // +0x14
    i32 m_18; // +0x18
    i32 m_1c; // +0x1c
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
struct CGameObjLayer {
    char m_pad00[0x10];
    i32 m_10; // +0x10  z-clamp bound (eyecandy)
    i32 m_14; // +0x14  z-clamp bound (eyecandy)
    i32 m_18; // +0x18  layer base X (path/dropper screen-rect origin)
    i32 m_1c; // +0x1c  layer base Y / base offset
};

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
    char m_pad00[0x04];
    i32 m_04; // +0x04
    i32 m_08; // +0x08
    i32 m_0c; // +0x0c
    i32 m_10; // +0x10  (worker getters pass src->m_10 through slot 9)
    char m_pad14[0x38 - 0x14];
    i32 m_38; // +0x38
    char m_pad3c[0x40 - 0x3c];
    i32 m_40; // +0x40
    char m_pad44[0x4c - 0x44];
    i32 m_4c; // +0x4c
    i32 m_50; // +0x50  draw-fill command type (0xb = decay fill-bar)
    i32 m_54; // +0x54  fill fraction (0..256)
    i32 m_58; // +0x58  dirty/active flag
    i32 m_5c; // +0x5c  screen x
    i32 m_60; // +0x60  screen y
    i32 m_64; // +0x64  captured config triple (checkpoint state slots 12..14)
    i32 m_68; // +0x68
    i32 m_6c; // +0x6c
    char m_pad70[0x74 - 0x70];
    i32 m_74; // +0x74
    char m_pad78[0x7c - 0x78];
    CGameObjAux* m_7c; // +0x7c
    CAnimWorker* m_80; // +0x80  lazily-built worker (EnsureWorker80)
    char m_pad84[0x88 - 0x84];
    CAnimWorker* m_88; // +0x88  lazily-built worker (EnsureWorker88)
    char m_pad8c[0x90 - 0x8c];
    CAnimWorker* m_90; // +0x90  lazily-built worker (EnsureWorker90)
    char m_pad94[0x114 - 0x94];
    i32 m_114; // +0x114  (teleporter spawn: source-tile coordinate mirror)
    i32 m_118; // +0x118  CSpotLight ctor: pi/0 mode gate
    i32 m_11c; // +0x11c  CSpotLight ctor: settings-table index
    i32 m_120; // +0x120  CSpotLight ctor: SpotLightTime override
    i32 m_124; // +0x124  sprite-selector row key (leaf ctors pass it to ApplyLookupSprite)
    i32 m_128; // +0x128  visibility/place mode (1 or 2; the on-screen gate discriminator)
    i32 m_12c; // +0x12c  CSpotLight ctor: m_58 scale gate
    char m_pad130[0x134 - 0x130];
    i32 m_134; // +0x134  per-side tile-span config (checkpoint/voice/exit/slime bounds)
    i32 m_138; // +0x138
    i32 m_13c; // +0x13c
    i32 m_140; // +0x140
    i32 m_144; // +0x144  (CSpotLight ctor zeros 0x144/0x148/0x14c/0x150)
    i32 m_148; // +0x148
    i32 m_14c; // +0x14c
    i32 m_150; // +0x150
    i32 m_154; // +0x154  captured config block (checkpoint state slots 8..11)
    i32 m_158; // +0x158
    i32 m_15c; // +0x15c
    i32 m_160; // +0x160
    i32 m_164; // +0x164
    i32 m_168; // +0x168
    char m_pad16c[0x188 - 0x16c];
    i32 m_188; // +0x188  object id (warlord battle-event id / game-object archive-cue id)
    char m_pad18c[0x198 - 0x18c];
    CGameObjLayer* m_198; // +0x198
    char m_pad19c[0x1b4 - 0x19c];
    i32 m_1b4; // +0x1b4
    char m_pad1b8[0x1c0 - 0x1b8];
    i32 m_1c0; // +0x1c0  the +0x1a0 anim sub-mgr's idle flag  (sink+0x20)
    char m_pad1c4[0x1c8 - 0x1c4];
    i32 m_1c8; // +0x1c8  the +0x1a0 anim sub-mgr's active flag (sink+0x28)
};

// The +0x7c sub-object: its +0x08 flags, +0x1c bute-node and +0x130 timer are
// touched by the eyecandy/sparkle ctors.
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
// CUserLogic : CUserBase (vftable 0x5e705c, 16 slots; only the first 12 modeled
// here - the tile-logic leaves stamp the vftable by address, so the extra slots
// need not be declared). Owns the shared data layout + the +0x18 link sub-object.
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

    // The shared serialize-chain helper (0x16e7f0, __thiscall ret 0x10). Run on
    // `this` by the leaf Serialize overrides. External/no-body (reloc-masked;
    // pinned in src/Stub/Discovered.cpp).
    i32 SerializeChain(i32 a, i32 b, i32 c, i32 d); // 0x16e7f0

    // Copies the bound object's screen position into the out point (m_10->m_5c =
    // x, m_10->m_60 = y). 0x29a50, __thiscall ret 4.
    struct ScreenPoint {
        i32 x;
        i32 y;
    };
    void GetScreenPos(ScreenPoint* out); // 0x29a50

    // True when the bound object's current screen pos (m_10->m_5c/m_60) still
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

    i32 m_04;             // +0x04
    i32 m_08;             // +0x08
    CGameObject* m_0c;    // +0x0c
    CGameObject* m_10;    // +0x10
    CGameObjAux* m_14;    // +0x14
    CUserBaseLink m_link; // +0x18..+0x27 (ctor 0x16d710, can throw)
    i32 m_28;             // +0x28
    i32 m_2c;             // +0x2c
    void* m_30;           // +0x30
    CGameObject* m_34;    // +0x34
    CGameObject* m_38;    // +0x38
    CGameObjAux* m_3c;    // +0x3c
};
// NOTE - the ONE TRUE CUserLogic size is 0x30, NOT 0x40. Evidence (retail):
//   * The base ctor CUserLogic(CGameObject*) @0x58cd0 initializes fields only
//     through m_2c (the highest write is `mov [esi+0x2c],2`), then returns. It
//     never writes 0x30..0x3c. -> the base object ends at 0x30.
//   * CGrunt : CUserLogic (<Gruntz/Grunt.h>) places its OWN, byte-exact-matched
//     members at 0x30 (m_prevAnimSetNode), 0x38 (anim player), 0x40 (activeAnim)
//     - proving the base it inherits is exactly 0x30.
//   * The ??_7CUserLogic@@6B@ vftable is 16 slots (0x40 bytes); ??_7CUserBase is
//     3 slots (config/vtable_names.csv).
// m_30/m_34/m_38/m_3c above are NOT base fields: they are the tile-logic leaves'
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
    m_10 = obj;
    m_14 = obj->m_7c;
    {
        EngStr tmp(g_emptyString, 0);
        m_link.m_str = tmp;
    }
    RegisterLogicTypesOnce();
    m_10->AddLogicHit("LogicHit");
    m_10->AddLogicAttack("LogicAttack");
    m_10->AddLogicBump("LogicBump");
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

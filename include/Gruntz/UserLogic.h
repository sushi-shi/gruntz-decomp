#ifndef GRUNTZ_USERLOGIC_H
#define GRUNTZ_USERLOGIC_H

#include <rva.h>
#include <Gruntz/LogicTypeId.h>

#include <Gruntz/UserBaseLink.h>

#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/WwdGridIter.h> // WwdRegion - the embedded +0x9c region node

struct CGameObject;      // fwd (the worker's collide callback takes the object)
struct LeafCue;          // the +0x19c resolved leaf-scan cue (<Gruntz/LeafCue.h>)
class CDDrawSurfacePair; // slots 11-14 params (<DDrawMgr/DDrawSurfacePair.h>)
class CUserLogic;        // fwd (AnimWorkerObj::m_logic is the object's bound logic leaf)
struct Coord;

#include <DDrawMgr/AnimWorkerObj.h>

class CDDrawWorker; // the +0x194 cached sprite IS CDDrawWorker

class CImage;

#include <Wwd/WwdGameObjectFamily.h>

#include <Bute/ButeMgr.h>
extern CButeMgr g_buteMgr;

extern i32 g_logicTypesRegistered;

class CFileMemBase;

class CUserBase {
public:
    CUserBase() {}
    virtual ~CUserBase() {} // inline: folds into leaf dtors (final base vptr store)
    virtual i32 SerializeMove(CFileMemBase* ar, i32 mode, i32 a3, i32 a4); // slot 1
    virtual LogicTypeId
    GetTypeTag(); // slot 2 (per-class logic-type id)                                           // slot 2
};
SIZE_UNKNOWN(); // (was covered by the BoundaryMisc placeholder before its rename)

class CUserLogic : public CUserBase {
public:
    CUserLogic() {}
    CUserLogic(CGameObject* obj);
    virtual ~CUserLogic() OVERRIDE {} // inline: folds into leaf dtors (link teardown + vptr stores)
    virtual i32 SerializeMove(CFileMemBase*, i32, i32, i32) OVERRIDE; // slot 1
    virtual LogicTypeId GetTypeTag() OVERRIDE;                        // slot 2
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
    // UserLogic.cpp - fires the two m_04/m_08 deferred callbacks, resets
    // m_28), CMovingLogic's 0x13c70 override (QAEXH, 100% EXACT; + MovingSlot16
    // tail) and CGrunt's 0x5ecd0 (`ret 4`, NO exit materializes eax -> void).
    // Was the no-arg/i32 `UserLogicVfunc3` placeholder + a non-virtual
    // FinalizeStep twin of the same 0x8b90 body.
    virtual void FinalizeStep(i32 unused);
    // slots 6/11 (Activate/Vfunc9): VOID - the base defaults (0x88d0/0x8970) are
    // bare `c3` (cl5 C2561 forbids a return-less i32 body), and the CGrunt
    // override tails (0x5caa0/0x48360) materialize NO eax either. Every dispatch
    // site discards the slot.
    virtual void Activate();
    virtual i32 UserLogicVfunc5();
    virtual i32 UserLogicVfunc6();
    // slot 9: a return-0 default; the one known override is CGrunt's per-frame
    // attack-fire step (@0x61cb0), which names the slot.
    virtual i32 StepAttackFire();
    // slots 10/12/13/14/15: inert `ret` defaults (retail 0x8950/8990/89b0/89d0/89f0 are
    // bare `ret`), so VOID - no override exists.
    virtual void UserLogicVfunc8();
    virtual void UserLogicVfunc9();
    virtual void UserLogicVfuncA();
    virtual void UserLogicVfuncB();
    virtual void UserLogicVfuncC(); // slot 14 (retail impl 0x001730)
    virtual void UserLogicVfuncD(); // slot 15 (retail impl 0x003607)

    // (The shared serialize chain at 0x16e7f0 is the `SerializeMove` slot-1 virtual
    // declared above - it is THIS class's own override, defined in MovingLogic.cpp.
    // The `SerializeChain` non-virtual twin that used to be declared here, and the
    // fake CMovingLogicBase the leaves cast to, were both dissolved onto it.)

    // (The former SerialRef34() +0x34 facet hop is gone: the "+0x34 CSerialObjRef"
    // IS the CWapX second base below - leaves call the inherited Chain directly.)

    // Copies the bound object's screen position into the out point (m_object->m_5c
    // = x, m_object->m_60 = y). 0x29a50, __thiscall ret 4. The out-point is the
    // shared {m_x,m_y} Coord (CGrunt reaches this same method inherited).

    void GetScreenPos(Coord* out); // 0x29a50 (out-of-line in BattlezMapConfig.cpp)

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
    // (FinalizeStep - 0x8b90, body in UserLogic.cpp - is now the slot-5
    // virtual above; retail's slot holds its ILT thunk 0x3913, which
    // reloc_fidelity thunk-resolves onto the body.)
    i32 Place(i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32, i32); // 0x4c1c4
    void Arm(const char* lighting, const char* cursor, i32 kind, i32 one); // 0x4e517

    i32 m_deferredCallback; // +0x04
    i32 m_gatedCallback;    // +0x08
    CGameObject* m_0c;      // +0x0c
    // +0x10  bound game object (== m_38): the created A-kind sprite (every binding
    // site hands a CreateSprite/ReadPlaneObjects product; leaves read its m_1a0
    // cache). The ex-"m_10" arm was the same type/role - one name now.
    CWwdGameObjectA* m_object;
    // +0x14  aux sub-object (obj->m_7c, the bound object's AnimWorkerObj; the ex
    // "CAnimLookupNode* m_14" arm was a pad-view of its +0x1c anim-set handle).
    AnimWorkerObj* m_objAux;
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
SIZE(0x34); // base size 0x34 (see the NOTE). The tile-logic leaves'

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
    m_deferredCallback = 0;
    m_gatedCallback = 0;
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

#include <Gruntz/SerialArchive.h> // CFileMemBase == CFileMemBase (typedef; NEVER fwd-declare it)
class CWapX {
public:
    CWapX() {}
    CWapX(CGameObject* obj) {
        m_34 = obj;
        m_38 = static_cast<CWwdGameObjectA*>(obj); // the bound obj IS the created A-kind sprite
        m_3c = obj->m_7c;
    }
    ~CWapX() {} // EMPTY INLINE (see the 0x8be0 evidence above); out-of-line COMDAT
                // pinned by RVA_COMPGEN in ActionArea.cpp
    // Serialize the referenced object by its registry key name (read/write per mode).
    i32 Chain(CFileMemBase* arc, i32 mode, i32 unused, CGameObject* obj); // 0x8c00

    // Field names keep the tile-leaf +0x34 spellings (this class is reached at THREE
    // displacements - see the note above - so no one spelling can be offset-accurate).
    // The union of what the three worlds independently recovered about each slot:
    CGameObject* m_34;     // +0x00 (leaf +0x34, moving +0x150)  the referenced object
    CWwdGameObjectA* m_38; // +0x04 (leaf +0x38, moving +0x154)  == m_34: the bound/render
                           //   CREATED SPRITE (the A kind - every binding site hands a
                           //   CreateSprite product; leaves read its m_1a0 cursor/frame cache)
                           //   object (leaves read m_38->m_flags; the projectile world
                           //   called this m_sprite, "primary sprite/render object")
    AnimWorkerObj* m_3c;   // +0x08 (leaf +0x3c, moving +0x158)  obj->m_7c - the bound
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
SIZE(0x20);

class CTileTrigger : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeMove(CFileMemBase*, i32, i32, i32) OVERRIDE; // slot 1
    virtual LogicTypeId GetTypeTag() OVERRIDE;                        // slot 2
public:
    CTileTrigger();                 // 0x011160 (no-arg)
    CTileTrigger(CGameObject* obj); // 0x10e220 (1-arg)
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
    // FOLD it; the out-of-line COMDAT (called by ??_G) is labeled via the RVA_COMPGEN
    // pin in src/Gruntz/UserLogic.cpp.
};
SIZE(0x54);

// --- C-linkage carriers for the TU's extern-C definitions (the defs
// inherit the linkage from these decls; the .cpp wrappers are gone) ---
extern "C" i32 LogicHitFactory(CGameObject* obj);
extern "C" i32 LogicAttackFactory(CGameObject* obj);
extern "C" i32 LogicBumpFactory(CGameObject* obj);

#endif // GRUNTZ_USERLOGIC_H

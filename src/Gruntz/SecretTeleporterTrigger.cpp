// SecretTeleporterTrigger.cpp - the secret-teleporter + secret-level trigger
// tile-logic game-objects (C:\Proj\Gruntz), both CUserLogic leaves.
//
// ONE original TU (wave3-J): the 0x041e90-0x042cd3 interval's text is a T-L-T
// sandwich (teleporter 0x41e90..0x422b0 | level 0x424b0..0x42ac0 | teleporter
// SpawnTeleporter @0x42b80) - impossible for two first-link objs; the two init
// frags (i40 @0x420b0 teleporter / i41 @0x426c0 level) are one adjacent run.
// The ex SecretLevelTrigger.cpp folds in here in ascending retail-RVA order.
//
// Both classes share the activation-registry archetype: a per-class
// coordinate registry (g_actColl @0x644688 / g_secretActReg @0x644598) + the
// SHARED activation-name registry (<Gruntz/ActNameRegistry.h>, @0x6bf650; the
#include <Bute/ButeTree.h>          // CVariantSlot::Set (0x16d850)
#include <Gruntz/GameRegMfcPtr.h> // g_gameReg at its REAL type (CGruntzMgr)
#include <Gruntz/GruntzMgr.h>
#include <Wap32/ZVec.h>             // _zvec::GrowTo (Find 0x16da80)
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/GameLevel.h> // canonical CGameLevel/CLevelPlane (m_world->m_level visible rect)      // CTriggerMgr::HitTestCell (0x75af0) / CellDispatch (0x6bcb0)
#include <Gruntz/GruntSpawnConfig.h>  // CGruntSpawnConfig::SpawnVoiceDriver (the cue)
#include <Gruntz/Trigger.h>           // CTrigger (point-probe result, its m_10 HUD sprite)
#include <Gruntz/GameRegistry.h>      // the canonical *0x24556c singleton (m_world/m_cmdGrid/
                                      // m_cueSink/m_scoreHud typed; CDDrawSurfaceMgr)
#include <Gruntz/BattlezData.h>       // CBattlezData (g_gameReg->m_scoreHud; +0x3c armed counter)
#include <DDrawMgr/DDrawChildGroup.h> // the ONE CDDrawChildGroup (CreateSprite @0x1597b0)
#include <Gruntz/ActReg.h>
#include <Gruntz/ActNameRegistry.h>   // the SHARED activation-name registry (g_buteTree/
                                      // g_typeCounter/s_codeA/g_typeColl*/ActNameLookup)
#include <Gruntz/ActReg.h>            // the shared CActReg coordinate-registry archetype
#include <Gruntz/SecretTeleporterTrigger.h> // the canonical class
#include <Gruntz/SecretLevelTrigger.h>      // canonical CSecretLevelTrigger : CUserLogic
#include <Gruntz/SerialArchive.h> // CSerialArchive (the inherited CWapX::Chain arg; ex SerialObjRef.h)
#include <Globals.h>                        // g_actLo/Hi/Base/Stride/Scratch/Cur/Coll2 (fast range)
#include <rva.h>
#include <Gruntz/SerialArchive.h> // the serialize stream (== the real CFileMemBase)

// The global game registry both trigger families poll (the canonical
// CGameRegistry view of *0x64556c; the ex-teleporter-side WwdGameReg extern is
// unified onto it - same offsets: m_world +0x30, m_cueSink +0x60, m_cmdGrid
// +0x68, m_scoreHud +0x7c).

// The +0x7c aux facet g_gameReg->m_scoreHud IS the canonical CBattlezData
// (<Gruntz/BattlezData.h>): the teleporter ctor bumps its +0x3c "teleporter armed"
// counter directly on the singleton's typed member (the ex WwdGameRegAux view is
// main plane (g_gameReg->m_world->m_level->m_mainPlane): its near-origin/far-extent
// pair {m_originX,m_originY,m_extentX,m_extentY} IS the {left,top,right,bottom}

// ---------------------------------------------------------------------------
// The teleporter's per-coordinate activation registry FireActivation (0x042150)
// dispatches through. A coordinate maps to an Entry* either directly (when
// within the fast [g_actLo,g_actHi] range) via g_actBase + (coord-g_actLo)*
// g_actStride, or by a slow lookup in g_actColl (0x16da80, __thiscall ret 8),
// which on miss rebuilds the table and yields g_actCur. The entry's first dword
// is a PMF of the trigger class; a nonzero entry's handler is called __thiscall
// on `this`. g_actColl (0x644688) is the teleporter's own collection singleton.
DATA(0x00244688)
extern CActReg g_actColl; // the WHOLE 0x24-byte registry object (ex 8 exploded per-field scalars)

// The entry record (ActHandler/CActEntry, the PMF slot) is defined in
// <Gruntz/SecretTeleporterTrigger.h> after the complete class.

// The inlined coordinate->Entry* lookup FireActivation folds in twice (the
// registry-archetype ResolveEntry over g_actColl; per-field scalars reunified
// into the one CActReg object @0x644688).
static inline CActEntry* ActLookup(i32 coord) {
    return reinterpret_cast<CActEntry*>(g_actColl.ResolveEntry(coord));
}

// The SpawnTeleporter entry (SpawnHandler/CTelActEntry) is defined in
// <Gruntz/SecretTeleporterTrigger.h>; the secret-level Tick entry
// (SecretActHandler/CSecretActEntry) in <Gruntz/SecretLevelTrigger.h> - both after
// the complete class so the PMF stays 4 bytes.

// The secret-level trigger's activation-coordinate registry singleton (@0x644598):
// the fixed [2000,2010] range built by the shared registry ctor (0x408710). It is
// the shared <Gruntz/ActReg.h> CActReg archetype directly (the ex empty-derived
DATA(0x00244598)
extern CActReg g_secretActReg; // 0x644598 (owner TU: real definition; interior
                        // fields 0x24459c..0x2445b8 are this object's members)

// The probed trigger object is the shared <Gruntz/Trigger.h> class: its
// +0x170/+0x198 are the level/layer ids the bound sprite's +0x11c/+0x120 must
// match for the secret-level trigger to fire.
SIZE_UNKNOWN(CTrigger);

// ===========================================================================
// Definitions in ascending-RVA order.
// ===========================================================================

// --- CSecretTeleporterTrigger::Serialize (0x010a10), vtable slot 1 ---
// Chains the shared serialize helper on `this`, and (only on success) the +0x34
// serializable sub-object's chain; normalizes the result to a strict bool.
RVA(0x00010a10, 0x47)
i32 CSecretTeleporterTrigger::SerializeMove(CGruntArchive* a, i32 b, i32 c, i32 d) {
    if (!CUserLogic::SerializeMove(a, b, c, d)) {
        return 0;
    }
    return Chain(a, b, c, reinterpret_cast<CGameObject*>(d)) != 0;
}

// --- CSecretTeleporterTrigger::~CSecretTeleporterTrigger (0x010ab0) ---
// The leaf adds no members, so its dtor folds the bare CUserLogic teardown:
// store the CUserLogic vptr, inline-destruct the +0x18 link (the embedded
// ~EngStr call), store the CUserBase vptr. The destructible link forces the /GX
// EH frame. The fold requires ~CUserBase/~CUserLogic/~CUserBaseLink to be inline
// (see UserLogic.h); the empty leaf body below is enough for cl to emit it.
// IMPLICIT dtor (retail is COMPILER-GENERATED - eh-dtor-vptr-restamp CAUSE B):
// a user-declared `~CSecretTeleporterTrigger() {}` emits the leaf-vptr restamp, and the CWapX
// base EH state blocks the dead-store elision that used to hide it. The ??_G
// in the vtable-emitting TU forces the implicit ??1 COMDAT; pinned by name.
// @rva-symbol: ??1CSecretTeleporterTrigger@@UAE@XZ 0x00010ab0 0x44

// --- CSecretLevelTrigger no-arg ctor (0x010b20) --- the deserialize-path ctor:
// base prologue + link + leaf vptr stamp (the empty body is enough for cl).
RVA(0x00010b20, 0x4b)
CSecretLevelTrigger::CSecretLevelTrigger() {}

// CSecretLevelTrigger::Serialize @0x010bb0 - the vtable slot-1 override: chain the
// shared CUserLogic serialize helper on `this`, and (only on success) the +0x34
// serializable sub-object's chain; normalize the second chain's success to a strict
// bool. The byte-identical chain-Serialize archetype (differs only in the two call
// displacements) - the direct sibling of CSecretTeleporterTrigger::Serialize (0x010a10).
RVA(0x00010bb0, 0x47)
i32 CSecretLevelTrigger::SerializeMove(CGruntArchive* ar, i32 tag, i32 c, i32 d) {
    if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
        return 0;
    }
    return Chain(ar, tag, c, reinterpret_cast<CGameObject*>(d)) != 0;
}

// CSecretLevelTrigger::~CSecretLevelTrigger @0x010c50 - the leaf adds no
// destructible members beyond CUserLogic, so its dtor folds the bare CUserLogic
// teardown: store the CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link
// (the embedded ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The
// destructible link forces the /GX EH frame. Byte-identical in shape to
// ~CSecretTeleporterTrigger @0x010ab0; the empty body is enough for cl.
// IMPLICIT dtor (retail is COMPILER-GENERATED - eh-dtor-vptr-restamp CAUSE B):
// a user-declared `~CSecretLevelTrigger() {}` emits the leaf-vptr restamp, and the CWapX
// base EH state blocks the dead-store elision that used to hide it. The ??_G
// in the vtable-emitting TU forces the implicit ??1 COMDAT; pinned by name.
// @rva-symbol: ??1CSecretLevelTrigger@@UAE@XZ 0x00010c50 0x44

// --- CSecretTeleporterTrigger (0x041e90), vptr 0x5e7564 ---
RVA(0x00041e90, 0x1ac)
CSecretTeleporterTrigger::CSecretTeleporterTrigger(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
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
        g_gameReg->m_scoreHud->m_3c++;
    }
}

// --- CSecretTeleporterTrigger::InitActReg (0x0420d0) ---
// Construct the class's activation-coordinate registry singleton (g_actColl
// @0x644688) over the fixed range [2000, 2010] via the shared registry ctor
// (0x408710). Free init thunk; reloc-masked.
RVA(0x000420d0, 0x15)
void CSecretTeleporterTrigger::InitActReg() {
    g_actColl.Construct(2000, 2010);
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
    i32 id = reinterpret_cast<i32>(g_buteTree.Find("A"));
    if (id == 0) {
        id = g_typeCounter;
        g_buteTree.Insert("A", reinterpret_cast<void*>(id));
        char* slot = ActNameLookup(id);
        i32 n = g_typeColl.m_grown;
        void** list = reinterpret_cast<void**>(g_typeColl.m_alloc);
        while (n-- != 0) {
            if (list != 0) {
                (reinterpret_cast<CString*>(list))->CString::~CString();
            }
            list++;
        }
        (reinterpret_cast<CString*>(slot))->operator=("A");
        g_typeCounter++;
    }
    (reinterpret_cast<CTelActEntry*>(ActLookup(id)))->m_fn = static_cast<i32 (CUserLogic::*)()>(&CSecretTeleporterTrigger::SpawnTeleporter);
}

// --- CSecretLevelTrigger 1-arg ctor (0x0424b0), vptr 0x5e8804 --- folds the inline
// CUserLogic(obj) base + the tile-snap/anim tail (gated on the play sub-mode).
RVA(0x000424b0, 0x1a0)
CSecretLevelTrigger::CSecretLevelTrigger(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
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

// CSecretLevelTrigger::InitActReg @0x0426e0 - construct the class's activation-
// coordinate registry singleton (g_secretActReg @0x644598) over the fixed range
// [2000, 2010] via the shared registry ctor (FUN_00408710). Free init thunk;
// reloc-masked.
RVA(0x000426e0, 0x15)
void CSecretLevelTrigger::InitActReg() {
    g_secretActReg.Construct(2000, 2010);
}

// CSecretLevelTrigger::FireActivation @0x042760 - look the activation coordinate up
// in the class registry (g_secretActReg); if the resolved entry carries a registered
// handler PMF, resolve it again and dispatch it __thiscall on `this`. Same archetype
// as CParticlez::FireActivation (double ResolveEntry + PMF dispatch).
RVA(0x00042760, 0x102)
void CSecretLevelTrigger::FireActivation(i32 coord) {
    CSecretActEntry* e = reinterpret_cast<CSecretActEntry*>(g_secretActReg.ResolveEntry(coord));
    if (e->m_fn != 0) {
        CSecretActEntry* e2 = reinterpret_cast<CSecretActEntry*>(g_secretActReg.ResolveEntry(coord));
        (this->*(e2->m_fn))();
    }
}

// CSecretLevelTrigger::RegisterActs @0x0428c0 - bind the class's per-frame handler
// (Tick @0x042ac0) to the activation key "A" (the SAME activation-name-intern
// archetype as CGruntHealthSprite::RegisterActs; see that TU for the full notes).
//
// @early-stop
// register-pinning wall (docs/patterns/zero-register-pinning.md +
// test-old-value-decrement-loop-while-postdec.md, topic:wall topic:regalloc): logic
// byte-faithful (every call/immediate/branch/offset + the `mov [entry],offset Tick`
// handler store match retail); residual is the slot-vs-id callee-saved register
// choice cascading into the free-loop count materialization. Deferred.
RVA(0x000428c0, 0x18d)
void CSecretLevelTrigger::RegisterActs() {
    i32 id = reinterpret_cast<i32>(g_buteTree.Find("A"));
    if (id == 0) {
        id = g_typeCounter;
        g_buteTree.Insert("A", reinterpret_cast<void*>(id));
        char* slot = ActNameLookup(id);
        i32 n = g_typeColl.m_grown;
        void** list = reinterpret_cast<void**>(g_typeColl.m_alloc);
        while (n-- != 0) {
            if (list != 0) {
                (reinterpret_cast<CString*>(list))->CString::~CString();
            }
            list++;
        }
        (reinterpret_cast<CString*>(slot))->operator=("A");
        g_typeCounter++;
    }
    (reinterpret_cast<CSecretActEntry*>(g_secretActReg.ResolveEntry(id)))->m_fn = static_cast<i32 (CUserLogic::*)()>(&CSecretLevelTrigger::Tick);
}

// CSecretLevelTrigger::Tick @0x042ac0 - probe the trigger's screen position; if a
// trigger object is under it whose level/layer ids match the bound sprite's
// (or are unset), post a scroll to bring it on screen and mark the window stalled
// this frame. Always returns 0.
RVA(0x00042ac0, 0x90)
i32 CSecretLevelTrigger::Tick() {
    i32 outA, outB;
    CGameObject* spr = m_object;
    CTrigger* hit =
        reinterpret_cast<CTrigger*>(g_gameReg->m_cmdGrid->HitTestCell(spr->m_screenX, spr->m_screenY, &outB, &outA, 1));
    if (hit) {
        spr = m_object;
        i32 ok = 1;
        i32 lvl = spr->m_11c;
        i32 lyr = spr->m_120;
        // (m_11c/m_120 = required level/layer ids on the bound CGameObject)
        if (lvl != 0 && hit->m_170 != lvl) {
            ok = 0;
        }
        if (lyr != 0 && hit->m_198 != lyr) {
            ok = 0;
        }
        if (ok) {
            g_gameReg->m_cmdGrid->CellDispatch(outB, outA, 0xc, -1);
        }
        m_38->m_flags |= 0x10000;
    }
    return 0;
}

// --- CSecretTeleporterTrigger::SpawnTeleporter (0x042b80) ---
// The registered point-activation callback: probe the trigger's screen point for
// a hit grunt; if hit, spawn the "Teleporter" HUD sprite at the (tile<<5)+0x10
// position, clone the trigger's teleport-link/tile fields into it, and (when the
// hit grunt is on-screen) fire the 6-arg cue. Always closes by marking the
// trigger sub-object hidden (m_38->m_08 |= 0x10000). The sprite factory is the
// canonical g_gameReg->m_world->m_childGroup CDDrawChildGroup (the ex CTeleResHolder/
RVA(0x00042b80, 0x153)
i32 CSecretTeleporterTrigger::SpawnTeleporter() {
    i32 loc0, loc4;
    CGameObject* o = m_object;
    CTrigger* hit =
        reinterpret_cast<CTrigger*>(g_gameReg->m_cmdGrid->HitTestCell(o->m_screenX, o->m_screenY, &loc0, &loc4, 1));
    if (hit) {
        o = m_object;
        CDDrawChildGroup* fac = g_gameReg->m_world->m_childGroup;
        CGameObject* spr = fac->CreateSprite(
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
            CGruntzMgr* g = g_gameReg;
            i32 ey = eo->m_screenY;
            i32 ex = eo->m_screenX;
            CLevelPlane* rc = g->m_world->m_level->m_mainPlane;
            if (ex < rc->m_extentX && ex >= rc->m_originX && ey < rc->m_extentY
                && ey >= rc->m_originY) {
                g->m_cueSink
                    ->SpawnVoiceDriver(reinterpret_cast<i32>(hit), 0x3fc, -1, 0, -1, -1);
            }
        }
        m_38->m_flags |= 0x10000;
    }
    return 0;
}

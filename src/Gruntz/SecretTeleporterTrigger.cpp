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
// ex TU-local duplicate decl block is dissolved onto the shared header).
#include <Bute/ButeTree.h>          // CVariantSlot::Set (0x16d850)
#include <Gruntz/MovingLogicBase.h> // CMovingLogicBase::Serialize (0x16e7f0) - shared serialize chain
#include <Wap32/ZVec.h>             // _zvec::GrowTo (Find 0x16da80)
#include <Wap32/ZDArrayDerived.h>   // CZDArrayDerived::Construct (0x408710)
#include <Gruntz/TriggerMgr.h>      // CTriggerMgr::HitTestCell (0x75af0) / CellDispatch (0x6bcb0)
#include <Gruntz/GruntSpawnConfig.h> // CGruntSpawnConfig::SpawnVoiceDriver (the cue)
#include <Gruntz/Trigger.h>          // CTrigger (point-probe result, its m_10 HUD sprite)
#include <Gruntz/Viewport.h>         // CViewport (visible-rect base at +0x5c)
#include <Gruntz/GameRegistry.h>     // the canonical *0x24556c singleton (m_world/m_cmdGrid/
                                     // m_cueSink/m_scoreHud typed; CSpriteFactoryHolder)
#include <Gruntz/SpriteFactory.h>    // the ONE CSpriteFactory (CreateSprite @0x1597b0)
#include <Gruntz/ActColl.h>          // CActColl/GetRetAddr + g_projActCache/g_retAddrBreadcrumb
#include <Gruntz/ActNameRegistry.h>  // the SHARED activation-name registry (g_buteTree/
                                     // g_typeCounter/s_codeA/g_typeColl*/ActNameLookup)
#include <Gruntz/ActReg.h>           // the shared CActReg coordinate-registry archetype
#include <Gruntz/SecretTeleporterTrigger.h> // the canonical class
#include <Gruntz/SecretLevelTrigger.h>      // canonical CSecretLevelTrigger : CUserLogic
#include <Gruntz/SerialObjRef.h>            // CSerialObjRef::Chain (0x8c00) - the +0x34 round-trip
#include <Globals.h>                        // g_actLo/Hi/Base/Stride/Scratch/Cur/Coll2 (fast range)
#include <rva.h>

// The global game registry both trigger families poll (the canonical
// CGameRegistry view of *0x64556c; the ex-teleporter-side WwdGameReg extern is
// unified onto it - same offsets: m_world +0x30, m_cueSink +0x60, m_cmdGrid
// +0x68, m_scoreHud +0x7c).
DATA(0x0024556c)
extern "C" CGameRegistry* g_gameReg;

// The +0x7c aux facet (g_gameReg->m_scoreHud): the teleporter ctor bumps its
// +0x3c "teleporter armed" counter. @identity-TODO (the scoreHud/aux slot's
// concrete class is unrecovered).
SIZE_UNKNOWN(WwdGameRegAux);
struct WwdGameRegAux {
    char m_pad00[0x3c];
    i32 m_3c; // +0x3c
};

// The viewport rect base reached as g_gameReg->m_world->m_24->m_5c + 0x40; the
// on-screen test reads its left/top/right/bottom (m_0/m_4/m_8/m_c).
SIZE_UNKNOWN(CViewRect);
struct CViewRect {
    i32 m_left;   // +0x00
    i32 m_top;    // +0x04
    i32 m_right;  // +0x08
    i32 m_bottom; // +0x0c
};

// ---------------------------------------------------------------------------
// The teleporter's per-coordinate activation registry FireActivation (0x042150)
// dispatches through. A coordinate maps to an Entry* either directly (when
// within the fast [g_actLo,g_actHi] range) via g_actBase + (coord-g_actLo)*
// g_actStride, or by a slow lookup in g_actColl (0x16da80, __thiscall ret 8),
// which on miss rebuilds the table and yields g_actCur. The entry's first dword
// is a PMF of the trigger class; a nonzero entry's handler is called __thiscall
// on `this`. g_actColl (0x644688) is the teleporter's own collection singleton.
DATA(0x00244688)
extern CActColl g_actColl;

// The entry's first dword is a pointer-to-member-function of the trigger class
// (single inheritance -> a 4-byte code pointer); FireActivation invokes it on
// `this`, emitting `mov ecx,this; call [entry]`.
typedef void (CSecretTeleporterTrigger::*ActHandler)();
SIZE_UNKNOWN(CActEntry);
struct CActEntry {
    ActHandler m_fn; // [entry]
};

// The inlined coordinate->Entry* lookup FireActivation folds in twice.
// g_act* registry-field globals (referenced only from this TU): real
// definitions DATA-pinned here; the single extern is in <Globals.h>.
DATA(0x0024468c)
CVariantSlot* g_actColl2;
DATA(0x00244690)
i32 g_actLo;
DATA(0x00244694)
i32 g_actHi;
DATA(0x00244698)
char* g_actBase;
DATA(0x0024469c)
CActEntry* g_actCur;
DATA(0x002446a0)
i32 g_actStride;
DATA(0x002446a8)
i32 g_actScratch;

static inline CActEntry* ActLookup(i32 coord) {
    g_actScratch = 0;
    if (coord >= g_actLo && coord <= g_actHi) {
        return (CActEntry*)(g_actBase + (coord - g_actLo) * g_actStride);
    }
    if ((i32)((_zvec*)&g_actColl)->GrowTo(coord, 0)) {
        return (CActEntry*)(g_actBase + (coord - g_actLo) * g_actStride);
    }
    void* item = g_projActCache;
    g_retAddrBreadcrumb = GetRetAddr();
    g_actColl2->Set(&g_actColl, (i32)item, 0xc);
    return g_actCur;
}

// The activation-registry entry for SpawnTeleporter (an i32-returning handler PMF
// on the complete single-inheritance class).
typedef i32 (CSecretTeleporterTrigger::*SpawnHandler)();
SIZE_UNKNOWN(CTelActEntry);
struct CTelActEntry {
    SpawnHandler m_fn;
};

// The secret-level trigger's registry entry: its first dword receives the Tick
// handler PMF (a 4-byte code pointer on this complete single-inheritance class).
typedef i32 (CSecretLevelTrigger::*SecretActHandler)();
struct CSecretActEntry {
    SecretActHandler m_fn;
};
SIZE_UNKNOWN(CSecretActEntry);

// The secret-level trigger's activation-coordinate registry singleton
// (@0x644598): the fixed [2000,2010] range built by the shared registry ctor
// (0x408710). CSecretActReg is the shared <Gruntz/ActReg.h> CActReg archetype;
// it keeps its own placeholder name so the DATA-pinned global symbol is unchanged.
struct CSecretActReg : public CActReg {};
SIZE_UNKNOWN(CSecretActReg);
DATA(0x00244598)
CSecretActReg g_secretActReg; // 0x644598 (owner TU: real definition; interior
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
i32 CSecretTeleporterTrigger::Serialize(i32 a, i32 b, i32 c, i32 d) {
    if (!((CMovingLogicBase*)this)->Serialize((CSerialArchive*)(a), b, c, d)) {
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
i32 CSecretLevelTrigger::Serialize(i32 ar, i32 tag, i32 c, i32 d) {
    if (!((CMovingLogicBase*)this)->Serialize((CSerialArchive*)(ar), tag, c, d)) {
        return 0;
    }
    return ((CSerialObjRef*)&m_34)->Chain((CSerialArchive*)ar, tag, c, (CSerialObj*)d) != 0;
}

// CSecretLevelTrigger::~CSecretLevelTrigger @0x010c50 - the leaf adds no
// destructible members beyond CUserLogic, so its dtor folds the bare CUserLogic
// teardown: store the CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link
// (the embedded ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The
// destructible link forces the /GX EH frame. Byte-identical in shape to
// ~CSecretTeleporterTrigger @0x010ab0; the empty body is enough for cl.
RVA(0x00010c50, 0x44)
CSecretLevelTrigger::~CSecretLevelTrigger() {}

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
        ((WwdGameRegAux*)g_gameReg->m_scoreHud)->m_3c++;
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
    i32 id = (i32)g_buteTree.Find(s_codeA);
    if (id == 0) {
        id = g_typeCounter;
        g_buteTree.Insert(s_codeA, (void*)id);
        char* slot = ActNameLookup(id);
        i32 n = g_typeCount;
        void** list = (void**)g_typeNodes;
        while (n-- != 0) {
            if (list != 0) {
                ((CString*)list)->CString::~CString();
            }
            list++;
        }
        ((CString*)slot)->operator=(s_codeA);
        g_typeCounter++;
    }
    ((CTelActEntry*)ActLookup(id))->m_fn = &CSecretTeleporterTrigger::SpawnTeleporter;
}

// --- CSecretLevelTrigger 1-arg ctor (0x0424b0), vptr 0x5e8804 --- folds the inline
// CUserLogic(obj) base + the tile-snap/anim tail (gated on the play sub-mode).
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

// CSecretLevelTrigger::InitActReg @0x0426e0 - construct the class's activation-
// coordinate registry singleton (g_secretActReg @0x644598) over the fixed range
// [2000, 2010] via the shared registry ctor (FUN_00408710). Free init thunk;
// reloc-masked.
RVA(0x000426e0, 0x15)
void CSecretLevelTrigger::InitActReg() {
    ((CZDArrayDerived*)&g_secretActReg)->Construct(2000, 2010);
}

// CSecretLevelTrigger::FireActivation @0x042760 - look the activation coordinate up
// in the class registry (g_secretActReg); if the resolved entry carries a registered
// handler PMF, resolve it again and dispatch it __thiscall on `this`. Same archetype
// as CParticlez::FireActivation (double ResolveEntry + PMF dispatch).
RVA(0x00042760, 0x102)
void CSecretLevelTrigger::FireActivation(i32 coord) {
    CSecretActEntry* e = (CSecretActEntry*)g_secretActReg.ResolveEntry(coord);
    if (e->m_fn != 0) {
        CSecretActEntry* e2 = (CSecretActEntry*)g_secretActReg.ResolveEntry(coord);
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
    i32 id = (i32)g_buteTree.Find(s_codeA);
    if (id == 0) {
        id = g_typeCounter;
        g_buteTree.Insert(s_codeA, (void*)id);
        char* slot = ActNameLookup(id);
        i32 n = g_typeCount;
        void** list = (void**)g_typeNodes;
        while (n-- != 0) {
            if (list != 0) {
                ((CString*)list)->CString::~CString();
            }
            list++;
        }
        ((CString*)slot)->operator=(s_codeA);
        g_typeCounter++;
    }
    ((CSecretActEntry*)g_secretActReg.ResolveEntry(id))->m_fn = &CSecretLevelTrigger::Tick;
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
        (CTrigger*)
            g_gameReg->m_cmdGrid->HitTestCell(spr->m_screenX, spr->m_screenY, &outB, &outA, 1);
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
// canonical g_gameReg->m_world->m_8 CSpriteFactory (the ex CTeleResHolder/
// CTeleSpriteFactory views are dissolved onto the canonical holder).
RVA(0x00042b80, 0x153)
i32 CSecretTeleporterTrigger::SpawnTeleporter() {
    i32 loc0, loc4;
    CGameObject* o = m_object;
    CTrigger* hit =
        (CTrigger*)g_gameReg->m_cmdGrid->HitTestCell(o->m_screenX, o->m_screenY, &loc0, &loc4, 1);
    if (hit) {
        o = m_object;
        CSpriteFactory* fac = g_gameReg->m_world->m_8;
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
            CGameRegistry* g = g_gameReg;
            i32 ey = eo->m_screenY;
            i32 ex = eo->m_screenX;
            CViewRect* rc = (CViewRect*)(g->m_world->m_24->m_5c + 0x40);
            if (ex < rc->m_right && ex >= rc->m_left && ey < rc->m_bottom && ey >= rc->m_top) {
                ((CGruntSpawnConfig*)g->m_cueSink)
                    ->SpawnVoiceDriver((i32)hit, 0x3fc, -1, 0, -1, -1);
            }
        }
        m_38->m_flags |= 0x10000;
    }
    return 0;
}

// SecretTeleporterTrigger.cpp - the secret-teleporter trigger tile-logic
// game-object (C:\Proj\Gruntz), a CUserLogic leaf (vftable 0x5e7564).
//
// The whole CSecretTeleporterTrigger band, in ascending retail-RVA order: the
// slot-1 Serialize + /GX leaf dtor, the 1-arg ctor (vtable-emission anchor), the
// activation-coordinate registry's Init/Fire/RegisterActs, and the registered
// point-activation callback SpawnTeleporter. Re-homed out of the UserLogic god-TU;
// the class lives in <Gruntz/SecretTeleporterTrigger.h>. The per-coordinate
// registry (g_actColl @0x644688 + the g_act* fast-range globals) and the shared
// activation-name registry (g_nameReg @0x6bf650) are carried here as the TU-local
// inline lookups the trace tuned to this TU's codegen.
#include <Bute/ButeTree.h>              // CVariantSlot::Set (0x16d850)
#include <Wap32/ZVec.h>                 // _zvec::GrowTo (Find 0x16da80)
#include <Wap32/ZDArrayDerived.h>       // CZDArrayDerived::Construct (0x408710)
#include <Gruntz/TriggerMgr.h>          // CTriggerMgr::HitTestCell (0x75af0)
#include <Gruntz/GruntSpawnConfig.h>    // CGruntSpawnConfig::SpawnVoiceDriver (the cue)
#include <Gruntz/TeleSpriteFactory.h>   // CTeleSpriteFactory (CSpriteFactory) CreateSprite
#include <Gruntz/Trigger.h>             // CTrigger (point-probe result, its m_10 HUD sprite)
#include <Gruntz/Viewport.h>            // CViewport (visible-rect base at +0x5c)
#include <Gruntz/WwdGameReg.h>          // WwdGameReg facet (g_gameReg singleton)
#include <Gruntz/ActColl.h>             // CActColl/GetRetAddr + g_actCache/g_retAddrBreadcrumb
#include <Gruntz/SecretTeleporterTrigger.h> // the canonical class
#include <Gruntz/SerialObjRef.h>        // CSerialObjRef::Chain (0x8c00) - the +0x34 round-trip
#include <Gruntz/ActName.h>             // CString (~CString 0x1b9b93 / operator= 0x1b9e74)
#include <Globals.h>                    // g_actLo/Hi/Base/Stride/Scratch/Cur/Coll2 (fast range)
#include <rva.h>

// The global game registry the teleporter tails poll (WwdGameReg, the same symbol
// wwdfile labels at RVA 0x24556c; only the touched fields are modeled). Declared
// extern only - wwdfile owns the DATA label. m_world downcasts to CTeleResHolder*,
// m_cueSink to the cue driver, m_68 to CTriggerMgr*, m_7c to WwdGameRegAux*.
extern WwdGameReg* g_gameReg;

// The +0x7c aux facet: the ctor bumps its +0x3c "teleporter armed" counter.
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

// The +0x30 resource/sprite-factory holder reached as g_gameReg->m_world.
SIZE_UNKNOWN(CTeleResHolder);
struct CTeleResHolder {
    char m_pad0[0x8];
    CTeleSpriteFactory* m_8; // +0x08  HUD sprite factory
    char m_pad0c[0x24 - 0xc];
    CViewport* m_24; // +0x24  viewport (visible-bounds source)
};

// ---------------------------------------------------------------------------
// The per-coordinate activation registry FireActivation (0x042150) dispatches
// through. A coordinate maps to an Entry* either directly (when within the fast
// [g_actLo,g_actHi] range) via g_actBase + (coord-g_actLo)*g_actStride, or by a
// slow lookup in g_actColl (0x16da80, __thiscall ret 8), which on miss rebuilds the
// table (g_actAlloc 0x16d990 -> g_actCache, g_actColl2 insert 0x16d850 __thiscall
// ret 0xc) and yields g_actCur. The entry's first dword is a PMF of the trigger
// class; a nonzero entry's handler is called __thiscall on `this`. All globals are
// unnamed BSS (DATA-pinned here so the loads reloc-mask); the collection methods are
// external/no-body. g_actColl (0x644688) is this TU's own collection singleton.
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
// g_actCache/g_retAddrBreadcrumb collection methods declared above.
// ---------------------------------------------------------------------------
DATA(0x002bf620)
extern CButeTree g_buteTree; // 0x6bf620
DATA(0x0021aea8)
extern i32 g_nextActId;
DATA(0x0020a454)
extern char s_actKeyA[];
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

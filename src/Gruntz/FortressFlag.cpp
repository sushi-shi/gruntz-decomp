#include <Gruntz/ActNameRegistry.h> // the shared activation-name registry archetype
#include <Rez/FrameClock.h> // frame-clock band (g_frameDelta/g_frameTime/g_killCueClock/g_engineFrameDelta)
#include <Gruntz/GameRegMfcPtr.h> // g_gameReg at its REAL type (CGruntzMgr)
#include <Gruntz/GruntzMgr.h>
#include <Wap32/zBitVec.h> // GetRetAddr/g_projActCache/g_retAddrBreadcrumb
#include <Wap32/ZVec.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/ActReg.h> // the shared CActReg coordinate-registry archetype
#include <Gruntz/FortressFlag.h>
#include <Gruntz/Particlez.h>
#include <Gruntz/Explosion.h>
#include <Gruntz/AnimWorker.h>    // shared Owner / Worker views + Worker_DefaultPump
#include <Gruntz/UserLogic.h>     // CUserLogic leaves the worker handlers build
#include <Gruntz/SerialArchive.h> // CSerialArchive (the inherited CWapX::Chain arg; ex SerialObjRef.h)
#include <Image/CImage.h>         // the +0x198 cached frame (ex CGameObjLayer view)
#include <Gruntz/SpriteRefTable.h> // the shared CSpriteRefTable (g_gameReg->m_spriteFactory->GetSel)
#include <Gruntz/Enums.h> // Warlord - the m_124 flag-owner roster (KING/NAPOLEAN/PATTON/VIKING)
#include <Gruntz/AnimSink.h>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/WwdGameReg.h> // the canonical WwdGameReg singleton (g_gameReg)
#include <Bute/ButeTree.h>     // CVariantSlot::Set (the grow-path node inserter m_4->Set)
#include <Globals.h>           // g_part* (CParticlez's registry scalars)

// The handler entry record (FortressFlagHandler/CFortressFlagActEntry, the PMF slot,
// proven at 0x46080/0x461e0) is defined in <Gruntz/FortressFlag.h> after the
// complete class. (The retail grow-path allocs 0xc-byte nodes, so the real record may
// carry 2 more fields at +4/+8 that no code in this TU touches - @identity-TODO; the
// stride is a runtime DATA value so the 4-byte model stays byte-exact here.)

VTBL(CFortressFlag, 0x001e725c);
VTBL(CParticlez, 0x001e7614);
VTBL(CExplosion, 0x001e766c);

DATA(0x00244870)
extern CActReg g_partColl;

static inline CPartEntry* PartLookup(i32 coord) {
    return reinterpret_cast<CPartEntry*>(g_partColl.ResolveEntry(coord));
}

DATA(0x002447f8)
extern CLogicActTable g_logicActReg_6447f8; // 0x6447f8 (owner TU: the merged CExplosion

extern "C" void LogicHandler_0466b0(); // thunk 0x4041ec -> 0x466b0

static inline i32 RegisterActionName() {
    i32 id = reinterpret_cast<i32>(g_buteTree.Find("A"));
    if (id == 0) {
        g_buteTree.Insert("A", reinterpret_cast<void*>(g_typeCounter));
        i32 key = g_typeCounter;
        id = key;
        char* slot = ActNameLookup(key);
        i32 cnt = g_typeColl.m_grown;
        void** nodes = reinterpret_cast<void**>(g_typeColl.m_alloc);
        if (cnt != 0) {
            do {
                if (nodes != 0) {
                    (reinterpret_cast<CString*>(nodes))->CString::~CString();
                }
                nodes++;
            } while (--cnt);
        }
        (reinterpret_cast<CString*>(slot))->operator=("A");
        g_typeCounter++;
    }
    return id;
}

// CFortressFlag::~CFortressFlag @0x010e90 - the leaf adds no destructible members
// beyond CUserLogic, so its dtor folds the bare CUserLogic teardown: store the
// CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link (the embedded
// ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The destructible
// link forces the /GX EH frame. Byte-identical in shape to
// ~CSecretTeleporterTrigger @0x010ab0; the empty body is enough for cl.
// IMPLICIT dtor (retail is COMPILER-GENERATED - eh-dtor-vptr-restamp CAUSE B):
// a user-declared `~CFortressFlag() {}` emits the leaf-vptr restamp, and the CWapX
// base EH state blocks the dead-store elision that used to hide it. The ??_G
// in the vtable-emitting TU forces the implicit ??1 COMDAT; pinned by name.
#include <rva.h>
RVA_COMPGEN(0x00010e90, 0x44, ??1CFortressFlag@@UAE@XZ)

RVA(0x00012cf0, 0x47)
i32 CParticlez::SerializeMove(CGruntArchive* ar, i32 tag, i32 c, i32 d) {
    if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
        return 0;
    }
    return Chain(ar, tag, c, reinterpret_cast<CGameObject*>(d)) != 0;
}

// CParticlez::~CParticlez @0x012d90 - the leaf adds no destructible members
// beyond CUserLogic, so its dtor folds the bare CUserLogic teardown. The
// destructible link forces the /GX EH frame. Byte-identical in shape to
// ~CTimeBomb @0x012a70; the empty body is enough for cl.
// IMPLICIT dtor (retail is COMPILER-GENERATED - eh-dtor-vptr-restamp CAUSE B):
// a user-declared `~CParticlez() {}` emits the leaf-vptr restamp, and the CWapX
// base EH state blocks the dead-store elision that used to hide it. The ??_G
// in the vtable-emitting TU forces the implicit ??1 COMDAT; pinned by name.
RVA_COMPGEN(0x00012d90, 0x44, ??1CParticlez@@UAE@XZ)

RVA(0x00012e20, 0x47)
i32 CExplosion::SerializeMove(CGruntArchive* ar, i32 tag, i32 c, i32 d) {
    if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
        return 0;
    }
    return Chain(ar, tag, c, reinterpret_cast<CGameObject*>(d)) != 0;
}

// CExplosion::~CExplosion (0x12ec0) - the /GX leaf dtor folds the bare CUserLogic
// teardown: store the CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link
// (the embedded ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The
// destructible link forces the /GX EH frame; the leaf vptr store is dead-eliminated.
// IMPLICIT dtor (retail is COMPILER-GENERATED - eh-dtor-vptr-restamp CAUSE B):
// a user-declared `~CExplosion() {}` emits the leaf-vptr restamp, and the CWapX
// base EH state blocks the dead-store elision that used to hide it. The ??_G
// in the vtable-emitting TU forces the implicit ??1 COMDAT; pinned by name.
RVA_COMPGEN(0x00012ec0, 0x44, ??1CExplosion@@UAE@XZ)

// CFortressFlag::CFortressFlag @0x045d30 - fold the shared CUserLogic(obj) init,
// run the eyecandy z-clamp, pick the flag's faction name (a 4-way switch on the
// sprite-selector m_124: KING/NAPOLEAN/PATTON/VIKING - any other selector hides
// the sub-object and bails), name + cycle-geometry the bound object, bind the "A"
// bute node, then resolve the selected sprite handle from g_gameReg's ref-index
// array and stamp it back (+0x58/+0x50/+0x4c). GetByIndex (0x4165) is the engine
// routine GetSel thunks to, so the reused GetSel call reloc-masks.
//
// @early-stop
// eh-ctor-vptr-restamp-position wall (docs/patterns/eh-ctor-vptr-restamp-position.md):
// body byte-identical (incl. the m_124 jump table); residual is the /GX leaf-vptr
// re-stamp position + EH-state ids.
RVA(0x00045d30, 0x203)
CFortressFlag::CFortressFlag(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    CWwdGameObjectA* o = m_object;
    i32 v = o->m_layer->m_anchorY + o->m_screenY + 0x186a0;
    if (o->m_sortKey != v) {
        o->m_sortKey = v;
        o->m_flags |= 0x20000;
    }
    const char* name;
    switch (m_object->m_124) {
        case WARLORD_KING:
            name = "GAME_FORTRESSFLAGZ_KING";
            break;
        case WARLORD_NAPOLEAN:
            name = "GAME_FORTRESSFLAGZ_NAPOLEAN";
            break;
        case WARLORD_PATTON:
            name = "GAME_FORTRESSFLAGZ_PATTON";
            break;
        case WARLORD_VIKING:
            name = "GAME_FORTRESSFLAGZ_VIKING";
            break;
        default:
            m_38->m_flags |= 0x10000;
            return;
    }
    m_38->ApplyName(name);
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    m_value = m_38->m_1a0.m_14;
    m_38->ApplyLookupGeometry("GAME_CYCLE100", 0);
    m_38->m_flags |= 3;
    i32 idx = g_gameReg->m_options[m_object->m_124]
                  .m_008; // the per-player sprite descriptor (GetSel arg)
    i32 sel = g_gameReg->m_spriteFactory->GetSel(idx, 0);
    CWwdGameObjectA* spr = m_object;
    spr->m_drawActive = 1;
    spr->m_drawFillCmd = 0xa;
    spr->m_drawFillArg = sel;
}

RVA(0x00046000, 0x15)
void CFortressFlag::InitActReg() {
    g_fortressFlagActReg.Construct(2000, 2010);
}

RVA(0x00046080, 0x102)
void CFortressFlag::FireActivation(i32 coord) {
    CFortressFlagActEntry* e =
        reinterpret_cast<CFortressFlagActEntry*>(g_fortressFlagActReg.ResolveEntry(coord));
    if (e->m_fn != 0) {
        CFortressFlagActEntry* e2 =
            reinterpret_cast<CFortressFlagActEntry*>(g_fortressFlagActReg.ResolveEntry(coord));
        (this->*(e2->m_fn))();
    }
}

// CFortressFlag::RegisterActs @0x0461e0 - bind the per-frame handler (AdvanceAnim
// @0x0463e0) to the activation key "A" via the shared name registry. The SAME
// archetype as CBehindCandyAni::RegisterActs.
//
// @early-stop
// register-pinning wall (docs/patterns/zero-register-pinning.md +
// test-old-value-decrement-loop-while-postdec.md, topic:wall topic:regalloc): logic
// byte-faithful (every call/immediate/branch/offset + the `mov [entry],offset
// AdvanceAnim` handler store match retail); residual is the slot-vs-id callee-saved
// register choice cascading into the free-loop count materialization. Deferred.
RVA(0x000461e0, 0x18d)
void CFortressFlag::RegisterActs() {
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
    (reinterpret_cast<CFortressFlagActEntry*>(g_fortressFlagActReg.ResolveEntry(id)))->m_fn =
        static_cast<i32 (CUserLogic::*)()>(&CFortressFlag::AdvanceAnim);
}

RVA(0x000463e0, 0x17)
i32 CFortressFlag::AdvanceAnim() {
    m_38->m_1a0.Advance(g_engineFrameDelta);
    return 0;
}

RVA(0x00046410, 0x92)
i32 CFortressFlag::SerializeMove(CGruntArchive* ar, i32 tag, i32 c, i32 d) {
    if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
        return 0;
    }
    if (!Chain(ar, tag, c, reinterpret_cast<CGameObject*>(d))) {
        return 0;
    }
    if (tag == 8) {
        CWwdGameObjectA* spr = m_object;
        i32 idx =
            g_gameReg->m_options[spr->m_124].m_008; // the per-player sprite descriptor (GetSel arg)
        i32 sel = g_gameReg->m_spriteFactory->GetSel(idx, 0);
        spr = m_object;
        spr->m_drawActive = 1;
        spr->m_drawFillCmd = 0xa;
        spr->m_drawFillArg = sel;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CActReg::Resolve (0x0464e0, RVA-homed from src/Stub/BoundaryLowerMethods.cpp) -
// the STANDALONE out-of-line copy of CActReg::ResolveEntry (<Gruntz/ActReg.h>):
// byte-for-byte the same fast-range -> GrowTo -> breadcrumb+Set slot resolver.
// The two big act-register fns CALL it via ILT thunk 0x3544 (their size exhausts
// cl's inline budget) - RegisterWarlordActions on g_actionTable@0x644610,
// RegisterActs_644af0 on g_reg_644af0@0x644af0, both CActReg registries - where
// the small per-class dispatchers inline ResolveEntry. Ex fake view
// "CTypeColl464" (its m_4/m_buf/m_buf2/m_20 were CActReg's canonical
// m_coll2/m_base/m_cur/m_scratch).
// @early-stop
// esi/edi regalloc wall: cl assigns this->esi, key->edi; retail swaps (key->esi,
// this->edi). Full fast-range/GrowTo/breadcrumb logic + offsets byte-faithful;
// not steerable.
RVA(0x000464e0, 0x74)
char* CActReg::Resolve(i32 id) {
    m_grown = 0;
    if (id >= m_lo && id <= m_hi) {
        return m_base + (id - m_lo) * m_stride;
    }
    if (GrowTo(id, 0)) { // 0x16da80 = _zvec::GrowTo (inherited)
        return m_base + (id - m_lo) * m_stride;
    }
    void* item = g_projActCache;
    g_retAddrBreadcrumb = GetRetAddr();
    m_errSink->Set(this, reinterpret_cast<i32>(item), 0xc);
    return m_spare;
}

RVA(0x00046850, 0xf1)
i32 LogicDispatchC(CGameObject* owner) {
    AnimWorkerObj* rec = owner->m_7c;
    switch (reinterpret_cast<u32>(rec->m_1c)) {
        case 0: {
            rec->m_1c = reinterpret_cast<void*>(0x3e8);
            CUserLogic* sub = new CParticlez(owner);
            sub->Activate();
            rec->m_logic = sub;
            break;
        }
        case 0x1d:
            rec->m_logic->UserLogicVfunc9();
            break;
        case 0x1e:
            rec->m_logic->UserLogicVfunc8();
            break;
        case 0x50:
            rec->m_logic->UserLogicVfuncC();
            break;
        case 0x53:
            rec->m_logic->UserLogicVfuncD();
            break;
        case 0x52:
            rec->m_logic->UserLogicVfuncA();
            break;
        case 0x51:
            rec->m_logic->UserLogicVfuncB();
            break;
        case 0x3e8:
            break;
        default:
            Worker_DefaultPump(rec->m_logic);
            break;
    }
    return 1;
}

RVA(0x00046990, 0xf1)
i32 Handler046990(CGameObject* owner) {
    AnimWorkerObj* rec = owner->m_7c;
    switch (reinterpret_cast<u32>(rec->m_1c)) {
        case 0: {
            rec->m_1c = reinterpret_cast<void*>(0x3e8);
            CUserLogic* sub = new CExplosion(owner);
            sub->Activate();
            rec->m_logic = sub;
            break;
        }
        case 0x1d:
            rec->m_logic->UserLogicVfunc9();
            break;
        case 0x1e:
            rec->m_logic->UserLogicVfunc8();
            break;
        case 0x50:
            rec->m_logic->UserLogicVfuncC();
            break;
        case 0x53:
            rec->m_logic->UserLogicVfuncD();
            break;
        case 0x52:
            rec->m_logic->UserLogicVfuncA();
            break;
        case 0x51:
            rec->m_logic->UserLogicVfuncB();
            break;
        case 0x3e8:
            break;
        default:
            Worker_DefaultPump(rec->m_logic);
            break;
    }
    return 1;
}

RVA(0x00046ad0, 0x15e)
CParticlez::CParticlez(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    m_38->m_flags |= 0x2000002;
    if (m_object->m_sortKey != 0xcf84f) {
        m_object->m_sortKey = 0xcf84f;
        m_object->m_flags |= 0x20000;
    }
    m_object->m_dirtyArmed = 0;
}

RVA(0x00046cb0, 0x15)
void CParticlez::InitActReg() {
    g_partColl.Construct(2000, 2010);
}

RVA(0x00046d30, 0x102)
void CParticlez::FireActivation(i32 coord) {
    CPartEntry* e = PartLookup(coord);
    if (e->m_fn != 0) {
        CPartEntry* e2 = PartLookup(coord);
        (this->*(e2->m_fn))();
    }
}

// CParticlez::RegisterActs @0x046e90 - bind the per-frame handler (Update
// @0x047090) to the activation key "A" via the shared name registry, then bind
// id->entry in the class's own coordinate registry (g_partColl). The SAME
// archetype as CSecretTeleporterTrigger::RegisterActs.
//
// @early-stop
// register-pinning wall (docs/patterns/zero-register-pinning.md +
// test-old-value-decrement-loop-while-postdec.md, topic:wall topic:regalloc): logic
// byte-faithful (every call/immediate/branch/offset + the `mov [entry],offset
// Update` handler store match retail); residual is the slot-vs-id callee-saved
// register choice cascading into the free-loop count materialization. Deferred.
RVA(0x00046e90, 0x18d)
void CParticlez::RegisterActs() {
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
    (reinterpret_cast<CPartEntryI32*>(PartLookup(id)))->m_fn =
        static_cast<i32 (CUserLogic::*)()>(&CParticlez::Update);
}

RVA(0x00047090, 0x4c)
i32 CParticlez::Update() {
    m_38->m_1a0.Advance(g_engineFrameDelta);
    CWwdGameObjectA* o = m_38;
    if (o->m_1a0.m_28 != 0 && o->m_1a0.m_20 == 0) {
        o->m_flags |= 0x10000;
    }
    return 0;
}

// CExplosion::CExplosion (0x470e0) - the eyecandy ctor: fold the shared
// CUserLogic(obj) init, then name the bound object "GAME_EXPLOSION", bind its "A"
// bute node, flag the sub-object, lock the draw order to 0xf4240 and clear m_38.
//
// @early-stop
// eh-ctor-vptr-restamp-position wall (docs/patterns/eh-ctor-vptr-restamp-position.md,
// topic:wall topic:eh): body byte-identical; residual is the /GX leaf-vptr re-stamp
// position + the EH-state ids, not source-steerable - the established leaf-ctor
// baseline (cf. CMenuSparkle 92.8% / CEyeCandy 92.5% in userlogic).
RVA(0x000470e0, 0x16b)
CExplosion::CExplosion(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_38->ApplyName("GAME_EXPLOSION");
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    m_38->m_flags |= 0x2000002;
    CWwdGameObjectA* o = m_object;
    if (o->m_sortKey != 0xf4240) {
        o->m_sortKey = 0xf4240;
        o->m_flags |= 0x20000;
    }
    m_object->m_dirtyArmed = 0;
}

RVA(0x000472d0, 0x15)
void InitLogicDispatch_6447f8() {
    g_logicActReg_6447f8.Construct(0x7d0, 0x7da);
}

RVA(0x00047350, 0x102)
void CExplosion::FireActivation(i32 id) {
    CExplosionActEntry* e =
        reinterpret_cast<CExplosionActEntry*>(g_logicActReg_6447f8.ResolveEntry(id));
    if (e->m_fn != 0) {
        CExplosionActEntry* e2 =
            reinterpret_cast<CExplosionActEntry*>(g_logicActReg_6447f8.ResolveEntry(id));
        (this->*(e2->m_fn))();
    }
}

// RegisterXLogic_6447f8 @0x0474b0 - bind the CExplosion logic class to its
// activation handler under the shared action name "A". (Moved from
// LogicActReg.cpp - text-contained in this TU.)
// @early-stop
// register-pinning wall (docs/patterns/zero-register-pinning.md): logic
// byte-faithful, residual is the action-id register coloring + count-down
// induction. Deferred.
RVA(0x000474b0, 0x18d)
void RegisterXLogic_6447f8() {
    i32 id = RegisterActionName();
    *reinterpret_cast<void**>(g_logicActReg_6447f8.ResolveEntry(id)) =
        static_cast<void*>(&LogicHandler_0466b0);
}

#include <rva.h>

// g_fortressFlagActReg (0x00244638): CActReg - no provable static init (the type has no
// default ctor / is runtime-Init'd), so the datum is named by symbol.
DATA_SYMBOL(0x00244638, 0x0, ?g_fortressFlagActReg@@3UCActReg@@A)
// (CFortressFlagActEntry/CPartEntry/CPartEntryI32 SIZE_UNKNOWN live beside their
//  CActReg.) The ex-WwdRefSlot "+0x158 ref-index array" view is DISSOLVED: it was
//  m_options[i].m_008 - the per-player sprite descriptor (stride 0x238, base +0x150+8).

// FortressFlag.cpp - the ORIGINAL fortress-flag TU: CFortressFlag + CParticlez +
// CExplosion (+ their worker handlers and registrars); one obj, text
// 0x45d30-0x4763d (wave3-I grunt-region partition).
//
// ONE-TU evidence (TU_MIGRATION 0x42d40 interval resolution):
//   * init-frag SANDWICH: the CRT-table runs i303-i304 (fortressflag x2 @0x45fe0),
//     i305 (explosion @0x465d0), i306-i312 (fortressflag x7 @0x46620), i313
//     (particlez @0x46c90) - the explosion frag INSIDE the fortressflag frag run
//     is impossible for separate objs.
//   * text containment: with ff+explosion one obj, its contribution spans
//     0x45d30..0x4763d, which contains the particlez block (0x46ad0-0x470b5),
//     LogicDispatchC @0x46850 (state-0 news a CPARTICLEZ - thunk 0x2a04 ->
//     0x46ad0), Handler046990 @0x46990 (state-0 news a CEXPLOSION), and the
//     explosion dispatch triple (InitLogicDispatch_6447f8 @0x472d0 /
//     FireActivation @0x47350 / RegisterXLogic_6447f8 @0x474b0) woven between
//     the explosion ctor and its registrar.
//   * private .data extent 0x20d384-0x20d3e8 (the ff ctor's statics) sits
//     between the warlord obj's band and grunt-main's band, in TU link order.
// NOT this TU: HandleFortConquered @0x3f5f0 (its text + its private .data cells
// 0x20d154-0x20d16c sit between WormholeActs and the wormhole trio -> moved to
// FortConquered.cpp); Update@RbEffect @0x476b0 (boundary-ambiguous, left in
// RockBreakEffectUpdate.cpp with a note).
//
// Function roster in strict retail-RVA order:
//   0x010e90 ~CFortressFlag  0x012cf0 P::Serialize  0x012d90 ~CParticlez
//   0x012e20 E::Serialize    0x012ec0 ~CExplosion
//   0x045d30 CFortressFlag ctor  0x046000 FF::InitActReg  0x046080 FF::FireActivation
//   0x0461e0 FF::RegisterActs  0x0463e0 FF::AdvanceAnim  0x046410 FF::Serialize
//   0x0464e0 CActReg::Resolve  0x046850 LogicDispatchC  0x046990 Handler046990
//   0x046ad0 CParticlez ctor  0x046cb0 P::InitActReg  0x046d30 P::FireActivation
//   0x046e90 P::RegisterActs  0x047090 P::Update
//   0x0470e0 CExplosion ctor  0x0472d0 InitLogicDispatch_6447f8
//   0x047350 E::FireActivation  0x0474b0 RegisterXLogic_6447f8
#include <Gruntz/ActNameRegistry.h> // the shared activation-name registry archetype
#include <Gruntz/WwdGameRegPtr.h>
#include <Wap32/zBitVec.h>          // GetRetAddr/g_projActCache/g_retAddrBreadcrumb
#include <Gruntz/MovingLogicBase.h> // CMovingLogicBase::Serialize (0x16e7f0) - shared serialize chain
#include <Wap32/ZVec.h>
#include <Wap32/ZDArrayDerived.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/ActReg.h> // the shared CActReg coordinate-registry archetype
#include <Gruntz/FortressFlag.h>
#include <Gruntz/Particlez.h>
#include <Gruntz/Explosion.h>
#include <Gruntz/AnimWorker.h>     // shared Owner / Worker views + Worker_DefaultPump
#include <Gruntz/UserLogic.h>      // CUserLogic leaves the worker handlers build
#include <Gruntz/SerialObjRef.h>   // the shared serialized-object-reference (Chain @0x8c00)
#include <Gruntz/SpriteRefTable.h> // the shared CSpriteRefTable (g_gameReg->m_74->GetSel)
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

// The class's activation-coordinate registry singleton (@0x644638), built over the
// fixed [2000,2010] range by the shared registry ctor (0x408710). It is the shared
// <Gruntz/ActReg.h> CActReg archetype directly (the ex empty-derived
DATA(0x00244638)
CActReg g_fortressFlagActReg; // 0x644638 (owner TU: real definition;
                              // interior fields 0x24463c..0x244658 are members)

// The per-frame draw-delta mirror (_g_6bf3bc); the value-load reloc-masks.
extern "C" u32 g_engineFrameDelta;

// The bound sprite/game-object is the inherited CUserLogic m_10 (a CGameObject*):
// the tag-8 fixup reads its +0x124 sprite-selector key and re-seeds the
// +0x4c/+0x50/+0x58 state trio directly (all modeled on CGameObject).

// The level sprite-ref table (g_gameReg->m_74). GetSel(i, bAlt) (0xe23c0) returns
// the selected sprite handle for ref-row i; the body lives in
// src/Gruntz/SpriteRefTable.cpp (reloc-masked). CSpriteRefTable is the shared
// <Gruntz/SpriteRefTable.h> shape.

// One ref-index array slot: an 8-byte entry whose first dword is the ref-row
// index. The fixup indexes the array at g_gameReg+0x158 by (selector-row * 71)
// (retail: lea+shl+sub = *71, then the *8 element stride folds into the [...*8]
// addressing-mode scale), so the row multiply is materialized but the *8 is free.
struct WwdRefSlot {
    i32 m_idx; // +0x00  ref-row index (passed to GetSel)
    i32 m_04;  // +0x04
};

// The global game registry (canonical <Gruntz/WwdGameReg.h>, RVA 0x24556c; wwdfile
// owns the DATA label). The tag-8 fixup reads the level sprite-ref table at +0x74
// (m_74) and the ref-index array in the m_options block at +0x158 (raw offset - the
// established 0x150-region idiom; WwdRefSlot is this TU's element view).

// ---------------------------------------------------------------------------
// CParticlez's per-coordinate activation registry (@0x644870) - ONE 0x24-byte
// CActReg object, the SAME archetype every CUserLogic leaf reuses (<Gruntz/ActReg.h>).
//
// SHREDDED-OBJECT FIX: 0x644874..0x644890 are this object's INTERIOR FIELDS
// (m_coll2 / m_lo / m_hi / m_base / m_cur / m_stride / m_scratch), and this TU used
// to declare them as seven separate DATA()-pinned scalar globals - seven .bss
// variables where retail has one object. The hand-inlined PartLookup over those
// scalars was CActReg::ResolveEntry spelled out; it is the archetype inline now.
//
// DEFINED here (the owner TU) as zero-init .bss with NO constructor - which is what
// retail has, proven twice over: (1) the CRT dynamic-init table (30 entries, rva
// 0x2096e4..0x20975c, NULL-bounded) holds no initializer for 0x644870 - it lists only
// 0x653c88 / 0x683ec8 / 0x6bf408 / 0x6bf430 / 0x6bf468 / 0x6bf480 / 0x6bf620 (g_buteTree)
// / 0x6bf650 (g_typeColl) / 0x6bf848; and (2) the address lies past .data's raw extent
// (file-backed only through rva 0x229400), so the loader zero-fills it. The object is
// ctor'd IN PLACE at runtime by InitActReg below (Construct 0x408710 -> the real
// CTypeKeyColl ctor 0x16dda0, then the live-vtable stamp) - which is exactly why the
// engine call sites cast it to (_zvec*): the storage type and the runtime class differ
// in retail itself. Declaring it `CTypeKeyColl` instead would be unbuildable (no default
// ctor) and would fabricate a 31st CRT initializer the binary does not have.
DATA(0x00244870)
CActReg g_partColl;

// The CParticlez entry records (PartHandler/CPartEntry, PartHandlerI32/CPartEntryI32,
// the PMF slot) are defined in <Gruntz/Particlez.h> after the complete class.

// The coordinate->Entry* lookup FireActivation folds in twice: the shared archetype
// inline, typed to this registry's entry.
static inline CPartEntry* PartLookup(i32 coord) {
    return (CPartEntry*)g_partColl.ResolveEntry(coord);
}

// The CExplosion class dispatch table (@0x6447f8), constructed by
// InitLogicDispatch_6447f8 below and bound by RegisterXLogic_6447f8.
DATA(0x002447f8)
CLogicActTable g_logicActReg_6447f8; // 0x6447f8 (owner TU: the merged CExplosion
                                     // dispatch table; interior 0x2447fc..0x244818 are members)

// The explosion activation handler (ILT thunk; referenced by address so the
// entry store emits a reloc-masked DIR32 to the named symbol).
extern "C" void LogicHandler_0466b0(); // thunk 0x4041ec -> 0x466b0

// The shared name-registry build (action key "A"), CKitchenSlime::RegisterType
// ordering: register the action name on first use (g_buteTree maps name->id),
// resolve its name-table slot, free the slot's old CString nodes, assign the key,
// bump the global counter; returns the (possibly newly-allocated) action id.
static inline i32 RegisterActionName() {
    i32 id = (i32)g_buteTree.Find(s_codeA);
    if (id == 0) {
        g_buteTree.Insert(s_codeA, (void*)g_typeCounter);
        i32 key = g_typeCounter;
        id = key;
        char* slot = ActNameLookup(key);
        i32 cnt = g_typeColl.m_grown;
        void** nodes = (void**)g_typeColl.m_alloc;
        if (cnt != 0) {
            do {
                if (nodes != 0) {
                    ((CString*)nodes)->CString::~CString();
                }
                nodes++;
            } while (--cnt);
        }
        ((CString*)slot)->operator=(s_codeA);
        g_typeCounter++;
    }
    return id;
}

// CFortressFlag::GetTypeTag (0x00010e40) is now an inline member in the class header.

// CFortressFlag::~CFortressFlag @0x010e90 - the leaf adds no destructible members
// beyond CUserLogic, so its dtor folds the bare CUserLogic teardown: store the
// CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link (the embedded
// ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The destructible
// link forces the /GX EH frame. Byte-identical in shape to
// ~CSecretTeleporterTrigger @0x010ab0; the empty body is enough for cl.
RVA(0x00010e90, 0x44)
CFortressFlag::~CFortressFlag() {}

// CParticlez::Serialize @0x012cf0 - the vtable slot-1 override: chain the shared
// CUserLogic serialize helper on `this`, and (only on success) the +0x34 serializable
// sub-object's chain; normalize the second chain's success to a strict bool. The
// byte-identical chain-Serialize archetype (differs only in the two call displacements).
RVA(0x00012cf0, 0x47)
i32 CParticlez::Serialize(i32 ar, i32 tag, i32 c, i32 d) {
    if (!((CMovingLogicBase*)this)->Serialize((CSerialArchive*)(ar), tag, c, d)) {
        return 0;
    }
    return ((CSerialObjRef*)&m_34)->Chain((CSerialArchive*)ar, tag, c, (CGameObject*)d) != 0;
}

// CParticlez::~CParticlez @0x012d90 - the leaf adds no destructible members
// beyond CUserLogic, so its dtor folds the bare CUserLogic teardown. The
// destructible link forces the /GX EH frame. Byte-identical in shape to
// ~CTimeBomb @0x012a70; the empty body is enough for cl.
RVA(0x00012d90, 0x44)
CParticlez::~CParticlez() {}

// CExplosion::Serialize @0x012e20 - the vtable slot-1 override: chain the shared
// CUserLogic serialize helper on `this`, and (only on success) the +0x34 serializable
// sub-object's chain; normalize the second chain's success to a strict bool. The
// byte-identical chain-Serialize archetype (differs only in the two call displacements).
RVA(0x00012e20, 0x47)
i32 CExplosion::Serialize(i32 ar, i32 tag, i32 c, i32 d) {
    if (!((CMovingLogicBase*)this)->Serialize((CSerialArchive*)(ar), tag, c, d)) {
        return 0;
    }
    return ((CSerialObjRef*)&m_34)->Chain((CSerialArchive*)ar, tag, c, (CGameObject*)d) != 0;
}

// CExplosion::~CExplosion (0x12ec0) - the /GX leaf dtor folds the bare CUserLogic
// teardown: store the CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link
// (the embedded ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The
// destructible link forces the /GX EH frame; the leaf vptr store is dead-eliminated.
RVA(0x00012ec0, 0x44)
CExplosion::~CExplosion() {}

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
CFortressFlag::CFortressFlag(CGameObject* obj) : CUserLogic(obj) {
    TILE_LOGIC_SEED(obj);
    CGameObject* o = m_object;
    i32 v = o->m_layer->m_halfHeight + o->m_screenY + 0x186a0;
    if (o->m_latchedAnimId != v) {
        o->m_latchedAnimId = v;
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
    m_objAux->m_1c = g_buteTree.Find(s_codeA);
    m_prevAnimNode = m_38->m_1a0.m_14;
    m_38->ApplyLookupGeometry("GAME_CYCLE100", 0);
    m_38->m_flags |= 3;
    i32 idx = ((WwdRefSlot*)((char*)g_gameReg + 0x158))[m_object->m_124 * 71].m_idx;
    i32 sel = g_gameReg->m_74->GetSel(idx, 0);
    CGameObject* spr = m_object;
    spr->m_drawActive = 1;
    spr->m_drawFillCmd = 0xa;
    spr->m_drawFillArg = sel;
}

// CFortressFlag::InitActReg @0x046000 - construct the class's activation-
// coordinate registry singleton (g_fortressFlagActReg @0x644638) over the fixed
// range [2000, 2010] via the shared registry ctor (0x408710). Free init thunk.
RVA(0x00046000, 0x15)
void CFortressFlag::InitActReg() {
    ((CZDArrayDerived*)&g_fortressFlagActReg)->Construct(2000, 2010);
}

// CFortressFlag::FireActivation @0x046080 - look the activation coordinate up in the
// class registry (g_fortressFlagActReg); if the resolved entry carries a registered
// handler PMF, resolve it again and dispatch it __thiscall on `this`. Same archetype
// as CParticlez::FireActivation (double ResolveEntry + PMF dispatch).
RVA(0x00046080, 0x102)
void CFortressFlag::FireActivation(i32 coord) {
    CFortressFlagActEntry* e = (CFortressFlagActEntry*)g_fortressFlagActReg.ResolveEntry(coord);
    if (e->m_fn != 0) {
        CFortressFlagActEntry* e2 =
            (CFortressFlagActEntry*)g_fortressFlagActReg.ResolveEntry(coord);
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
    i32 id = (i32)g_buteTree.Find(s_codeA);
    if (id == 0) {
        id = g_typeCounter;
        g_buteTree.Insert(s_codeA, (void*)id);
        char* slot = ActNameLookup(id);
        i32 n = g_typeColl.m_grown;
        void** list = (void**)g_typeColl.m_alloc;
        while (n-- != 0) {
            if (list != 0) {
                ((CString*)list)->CString::~CString();
            }
            list++;
        }
        ((CString*)slot)->operator=(s_codeA);
        g_typeCounter++;
    }
    ((CFortressFlagActEntry*)g_fortressFlagActReg.ResolveEntry(id))->m_fn =
        &CFortressFlag::AdvanceAnim;
}

// CFortressFlag::AdvanceAnim @0x0463e0 - re-target the bound object's animation
// sub-object (m_38 + 0x1a0) to the current draw-delta (g_engineFrameDelta) and return 0.
// Same archetype as CGruntCreationPoint::AdvanceAnim (0x03ecc0).
RVA(0x000463e0, 0x17)
i32 CFortressFlag::AdvanceAnim() {
    m_38->m_1a0.Advance(g_engineFrameDelta);
    return 0;
}

// CFortressFlag::Serialize @0x046410 - chain the shared CUserLogic serialize
// helper on `this`, and (only on success) the +0x34 sub-object's chain; both run
// the same (ar, tag, c, d) tuple. On the post-load tag (tag == 8), look the
// flag's sprite selector (m_object->m_124 * 71) up in g_gameReg's ref-index array,
// resolve it through the level sprite-ref table, and re-seed the bound sprite's
// state trio. Always returns 1 once the two chains succeed.
RVA(0x00046410, 0x92)
i32 CFortressFlag::Serialize(i32 ar, i32 tag, i32 c, i32 d) {
    if (!((CMovingLogicBase*)this)->Serialize((CSerialArchive*)(ar), tag, c, d)) {
        return 0;
    }
    if (!SerialRef34()->Chain((CSerialArchive*)ar, tag, c, (CGameObject*)d)) {
        return 0;
    }
    if (tag == 8) {
        CGameObject* spr = m_object;
        i32 idx = ((WwdRefSlot*)((char*)g_gameReg + 0x158))[spr->m_124 * 71].m_idx;
        i32 sel = g_gameReg->m_74->GetSel(idx, 0);
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
    m_scratch = 0;
    if (id >= m_lo && id <= m_hi) {
        return m_base + (id - m_lo) * m_stride;
    }
    if ((i32)((_zvec*)this)->GrowTo(id, 0)) { // 0x16da80 = _zvec::GrowTo
        return m_base + (id - m_lo) * m_stride;
    }
    void* item = g_projActCache;
    g_retAddrBreadcrumb = GetRetAddr();
    m_coll2->Set(this, (i32)item, 0xc);
    return m_cur;
}

// ---------------------------------------------------------------------------
// LogicDispatchC @0x046850 - the CParticlez worker-pump handler (moved from
// LogicRecordDispatch.cpp; state-0 news a CPARTICLEZ - the 0x54 `new` + ctor
// thunk 0x2a04 -> 0x46ad0 == ??0CParticlez). __cdecl FREE function; the shared
// AnimWorker Owner/Worker pump shape (same as Handler046990 below).
RVA(0x00046850, 0xf1)
i32 LogicDispatchC(Owner* owner) {
    Worker* rec = owner->m_7c;
    switch (rec->m_1c) {
        case 0: {
            rec->m_1c = 0x3e8;
            CUserLogic* sub = new CParticlez((CGameObject*)owner);
            sub->Activate();
            rec->m_18 = sub;
            break;
        }
        case 0x1d:
            rec->m_18->UserLogicVfunc9();
            break;
        case 0x1e:
            rec->m_18->UserLogicVfunc8();
            break;
        case 0x50:
            rec->m_18->UserLogicVfuncC();
            break;
        case 0x53:
            rec->m_18->UserLogicVfuncD();
            break;
        case 0x52:
            rec->m_18->UserLogicVfuncA();
            break;
        case 0x51:
            rec->m_18->UserLogicVfuncB();
            break;
        case 0x3e8:
            break;
        default:
            Worker_DefaultPump(rec->m_18);
            break;
    }
    return 1;
}

// Handler046990 @0x046990 - the CExplosion worker-pump handler (moved from
// AnimWorkerHandlers.cpp; state-0 news a CEXPLOSION). Same __cdecl dispatch shape.
RVA(0x00046990, 0xf1)
i32 Handler046990(Owner* owner) {
    Worker* rec = owner->m_7c;
    switch (rec->m_1c) {
        case 0: {
            rec->m_1c = 0x3e8;
            CUserLogic* sub = new CExplosion((CGameObject*)owner);
            sub->Activate();
            rec->m_18 = sub;
            break;
        }
        case 0x1d:
            rec->m_18->UserLogicVfunc9();
            break;
        case 0x1e:
            rec->m_18->UserLogicVfunc8();
            break;
        case 0x50:
            rec->m_18->UserLogicVfuncC();
            break;
        case 0x53:
            rec->m_18->UserLogicVfuncD();
            break;
        case 0x52:
            rec->m_18->UserLogicVfuncA();
            break;
        case 0x51:
            rec->m_18->UserLogicVfuncB();
            break;
        case 0x3e8:
            break;
        default:
            Worker_DefaultPump(rec->m_18);
            break;
    }
    return 1;
}

// --- CParticlez (0x046ad0), vptr 0x5e7614 --- the ctor anchors GetTypeTag @0x12cd0
// + the ??_7CParticlez vtable in this TU. Folds the inline CUserLogic(obj) base.
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

// CParticlez::InitActReg @0x046cb0 - construct the class's activation-coordinate
// registry singleton (g_partColl @0x644870) over the fixed range [2000, 2010]
// via the shared registry ctor (FUN_00408710, __thiscall ret 8). A free init
// thunk (no `this`); reloc-masked.
RVA(0x00046cb0, 0x15)
void CParticlez::InitActReg() {
    ((CZDArrayDerived*)&g_partColl)->Construct(2000, 2010);
}

// CParticlez::FireActivation @0x046d30 - look the activation coordinate up in
// the registry; if the entry has a registered handler, look it up again and
// dispatch it __thiscall on this. Same archetype as CTimeBomb::FireActivation
// (0x0e1830).
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
    i32 id = (i32)g_buteTree.Find(s_codeA);
    if (id == 0) {
        id = g_typeCounter;
        g_buteTree.Insert(s_codeA, (void*)id);
        char* slot = ActNameLookup(id);
        i32 n = g_typeColl.m_grown;
        void** list = (void**)g_typeColl.m_alloc;
        while (n-- != 0) {
            if (list != 0) {
                ((CString*)list)->CString::~CString();
            }
            list++;
        }
        ((CString*)slot)->operator=(s_codeA);
        g_typeCounter++;
    }
    ((CPartEntryI32*)PartLookup(id))->m_fn = &CParticlez::Update;
}

// CParticlez::Update @0x047090 - re-target the bound object's animation sub-object
// (m_38 + 0x1a0) to the current draw-delta (g_engineFrameDelta); then, if its +0x1c8 latch is
// set and its +0x1c0 latch is clear, mark it on-screen this frame (+0x8 |= 0x10000).
// Always returns 0. The extended AdvanceAnim archetype.
RVA(0x00047090, 0x4c)
i32 CParticlez::Update() {
    m_38->m_1a0.Advance(g_engineFrameDelta);
    CGameObject* o = m_38;
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
CExplosion::CExplosion(CGameObject* obj) : CUserLogic(obj) {
    TILE_LOGIC_SEED(obj);
    m_38->ApplyName("GAME_EXPLOSION");
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    m_38->m_flags |= 0x2000002;
    CGameObject* o = m_object;
    if (o->m_latchedAnimId != 0xf4240) {
        o->m_latchedAnimId = 0xf4240;
        o->m_flags |= 0x20000;
    }
    m_object->m_38 = 0;
}

// InitLogicDispatch_6447f8 @0x0472d0 - construct the CExplosion dispatch table
// (@0x6447f8) over [0x7d0, 0x7da]. (Moved from LogicDispatchInit.cpp - the frag
// i314 @0x472b0 is text-contained in this TU, between the explosion ctor and its
// FireActivation.)
RVA(0x000472d0, 0x15)
void InitLogicDispatch_6447f8() {
    ((CZDArrayDerived*)&g_logicActReg_6447f8)->Construct(0x7d0, 0x7da);
}

// CExplosion::FireActivation @0x047350 - slot-4 (UserLogicVfunc2) override: resolve
// `id` in the class dispatch table; if the entry carries a handler, re-resolve and
// dispatch it __thiscall on `this`. Same archetype as CTeleporter::FireActivation
// (the ResolveEntry inline expands twice).
RVA(0x00047350, 0x102)
void CExplosion::FireActivation(i32 id) {
    CExplosionActEntry* e = (CExplosionActEntry*)g_logicActReg_6447f8.ResolveEntry(id);
    if (e->m_fn != 0) {
        CExplosionActEntry* e2 = (CExplosionActEntry*)g_logicActReg_6447f8.ResolveEntry(id);
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
    *(void**)g_logicActReg_6447f8.ResolveEntry(id) = (void*)&LogicHandler_0466b0;
}

// class-metadata SIZE sweep (misc-Gruntz A-C): matching-neutral, hosted at
// .cpp EOF (see docs/class-metadata-sweep-log.md). SIZE_UNKNOWN = size not yet pinned.
#include <rva.h>
#include <Gruntz/SerialArchive.h> // the serialize stream (== the real CFileMemBase)
// (CFortressFlagActEntry/CPartEntry/CPartEntryI32 SIZE_UNKNOWN live beside their
//  CActReg.) WwdRefSlot stays a flagged .cpp view - it is a genuine element of the
//  unmodeled CGameRegistry m_focusSlots ref-index sub-array (@identity-TODO).
SIZE_UNKNOWN(WwdRefSlot);
SIZE_UNKNOWN(CParticlez);

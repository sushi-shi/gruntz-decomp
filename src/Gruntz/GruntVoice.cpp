#include <Gruntz/GameObjectFactory.h> // C linkage for the definitions below (inherited, not restated)
#include <Mfc.h>                      // CMapPtrToPtr (the id->object map, Lookup @0x1b8760)
#include <Gruntz/CurPlayer.h>         // g_curPlayer
#include <Gruntz/GruntzMgr.h>         // complete CGruntzMgr (g_gameReg real type)
#include <rva.h>

#include <Gruntz/GruntVoice.h>
#include <Gruntz/VoiceTrigger.h>          // canonical CVoiceTrigger : CUserLogic
#include <Gruntz/TileTriggerTransition.h> // CTileTransitionController/State worker-pump view
#include <Gruntz/GameRegistry.h>          // g_gameReg / g_gameReg->m_world->m_childGroup
#include <DDrawMgr/DDrawChildGroup.h>     // CDDrawChildGroup - m_map48 (the id->object map)
#include <Gruntz/TriggerMgr.h> // CTriggerMgr::FindGruntAt (m_cmdGrid @0x75c60, cast-free); typedef CGrunt CGrunt
#include <Gruntz/Grunt.h>            // complete CGrunt - FindGruntAt result (m_object reads)
#include <Gruntz/GruntSpawnConfig.h> // canonical CGruntSpawnConfig (SpawnVoiceDriver @0x11b3b0)
#include <Gruntz/Ufo.h>              // the REAL CUFO (the ex-L_13400 shell is dissolved)
#include <Gruntz/SerialArchive.h> // CFileMemBase (the inherited CWapX::Chain arg; ex SerialObjRef.h)
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/ActName.h> // CActName (shared)
#include <Gruntz/ActReg.h> // CActReg - the ONE registry archetype (subsumes the old per-field globals)
#include <Bute/ButeTree.h>
#include <Dsndmgr/StreamVoice.h>
#include <Wap32/ZVec.h>
#include <Gruntz/SerialArchive.h> // the serialize stream (== the real CFileMemBase)
#include <Image/CImage.h>         // the +0x198 cached frame (ex CGameObjLayer view)

// CActRegPool<CVoiceTrigger>::s_table (0x00251500): CActReg - no provable static init (the type has no
// default ctor / is runtime-Init'd), so the datum is named by symbol.
#include <Gruntz/GruntVoiceActReg.h> // CActRegPool<CGruntVoice>::s_table (ex .cpp extern)
#include <Wap32/zBitVec.h>           // ex Globals.h
#include <Utils/MapTyped.h>          // typed MFC map lookups
template<> DATA(0x002514d8)
CActReg CActRegPool<CGruntVoice>::s_table(2000, 2010);
template<> DATA(0x00251500)
CActReg CActRegPool<CVoiceTrigger>::s_table(2000, 2010);

VTBL(CVoiceTrigger, 0x001e885c);
VTBL(CGruntVoice, 0x001eaf6c);

struct CString; // canonical g_typeColl.m_spare slot record (<Gruntz/TypeNameEntry.h>)

static inline CActHandler* VActLookup(i32 coord) {
    return (CActRegPool<CGruntVoice>::s_table.ResolveEntry(coord));
}

static inline CString* ActNameSlots() {
    return g_typeColl.Slots();
}

static inline CString* ActNameLookup(i32 id) {
    g_typeColl.m_grown = 0;
    if (id >= g_typeColl.m_lo && id <= g_typeColl.m_hi) {
        return g_typeColl.Elem(id);
    }
    if ((static_cast<_zvec*>(&g_typeColl))->GrowTo(id, 0) != 0) {
        return g_typeColl.Elem(id);
    }
    char* msg = g_errOutOfMem;
    g_retAddrBreadcrumb = GetRetAddr();
    g_typeColl.m_errSink->Set(&g_typeColl, msg, 0xc);
    return g_typeColl.Scratch();
}

RVA_COMPGEN(0x00013400, 0x44, ??1CUFO@@UAE@XZ)
void RealizeUfoDtor(CUFO* p);
void RealizeUfoDtor(CUFO* p) {
    p->CUFO::~CUFO(); // qualified direct call - odr-uses the implicit ??1CUFO COMDAT
}

RVA(0x00013470, 0x4b)
CVoiceTrigger::CVoiceTrigger() {}

RVA(0x000134e0, 0x47)
i32 CVoiceTrigger::SerializeMove(CFileMemBase* ar, i32 tag, i32 c, CGameObject* d) {
    if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
        return 0;
    }
    return Chain(ar, tag, c, d) != 0;
}

// CVoiceTrigger::~CVoiceTrigger @0x0135a0 - the leaf adds no destructible members
// beyond CUserLogic, so its dtor folds the bare CUserLogic teardown: store the
// CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link (the embedded
// ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The destructible
// link forces the /GX EH frame. Byte-identical in shape to
// ~CSecretTeleporterTrigger @0x010ab0; the empty body is enough for cl. It is also
// the out-of-line virtual key function, so cl emits ??_7CVoiceTrigger@@6B@
// (0x5e885c) directly - no manual g_voiceTriggerVtbl extern.
// IMPLICIT dtor (retail is COMPILER-GENERATED - eh-dtor-vptr-restamp CAUSE B):
// a user-declared `~CVoiceTrigger() {}` emits the leaf-vptr restamp, and the CWapX
// base EH state blocks the dead-store elision that used to hide it. The ??_G
// in the vtable-emitting TU forces the implicit ??1 COMDAT; pinned by name.
RVA_COMPGEN(0x00013570, 0x1e, ??_GCVoiceTrigger@@UAEPAXI@Z)
RVA_COMPGEN(0x000135a0, 0x44, ??1CVoiceTrigger@@UAE@XZ)

RVA(0x00119620, 0xf1)
i32 CreateGruntVoice(CGameObject* obj) {
    AnimWorkerObj* ctl = obj->m_7c;
    switch (static_cast<u32>(ctl->ActKey())) {
        case 0: {
            ctl->SetActKey(0x3e8);
            CGruntVoice* t = new CGruntVoice(obj);
            t->Activate();
            ctl->m_logic = t;
            break;
        }
        case 0x1d:
            ctl->m_logic->UserLogicVfunc9();
            break;
        case 0x1e:
            ctl->m_logic->UserLogicVfunc8();
            break;
        case 0x50:
            ctl->m_logic->UserLogicVfuncC();
            break;
        case 0x51:
            ctl->m_logic->UserLogicVfuncB();
            break;
        case 0x52:
            ctl->m_logic->UserLogicVfuncA();
            break;
        case 0x53:
            ctl->m_logic->UserLogicVfuncD();
            break;
        case 0x3e8:
            break;
        default:
            ProjTypeXfer(ctl->m_logic);
            break;
    }
    return 1;
}

RVA(0x00119760, 0xf1)
i32 CreateVoiceTrigger(CGameObject* obj) {
    AnimWorkerObj* ctl = obj->m_7c;
    switch (static_cast<u32>(ctl->ActKey())) {
        case 0: {
            ctl->SetActKey(0x3e8);
            CVoiceTrigger* t = new CVoiceTrigger(obj);
            t->Activate();
            ctl->m_logic = t;
            break;
        }
        case 0x1d:
            ctl->m_logic->UserLogicVfunc9();
            break;
        case 0x1e:
            ctl->m_logic->UserLogicVfunc8();
            break;
        case 0x50:
            ctl->m_logic->UserLogicVfuncC();
            break;
        case 0x51:
            ctl->m_logic->UserLogicVfuncB();
            break;
        case 0x52:
            ctl->m_logic->UserLogicVfuncA();
            break;
        case 0x53:
            ctl->m_logic->UserLogicVfuncD();
            break;
        case 0x3e8:
            break;
        default:
            ProjTypeXfer(ctl->m_logic);
            break;
    }
    return 1;
}

// ===========================================================================
// CGruntVoice::CGruntVoice(CGameObject*)  (0x1198a0)
// ===========================================================================
// The 1-arg game-object ctor: fold the shared CUserLogic(obj) init (base vptr,
// the +0x18 link, the empty-string name, the one-shot logic-type table, the three
// built-in handler registrations, the data fields) - the /GX EH frame the link's
// throwing ctor forces - then run the voice tail: zero the play/icon state, stamp
// the most-derived CGruntVoice vptr (0x5eaf6c, compiler-emitted, reloc-masked),
// register the "GAME_EXCLAMATION" name on the bound object, set the object's
// render flags (m_74 sentinel + the +0x8/+0x40 bitsets), then bind the "A" idle-anim
// bute node (stashing the previous in m_prevAnimSetNode). Returns this. __thiscall, ret 4.
//
// @early-stop
// /GX EH-state + store-scheduling wall: the CUserLogic(obj) fold, the vptr stamp, the
// ApplyName/flag/Find chain and every member offset are byte-faithful, but MSVC5
// schedules the repeated {m_54..m_64}=0 zeroing and the trailing /GX trylevel numbers
// (states 3/4 around ApplyName) differently than retail. Logic complete; the same
// store-schedule + eh-state walls the sibling Setup (0x11a7e0) carries. Final sweep.
RVA(0x001198a0, 0x195)
CGruntVoice::CGruntVoice(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_icon = 0;
    m_5c = 0;
    m_durationMs = 0;
    m_64 = 0;
    m_38->ApplyName("GAME_EXCLAMATION");
    if (m_object->m_sortKey != 0xdbba1) {
        m_object->m_sortKey = 0xdbba1;
        m_object->m_flags |= 0x20000;
    }
    m_sample = 0;
    m_icon = 0;
    m_durationMs = 0;
    m_5c = 0;
    m_64 = 0;
    m_38->m_flags |= 0x4000002;
    m_38->m_stateFlags |= 1;
    m_playFlags = 0;
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = ActFindId("A");
    m_source = 0;
    m_owner = 0;
}

// ===========================================================================
// CGruntVoice::~CGruntVoice  (0x119ae0)
// ===========================================================================
// The leaf adds no destructible members, so its dtor folds the bare CUserLogic
// teardown: store the CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link
// (the embedded ~EngStr call), store the CUserBase vptr (0x5e70b4). The
// destructible link forces the /GX EH frame. The empty body is enough.
// IMPLICIT dtor (retail is COMPILER-GENERATED - eh-dtor-vptr-restamp CAUSE B):
// a user-declared `~CGruntVoice() {}` emits the leaf-vptr restamp, and the CWapX
// base EH state blocks the dead-store elision that used to hide it. The ??_G
// in the vtable-emitting TU forces the implicit ??1 COMDAT; pinned by name.
RVA_COMPGEN(0x00119ab0, 0x1e, ??_GCGruntVoice@@UAEPAXI@Z)
RVA_COMPGEN(0x00119ae0, 0x44, ??1CGruntVoice@@UAE@XZ)

// CVoiceTrigger::CVoiceTrigger(CGameObject*) @0x119b50 - the 1-arg leaf ctor: the
// standard CUserLogic(obj) init (folded inline) plus the voice-trigger tail - cl
// emits the implicit leaf vftable (??_7CVoiceTrigger @0x5e885c) stamp, then raise
// the bound object's logic + pending bits, cache the "A" bute node, snap the bound
// object's screen position to the 0x20 tile grid, then derive the on-screen probe
// rect from the per-side tile spans (m_134..m_140). Constructs a throwing
// CUserBaseLink, so MSVC emits the /GX EH frame.
//
// @early-stop
// EH-state-numbering wall (docs/patterns/eh-state-numbering-base.md): the body is
// byte-faithful (the CUserLogic init, the implicit leaf vptr stamp, the two flag
// RMWs, the "A" cache, the two tile snaps, the four rect derivations); the residue
// is this ctor's own __ehfuncinfo state numbering + the leaf vptr-restamp scheduling
// position (docs/patterns/eh-ctor-vptr-restamp-position.md) + the `and al,0xe0`
// byte-AND codegen pick. The SAME plateau as CTimeBomb / the other bute ctors; not
// source-steerable. Parked for the final sweep.
RVA(0x00119b50, 0x1ce)
CVoiceTrigger::CVoiceTrigger(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_38->m_flags |= 2;
    m_38->m_stateFlags |= 1;
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = ActFindId("A");
    m_object->m_screenX = (m_object->m_screenX & ~0x1f) + 0x10;
    m_object->m_screenY = (m_object->m_screenY & ~0x1f) + 0x10;
    m_object->m_area.left = m_object->m_screenX - (m_object->m_extent.left << 5) - 7;
    m_object->m_area.right = m_object->m_screenX + (m_object->m_extent.right << 5) + 7;
    m_object->m_area.top = m_object->m_screenY - (m_object->m_extent.top << 5) - 7;
    m_object->m_area.bottom = m_object->m_screenY + (m_object->m_extent.bottom << 5) + 7;
}

RVA(0x00119e40, 0x102)
void CGruntVoice::FireActivation(i32 coord) {
    CActHandler* e = VActLookup(coord);
    if ((*e) != 0) {
        CActHandler* e2 = VActLookup(coord);
        (this->*((*e2)))();
    }
}

RVA(0x0011a3a0, 0x102)
void CVoiceTrigger::FireActivation(i32 coord) {
    CActHandler* e = (CActRegPool<CVoiceTrigger>::s_table.ResolveEntry(coord));
    if ((*e) != 0) {
        CActHandler* e2 = (CActRegPool<CVoiceTrigger>::s_table.ResolveEntry(coord));
        (this->*((*e2)))();
    }
}

// CVoiceTrigger::RegisterActs @0x11a500 - bind the per-frame Tick handler to the
// activation key "A" in the trigger's OWN registry (g_vtrigColl). The SAME
// archetype as CParticlez::RegisterActs.
//
// The create path feeds the name-slot lookup the GLOBAL g_typeCounter (not the local
// id copy), and the scratch-slot free loop is the POST-decrement `while (n-- != 0)`
// form - together they are retail's `mov eax,[g_typeCounter]; push eax; mov <id>,eax`
// CSE and its `mov ecx,n; dec eax; test ecx,ecx; je; lea <cnt>,[eax+1]` trip count.
// The old note called this a register-pinning wall; it was a source bug. Now EXACT.
RVA(0x0011a500, 0x18d)
void CVoiceTrigger::RegisterActs() {
    i32 id = ActFindId("A");
    if (id == 0) {
        ActInsertId("A", g_typeCounter);
        id = g_typeCounter;
        CString* slot = ActNameLookup(g_typeCounter);
        i32 n = g_typeColl.m_grown;
        CString* list = ActNameSlots();
        while (n-- != 0) {
            if (list != 0) {
                list->CString::~CString();
            }
            list++;
        }
        *slot = "A";
        g_typeCounter++;
    }
    *(CActRegPool<CVoiceTrigger>::s_table.ResolveEntry(id)) =
        static_cast<i32 (CUserLogic::*)()>(&CVoiceTrigger::Tick);
}

RVA(0x0011a700, 0xae)
i32 CVoiceTrigger::Tick() {
    i32 outA, outB;
    CGrunt* hit = g_gameReg->m_cmdGrid->FindGruntAt(
        m_object->m_screenX,
        m_object->m_screenY,
        &m_object->m_extent,
        &outA,
        &outB,
        &m_object->m_area
    );
    if (hit && outA == g_curPlayer) {
        CGameObject* hs = hit->m_object;
        i32 hy = hs->m_screenY;
        i32 hx = hs->m_screenX;
        if (hx < g_gameReg->m_viewBounds.right && hx >= g_gameReg->m_viewBounds.left
            && hy < g_gameReg->m_viewBounds.bottom && hy >= g_gameReg->m_viewBounds.top) {
            if (g_gameReg->m_cueSink
                    ->SpawnVoiceDriver(hit, m_object->m_124, m_object->m_placeMode, 0, -1, -1)) {
                m_38->m_flags |= 0x10000;
            }
        }
    }
    return 0;
}

// ===========================================================================
// CGruntVoice::Setup  (0x11a7e0)
// ===========================================================================
// Seed the voice from a play request. The `sample` arg (the 2nd, guard) is
// required - on a null sample bail with 0. Otherwise stash the source/sample/
// flag/owner params, snapshot the sample's play duration (sample->ComputeDuration
// 0x137590, __thiscall), seed the icon to the default, clear the running state,
// and swap the +0x14 sub-object's bute node to the "B" key (stashing the
// previous in CUserLogic::m_prevAnimSetNode). Returns 1.
//
// @early-stop
// store-scheduling wall (docs/patterns/statement-schedule-faithful.md): the body
// is structurally byte-exact - every offset, immediate, call arg and branch
// matches retail. The sole residual is a ~2-3 instruction phase shift: MSVC fills
// the latency slot after the ComputeDuration call by hoisting the `m_14` load
// (edx) up, which in turn swaps the adjacent {m_58:=g_frameTime, m_5c:=0}
// stores; retail loads m_14 late (after the icon stores) and keeps m_58 before
// m_5c. No source reorder flips the allocator (m_58/m_5c swap -> 83%, a captured
// m_14 local -> 70%, m_6c hoist -> 83%); the in-order spelling at 85% is the best.
// Deferred to the final sweep.
RVA(0x0011a7e0, 0x6e)
// The three scalars are the members the body snapshots them into, already named in
// <Gruntz/GruntVoice.h>: m_source (+0x68), m_playFlags (+0x6c), m_owner (+0x70).
i32 CGruntVoice::Setup(i32 source, StreamVoice* sample, i32 playFlags, i32 owner) {
    if (sample == 0) {
        return 0;
    }
    m_source = source;
    m_owner = owner;
    m_sample = sample;
    m_durationMs = sample->ComputeRatio();
    m_64 = 0;
    m_icon = g_frameTime;
    m_5c = 0;
    m_playFlags = playFlags;
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = ActFindId("B");
    return 1;
}

RVA(0x0011a870, 0x38)
void CGruntVoice::Reset() {
    m_sample = 0;
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = ActFindId("A");
    m_playFlags = 0;
    m_source = 0;
}

// The act-"A" slot: mark the voice sprite hidden and report "not running". Retail
// 0x11a8c0 is `mov eax,[ecx+0x10]; mov ecx,[eax+0x40]; or ecx,1; mov [eax+0x40],ecx;
// xor eax,eax; ret` - the same stateFlags|=1 the Update stop path takes.
RVA(0x0011a8c0, 0xf)
i32 CGruntVoice::IdleHidden() {
    m_object->m_stateFlags |= 1;
    return 0;
}

// 73.24 -> 85.84 -> 100.00 EXACT. Two shape bugs, both found by the jcc sieve:
// (1) the three "lost the source object" refusals each carried their own
// `m_object->m_stateFlags |= 1; return 0;` block with a full epilogue - retail
// 0x11aa1e has ONE such block that all three reach (bottom `stopped:` label);
// (2) the LAST gate (the m_owner != 0 arm's resolve miss) is spelled POSITIVELY -
// `if (resolved != 0) { ...; return 0; }` - so retail's `cmp eax,edi / je` keeps
// that arm's body inline and puts the lost-source copy after it. The negative
// `if (resolved == 0) goto stopped;` inverted the two blocks.
RVA(0x0011a8e0, 0x198)
i32 CGruntVoice::Update() {
    if (m_sample == 0 || static_cast<i64>(g_frameTime) - m_startStamp.m_v >= m_duration.m_v) {
        m_sample = 0;
        m_source = 0;
        m_object->m_stateFlags |= 1;
        m_prevAnimSetNode = m_objAux->m_1c;
        m_objAux->m_1c = ActFindId("A");
        m_playFlags = 0;
        return 0;
    }
    if (m_owner == 0) {
        CGameObject* out = 0;
        i32 src = m_source;
        CGameObject* resolved;
        if (MapLookupById(g_gameReg->m_world->m_childGroup->m_map48, src, out) == 0) {
            resolved = 0;
        } else if (out == 0) {
            resolved = 0;
        } else {
            resolved = (out->GetClassId() == CLASSID_SERIALREF) ? out : 0;
        }
        if (resolved == 0) {
            goto stopped;
        }
        CUserLogic* logic = resolved->m_7c->m_logic;
        if (logic == 0) {
            goto stopped;
        }
        m_object->m_stateFlags &= ~1;
        m_object->m_screenX = logic->m_object->m_screenX;
        m_object->m_screenY = logic->m_object->m_screenY - 0x32;
    } else {
        CGameObject* out = 0;
        i32 src = m_source;
        CGameObject* resolved;
        if (MapLookupById(g_gameReg->m_world->m_childGroup->m_map48, src, out) == 0) {
            resolved = 0;
        } else if (out == 0) {
            resolved = 0;
        } else {
            resolved = (out->GetClassId() == CLASSID_SERIALREF) ? out : 0;
        }
        // POSITIVE gate here (unlike the two above): retail's `cmp eax,edi / je`
        // keeps this arm's body inline and puts the lost-source copy AFTER it,
        // which only the `if (resolved != 0) { ...; return 0; }` shape gives.
        if (resolved != 0) {
            m_object->m_stateFlags &= ~1;
            i32 dx = 0, dy = 0;
            CImage* layer = static_cast<CWwdGameObjectA*>(resolved)->m_layer;
            if (layer != 0) {
                dx = layer->m_originX;
                dy = layer->m_originY;
            }
            m_object->m_screenX = resolved->m_screenX + dx;
            m_object->m_screenY = resolved->m_screenY + dy - 0x32;
            return 0;
        }
        goto stopped;
    }
    return 0;
    // retail 0x11aa1e: ONE "lost the source object" tail; all three resolve-miss
    // gates in both arms branch into it
stopped:
    m_object->m_stateFlags |= 1;
    return 0;
}

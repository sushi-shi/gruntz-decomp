#include <Mfc.h>              // CMapPtrToPtr (the id->object map, Lookup @0x1b8760)
#include <Gruntz/CurPlayer.h> // g_curPlayer
#include <Gruntz/GruntzMgr.h> // complete CGruntzMgr (g_gameReg real type)
#include <rva.h>

#include <Gruntz/GruntVoice.h>
#include <Gruntz/VoiceTrigger.h>          // canonical CVoiceTrigger : CUserLogic
#include <Gruntz/TileTriggerTransition.h> // CTileTransitionController/State worker-pump view
#include <Gruntz/GameRegistry.h>          // g_gameReg / g_gameReg->m_world->m_childGroup
#include <DDrawMgr/DDrawChildGroup.h>     // CDDrawChildGroup - m_map48 (the id->object map)
#include <Gruntz/TriggerMgr.h> // CTriggerMgr::FindGruntAt (m_cmdGrid @0x75c60, cast-free); typedef CGrunt CTmCell
#include <Gruntz/Grunt.h>            // complete CGrunt - FindGruntAt result (m_object reads)
#include <Gruntz/GruntSpawnConfig.h> // canonical CGruntSpawnConfig (SpawnVoiceDriver @0x11b3b0)
#include <Gruntz/Ufo.h>              // the REAL CUFO (the ex-L_13400 shell is dissolved)
#include <Gruntz/SerialArchive.h> // CSerialArchive (the inherited CWapX::Chain arg; ex SerialObjRef.h)
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/ActName.h> // CActName (shared)
#include <Gruntz/ActReg.h> // CActReg - the ONE registry archetype (subsumes the old per-field globals)
#include <Bute/ButeTree.h>
#include <Dsndmgr/StreamVoice.h>
#include <Wap32/ZVec.h>
#include <Globals.h>
#include <Gruntz/SerialArchive.h> // the serialize stream (== the real CFileMemBase)
#include <Image/CImage.h>         // the +0x198 cached frame (ex CGameObjLayer view)

extern CActReg g_actReg_6514d8; // 0x6514d8 (defined in GruntVoiceActReg.cpp)

VTBL(CVoiceTrigger, 0x001e885c);
VTBL(CGruntVoice, 0x001eaf6c);
DATA(0x00251500)
extern CActReg g_vtrigActReg; // 0x651500 (CVoiceTrigger's own activation registry)

struct CTypeNameEntry; // canonical g_typeColl.m_spare slot record (<Gruntz/TypeNameEntry.h>)

static inline char* ActNameLookup(i32 id) {
    g_typeColl.m_grown = 0;
    if (id >= g_typeColl.m_lo && id <= g_typeColl.m_hi) {
        return reinterpret_cast<char*>(
            (g_typeColl.m_base + (id - g_typeColl.m_lo) * g_typeColl.m_stride)
        );
    }
    if (reinterpret_cast<i32>((static_cast<_zvec*>(&g_typeColl))->GrowTo(id, 0))) {
        return reinterpret_cast<char*>(
            (g_typeColl.m_base + (id - g_typeColl.m_lo) * g_typeColl.m_stride)
        );
    }
    void* item = g_projActCache;
    g_retAddrBreadcrumb = GetRetAddr();
    g_typeColl.m_errSink->Set(&g_typeColl, reinterpret_cast<i32>(item), 0xc);
    return reinterpret_cast<char*>(g_typeColl.m_spare);
}

extern i32 VTrigLogic_11a700();

// @early-stop
// MSVC5 /O2 dead-vptr-store elimination wall (byte-proven). 0x13400 IS CUFO::
// ~CUFO, but retail's /O2 collapsed the CUFO:CPathHazard:CUserLogic dtor chain to a
// ~CUFO @0x13400 (ex the L_13400 flat-leaf shell): retail's dtor stamps ONLY the
// CUserBase vtable - the intermediate CUFO/CPathHazard stamps are dead-store-
// eliminated. An EXPLICIT chained dtor emits them (byte-proven 4.7% crater); the
// IMPLICIT compiler-generated dtor gets the elision (the CDoNothingNormal
// precedent). Nothing else constructs a CUFO in reconstructed code, so this
// unpaired Realize emitter forces the ??_G/??1 COMDATs (the RealizeCDoNothingNormal
// pattern); the ??1 is pinned by name below.
RVA_COMPGEN(0x00013400, 0x44, ??1CUFO@@UAE@XZ)
void RealizeUfoDtor(CUFO* p);
void RealizeUfoDtor(CUFO* p) {
    p->CUFO::~CUFO(); // qualified direct call - odr-uses the implicit ??1CUFO COMDAT
}

RVA(0x00013470, 0x4b)
CVoiceTrigger::CVoiceTrigger() {}

RVA(0x000134e0, 0x47)
i32 CVoiceTrigger::SerializeMove(CGruntArchive* ar, i32 tag, i32 c, i32 d) {
    if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
        return 0;
    }
    return Chain(ar, tag, c, reinterpret_cast<CGameObject*>(d)) != 0;
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
RVA_COMPGEN(0x000135a0, 0x44, ??1CVoiceTrigger@@UAE@XZ)

RVA(0x00119620, 0xf1)
i32 GruntVoiceStep(CGameObject* obj) {
    AnimWorkerObj* ctl = obj->m_7c;
    switch (reinterpret_cast<u32>(ctl->m_1c)) {
        case 0: {
            ctl->m_1c = reinterpret_cast<void*>(0x3e8);
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
i32 VoiceTriggerStep(CGameObject* obj) {
    AnimWorkerObj* ctl = obj->m_7c;
    switch (reinterpret_cast<u32>(ctl->m_1c)) {
        case 0: {
            ctl->m_1c = reinterpret_cast<void*>(0x3e8);
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
    m_objAux->m_1c = g_buteTree.Find("A");
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
    m_objAux->m_1c = g_buteTree.Find("A");
    m_object->m_screenX = (m_object->m_screenX & ~0x1f) + 0x10;
    m_object->m_screenY = (m_object->m_screenY & ~0x1f) + 0x10;
    m_object->m_area.left = m_object->m_screenX - (m_object->m_extent.left << 5) - 7;
    m_object->m_area.right = m_object->m_screenX + (m_object->m_extent.right << 5) + 7;
    m_object->m_area.top = m_object->m_screenY - (m_object->m_extent.top << 5) - 7;
    m_object->m_area.bottom = m_object->m_screenY + (m_object->m_extent.bottom << 5) + 7;
}

RVA(0x00119dc0, 0x15)
void CGruntVoice::InitActReg() {
    g_actReg_6514d8.Construct(2000, 2010);
}

RVA(0x00119e40, 0x102)
void CGruntVoice::FireActivation(i32 coord) {
    CVActEntry* e = VActLookup(coord);
    if (e->m_fn != 0) {
        CVActEntry* e2 = VActLookup(coord);
        (this->*(e2->m_fn))();
    }
}

RVA(0x0011a320, 0x15)
void CVoiceTrigger::InitActReg() {
    g_vtrigActReg.Construct(2000, 2010);
}

RVA(0x0011a3a0, 0x102)
void CVoiceTrigger::FireActivation(i32 coord) {
    CVTrigEntry* e = reinterpret_cast<CVTrigEntry*>(g_vtrigActReg.ResolveEntry(coord));
    if (e->m_fn != 0) {
        CVTrigEntry* e2 = reinterpret_cast<CVTrigEntry*>(g_vtrigActReg.ResolveEntry(coord));
        (this->*(e2->m_fn))();
    }
}

// CVoiceTrigger::RegisterActs @0x11a500 - bind the per-frame Tick handler to the
// activation key "A" in the trigger's OWN registry (g_vtrigColl). The SAME
// archetype as CParticlez::RegisterActs.
//
// @early-stop
// zvec/name-vec IndexToPtr regalloc wall (docs/patterns/zero-register-pinning.md +
// the documented ZVec family): logic + the bute find/insert + the fn-ptr store are
// byte-faithful; cl pins the index/this/base across the grow branches differently
// than retail. Not source-steerable; the SAME plateau as CParticlez::RegisterActs.
RVA(0x0011a500, 0x18d)
void CVoiceTrigger::RegisterActs() {
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
    *reinterpret_cast<void**>(g_vtrigActReg.ResolveEntry(id)) =
        static_cast<void*>(&VTrigLogic_11a700);
}

RVA(0x0011a700, 0xae)
i32 CVoiceTrigger::Tick() {
    i32 outA, outB;
    CTmCell* hit = g_gameReg->m_cmdGrid->FindGruntAt(
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
        if (hx < g_gameReg->m_viewOriginR && hx >= g_gameReg->m_viewOriginL
            && hy < g_gameReg->m_viewOriginB && hy >= g_gameReg->m_viewOriginT) {
            if (g_gameReg->m_cueSink->SpawnVoiceDriver(
                    reinterpret_cast<i32>(hit),
                    m_object->m_124,
                    m_object->m_placeMode,
                    0,
                    -1,
                    -1
                )) {
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
i32 CGruntVoice::Setup(i32 a0, void* sample, i32 a2, i32 a3) {
    if (sample == 0) {
        return 0;
    }
    m_source = a0;
    m_owner = a3;
    m_sample = reinterpret_cast<i32>(sample);
    m_durationMs = (static_cast<StreamVoice*>(sample))->ComputeRatio();
    m_64 = 0;
    m_icon = g_frameTime;
    m_5c = 0;
    m_playFlags = a2;
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("B");
    return 1;
}

RVA(0x0011a870, 0x38)
void CGruntVoice::Reset() {
    m_sample = 0;
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    m_playFlags = 0;
    m_source = 0;
}

RVA(0x0011a8e0, 0x198)
i32 CGruntVoice::Update() {
    if (m_sample == 0
        || static_cast<i64>(g_frameTime) - *reinterpret_cast<i64*>(&m_icon)
               >= *reinterpret_cast<i64*>(&m_durationMs)) {
        m_sample = 0;
        m_source = 0;
        m_object->m_stateFlags |= 1;
        m_prevAnimSetNode = m_objAux->m_1c;
        m_objAux->m_1c = g_buteTree.Find("A");
        m_playFlags = 0;
        return 0;
    }
    if (m_owner == 0) {
        CGameObject* out = 0;
        i32 src = m_source;
        i32 resolved = g_gameReg->m_world->m_childGroup->m_map48.Lookup(
            reinterpret_cast<void*>(src),
            reinterpret_cast<void*&>(out)
        );
        if (resolved != 0) {
            if (out == 0) {
                resolved = 0;
            } else {
                resolved =
                    (out->GetClassId() == CLASSID_SERIALREF) ? reinterpret_cast<i32>(out) : 0;
            }
        }
        if (resolved == 0) {
            m_object->m_stateFlags |= 1;
            return 0;
        }
        CUserLogic* logic = (reinterpret_cast<CGameObject*>(resolved))->m_7c->m_logic;
        if (logic == 0) {
            m_object->m_stateFlags |= 1;
            return 0;
        }
        m_object->m_stateFlags &= ~1;
        m_object->m_screenX = logic->m_object->m_screenX;
        m_object->m_screenY = logic->m_object->m_screenY - 0x32;
    } else {
        CGameObject* out = 0;
        i32 src = m_source;
        i32 resolved = g_gameReg->m_world->m_childGroup->m_map48.Lookup(
            reinterpret_cast<void*>(src),
            reinterpret_cast<void*&>(out)
        );
        if (resolved != 0) {
            if (out == 0) {
                resolved = 0;
            } else {
                resolved =
                    (out->GetClassId() == CLASSID_SERIALREF) ? reinterpret_cast<i32>(out) : 0;
            }
        }
        if (resolved == 0) {
            m_object->m_stateFlags |= 1;
            return 0;
        }
        m_object->m_stateFlags &= ~1;
        i32 dx = 0, dy = 0;
        CImage* layer = (reinterpret_cast<CWwdGameObjectA*>(resolved))->m_layer;
        if (layer != 0) {
            dx = layer->m_originX;
            dy = layer->m_originY;
        }
        m_object->m_screenX = (reinterpret_cast<CGameObject*>(resolved))->m_screenX + dx;
        m_object->m_screenY = (reinterpret_cast<CGameObject*>(resolved))->m_screenY + dy - 0x32;
    }
    return 0;
}

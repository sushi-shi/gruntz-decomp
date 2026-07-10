// VoiceTrigger.cpp - the voice-trigger game-object (C:\Proj\Gruntz).
//
// Two trace-discovered CVoiceTrigger methods, defined in ascending retail-RVA
// order:
//   ~CVoiceTrigger @0x0135a0 - the /GX leaf dtor (folds the CUserLogic teardown).
//   Tick           @0x11a700 - the per-frame "play the voice cue when on screen" probe.
//
// CVoiceTrigger : CUserLogic. The class also has its no-arg ctor in
// src/Gruntz/UserLogic.cpp; this TU adds the out-of-line dtor copy + the Tick
// method. A minimal local class redeclaration is enough (the dtor fold depends
// only on the CUserLogic base hierarchy from <Gruntz/UserLogic.h>, and Tick is a
// plain __thiscall member whose codegen depends only on its body + offsets).
// Only offsets / code bytes are load-bearing; names are placeholders.
#include <Gruntz/VoiceTrigger.h>          // canonical CVoiceTrigger : CUserLogic
#include <Gruntz/TileTriggerTransition.h> // CTileTransitionController/State worker-pump view
#include <Gruntz/BoundaryLeafLogicViews.h> // L_13400 (CUFO fold-flat leaf dtor, RVA-homed here)
#include <Gruntz/SerialObjRef.h> // CSerialObjRef::Chain (0x8c00) - the +0x34 sub-object round-trip
#include <Wap32/ZVec.h>
#include <Wap32/ZDArrayDerived.h>
#include <Gruntz/TypeKeyColl.h>
#include <Bute/ButeTree.h>
#include <Gruntz/GameRegistry.h>
#include <Globals.h>

// CVoiceTrigger : CUserLogic - the canonical class shape now lives in
// <Gruntz/VoiceTrigger.h> (shared with the no-arg ctor + GetTypeTag in UserLogic.cpp).

// The bound object is the inherited CUserLogic m_10/m_38 (CGameObject*); the ctor
// and Tick use them directly (the CTeleporter idiom - no per-TU view cast).
// CGameObject models the touched fields: +0x08 flag word, +0x40 pending bit, the
// tile-config bound counts at +0x134..+0x140 (the SAME shape CExitTrigger/
// CCheckpointTrigger seed), the derived screen-rect bounds at +0x144..+0x150, the
// screen pos +0x5c/+0x60, and the voice-cue ids +0x124/+0x128.

// ---------------------------------------------------------------------------
// The activation registry CVoiceTrigger::RegisterActs (0x11a500) binds into - the
// trigger's OWN instance at 0x651500 (the SAME range/cache shape as every
// FireActivation registry: g_vtrigColl base + the lo/hi/base/stride/cur/scratch
// fields). The slow path Finds (0x16da80), and on miss rebuilds (GetRetAddr 0x16d990
// -> g_actCache, Insert 0x16d850) yielding g_vtrigCur. All BSS globals DATA-pinned
// so the loads reloc-mask; the collection methods are external/no-body.
// ---------------------------------------------------------------------------
struct CVTrigEntry;        // an entry: first dword is the registered handler
extern void* GetRetAddr(); // 0x16d990

DATA(0x00251500)
extern CTypeKeyColl g_vtrigColl;
DATA(0x002bf464)
extern void* g_actCache;
extern void* g_retAddrBreadcrumb;

struct CVTrigEntry {
    void (CVoiceTrigger::*m_fn)(); // [entry]
};
SIZE_UNKNOWN(CVTrigEntry);

// The inlined coordinate->Entry* lookup the registration resolves the slot with.
static inline CVTrigEntry* VTrigLookup(i32 coord) {
    g_vtrigScratch = 0;
    if (coord >= g_vtrigLo && coord <= g_vtrigHi) {
        return (CVTrigEntry*)(g_vtrigBase + (coord - g_vtrigLo) * g_vtrigStride);
    }
    if (g_vtrigColl.Find(coord, 0)) {
        return (CVTrigEntry*)(g_vtrigBase + (coord - g_vtrigLo) * g_vtrigStride);
    }
    void* item = g_actCache;
    g_retAddrBreadcrumb = GetRetAddr();
    g_vtrigColl2->Set(&g_vtrigColl, (i32)item, 0xc);
    return g_vtrigCur;
}

// The shared activation-NAME registry (the first block interns "A"). g_buteTree
// (0x6bf620, mangled-named) doubles as the name->id map; g_nextActId (0x61aea8)
// is the running id counter; s_actKeyA (0x60a454) is the "A" key; the scratch
// name registry is @0x6bf650 (same shape as g_vtrigColl).
DATA(0x0021aea8)
extern i32 g_nextActId;
DATA(0x0020a454)
extern char s_actKeyA[];
DATA(0x002bf650)
extern CTypeKeyColl g_nameReg; // 0x6bf650
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
extern char* g_nameRegCur;
DATA(0x002bf66c)
extern void** g_nameRegCurList;
DATA(0x002bf670)
extern i32 g_nameRegScratch;

extern CButeTree g_buteTree; // ?g_buteTree@@3VCButeTree@@A @0x6bf620

#include <Gruntz/ActName.h> // CActName (shared)
#include <Wap32/ZVec.h>
#include <Wap32/ZDArrayDerived.h>

static inline char* ActNameLookup(i32 id) {
    g_nameRegScratch = 0;
    if (id >= g_nameRegLo && id <= g_nameRegHi) {
        return g_nameRegBase + (id - g_nameRegLo) * g_nameRegStride;
    }
    if (g_nameReg.Find(id, 0)) {
        return g_nameRegBase + (id - g_nameRegLo) * g_nameRegStride;
    }
    void* item = g_actCache;
    g_retAddrBreadcrumb = GetRetAddr();
    g_nameReg2->Set(&g_nameReg, (i32)item, 0xc);
    return g_nameRegCur;
}

// The logic handler bound into the slot (the ILT to CVoiceTrigger::Tick @0x11a700);
// referenced by address so the DIR32 operand reloc-masks.
extern i32 VTrigLogic_11a700();

// The on-screen-cue receiver (g_gameReg->m_68). QueryAt (0x75c60, via the 0x32ce
// thunk) resolves the entity whose screen rect overlaps the trigger; CueA
// (0x11b3b0, via the 0x39f4 thunk) fires the voice cue on it. Both __thiscall,
// modeled NO-body so the calls reloc-mask.
struct CVoiceHit; // the entity QueryAt returns
namespace m4 {
    struct HitGrunt;
    struct HitTileRect;
    class GruntHitMgr {
    public:
        HitGrunt* FindGruntAt(i32 x, i32 y, HitTileRect* r, i32* a, i32* b, struct tagRECT* rect);
    };
} // namespace m4
class CGruntSpawnConfig {
public:
    i32 SpawnVoiceDriver(
        i32 a,
        i32 b,
        i32 c,
        i32 d,
        i32 e,
        i32 f
    ); // real @0x11b3b0 = 6 args (ret 0x18)
};
struct CVoiceSink {
    // QueryAt(x, y, &m_object->m_134, &outA, &outB, &m_object->m_144) -> entity* (or 0).
    // QueryAt IS m4::GruntHitMgr::FindGruntAt; cast at the call.
    // CueA(hit, m_object->m_124, m_object->m_128, 0, -1, -1) -> nonzero on fire.
    // CueA IS CGruntSpawnConfig::SpawnVoiceDriver (padded); cast at the call.
};
SIZE_UNKNOWN(CVoiceSink);

// The entity QueryAt returns: its bound sprite (+0x10) carries the screen x/y
// (+0x5c/+0x60) the on-screen window test reads.
struct CVoiceHitSprite {
    char m_pad0[0x5c];
    i32 m_screenX; // +0x5c screen x
    i32 m_screenY; // +0x60 screen y
};
SIZE_UNKNOWN(CVoiceHitSprite);
struct CVoiceHit {
    char m_pad0[0x10];
    CVoiceHitSprite* m_sprite; // +0x10 bound sprite
};
SIZE_UNKNOWN(CVoiceHit);

// Tick reads the bound object (m_10, CGameObject*) directly: +0x5c/+0x60 screen
// x/y, +0x124/+0x128 the voice-cue ids passed to CueA, +0x134/+0x144 the probe rect
// bounds; the on-screen window (m_38)'s +0x08 status bits carry bit 0x10000
// ("fired / handled this frame"). All modeled on CGameObject (<Gruntz/UserLogic.h>).

// The global game registry (CGameRegistry, RVA 0x24556c; wwdfile owns the DATA
// label). The on-screen window bounds are at +0x13c/+0x140/+0x144/+0x148; the
// cue receiver at +0x60 (CueA's `this`) and the probe sink at +0x68 (QueryAt's
// `this`).
DATA(0x0024556c)
extern CGameRegistry* g_gameReg;

// The current-area index (DAT_00644c54, VA 0x644c54 / RVA 0x244c54); the trigger
// only fires for the active area. extern "C" so the load reloc-masks against the
// already-named global.
extern "C" i32 g_644c54;

// @early-stop
// @flag: MSVC5 /O2 dead-vptr-store elimination wall (byte-proven). 0x13400 IS CUFO::
// ~CUFO, but retail's /O2 collapsed the CUFO:CPathHazard:CUserLogic dtor chain to a
// FLAT CUserLogic teardown - it stamps ONLY 0x5e705c (CUserLogic vtable) then 0x5e70b4
// (CUserBase vtable), never CUFO's (0x5e72b4) or CPathHazard's vptr (the intermediate
// stamps are dead stores, eliminated). The real CUFO:CPathHazard model (Ufo.h) emits
// those intermediate stamps -> byte-proven crater to 4.7%. This flat `: CUserLogic`
// L_13400 model is the SAME faithful shape as the sibling CPathHazard leaf. 100%
// byte-exact. A future pass could rename to CUFO by remodeling Ufo.h as a flat leaf.
RVA(0x00013400, 0x44)
L_13400::~L_13400() {}

// --- CVoiceTrigger no-arg ctor (0x013470) --- the deserialize-path ctor: base
// prologue + link + leaf vptr stamp (the empty body is enough for cl). It anchors
// GetTypeTag @0x133b0 in this TU alongside the class's dtor/Serialize band.
RVA(0x00013470, 0x4b)
CVoiceTrigger::CVoiceTrigger() {}

// CVoiceTrigger::Serialize @0x0134e0 - the vtable slot-1 override: chain the shared
// CUserLogic serialize helper on `this`, and (only on success) the +0x34 serializable
// sub-object's chain; normalize the second chain's success to a strict bool. The
// byte-identical chain-Serialize archetype (differs only in the two call displacements).
RVA(0x000134e0, 0x47)
i32 CVoiceTrigger::Serialize(i32 ar, i32 tag, i32 c, i32 d) {
    if (!SerializeChain(ar, tag, c, d)) {
        return 0;
    }
    return ((CSerialObjRef*)&m_34)->Chain((CSerialArchive*)ar, tag, c, (CSerialObj*)d) != 0;
}

// CVoiceTrigger::~CVoiceTrigger @0x0135a0 - the leaf adds no destructible members
// beyond CUserLogic, so its dtor folds the bare CUserLogic teardown: store the
// CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link (the embedded
// ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The destructible
// link forces the /GX EH frame. Byte-identical in shape to
// ~CSecretTeleporterTrigger @0x010ab0; the empty body is enough for cl. It is also
// the out-of-line virtual key function, so cl emits ??_7CVoiceTrigger@@6B@
// (0x5e885c) directly - no manual g_voiceTriggerVtbl extern.
RVA(0x000135a0, 0x44)
CVoiceTrigger::~CVoiceTrigger() {}

// VoiceTriggerStep @0x119760 - the CVoiceTrigger worker-pump (free __cdecl, /GX):
// the controller lives at obj->m_7c; dispatch on its state id, building the
// CVoiceTrigger state object on state 0 and dispatching to the state object's vtable
// slots otherwise. Byte-identical to StepController @0x10d150 bar the leaf `new`d on
// state 0 (CVoiceTrigger is 0x54).
RVA(0x00119760, 0xf1)
i32 VoiceTriggerStep(CGameObject* obj) {
    CTileTransitionController* ctl = (CTileTransitionController*)obj->m_7c;
    switch (ctl->m_stateId) {
        case 0: {
            ctl->m_stateId = 0x3e8;
            CVoiceTrigger* t = new CVoiceTrigger(obj);
            ((CTileTransitionState*)t)->Activate();
            ctl->m_state = (CTileTransitionState*)t;
            break;
        }
        case 0x1d:
            ctl->m_state->Vfunc2C();
            break;
        case 0x1e:
            ctl->m_state->Vfunc28();
            break;
        case 0x50:
            ctl->m_state->Vfunc38();
            break;
        case 0x51:
            ctl->m_state->Vfunc34();
            break;
        case 0x52:
            ctl->m_state->Vfunc30();
            break;
        case 0x53:
            ctl->m_state->Vfunc3C();
            break;
        case 0x3e8:
            break;
        default:
            TileTransitionDefaultStep(ctl->m_state);
            break;
    }
    return 1;
}

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
CVoiceTrigger::CVoiceTrigger(CGameObject* obj) : CUserLogic(obj) {
    TILE_LOGIC_SEED(obj);
    m_38->m_flags |= 2;
    m_38->m_stateFlags |= 1;
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find(s_actKeyA);
    m_object->m_screenX = (m_object->m_screenX & ~0x1f) + 0x10;
    m_object->m_screenY = (m_object->m_screenY & ~0x1f) + 0x10;
    m_object->m_areaL = m_object->m_screenX - (m_object->m_extentL << 5) - 7;
    m_object->m_areaR = m_object->m_screenX + (m_object->m_extentR << 5) + 7;
    m_object->m_areaT = m_object->m_screenY - (m_object->m_extentT << 5) - 7;
    m_object->m_areaB = m_object->m_screenY + (m_object->m_extentB << 5) + 7;
}

// CVoiceTrigger::InitActReg @0x11a320 - construct the trigger's OWN activation-
// coordinate registry singleton (g_vtrigColl @0x651500) over the fixed range
// [2000, 2010] via the shared registry ctor (0x408710). Free init thunk.
RVA(0x0011a320, 0x15)
void CVoiceTrigger::InitActReg() {
    ((CZDArrayDerived*)&g_vtrigColl)->Construct(2000, 2010);
}

// CVoiceTrigger::FireActivation @0x11a3a0 - vtable slot 4. Look the activation
// coordinate up in the trigger's OWN registry (g_vtrigColl, via VTrigLookup); if the
// resolved entry carries a registered handler PMF, resolve it again and dispatch it
// __thiscall on `this`. Same double-lookup + PMF-dispatch archetype as
// CSecretTeleporterTrigger::FireActivation.
RVA(0x0011a3a0, 0x102)
void CVoiceTrigger::FireActivation(i32 coord) {
    CVTrigEntry* e = VTrigLookup(coord);
    if (e->m_fn != 0) {
        CVTrigEntry* e2 = VTrigLookup(coord);
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
    *(void**)VTrigLookup(id) = (void*)&VTrigLogic_11a700;
}

// CVoiceTrigger::Tick @0x11a700 - query the entity under the trigger's screen
// rect; if it is in the active area and its sprite sits inside the on-screen
// window, fire the voice cue and mark the window handled this frame. Always
// returns 0.
RVA(0x0011a700, 0xae)
i32 CVoiceTrigger::Tick() {
    i32 outA, outB;
    CVoiceHit* hit = (CVoiceHit*)((m4::GruntHitMgr*)g_gameReg->m_cmdGrid)
                         ->FindGruntAt(
                             m_object->m_screenX,
                             m_object->m_screenY,
                             (m4::HitTileRect*)&m_object->m_extentL,
                             &outA,
                             &outB,
                             (struct tagRECT*)&m_object->m_areaL
                         );
    if (hit && outA == g_644c54) {
        CVoiceHitSprite* hs = hit->m_sprite;
        i32 hy = hs->m_screenY;
        i32 hx = hs->m_screenX;
        if (hx < g_gameReg->m_viewOriginR && hx >= g_gameReg->m_viewOriginL
            && hy < g_gameReg->m_viewOriginB && hy >= g_gameReg->m_viewOriginT) {
            if (((CGruntSpawnConfig*)g_gameReg->m_cueSink)
                    ->SpawnVoiceDriver(
                        (i32)hit,
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

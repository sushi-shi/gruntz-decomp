// GruntVoice.cpp - the voice TU (C:\Proj\Gruntz): the WOVEN original obj at
// retail .text [0x119620 .. 0x11aa78] (TU_MIGRATION interval 0x119620, weave
// 0.45). The two former units gruntvoice + voicetrigger interleave function-by-
// function throughout the interval (Step/ctor/dtor/InitActReg/Dispatch of BOTH
// classes alternate) - impossible across objs at first link => ONE original TU
// (wave2-F merge). Classes: CGruntVoice + CVoiceTrigger (both CUserLogic-derived
// voice leaves; twin worker-pumps, twin activation registries). The low-RVA
// CVoiceTrigger band (0x13400..0x135a0: L_13400 + no-arg ctor + Serialize +
// ~dtor) is the boundary orphan-COMDAT pool emission - kept, ascending.
//
// Only offsets / code bytes are load-bearing; names are placeholders.
#include <Mfc.h>                    // CMapPtrToPtr (the id->object map, Lookup @0x1b8760)
#include <Gruntz/MovingLogicBase.h> // CMovingLogicBase::Serialize (0x16e7f0) - shared serialize chain
#include <rva.h>

#include <Gruntz/GruntVoice.h>
#include <Gruntz/VoiceTrigger.h>          // canonical CVoiceTrigger : CUserLogic
#include <Gruntz/TileTriggerTransition.h> // CTileTransitionController/State worker-pump view
#include <Gruntz/GameRegistry.h>          // g_gameReg / g_gameReg->m_world->m_8
#include <Gruntz/TriggerMgr.h> // CTriggerMgr::FindGruntAt (m_cmdGrid @0x75c60, cast-free)
#include <Gruntz/BoundaryLeafLogicViews.h> // L_13400 (CUFO fold-flat leaf dtor, RVA-homed here)
#include <Gruntz/SerialObjRef.h> // CSerialObjRef::Chain (0x8c00) - the +0x34 sub-object round-trip
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/ActName.h> // CActName (shared)
#include <Gruntz/ActReg.h> // CActReg - the ONE registry archetype (subsumes the old per-field globals)
#include <Bute/ButeTree.h>
#include <Dsndmgr/StreamVoice.h>
#include <Wap32/ZVec.h>
#include <Wap32/ZDArrayDerived.h>
#include <Globals.h>
#include <Gruntz/SerialArchive.h> // the serialize stream (== the real CFileMemBase)

// The global running game clock (DAT_00645588); the value-load reloc-masks.
DATA(0x00245588)
extern "C" u32 g_frameTime;

// Reloc-fidelity bindings for the registry statics whose plain externs live in
// GruntVoice.h (labels.py does not scan a header's DATA(), so they are bound here
// in the owning .cpp - same header-extern/cpp-DATA split as Globals). The shared
// alloc-scratch cache is the canonical g_projActCache @0x2bf464 (the old
// g_actCache spelling was an unbound VA-typo alias).
// (The "A" bute key @0x20a454 is the canonical s_codeA, bound in toobspikez and
// declared below; the former per-TU g_voiceKeyA alias lost the per-rva dedup and is
// folded away - all uses now reference s_codeA.)
// INTERIOR-OFFSET TRAP, removed 2026-07-13. This TU used to declare EIGHT separate globals
// here - g_vactColl (0x6514d8), g_vactColl2 (+4), g_vactLo (+8), g_vactHi (+0xc), g_vactBase
// (+0x10), g_vactCur (+0x14), g_vactStride (+0x18), g_vactScratch (+0x20) - and it DEFINED
// one of them (g_vactColl2) outright. They are not globals: those are the FIELDS of the one
// CActReg at 0x6514d8, whose offsets they reproduce exactly (m_coll2/+4, m_lo/+8, m_hi/+0xc,
// m_base/+0x10, m_cur/+0x14, m_stride/+0x18, m_scratch/+0x20 - see <Gruntz/ActReg.h>). The
// same object is already DATA-pinned and DEFINED as `CActReg g_actReg_6514d8` in
// GruntVoiceActReg.cpp, so 0x2514d8 carried TWO competing DATA() claims under two names.
// Subsumed: one object, one definition, one name; the per-field scalars are gone.
extern CActReg g_actReg_6514d8; // 0x6514d8 (defined in GruntVoiceActReg.cpp)
extern void* g_projActCache; // 0x2bf464 (?g_projActCache@@3PAXA), bound in GruntStartingPoint.cpp

// The global game/manager registry singleton (*0x64556c; _g_mgrSettings - the C
// alias of g_gameReg below; the 0x24556c DATA binding lives on the C++ name).
extern "C" CGameRegistry* g_gameReg;

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

struct CVTrigEntry {
    void (CVoiceTrigger::*m_fn)(); // [entry]
};
SIZE_UNKNOWN(CVTrigEntry);

// SAME INTERIOR-OFFSET TRAP as the 0x6514d8 block above, and the old comment right here
// spelled the shape out without drawing the conclusion: "the SAME range/cache shape as every
// FireActivation registry: g_vtrigColl base + the lo/hi/base/stride/cur/scratch fields".
// That IS <Gruntz/ActReg.h>'s CActReg. The seven globals this TU used to DEFINE
// (g_vtrigColl2 +4, g_vtrigLo +8, g_vtrigHi +0xc, g_vtrigBase +0x10, g_vtrigCur +0x14,
// g_vtrigStride +0x18, g_vtrigScratch +0x20) are its FIELDS, and VTrigLookup was
// CActReg::ResolveEntry hand-inlined over them - identical statement for statement.
// One object, one definition. The registry the voice trigger registers into is the CActReg
// at 0x651500; nothing else was ever there.
DATA(0x00251500)
CActReg g_vtrigActReg; // 0x651500 (CVoiceTrigger's own activation registry)

// The shared activation-NAME registry (the first block interns "A"). g_buteTree
// (0x6bf620, mangled-named) doubles as the name->id map; g_typeCounter (0x61aea8)
// is the running id counter; s_codeA (0x60a454) is the "A" key; the scratch
// name registry is @0x6bf650 (same shape as g_vtrigColl).
DATA(0x0021aea8)
extern i32 g_typeCounter;
// s_codeA is the "A" key byte-array @0x60a454 (RVA 0x20a454); the DATA binding lives
// in toobspikez (?s_codeA@@3PADA), so this is a plain extern here.
extern char s_codeA[];
struct CTypeNameEntry; // canonical g_typeColl.m_spare slot record (<Gruntz/TypeNameEntry.h>)
DATA(0x002bf650)
extern CTypeKeyColl g_typeColl; // 0x6bf650

extern CButeTree g_buteTree; // ?g_buteTree@@3VCButeTree@@A @0x6bf620

static inline char* ActNameLookup(i32 id) {
    g_typeColl.m_grown = 0;
    if (id >= g_typeColl.m_lo && id <= g_typeColl.m_hi) {
        return (char*)(g_typeColl.m_base + (id - g_typeColl.m_lo) * g_typeColl.m_stride);
    }
    if ((i32)((_zvec*)&g_typeColl)->GrowTo(id, 0)) {
        return (char*)(g_typeColl.m_base + (id - g_typeColl.m_lo) * g_typeColl.m_stride);
    }
    void* item = g_projActCache;
    g_retAddrBreadcrumb = GetRetAddr();
    g_typeColl.m_errSink->Set(&g_typeColl, (i32)item, 0xc);
    return (char*)g_typeColl.m_spare;
}

// The logic handler bound into the slot (the ILT to CVoiceTrigger::Tick @0x11a700);
// referenced by address so the DIR32 operand reloc-masks.
extern i32 VTrigLogic_11a700();

// The on-screen-cue receiver (g_gameReg->m_68). QueryAt (0x75c60, via the 0x32ce
// thunk) resolves the entity whose screen rect overlaps the trigger; CueA
// (0x11b3b0, via the 0x39f4 thunk) fires the voice cue on it. Both __thiscall,
// modeled NO-body so the calls reloc-mask.
struct CVoiceHit; // the entity FindGruntAt returns (a reduced view of CTmCell)
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
    // FindGruntAt(x, y, &m_object->m_134, &outA, &outB, &m_object->m_144) -> entity* (or 0).
    // The probe IS CTriggerMgr::FindGruntAt (m_cmdGrid), called cast-free.
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
extern "C" CGameRegistry* g_gameReg;

// The current-area index (DAT_00644c54, VA 0x644c54 / RVA 0x244c54); the trigger
// only fires for the active area. extern "C" so the load reloc-masks against the
// already-named global.
extern "C" i32 g_curPlayer;

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
    if (!((CMovingLogicBase*)this)->Serialize((CSerialArchive*)(ar), tag, c, d)) {
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

// GruntVoiceStep @0x119620 - the CGruntVoice worker-pump (free __cdecl, /GX): the
// controller lives at obj->m_7c; dispatch on its state id, building the CGruntVoice
// state object on state 0 and dispatching to the state object's vtable slots otherwise.
// Byte-identical to StepController @0x10d150 bar the leaf `new`d on state 0 (CGruntVoice
// is 0x78).
RVA(0x00119620, 0xf1)
i32 GruntVoiceStep(CGameObject* obj) {
    AnimWorkerObj* ctl = obj->m_7c;
    switch ((u32)ctl->m_1c) {
        case 0: {
            ctl->m_1c = (void*)0x3e8;
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
            ProjTypeXfer((CXferArchive*)ctl->m_logic);
            break;
    }
    return 1;
}

// VoiceTriggerStep @0x119760 - the CVoiceTrigger worker-pump (free __cdecl, /GX):
// the controller lives at obj->m_7c; dispatch on its state id, building the
// CVoiceTrigger state object on state 0 and dispatching to the state object's vtable
// slots otherwise. Byte-identical to StepController @0x10d150 bar the leaf `new`d on
// state 0 (CVoiceTrigger is 0x54).
RVA(0x00119760, 0xf1)
i32 VoiceTriggerStep(CGameObject* obj) {
    AnimWorkerObj* ctl = obj->m_7c;
    switch ((u32)ctl->m_1c) {
        case 0: {
            ctl->m_1c = (void*)0x3e8;
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
            ProjTypeXfer((CXferArchive*)ctl->m_logic);
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
CGruntVoice::CGruntVoice(CGameObject* obj) : CUserLogic(obj) {
    TILE_LOGIC_SEED(obj);
    m_icon = 0;
    m_5c = 0;
    m_durationMs = 0;
    m_64 = 0;
    m_38->ApplyName("GAME_EXCLAMATION");
    if (m_object->m_latchedAnimId != 0xdbba1) {
        m_object->m_latchedAnimId = 0xdbba1;
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
    m_objAux->m_1c = g_buteTree.Find(s_codeA);
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
RVA(0x00119ae0, 0x44)
CGruntVoice::~CGruntVoice() {}

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
    m_objAux->m_1c = g_buteTree.Find(s_codeA);
    m_object->m_screenX = (m_object->m_screenX & ~0x1f) + 0x10;
    m_object->m_screenY = (m_object->m_screenY & ~0x1f) + 0x10;
    m_object->m_areaL = m_object->m_screenX - (m_object->m_extentL << 5) - 7;
    m_object->m_areaR = m_object->m_screenX + (m_object->m_extentR << 5) + 7;
    m_object->m_areaT = m_object->m_screenY - (m_object->m_extentT << 5) - 7;
    m_object->m_areaB = m_object->m_screenY + (m_object->m_extentB << 5) + 7;
}

// ===========================================================================
// CGruntVoice::InitActReg  (0x119dc0)
// ===========================================================================
// Construct the class's activation-coordinate registry singleton (g_vactColl
// @0x6514d8) over the fixed range [2000, 2010] via the shared registry ctor
// (FUN_00408710, __thiscall ret 8). A free init thunk (no `this`); reloc-masked.
RVA(0x00119dc0, 0x15)
void CGruntVoice::InitActReg() {
    ((CZDArrayDerived*)&g_actReg_6514d8)->Construct(2000, 2010);
}

// ===========================================================================
// CGruntVoice::Dispatch  (0x119e40)
// ===========================================================================
// The per-coordinate activation registry dispatch - the SAME ActLookup/
// FireActivation shape as CSecretTeleporterTrigger::FireActivation (0x042150),
// on CGruntVoice's OWN registry statics (0x6514xx). Look the coordinate up to an
// Entry*; if its handler slot is set, look it up again and invoke the handler
// __thiscall on `this`.
RVA(0x00119e40, 0x102)
void CGruntVoice::Dispatch(i32 coord) {
    CVActEntry* e = VActLookup(coord);
    if (e->m_fn != 0) {
        CVActEntry* e2 = VActLookup(coord);
        (this->*(e2->m_fn))();
    }
}

// CVoiceTrigger::InitActReg @0x11a320 - construct the trigger's OWN activation-
// coordinate registry singleton (g_vtrigColl @0x651500) over the fixed range
// [2000, 2010] via the shared registry ctor (0x408710). Free init thunk.
RVA(0x0011a320, 0x15)
void CVoiceTrigger::InitActReg() {
    ((CZDArrayDerived*)&g_vtrigActReg)->Construct(2000, 2010);
}

// CVoiceTrigger::FireActivation @0x11a3a0 - vtable slot 4. Look the activation
// coordinate up in the trigger's OWN registry (g_vtrigColl, via VTrigLookup); if the
// resolved entry carries a registered handler PMF, resolve it again and dispatch it
// __thiscall on `this`. Same double-lookup + PMF-dispatch archetype as
// CSecretTeleporterTrigger::FireActivation.
RVA(0x0011a3a0, 0x102)
void CVoiceTrigger::FireActivation(i32 coord) {
    CVTrigEntry* e = (CVTrigEntry*)g_vtrigActReg.ResolveEntry(coord);
    if (e->m_fn != 0) {
        CVTrigEntry* e2 = (CVTrigEntry*)g_vtrigActReg.ResolveEntry(coord);
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
    *(void**)g_vtrigActReg.ResolveEntry(id) = (void*)&VTrigLogic_11a700;
}

// CVoiceTrigger::Tick @0x11a700 - query the entity under the trigger's screen
// rect; if it is in the active area and its sprite sits inside the on-screen
// window, fire the voice cue and mark the window handled this frame. Always
// returns 0.
RVA(0x0011a700, 0xae)
i32 CVoiceTrigger::Tick() {
    i32 outA, outB;
    CVoiceHit* hit = (CVoiceHit*)g_gameReg->m_cmdGrid->FindGruntAt(
        m_object->m_screenX,
        m_object->m_screenY,
        (RECT*)&m_object->m_extentL,
        &outA,
        &outB,
        (RECT*)&m_object->m_areaL
    );
    if (hit && outA == g_curPlayer) {
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
// (edx) up, which in turn swaps the adjacent {m_58:=g_iconDefault, m_5c:=0}
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
    m_sample = (i32)sample;
    m_durationMs = ((StreamVoice*)sample)->ComputeRatio();
    m_64 = 0;
    m_icon = g_iconDefault;
    m_5c = 0;
    m_playFlags = a2;
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find(g_iconBute);
    return 1;
}

// ===========================================================================
// CGruntVoice::Reset  (0x11a870)
// ===========================================================================
// Clear the voice: zero the sample slot (+0x54), swap the +0x14 sub-object's
// bute node to the "A" idle-anim key (stashing the previous in CUserLogic::m_prevAnimSetNode),
// then clear the flag/owner slots (+0x6c/+0x68).
RVA(0x0011a870, 0x38)
void CGruntVoice::Reset() {
    m_sample = 0;
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find(s_codeA);
    m_playFlags = 0;
    m_source = 0;
}

// ===========================================================================
// CGruntVoice::Update  (0x11a8e0)
// ===========================================================================
// The per-frame voice-bubble handler. If the voice is inactive (m_sample == 0) or
// its timed-play window has elapsed (game clock - the i64 start @+0x58 >= the i64
// duration @+0x60), reset to the idle "A" pose (the inlined Reset). Otherwise resolve
// the play's source object by its cookie (m_source) through the object factory's
// id->object map (g_gameReg->m_world->m_8 + 0x48, an MFC CMapPtrToPtr, keeping it
// only when its type tag == 5) and reposition the bound bubble object over it: when a
// carrier (m_owner) is set, follow the resolved object's bound logic leaf's object;
// otherwise offset by the resolved object's layer scroll. On a miss/wrong-type, mark
// the bubble object visible (stalled this frame). Always returns 0. The map-lookup +
// branchless `(GetTypeId()==5)?obj:0` resolve is the SAME idiom as
// CSpotLight::SerializeMove's Read path.
RVA(0x0011a8e0, 0x198)
i32 CGruntVoice::Update() {
    if (m_sample == 0 || (i64)g_frameTime - *(i64*)&m_icon >= *(i64*)&m_durationMs) {
        m_sample = 0;
        m_source = 0;
        m_object->m_stateFlags |= 1;
        m_prevAnimSetNode = m_objAux->m_1c;
        m_objAux->m_1c = g_buteTree.Find(s_codeA);
        m_playFlags = 0;
        return 0;
    }
    if (m_owner == 0) {
        CGameObject* out = 0;
        i32 src = m_source;
        i32 resolved = ((CMapPtrToPtr*)((char*)g_gameReg->m_world->m_8 + 0x48))
                           ->Lookup((void*)src, (void*&)out);
        if (resolved != 0) {
            if (out == 0) {
                resolved = 0;
            } else {
                resolved = (out->GetTypeId() == 5) ? (i32)out : 0;
            }
        }
        if (resolved == 0) {
            m_object->m_stateFlags |= 1;
            return 0;
        }
        CUserLogic* logic = ((CGameObject*)resolved)->m_7c->m_logic;
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
        i32 resolved = ((CMapPtrToPtr*)((char*)g_gameReg->m_world->m_8 + 0x48))
                           ->Lookup((void*)src, (void*&)out);
        if (resolved != 0) {
            if (out == 0) {
                resolved = 0;
            } else {
                resolved = (out->GetTypeId() == 5) ? (i32)out : 0;
            }
        }
        if (resolved == 0) {
            m_object->m_stateFlags |= 1;
            return 0;
        }
        m_object->m_stateFlags &= ~1;
        i32 dx = 0, dy = 0;
        CGameObjLayer* layer = ((CGameObject*)resolved)->m_layer;
        if (layer != 0) {
            dx = layer->m_20;
            dy = layer->m_24;
        }
        m_object->m_screenX = ((CGameObject*)resolved)->m_screenX + dx;
        m_object->m_screenY = ((CGameObject*)resolved)->m_screenY + dy - 0x32;
    }
    return 0;
}

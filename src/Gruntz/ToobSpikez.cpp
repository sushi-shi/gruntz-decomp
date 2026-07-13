// ToobSpikez.cpp - the toob-spikez hazard game-object (C:\Proj\Gruntz).
//
// Two trace-discovered CToobSpikez methods, defined in ascending retail-RVA
// order:
//   ~CToobSpikez      @0x012c60 - the /GX leaf dtor (folds the CUserLogic teardown).
//   FireActivation    @0x114860 - the per-coordinate activation-registry dispatcher.
//
// CToobSpikez : CUserLogic (the base hierarchy comes from <Gruntz/UserLogic.h>).
// Only offsets / code bytes are load-bearing; names are placeholders for the
// recovered engine identities.
#include <Gruntz/SerialObjRef.h>    // the shared serialized-object-reference (Chain @0x8c00)
#include <Gruntz/MovingLogicBase.h> // CMovingLogicBase::Serialize (0x16e7f0) - shared serialize chain
#include <Wap32/ZVec.h>
#include <Wap32/ZDArrayDerived.h>
#include <Bute/ButeTree.h>
#include <Gruntz/ToobSpikez.h>
#include <Gruntz/XferArchive.h> // the real 0x16e4f0 = ProjTypeXfer(CXferArchive*)
#include <Globals.h>

// ===========================================================================
// CToobSpikez logic dispatcher (0x114480) - the __cdecl per-object message pump
// the activation engine calls each phase. The bound CGameObject carries its
// logic record at +0x7c; that record's phase code (+0x1c) selects which CUserLogic
// virtual to run on the live CToobSpikez instance (+0x18). Phase 0 instantiates
// the CToobSpikez (operator new(0x54) + ctor under the /GX ctor-in-flight frame),
// runs its slot-6 init, latches it, and arms the idle sentinel (0x3e8). The phase
// codes 0x1d/0x1e/0x50-0x53 map to the CUserLogic vtable slots; the idle sentinel
// is a no-op; any other code falls to the shared logic-error reporter. The instance
// is the real CToobSpikez (0x54 bytes, <Gruntz/ToobSpikez.h>); the phase handlers
// are its inherited CUserLogic virtuals (real polymorphic dispatch, external ctor).
// ===========================================================================
// The bound object's logic record at CGameObject+0x7c: the live CToobSpikez instance
// (+0x18) and the current phase code (+0x1c). The dispatched phase handlers ARE
// CToobSpikez's inherited CUserLogic virtual slots (real polymorphic dispatch on the
// real class - no PMF vtable view):
//   phase 0    -> Activate        (slot 6,  +0x18)  the fresh instance's init
//   phase 0x1e -> UserLogicVfunc8 (slot 10, +0x28)
//   phase 0x1d -> UserLogicVfunc9 (slot 11, +0x2c)
//   phase 0x52 -> UserLogicVfuncA (slot 12, +0x30)
//   phase 0x51 -> UserLogicVfuncB (slot 13, +0x34)
//   phase 0x50 -> UserLogicVfuncC (slot 14, +0x38)
//   phase 0x53 -> UserLogicVfuncD (slot 15, +0x3c)
// The record is the game object's +0x7c sub-object (canonical AnimWorkerObj): m_logic
// is its bound logic leaf (typed CUserLogic*, so the shared virtual slots dispatch
// cast-free) and m_1c the phase code (a generic int|ptr slot - here a phase int; the
// int reinterpret at the site is authentic, cf. the field note in UserLogic.h).

// The unresolved-phase fallback is the shared 0x16e4f0 handler, bound as
// ?ProjTypeXfer@@YAHPAUCXferArchive@@@Z (a generic __cdecl per-object handler many
// logic pumps funnel through; <Gruntz/XferArchive.h>). The old "ToobLogicError"
// decl was an unbound alias of it.

// 0x114480: dispatch the bound object's current logic phase.
RVA(0x00114480, 0xf1)
i32 ToobSpikezLogic(CGameObject* obj) {
    AnimWorkerObj* rec = obj->m_7c;
    switch ((u32)rec->m_1c) {
        case 0: {
            rec->m_1c = (void*)0x3e8;
            CToobSpikez* inst = new CToobSpikez(obj);
            inst->Activate(); // slot 6
            rec->m_logic = inst;
            break;
        }
        case 0x1d:
            rec->m_logic->UserLogicVfunc9(); // slot 11
            break;
        case 0x1e:
            rec->m_logic->UserLogicVfunc8(); // slot 10
            break;
        case 0x50:
            rec->m_logic->UserLogicVfuncC(); // slot 14
            break;
        case 0x51:
            rec->m_logic->UserLogicVfuncB(); // slot 13
            break;
        case 0x52:
            rec->m_logic->UserLogicVfuncA(); // slot 12
            break;
        case 0x53:
            rec->m_logic->UserLogicVfuncD(); // slot 15
            break;
        case 0x3e8:
            break;
        default:
            ProjTypeXfer((CXferArchive*)rec->m_logic);
            break;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// The per-coordinate activation registry CToobSpikez::FireActivation
// (0x114860) dispatches through - the SAME archetype as CTimeBomb::FireActivation
// (0x0e1830), but CToobSpikez's OWN registry instance at 0x64e978. A
// coordinate maps to an Entry* either directly (when within the fast
// [g_toobLo, g_toobHi] range) via g_toobBase + (coord-lo)*stride, or by a slow
// Find in the collection (0x16da80, __thiscall ret 8), which on miss rebuilds
// (GetRetAddr 0x16d990 -> g_projActCache, Insert 0x16d850 __thiscall ret 0xc) and
// yields g_toobCur. The entry's first dword is a fn-ptr; a nonzero entry's
// handler is called __thiscall on `this`. All globals are unnamed BSS
// (DATA-pinned so the loads reloc-mask); the collection methods are
// external/no-body (the SAME shared engine functions every registry calls). The
// alloc-cache pair (g_projActCache 0x6bf464 / g_retAddrBreadcrumb 0x6bf428) is the SAME
// shared global every registry writes (already named by KitchenSlime.cpp -
// re-declared here, address-pinned).
struct CToobEntry; // an entry: first dword is the registered handler
struct CToobColl {
    i32 Find(i32 coord, i32 z); // 0x16da80 (__thiscall ret 8)
    // Reserve8710 @0x8710 IS CZDArrayDerived::Construct; cast at the call.
};
extern void* GetRetAddr(); // 0x16d990

DATA(0x0024e978)
CToobColl g_toobColl;
extern void* g_projActCache;
extern void* g_retAddrBreadcrumb;

// The entry's first dword is a pointer-to-member-function of CToobSpikez
// (single inheritance -> 4-byte code pointer); FireActivation invokes it on
// `this`, emitting `mov ecx,this; call [entry]`. CToobSpikez is defined
// COMPLETE in the header above this typedef so the PMF stays 4 bytes
// (pmf-complete-class-4byte).
typedef void (CToobSpikez::*ToobHandler)();
struct CToobEntry {
    ToobHandler m_fn; // [entry]
};

// The inlined coordinate->Entry* lookup FireActivation folds in twice.
// g_toob* registry-field globals (referenced only from this TU): real
// definitions DATA-pinned here; the single extern is in <Globals.h>.
DATA(0x0024e97c)
CVariantSlot* g_toobColl2;
DATA(0x0024e980)
i32 g_toobLo;
DATA(0x0024e984)
i32 g_toobHi;
DATA(0x0024e988)
char* g_toobBase;
DATA(0x0024e98c)
CToobEntry* g_toobCur;
DATA(0x0024e990)
i32 g_toobStride;
DATA(0x0024e998)
i32 g_toobScratch;

static inline CToobEntry* ToobLookup(i32 coord) {
    g_toobScratch = 0;
    if (coord >= g_toobLo && coord <= g_toobHi) {
        return (CToobEntry*)(g_toobBase + (coord - g_toobLo) * g_toobStride);
    }
    if ((i32)((_zvec*)&g_toobColl)->GrowTo(coord, 0)) {
        return (CToobEntry*)(g_toobBase + (coord - g_toobLo) * g_toobStride);
    }
    void* item = g_projActCache;
    g_retAddrBreadcrumb = GetRetAddr();
    g_toobColl2->Set(&g_toobColl, (i32)item, 0xc);
    return g_toobCur;
}

// ---------------------------------------------------------------------------
// The shared activation-NAME registry CToobSpikez::RegisterActs (0x1149c0) interns
// the key "A" into g_buteTree (Find returns the id, 0 == absent); on a fresh id it
// records the key in the shared scratch name registry (@0x6bf650, the SAME range/
// cache shape as g_toobColl) and bumps g_typeCounter. Then it resolves id->Entry in
// CToobSpikez's OWN registry (g_toobColl via ToobLookup, the SAME instance
// FireActivation uses) and stores the logic handler (the ILT to the logic method
// @0x114bc0). g_buteTree (0x6bf620, named by mangled symbol) doubles as the
// name->id map; g_typeCounter (0x61aea8) is the running id counter; s_codeA
// (0x60a454) is the "A" key.
// ---------------------------------------------------------------------------
extern i32 g_typeCounter;
extern char s_codeA[];
#include <Gruntz/TypeKeyColl.h> // the REAL class at 0x6bf650 (its fields were the shredded g_type* globals)
struct CTypeNameEntry; // canonical g_typeColl.m_spare slot record (<Gruntz/TypeNameEntry.h>)
DATA(0x002bf650)
extern CTypeKeyColl g_typeColl; // 0x6bf650

// The shared bute store the key is interned in (?g_buteTree@@3VCButeTree@@A
// @0x6bf620, pulled via UserLogic.h; named by mangled symbol so Find/Insert
// reloc-mask).
extern CButeTree g_buteTree;

// The CString in the resolved name slot: ~CString (0x1b9b93) frees the old list,
// operator= (0x1b9e74) assigns the new key. Modeled so the calls reloc-mask.
#include <Gruntz/ActName.h> // CActName (shared)

// The id->name-slot resolve (fast range path + slow Find/GetRetAddr/Insert rebuild).
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

// The logic handler bound into the registry slot (the ILT to the toob-spikez logic
// method @0x114bc0); referenced by address so the DIR32 operand reloc-masks.
extern i32 ToobLogic_114bc0();

// --- CToobSpikez (0x1145c0), vptr 0x5e7774 --- the ctor anchors GetTypeTag @0x12ba0
// + the ??_7CToobSpikez vtable in this TU. Folds the inline CUserLogic(obj) base.
RVA(0x001145c0, 0x18e)
CToobSpikez::CToobSpikez(CGameObject* obj) : CUserLogic(obj) {
    TILE_LOGIC_SEED(obj);
    m_40 = m_38->m_geoId;
    m_38->ApplyLookupGeometry("GAME_CYCLE100", 2);
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    m_38->m_flags |= 2;
    m_object->m_164 = m_object->m_screenX >> 5;
    m_object->m_168 = m_object->m_screenY >> 5;
    if (m_object->m_latchedAnimId != 0xc) {
        m_object->m_latchedAnimId = 0xc;
        m_object->m_flags |= 0x20000;
    }
}

// CToobSpikez::Register_1147e0 @0x1147e0 - reserve this class's activation
// coordinate range [0x7d0, 0x7da] in its registry (g_toobColl).  A trivial
// forwarder (mov ecx,&reg; push hi; push lo; call); ecx (this) is unused.
RVA(0x001147e0, 0x15)
void CToobSpikez::Register_1147e0() {
    ((CZDArrayDerived*)&g_toobColl)->Construct(0x7d0, 0x7da);
}

// CToobSpikez::GetTypeTag (0x00012ba0) is now an inline member in the class header.

// CToobSpikez::Serialize @0x012bc0 - vtable slot 1: chain the shared CUserLogic
// serialize helper on `this`, then (on success) the +0x34 sub-object's chain,
// normalized to a strict bool. Byte-identical to CSecretTeleporterTrigger::Serialize.
RVA(0x00012bc0, 0x47)
i32 CToobSpikez::Serialize(i32 a, i32 b, i32 c, i32 d) {
    if (!((CMovingLogicBase*)this)->Serialize((CSerialArchive*)(a), b, c, d)) {
        return 0;
    }
    return ((CSerialObjRef*)&m_34)->Chain((CSerialArchive*)a, b, c, (CSerialObj*)d) != 0;
}

// CToobSpikez::~CToobSpikez @0x012c60 - the leaf adds no destructible members
// beyond CUserLogic, so its dtor folds the bare CUserLogic teardown: store the
// CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link (the embedded
// ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The destructible
// link forces the /GX EH frame. Byte-identical in shape to ~CTimeBomb @0x012a70;
// the empty body is enough for cl.
RVA(0x00012c60, 0x44)
CToobSpikez::~CToobSpikez() {}

// CToobSpikez::FireActivation @0x114860 - look the activation coordinate up in
// the registry; if the entry has a registered handler, look it up again and
// dispatch it __thiscall on this. Same archetype as CTimeBomb::FireActivation
// (0x0e1830).
RVA(0x00114860, 0x102)
void CToobSpikez::FireActivation(i32 coord) {
    CToobEntry* e = ToobLookup(coord);
    if (e->m_fn != 0) {
        CToobEntry* e2 = ToobLookup(coord);
        (this->*(e2->m_fn))();
    }
}

// CToobSpikez::RegisterActs @0x1149c0 - bind the logic handler to the activation
// key "A" in the toob-spikez's OWN registry (g_toobColl). See the registration
// commentary above. The SAME archetype as CParticlez::RegisterActs.
//
// @early-stop
// zvec/name-vec IndexToPtr regalloc wall (docs/patterns/zero-register-pinning.md +
// the documented ZVec family): logic + the bute find/insert + the fn-ptr store are
// byte-faithful; cl pins the index/this/base across the grow branches differently
// than retail. Not source-steerable; the SAME plateau as CParticlez::RegisterActs.
RVA(0x001149c0, 0x18d)
void CToobSpikez::RegisterActs() {
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
    *(void**)ToobLookup(id) = (void*)&ToobLogic_114bc0;
}

// class-metadata SIZE sweep (misc-Gruntz A-C): matching-neutral, hosted at
// .cpp EOF (see docs/class-metadata-sweep-log.md). SIZE_UNKNOWN = size not yet pinned.
#include <rva.h>
#include <Wap32/ZVec.h>
#include <Wap32/ZDArrayDerived.h>
#include <Gruntz/SerialArchive.h> // the serialize stream (== the real CFileMemBase)
SIZE_UNKNOWN(CToobColl);
SIZE_UNKNOWN(CToobEntry);

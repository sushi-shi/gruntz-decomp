// CParticlez.cpp - the particlez effect game-object (C:\Proj\Gruntz).
//
// Two trace-discovered CParticlez methods, defined in ascending retail-RVA
// order:
//   ~CParticlez       @0x012d90 - the /GX leaf dtor (folds the CUserLogic teardown).
//   FireActivation    @0x046d30 - the per-coordinate activation-registry dispatcher.
//
// CParticlez : CUserLogic (the base hierarchy comes from <Gruntz/UserLogic.h>).
// Only offsets / code bytes are load-bearing; names are placeholders for the
// recovered engine identities.
#include <Gruntz/CParticlez.h>
#include <Globals.h>
#include <Gruntz/CAnimSink.h>

// ---------------------------------------------------------------------------
// The per-coordinate activation registry CParticlez::FireActivation
// (0x046d30) dispatches through - the SAME archetype as CTimeBomb::FireActivation
// (0x0e1830), but CParticlez's OWN registry instance at 0x644870. A coordinate
// maps to an Entry* either directly (when within the fast [g_partLo, g_partHi]
// range) via g_partBase + (coord-lo)*stride, or by a slow Find in the collection
// (0x16da80, __thiscall ret 8), which on miss rebuilds (ActAlloc 0x16d990 ->
// g_actCache, Insert 0x16d850 __thiscall ret 0xc) and yields g_partCur. The
// entry's first dword is a fn-ptr; a nonzero entry's handler is called
// __thiscall on `this`. All globals are unnamed BSS (DATA-pinned so the loads
// reloc-mask); the collection methods are external/no-body (the SAME shared
// engine functions every registry calls). The alloc-cache pair (g_actCache
// 0x6bf464 / g_actAllocResult 0x6bf428) is the SAME shared global every registry
// writes (already named by KitchenSlime.cpp - re-declared here, address-pinned).
struct CPartEntry; // an entry: first dword is the registered handler
struct CPartColl {
    void Construct(i32 lo, i32 hi); // 0x408710 (__thiscall ret 8: build the registry)
    i32 Find(i32 coord, i32 z);     // 0x16da80 (__thiscall ret 8)
};
struct CPartColl2 {
    void Insert(void* coll, void* item, i32 n); // 0x16d850 (__thiscall ret 0xc)
};
extern "C" i32 ActAlloc(); // 0x16d990

DATA(0x00244870)
extern CPartColl g_partColl;
DATA(0x002bf464)
extern void* g_actCache;
DATA(0x002bf428)
extern void* g_actAllocResult;

// The entry's first dword is a pointer-to-member-function of CParticlez (single
// inheritance -> 4-byte code pointer); FireActivation invokes it on `this`,
// emitting `mov ecx,this; call [entry]`. CParticlez is defined COMPLETE in the
// header above this typedef so the PMF stays 4 bytes (pmf-complete-class-4byte).
typedef void (CParticlez::*PartHandler)();
struct CPartEntry {
    PartHandler m_fn; // [entry]
};

// RegisterActs binds the i32-returning Update handler; a parallel entry view with
// the correct PMF signature for the `mov [entry],offset Update` store.
typedef i32 (CParticlez::*PartHandlerI32)();
struct CPartEntryI32 {
    PartHandlerI32 m_fn;
};

// ---------------------------------------------------------------------------
// The shared activation-NAME registry RegisterActs interns the key "A" through
// (@0x6bf650; same range/cache shape as g_partColl). g_buteTree (0x6bf620)
// doubles as the name->id map; g_nextActId (0x61aea8) is the running id counter;
// s_actKeyA (0x60a454) is the "A" key. The id->name-slot resolve reuses the
// shared Find/ActAlloc/Insert + g_actCache/g_actAllocResult collection methods.
// ---------------------------------------------------------------------------
DATA(0x002bf620)
extern CButeTree g_buteTree;
DATA(0x0021aea8)
extern i32 g_nextActId;
DATA(0x0020a454)
extern char s_actKeyA[];
DATA(0x002bf650)
extern CPartColl g_nameReg; // 0x6bf650
DATA(0x002bf654)
extern CPartColl2* g_nameReg2; // 0x6bf654
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

// The CString in the resolved name slot: ~CString (0x1b9b93) frees the old list,
// operator= (0x1b9e74) assigns the new key. Modeled so the calls reloc-mask.
#include <Gruntz/CActName.h> // CActName (shared)

// The id->name-slot resolve (the fast range path + the slow Find/ActAlloc/Insert
// rebuild). Folded inline by RegisterActs once, in the new-id branch.
static inline char* ActNameLookup(i32 id) {
    g_nameRegScratch = 0;
    if (id >= g_nameRegLo && id <= g_nameRegHi) {
        return g_nameRegBase + (id - g_nameRegLo) * g_nameRegStride;
    }
    if (g_nameReg.Find(id, 0)) {
        return g_nameRegBase + (id - g_nameRegLo) * g_nameRegStride;
    }
    void* item = g_actCache;
    g_actAllocResult = (void*)ActAlloc();
    g_nameReg2->Insert(&g_nameReg, item, 0xc);
    return g_nameRegCur;
}

// The Update handler touches the bound object (m_38, a CGameObject) directly:
// its +0x1a0 anim sink (a per-TU CAnimSink view), the +0x1c0/+0x1c8 latches and
// the +0x08 status flags (all modeled on CGameObject in <Gruntz/UserLogic.h>).

// The per-frame draw-delta mirror (_g_6bf3bc); the value-load reloc-masks.
DATA(0x002bf3bc)
extern "C" u32 g_6bf3bc;

// The inlined coordinate->Entry* lookup FireActivation folds in twice.
static inline CPartEntry* PartLookup(i32 coord) {
    g_partScratch = 0;
    if (coord >= g_partLo && coord <= g_partHi) {
        return (CPartEntry*)(g_partBase + (coord - g_partLo) * g_partStride);
    }
    if (g_partColl.Find(coord, 0)) {
        return (CPartEntry*)(g_partBase + (coord - g_partLo) * g_partStride);
    }
    void* item = g_actCache;
    g_actAllocResult = (void*)ActAlloc();
    g_partColl2->Insert(&g_partColl, item, 0xc);
    return g_partCur;
}

// CParticlez::~CParticlez @0x012d90 - the leaf adds no destructible members
// beyond CUserLogic, so its dtor folds the bare CUserLogic teardown: store the
// CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link (the embedded
// ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The destructible
// link forces the /GX EH frame. Byte-identical in shape to ~CTimeBomb @0x012a70;
// the empty body is enough for cl.
RVA(0x00012d90, 0x44)
CParticlez::~CParticlez() {}

// CParticlez::InitActReg @0x046cb0 - construct the class's activation-coordinate
// registry singleton (g_partColl @0x644870) over the fixed range [2000, 2010]
// via the shared registry ctor (FUN_00408710, __thiscall ret 8). A free init
// thunk (no `this`); reloc-masked.
RVA(0x00046cb0, 0x15)
void CParticlez::InitActReg() {
    g_partColl.Construct(2000, 2010);
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
    i32 id = (i32)g_buteTree.Find(s_actKeyA);
    if (id == 0) {
        id = g_nextActId;
        g_buteTree.Insert(s_actKeyA, (void*)id);
        char* slot = ActNameLookup(id);
        i32 n = g_nameRegScratch;
        void** list = g_nameRegCurList;
        while (n-- != 0) {
            if (list != 0) {
                ((CActName*)list)->Free();
            }
            list++;
        }
        ((CActName*)slot)->Assign(s_actKeyA);
        g_nextActId++;
    }
    ((CPartEntryI32*)PartLookup(id))->m_fn = &CParticlez::Update;
}

// CParticlez::Update @0x047090 - re-target the bound object's animation sub-object
// (m_38 + 0x1a0) to the current draw-delta (g_6bf3bc); then, if its +0x1c8 latch is
// set and its +0x1c0 latch is clear, mark it on-screen this frame (+0x8 |= 0x10000).
// Always returns 0. The extended AdvanceAnim archetype.
RVA(0x00047090, 0x4c)
i32 CParticlez::Update() {
    ((CAnimSink*)((char*)m_38 + 0x1a0))->SetAnim(g_6bf3bc);
    CGameObject* o = m_38;
    if (o->m_1c8 != 0 && o->m_1c0 == 0) {
        o->m_flags |= 0x10000;
    }
    return 0;
}

// class-metadata SIZE sweep (misc-Gruntz A-C): matching-neutral, hosted at
// .cpp EOF (see docs/class-metadata-sweep-log.md). SIZE_UNKNOWN = size not yet pinned.
#include <rva.h>
SIZE_UNKNOWN(CPartColl);
SIZE_UNKNOWN(CPartColl2);
SIZE_UNKNOWN(CPartEntry);
SIZE_UNKNOWN(CPartEntryI32);
SIZE_UNKNOWN(CParticlez);

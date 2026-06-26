// CToobSpikez.cpp - the toob-spikez hazard game-object (C:\Proj\Gruntz).
//
// Two trace-discovered CToobSpikez methods, defined in ascending retail-RVA
// order:
//   ~CToobSpikez      @0x012c60 - the /GX leaf dtor (folds the CUserLogic teardown).
//   FireActivation    @0x114860 - the per-coordinate activation-registry dispatcher.
//
// CToobSpikez : CUserLogic (the base hierarchy comes from <Gruntz/UserLogic.h>).
// Only offsets / code bytes are load-bearing; names are placeholders for the
// recovered engine identities.
#include <Gruntz/CToobSpikez.h>

// ---------------------------------------------------------------------------
// The per-coordinate activation registry CToobSpikez::FireActivation
// (0x114860) dispatches through - the SAME archetype as CTimeBomb::FireActivation
// (0x0e1830), but CToobSpikez's OWN registry instance at 0x64e978. A
// coordinate maps to an Entry* either directly (when within the fast
// [g_toobLo, g_toobHi] range) via g_toobBase + (coord-lo)*stride, or by a slow
// Find in the collection (0x16da80, __thiscall ret 8), which on miss rebuilds
// (ActAlloc 0x16d990 -> g_actCache, Insert 0x16d850 __thiscall ret 0xc) and
// yields g_toobCur. The entry's first dword is a fn-ptr; a nonzero entry's
// handler is called __thiscall on `this`. All globals are unnamed BSS
// (DATA-pinned so the loads reloc-mask); the collection methods are
// external/no-body (the SAME shared engine functions every registry calls). The
// alloc-cache pair (g_actCache 0x6bf464 / g_actAllocResult 0x6bf428) is the SAME
// shared global every registry writes (already named by KitchenSlime.cpp -
// re-declared here, address-pinned).
struct CToobEntry; // an entry: first dword is the registered handler
struct CToobColl {
    i32 Find(i32 coord, i32 z);       // 0x16da80 (__thiscall ret 8)
    void Reserve8710(i32 lo, i32 hi); // 0x008710 (__thiscall ret 8)
};
struct CToobColl2 {
    void Insert(void* coll, void* item, i32 n); // 0x16d850 (__thiscall ret 0xc)
};
extern "C" i32 ActAlloc(); // 0x16d990

DATA(0x0024e980)
extern i32 g_toobLo;
DATA(0x0024e984)
extern i32 g_toobHi;
DATA(0x0024e988)
extern char* g_toobBase;
DATA(0x0024e990)
extern i32 g_toobStride;
DATA(0x0024e98c)
extern CToobEntry* g_toobCur;
DATA(0x0024e998)
extern i32 g_toobScratch;
DATA(0x0024e978)
extern CToobColl g_toobColl;
DATA(0x0024e97c)
extern CToobColl2* g_toobColl2;
DATA(0x002bf464)
extern void* g_actCache;
DATA(0x002bf428)
extern void* g_actAllocResult;

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
static inline CToobEntry* ToobLookup(i32 coord) {
    g_toobScratch = 0;
    if (coord >= g_toobLo && coord <= g_toobHi) {
        return (CToobEntry*)(g_toobBase + (coord - g_toobLo) * g_toobStride);
    }
    if (g_toobColl.Find(coord, 0)) {
        return (CToobEntry*)(g_toobBase + (coord - g_toobLo) * g_toobStride);
    }
    void* item = g_actCache;
    g_actAllocResult = (void*)ActAlloc();
    g_toobColl2->Insert(&g_toobColl, item, 0xc);
    return g_toobCur;
}

// ---------------------------------------------------------------------------
// The shared activation-NAME registry CToobSpikez::RegisterActs (0x1149c0) interns
// the key "A" into g_buteTree (Find returns the id, 0 == absent); on a fresh id it
// records the key in the shared scratch name registry (@0x6bf650, the SAME range/
// cache shape as g_toobColl) and bumps g_nextActId. Then it resolves id->Entry in
// CToobSpikez's OWN registry (g_toobColl via ToobLookup, the SAME instance
// FireActivation uses) and stores the logic handler (the ILT to the logic method
// @0x114bc0). g_buteTree (0x6bf620, named by mangled symbol) doubles as the
// name->id map; g_nextActId (0x61aea8) is the running id counter; s_actKeyA
// (0x60a454) is the "A" key.
// ---------------------------------------------------------------------------
DATA(0x0021aea8)
extern i32 g_nextActId;
DATA(0x0020a454)
extern char s_actKeyA[];
DATA(0x002bf650)
extern CToobColl g_nameReg; // 0x6bf650
DATA(0x002bf654)
extern CToobColl2* g_nameReg2; // 0x6bf654
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

// The shared bute store the key is interned in (?g_buteTree@@3VCButeTree@@A
// @0x6bf620, pulled via UserLogic.h; named by mangled symbol so Find/Insert
// reloc-mask).
extern CButeTree g_buteTree;

// The CString in the resolved name slot: ~CString (0x1b9b93) frees the old list,
// operator= (0x1b9e74) assigns the new key. Modeled so the calls reloc-mask.
struct CActName {
    void Free();                  // 0x1b9b93 (~CString)
    void Assign(const char* key); // 0x1b9e74 (CString::operator=(char const*))
};

// The id->name-slot resolve (fast range path + slow Find/ActAlloc/Insert rebuild).
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

// The logic handler bound into the registry slot (the ILT to the toob-spikez logic
// method @0x114bc0); referenced by address so the DIR32 operand reloc-masks.
extern i32 ToobLogic_114bc0();

// CToobSpikez::Register_1147e0 @0x1147e0 - reserve this class's activation
// coordinate range [0x7d0, 0x7da] in its registry (g_toobColl).  A trivial
// forwarder (mov ecx,&reg; push hi; push lo; call); ecx (this) is unused.
RVA(0x001147e0, 0x15)
void CToobSpikez::Register_1147e0() {
    g_toobColl.Reserve8710(0x7d0, 0x7da);
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
    *(void**)ToobLookup(id) = (void*)&ToobLogic_114bc0;
}

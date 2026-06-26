// CTimeBomb.cpp - the timez-bomb pickup game-object (C:\Proj\Gruntz).
//
// Two trace-discovered CTimeBomb methods, defined in ascending retail-RVA order:
//   ~CTimeBomb        @0x012a70 - the /GX leaf dtor (folds the CUserLogic teardown).
//   FireActivation    @0x0e1830 - the per-coordinate activation-registry dispatcher.
//
// CTimeBomb : CUserLogic (the base hierarchy comes from <Gruntz/UserLogic.h>).
// Only offsets / code bytes are load-bearing; names are placeholders for the
// recovered engine identities.
#include <Gruntz/CTimeBomb.h>

// ---------------------------------------------------------------------------
// The per-coordinate activation registry CTimeBomb::FireActivation (0x0e1830)
// dispatches through - the SAME archetype as CKitchenSlime::FireActivation
// (0x0b2940, src/Gruntz/KitchenSlime.cpp), but CTimeBomb's OWN registry instance
// at 0x64c780. A coordinate maps to an Entry* either directly (when within the
// fast [g_tbombLo, g_tbombHi] range) via g_tbombBase + (coord-lo)*stride, or by a
// slow Find in the collection (0x16da80, __thiscall ret 8), which on miss rebuilds
// (ActAlloc 0x16d990 -> g_actCache, Insert 0x16d850 __thiscall ret 0xc) and yields
// g_tbombCur. The entry's first dword is a fn-ptr; a nonzero entry's handler is
// called __thiscall on `this`. All globals are unnamed BSS (DATA-pinned so the
// loads reloc-mask); the collection methods are external/no-body (the SAME shared
// engine functions both registries call). The alloc-cache pair (g_actCache
// 0x6bf464 / g_actAllocResult 0x6bf428) is the SAME shared global both registries
// write (already named by KitchenSlime.cpp - re-declared here, address-pinned).
struct CTBombEntry; // an entry: first dword is the registered handler
struct CTBombColl {
    i32 Find(i32 coord, i32 z); // 0x16da80 (__thiscall ret 8)
};
struct CTBombColl2 {
    void Insert(void* coll, void* item, i32 n); // 0x16d850 (__thiscall ret 0xc)
};
extern "C" i32 ActAlloc(); // 0x16d990

DATA(0x0024c788)
extern i32 g_tbombLo;
DATA(0x0024c78c)
extern i32 g_tbombHi;
DATA(0x0024c790)
extern char* g_tbombBase;
DATA(0x0024c798)
extern i32 g_tbombStride;
DATA(0x0024c794)
extern CTBombEntry* g_tbombCur;
DATA(0x0024c7a0)
extern i32 g_tbombScratch;
DATA(0x0024c780)
extern CTBombColl g_tbombColl;
DATA(0x0024c784)
extern CTBombColl2* g_tbombColl2;
DATA(0x002bf464)
extern void* g_actCache;
DATA(0x002bf428)
extern void* g_actAllocResult;

// The entry's first dword is a pointer-to-member-function of CTimeBomb (single
// inheritance -> 4-byte code pointer); FireActivation invokes it on `this`,
// emitting `mov ecx,this; call [entry]`. CTimeBomb is defined COMPLETE in the
// header above this typedef so the PMF stays 4 bytes (pmf-complete-class-4byte).
typedef void (CTimeBomb::*TBombHandler)();
struct CTBombEntry {
    TBombHandler m_fn; // [entry]
};

// The inlined coordinate->Entry* lookup FireActivation folds in twice.
static inline CTBombEntry* TBombLookup(i32 coord) {
    g_tbombScratch = 0;
    if (coord >= g_tbombLo && coord <= g_tbombHi) {
        return (CTBombEntry*)(g_tbombBase + (coord - g_tbombLo) * g_tbombStride);
    }
    if (g_tbombColl.Find(coord, 0)) {
        return (CTBombEntry*)(g_tbombBase + (coord - g_tbombLo) * g_tbombStride);
    }
    void* item = g_actCache;
    g_actAllocResult = (void*)ActAlloc();
    g_tbombColl2->Insert(&g_tbombColl, item, 0xc);
    return g_tbombCur;
}

// ---------------------------------------------------------------------------
// The shared activation-NAME registry CTimeBomb::RegisterActs (0x0e1990) interns
// the key "A" into g_buteTree (Find returns the id, 0 == absent); on a fresh id it
// records the key in the shared scratch name registry (@0x6bf650, the SAME range/
// cache shape as g_tbombColl) and bumps g_nextActId. Then it resolves id->Entry in
// CTimeBomb's OWN registry (g_tbombColl via TBombLookup, the SAME instance
// FireActivation uses) and stores the logic handler (the ILT to LoadAttributes
// @0x0e1e60). g_buteTree (0x6bf620, named by mangled symbol) doubles as the
// name->id map; g_nextActId (0x61aea8) is the running id counter; s_actKeyA
// (0x60a454) is the "A" key. The id->name-slot resolve reuses the shared
// Find/ActAlloc/Insert + g_actCache/g_actAllocResult collection methods.
// ---------------------------------------------------------------------------
DATA(0x0021aea8)
extern i32 g_nextActId;
DATA(0x0020a454)
extern char s_actKeyA[];
DATA(0x002bf650)
extern CTBombColl g_nameReg; // 0x6bf650
DATA(0x002bf654)
extern CTBombColl2* g_nameReg2; // 0x6bf654
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

// The logic handler bound into the registry slot (the ILT to CTimeBomb's
// attribute-loader @0x0e1e60); referenced by address so the DIR32 operand
// reloc-masks.
extern i32 TBombLogic_e1e60();

// CTimeBomb::~CTimeBomb @0x012a70 - the leaf adds no destructible members beyond
// CUserLogic, so its dtor folds the bare CUserLogic teardown: store the
// CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link (the embedded
// ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The destructible
// link forces the /GX EH frame. Byte-identical in shape to ~CKitchenSlime
// @0x013100 / the established leaf dtors; the empty body is enough for cl.
RVA(0x00012a70, 0x44)
CTimeBomb::~CTimeBomb() {}

// CTimeBomb::FireActivation @0x0e1830 - look the activation coordinate up in the
// timebomb's per-coordinate registry; if the entry has a registered handler, look
// it up again and dispatch it __thiscall on this. Same archetype as
// CKitchenSlime::FireActivation (0x0b2940).
RVA(0x000e1830, 0x102)
void CTimeBomb::FireActivation(i32 coord) {
    CTBombEntry* e = TBombLookup(coord);
    if (e->m_fn != 0) {
        CTBombEntry* e2 = TBombLookup(coord);
        (this->*(e2->m_fn))();
    }
}

// CTimeBomb::RegisterActs @0x0e1990 - bind the per-frame logic handler to the
// activation key "A" in the timebomb's OWN registry (g_tbombColl). See the
// registration commentary above. The SAME archetype as CParticlez::RegisterActs /
// RegisterSimpleAnimLogic.
//
// @early-stop
// zvec/name-vec IndexToPtr regalloc wall (docs/patterns/zero-register-pinning.md +
// the documented ZVec family): logic + the bute find/insert + the fn-ptr store are
// byte-faithful; cl pins the index/this/base across the grow branches differently
// than retail (the slot-vs-id callee-saved choice cascading into the free-loop
// count). Not source-steerable; the SAME plateau as CParticlez::RegisterActs.
RVA(0x000e1990, 0x18d)
void CTimeBomb::RegisterActs() {
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
    *(void**)TBombLookup(id) = (void*)&TBombLogic_e1e60;
}

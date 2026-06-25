// CStaticHazard.cpp - a static hazard game-object (C:\Proj\Gruntz).
//
// Two trace-discovered CStaticHazard methods, defined in ascending retail-RVA
// order:
//   ~CStaticHazard    @0x012b30 - the /GX leaf dtor (folds the CUserLogic teardown).
//   FireActivation    @0x0fbbf0 - the per-coordinate activation-registry dispatcher.
//
// CStaticHazard : CUserLogic (the base hierarchy comes from <Gruntz/UserLogic.h>).
// Only offsets / code bytes are load-bearing; names are placeholders for the
// recovered engine identities.
#include <Gruntz/CStaticHazard.h>

// ---------------------------------------------------------------------------
// The per-coordinate activation registry CStaticHazard::FireActivation
// (0x0fbbf0) dispatches through - the SAME archetype as CTimeBomb::FireActivation
// (0x0e1830), but CStaticHazard's OWN registry instance at 0x64e3d0. A
// coordinate maps to an Entry* either directly (when within the fast
// [g_haznLo, g_haznHi] range) via g_haznBase + (coord-lo)*stride, or by a slow
// Find in the collection (0x16da80, __thiscall ret 8), which on miss rebuilds
// (ActAlloc 0x16d990 -> g_actCache, Insert 0x16d850 __thiscall ret 0xc) and
// yields g_haznCur. The entry's first dword is a fn-ptr; a nonzero entry's
// handler is called __thiscall on `this`. All globals are unnamed BSS
// (DATA-pinned so the loads reloc-mask); the collection methods are
// external/no-body (the SAME shared engine functions every registry calls). The
// alloc-cache pair (g_actCache 0x6bf464 / g_actAllocResult 0x6bf428) is the SAME
// shared global every registry writes (already named by KitchenSlime.cpp -
// re-declared here, address-pinned).
struct CHaznEntry; // an entry: first dword is the registered handler
struct CHaznColl {
    i32 Find(i32 coord, i32 z); // 0x16da80 (__thiscall ret 8)
};
struct CHaznColl2 {
    void Insert(void* coll, void* item, i32 n); // 0x16d850 (__thiscall ret 0xc)
};
extern "C" i32 ActAlloc(); // 0x16d990

DATA(0x0024e3d8)
extern i32 g_haznLo;
DATA(0x0024e3dc)
extern i32 g_haznHi;
DATA(0x0024e3e0)
extern char* g_haznBase;
DATA(0x0024e3e8)
extern i32 g_haznStride;
DATA(0x0024e3e4)
extern CHaznEntry* g_haznCur;
DATA(0x0024e3f0)
extern i32 g_haznScratch;
DATA(0x0024e3d0)
extern CHaznColl g_haznColl;
DATA(0x0024e3d4)
extern CHaznColl2* g_haznColl2;
DATA(0x002bf464)
extern void* g_actCache;
DATA(0x002bf428)
extern void* g_actAllocResult;

// The entry's first dword is a pointer-to-member-function of CStaticHazard
// (single inheritance -> 4-byte code pointer); FireActivation invokes it on
// `this`, emitting `mov ecx,this; call [entry]`. CStaticHazard is defined
// COMPLETE in the header above this typedef so the PMF stays 4 bytes
// (pmf-complete-class-4byte).
typedef void (CStaticHazard::*HaznHandler)();
struct CHaznEntry {
    HaznHandler m_fn; // [entry]
};

// The inlined coordinate->Entry* lookup FireActivation folds in twice.
static inline CHaznEntry* HaznLookup(i32 coord) {
    g_haznScratch = 0;
    if (coord >= g_haznLo && coord <= g_haznHi) {
        return (CHaznEntry*)(g_haznBase + (coord - g_haznLo) * g_haznStride);
    }
    if (g_haznColl.Find(coord, 0)) {
        return (CHaznEntry*)(g_haznBase + (coord - g_haznLo) * g_haznStride);
    }
    void* item = g_actCache;
    g_actAllocResult = (void*)ActAlloc();
    g_haznColl2->Insert(&g_haznColl, item, 0xc);
    return g_haznCur;
}

// CStaticHazard::~CStaticHazard @0x012b30 - the leaf adds no destructible members
// beyond CUserLogic, so its dtor folds the bare CUserLogic teardown: store the
// CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link (the embedded
// ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The destructible
// link forces the /GX EH frame. Byte-identical in shape to ~CTimeBomb @0x012a70;
// the empty body is enough for cl.
RVA(0x00012b30, 0x44)
CStaticHazard::~CStaticHazard() {}

// CStaticHazard::FireActivation @0x0fbbf0 - look the activation coordinate up in
// the registry; if the entry has a registered handler, look it up again and
// dispatch it __thiscall on this. Same archetype as CTimeBomb::FireActivation
// (0x0e1830).
RVA(0x000fbbf0, 0x102)
void CStaticHazard::FireActivation(i32 coord) {
    CHaznEntry* e = HaznLookup(coord);
    if (e->m_fn != 0) {
        CHaznEntry* e2 = HaznLookup(coord);
        (this->*(e2->m_fn))();
    }
}

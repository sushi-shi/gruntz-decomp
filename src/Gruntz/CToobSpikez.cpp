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
    i32 Find(i32 coord, i32 z); // 0x16da80 (__thiscall ret 8)
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

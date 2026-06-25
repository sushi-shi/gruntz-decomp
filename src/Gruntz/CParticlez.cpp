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
    i32 Find(i32 coord, i32 z); // 0x16da80 (__thiscall ret 8)
};
struct CPartColl2 {
    void Insert(void* coll, void* item, i32 n); // 0x16d850 (__thiscall ret 0xc)
};
extern "C" i32 ActAlloc(); // 0x16d990

DATA(0x00244878)
extern i32 g_partLo;
DATA(0x0024487c)
extern i32 g_partHi;
DATA(0x00244880)
extern char* g_partBase;
DATA(0x00244888)
extern i32 g_partStride;
DATA(0x00244884)
extern CPartEntry* g_partCur;
DATA(0x00244890)
extern i32 g_partScratch;
DATA(0x00244870)
extern CPartColl g_partColl;
DATA(0x00244874)
extern CPartColl2* g_partColl2;
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

// CDroppedObject.cpp - a dropped game-object (C:\Proj\Gruntz).
//
// Two trace-discovered CDroppedObject methods, defined in ascending retail-RVA
// order:
//   ~CDroppedObject   @0x0125b0 - the /GX leaf dtor (folds the CUserLogic teardown).
//   FireActivation    @0x0c6bd0 - the per-coordinate activation-registry dispatcher.
//
// CDroppedObject : CUserLogic (the base hierarchy comes from <Gruntz/UserLogic.h>).
// Only offsets / code bytes are load-bearing; names are placeholders for the
// recovered engine identities.
#include <Gruntz/CDroppedObject.h>

// ---------------------------------------------------------------------------
// The per-coordinate activation registry CDroppedObject::FireActivation
// (0x0c6bd0) dispatches through - the SAME archetype as CTimeBomb::FireActivation
// (0x0e1830), but CDroppedObject's OWN registry instance at 0x64bed8. A
// coordinate maps to an Entry* either directly (when within the fast
// [g_dropLo, g_dropHi] range) via g_dropBase + (coord-lo)*stride, or by a slow
// Find in the collection (0x16da80, __thiscall ret 8), which on miss rebuilds
// (ActAlloc 0x16d990 -> g_actCache, Insert 0x16d850 __thiscall ret 0xc) and
// yields g_dropCur. The entry's first dword is a fn-ptr; a nonzero entry's
// handler is called __thiscall on `this`. All globals are unnamed BSS
// (DATA-pinned so the loads reloc-mask); the collection methods are
// external/no-body (the SAME shared engine functions every registry calls). The
// alloc-cache pair (g_actCache 0x6bf464 / g_actAllocResult 0x6bf428) is the SAME
// shared global every registry writes (already named by KitchenSlime.cpp -
// re-declared here, address-pinned).
struct CDropEntry; // an entry: first dword is the registered handler
struct CDropColl {
    i32 Find(i32 coord, i32 z); // 0x16da80 (__thiscall ret 8)
};
struct CDropColl2 {
    void Insert(void* coll, void* item, i32 n); // 0x16d850 (__thiscall ret 0xc)
};
extern "C" i32 ActAlloc(); // 0x16d990

DATA(0x0024bee0)
extern i32 g_dropLo;
DATA(0x0024bee4)
extern i32 g_dropHi;
DATA(0x0024bee8)
extern char* g_dropBase;
DATA(0x0024bef0)
extern i32 g_dropStride;
DATA(0x0024beec)
extern CDropEntry* g_dropCur;
DATA(0x0024bef8)
extern i32 g_dropScratch;
DATA(0x0024bed8)
extern CDropColl g_dropColl;
DATA(0x0024bedc)
extern CDropColl2* g_dropColl2;
DATA(0x002bf464)
extern void* g_actCache;
DATA(0x002bf428)
extern void* g_actAllocResult;

// The entry's first dword is a pointer-to-member-function of CDroppedObject
// (single inheritance -> 4-byte code pointer); FireActivation invokes it on
// `this`, emitting `mov ecx,this; call [entry]`. CDroppedObject is defined
// COMPLETE in the header above this typedef so the PMF stays 4 bytes
// (pmf-complete-class-4byte).
typedef void (CDroppedObject::*DropHandler)();
struct CDropEntry {
    DropHandler m_fn; // [entry]
};

// The inlined coordinate->Entry* lookup FireActivation folds in twice.
static inline CDropEntry* DropLookup(i32 coord) {
    g_dropScratch = 0;
    if (coord >= g_dropLo && coord <= g_dropHi) {
        return (CDropEntry*)(g_dropBase + (coord - g_dropLo) * g_dropStride);
    }
    if (g_dropColl.Find(coord, 0)) {
        return (CDropEntry*)(g_dropBase + (coord - g_dropLo) * g_dropStride);
    }
    void* item = g_actCache;
    g_actAllocResult = (void*)ActAlloc();
    g_dropColl2->Insert(&g_dropColl, item, 0xc);
    return g_dropCur;
}

// CDroppedObject::~CDroppedObject @0x0125b0 - the leaf adds no destructible
// members beyond CUserLogic, so its dtor folds the bare CUserLogic teardown:
// store the CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link (the
// embedded ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The
// destructible link forces the /GX EH frame. Byte-identical in shape to
// ~CTimeBomb @0x012a70; the empty body is enough for cl.
RVA(0x000125b0, 0x44)
CDroppedObject::~CDroppedObject() {}

// CDroppedObject::FireActivation @0x0c6bd0 - look the activation coordinate up
// in the registry; if the entry has a registered handler, look it up again and
// dispatch it __thiscall on this. Same archetype as CTimeBomb::FireActivation
// (0x0e1830).
RVA(0x000c6bd0, 0x102)
void CDroppedObject::FireActivation(i32 coord) {
    CDropEntry* e = DropLookup(coord);
    if (e->m_fn != 0) {
        CDropEntry* e2 = DropLookup(coord);
        (this->*(e2->m_fn))();
    }
}

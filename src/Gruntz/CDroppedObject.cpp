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
    i32 Find(i32 coord, i32 z);         // 0x16da80 (__thiscall ret 8)
    void RegisterRange(i32 lo, i32 hi); // 0x408710 (zDArray fast-range ctor, __thiscall ret 8)
};
struct CDropColl2 {
    void Insert(void* coll, void* item, i32 n); // 0x16d850 (__thiscall ret 0xc)
};
extern "C" i32 ActAlloc(); // 0x16d990

extern i32 g_dropLo;
extern i32 g_dropHi;
extern char* g_dropBase;
extern i32 g_dropStride;
extern CDropEntry* g_dropCur;
extern i32 g_dropScratch;
DATA(0x0024bed8)
extern CDropColl g_dropColl;
extern CDropColl2* g_dropColl2;
DATA(0x002bf464)
extern void* g_actCache;
DATA(0x002bf428)
extern void* g_actAllocResult;

// ---------------------------------------------------------------------------
// RegisterActs (0x0c6d30) binds TWO activation keys ("A" and "B") into the
// dropped-object registry, each carrying its own per-frame handler. Each key is
// interned into the shared bute store (g_buteTree, named symbol so Find/Insert
// reloc-mask); a fresh id records the key in the shared name registry (@0x6bf650,
// the SAME shared instance CTimeBomb/CKitchenSlime use), then the id->Entry
// resolve in CDroppedObject's OWN registry (g_dropColl, via DropLookup) stores the
// handler code pointer. g_nextActId (0x61aea8) is the running id counter.
DATA(0x0021aea8)
extern i32 g_nextActId;
DATA(0x0020a454)
extern char s_actKeyA[]; // "A"
DATA(0x0020d1bc)
extern char s_actKeyB[]; // "B"
DATA(0x002bf650)
extern CDropColl g_nameReg; // 0x6bf650
DATA(0x002bf654)
extern CDropColl2* g_nameReg2; // 0x6bf654
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

// The shared bute store the keys intern into (?g_buteTree@@3VCButeTree@@A
// @0x6bf620; named symbol so Find/Insert reloc-mask).
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

// The two per-frame handlers bound into the registry slots (referenced by address
// so the DIR32 store operands reloc-mask). 0xc7090 binds to "A", 0xc7be0 to "B".
extern i32 DropActA_c7090();
extern i32 DropActB_c7be0();

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

// CDroppedObject::RegisterRange @0x0c6b50 - seed the dropped-object activation
// table's fast-range bounds via the shared zDArray registry ctor
// (RegisterRange(0x7d0, 0x7da), 0x408710 through the 0x3742 ILT thunk). A static
// initializer; same archetype as CProjectile::RegisterRange (0x0df920).
RVA(0x000c6b50, 0x15)
void CDroppedObject::RegisterRange() {
    g_dropColl.RegisterRange(0x7d0, 0x7da);
}

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

// CDroppedObject::RegisterActs @0x0c6d30 - intern the "A" and "B" activation keys
// and bind each to its per-frame handler (0xc7090 / 0xc7be0) in the dropped-object
// registry. Two back-to-back single-key registrations; the SAME archetype as
// CTimeBomb::RegisterActs done twice.
//
// @early-stop
// register-pinning wall (docs/patterns/zero-register-pinning.md +
// test-old-value-decrement-loop-while-postdec.md, topic:wall topic:regalloc): logic
// byte-faithful (both intern/name-resolve blocks + the OWN-registry resolves + the
// `mov [entry],offset handler` stores match retail); residual is the slot-vs-id
// callee-saved register choice cascading into the free-loop counts. Deferred.
RVA(0x000c6d30, 0x2ac)
void CDroppedObject::RegisterActs() {
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
    *(void**)DropLookup(id) = (void*)&DropActA_c7090;

    i32 id2 = (i32)g_buteTree.Find(s_actKeyB);
    if (id2 == 0) {
        id2 = g_nextActId;
        g_buteTree.Insert(s_actKeyB, (void*)id2);
        char* slot = ActNameLookup(id2);
        i32 n = g_nameRegScratch;
        void** list = g_nameRegCurList;
        while (n-- != 0) {
            if (list != 0) {
                ((CActName*)list)->Free();
            }
            list++;
        }
        ((CActName*)slot)->Assign(s_actKeyB);
        g_nextActId++;
    }
    *(void**)DropLookup(id2) = (void*)&DropActB_c7be0;
}

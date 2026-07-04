// ProjActRegistry.cpp - the projectile-action type registry (C:\Proj\Gruntz).
//
// A global registry object (g_projReg @VA 0x629388) maps projectile-action keys to
// cache slots (g_projActCache / g_projActAllocResult) via a grid + bute-tree lookup.
// This TU sits between Utils/ApplyRange and Wap32/ZVec in retail-RVA order. Field
// names are placeholders; only OFFSETS + code bytes are load-bearing.
#include <Gruntz/StringNode.h> // the type-name teardown slot
#include <Gruntz/UserLogic.h>
#include <Globals.h>
#include <Gruntz/TypeNameEntryView.h>
#include <Gruntz/TypeColl.h>
#include <Gruntz/TypeColl2.h>

// The leaf game-object whose dtor opens this TU. A CUserLogic leaf: its only
// destructible member is the inherited +0x18 EngStr link, so the dtor folds the
// bare CUserLogic teardown (the established /GX leaf-dtor archetype).
class CProjActOwner : public CUserLogic {
public:
    virtual ~CProjActOwner() OVERRIDE;
};

// The global registry object at VA 0x629388. SetActiveRange reaches it through an
// ILT thunk (0x3742) -> modeled NO-body so the call reloc-masks. Find (0x16da80)
// is the slow binary-search probe the coordinate->entry lookup falls back to.
struct CProjReg {
    void SetActiveRange(i32 lo, i32 hi); // 0x3742 (ILT thunk; reloc-masked)
    i32 Find(i32 coord, i32 z);          // 0x16da80 (__thiscall ret 8)
};
DATA(0x00229388)
extern CProjReg g_projReg;

// The R3 per-coordinate activation table's field globals (the registry object IS
// the collection; the lo/hi/base/cur/stride/scratch fields are separate
// DATA-pinned BSS globals reached by direct ds: loads). Same archetype as the
// kitchen-slime / projectile activation tables.

// The shared alloc-cache pair + the alloc helper the rebuild path drives.
DATA(0x002bf464)
extern void* g_projActCache; // 0x6bf464
DATA(0x002bf428)
extern void* g_projActAllocResult; // 0x6bf428
extern void* GetRetAddr();     // 0x16d990

struct CProjReg2 {
    void Insert(void* coll, void* item, i32 n); // 0x16d850 (__thiscall ret 0xc)
};

// The dispatch object FireActivation runs on. The R3 entry's first dword is a
// registered handler invoked __thiscall on this; modeled as a 4-byte PMF (the
// class is COMPLETE before the typedef so the PMF stays 4 bytes).
class CProjActObj {
public:
    void FireActivation(i32 coord); // 0x80e0
    void RegisterType();            // 0x8240
};
typedef void (CProjActObj::*ProjActHandler)();
struct R3Entry {
    ProjActHandler m_fn; // [entry] - the registered handler
};

// The inlined coordinate->R3Entry* lookup (fast-range / slow-Find / rebuild),
// folded in twice by FireActivation and once by RegisterType.
static inline R3Entry* R3Lookup(i32 coord) {
    g_projRegScratch = 0;
    if (coord >= g_projRegLo && coord <= g_projRegHi) {
        return (R3Entry*)(g_projRegBase + (coord - g_projRegLo) * g_projRegStride);
    }
    if (g_projReg.Find(coord, 0)) {
        return (R3Entry*)(g_projRegBase + (coord - g_projRegLo) * g_projRegStride);
    }
    void* item = g_projActCache;
    g_projActAllocResult = GetRetAddr();
    g_projRegColl2->Insert(&g_projReg, item, 0xc);
    return g_projRegCur;
}

// 0x7fd0: ~CProjActOwner - the bare CUserLogic leaf teardown: store the CUserLogic
// vptr (0x5e705c), inline-destruct the +0x18 link (~EngStr @0x16d2a0), store the
// CUserBase vptr (0x5e70b4). The destructible link forces the /GX EH frame.
RVA(0x00007fd0, 0x44)
CProjActOwner::~CProjActOwner() {}

// 0x8060: register the default projectile-action id range [0x7d0, 0x7da] on the
// global registry.
RVA(0x00008060, 0x15)
void ProjActRegisterDefaults() {
    g_projReg.SetActiveRange(0x7d0, 0x7da);
}

// 0x80e0: CProjActObj::FireActivation - look the activation coordinate up in the
// R3 registry; if the entry has a registered handler, look it up again and
// dispatch it __thiscall on this. Same archetype as CKitchenSlime::FireActivation.
RVA(0x000080e0, 0x102)
void CProjActObj::FireActivation(i32 coord) {
    R3Entry* e = R3Lookup(coord);
    if (e->m_fn != 0) {
        R3Entry* e2 = R3Lookup(coord);
        (this->*(e2->m_fn))();
    }
}

// ---------------------------------------------------------------------------
// The shared game-object type-name registry (R1, @0x6bf650) RegisterType funnels
// through, keyed by the per-type id the global bute-tree assigns to a class name.
// Same fast-range/slow-Find/rebuild lookup as the per-class R3 table. All globals
// are BSS (DATA-pinned so the loads reloc-mask); collection/CString helpers are
// external/no-body. CTypeColl2 (the Insert facet) is the shared def in
// <Gruntz/TypeColl2.h>.
struct CTypeNameEntry;
DATA(0x002bf658)
extern i32 g_typeLo;
DATA(0x002bf65c)
extern i32 g_typeHi;
DATA(0x002bf660)
extern char* g_typeBase;
DATA(0x002bf668)
extern i32 g_typeStride;
DATA(0x002bf664)
extern CTypeNameEntry* g_typeCur;
DATA(0x002bf670)
extern i32 g_typeCount;
DATA(0x002bf650)
extern CTypeColl g_typeColl;
DATA(0x002bf654)
extern CTypeColl2* g_typeColl2;
DATA(0x002bf66c)
extern void* g_typeNodes;
DATA(0x0021aea8)
extern i32 g_typeCounter; // 0x61aea8

// The global bute store (CButeTree from <Bute/ButeMgr.h>; Find 0x16d190 / Insert
// 0x16db90) - the class-name -> type-id map.
DATA(0x002bf620)
extern CButeTree g_buteTree;

// R1 lookup: the type-id -> R1 entry resolution shared with the per-class table.
static inline CTypeNameEntry* TypeLookup(i32 key) {
    g_typeCount = 0;
    if (key >= g_typeLo && key <= g_typeHi) {
        return (CTypeNameEntry*)(g_typeBase + (key - g_typeLo) * g_typeStride);
    }
    if (g_typeColl.Find(key, 0)) {
        return (CTypeNameEntry*)(g_typeBase + (key - g_typeLo) * g_typeStride);
    }
    void* item = g_projActCache;
    g_projActAllocResult = GetRetAddr();
    g_typeColl2->Insert(&g_typeColl, item, 0xc);
    return g_typeCur;
}

// The R3 handler stored into the per-class table (LAB_00403517, an ILT thunk).
extern "C" void ProjActHandlerThunk(); // 0x403517 (ILT thunk)

// 0x8240: CProjActObj::RegisterType - the level-load class registrar. Assign the
// class a type-id via the global bute-tree (registering its name on first use),
// record the name into the shared type-name table, then store the activation
// handler (0x403517) into the per-class R3 table at that id. Same archetype as
// CKitchenSlime / CProjectile RegisterType.
// @early-stop
// ~91%: every operation/offset/string/call is byte-correct; the residual is the
// SAME regalloc + count-down-induction wall the other two RegisterTypes carry -
// the node-free loop materializes its trip count via the `ecx=cnt; eax=cnt-1; lea
// ebp,[eax+1]` strength-reduced idiom (cl loads the count plainly) and pins the
// type-id register differently. Not source-steerable; deferred to the final sweep.
RVA(0x00008240, 0x18d)
void CProjActObj::RegisterType() {
    i32 id = (i32)g_buteTree.Find("A");
    if (id == 0) {
        g_buteTree.Insert("A", (void*)g_typeCounter);
        i32 key = g_typeCounter;
        id = key;
        CTypeNameEntry* slot = TypeLookup(key);
        i32 cnt = g_typeCount;
        CStringNode* nodes = (CStringNode*)g_typeNodes;
        if (cnt != 0) {
            do {
                if (nodes != 0) {
                    nodes->Free();
                }
                nodes++;
            } while (--cnt);
        }
        ((CTypeNameEntryView*)slot)->Assign("A");
        g_typeCounter++;
    }
    *(void**)R3Lookup(id) = (void*)&ProjActHandlerThunk;
}
SIZE_UNKNOWN(CProjActObj);
SIZE_UNKNOWN(CProjActOwner);
SIZE_UNKNOWN(CProjReg);
SIZE_UNKNOWN(CProjReg2);
SIZE_UNKNOWN(R3Entry);

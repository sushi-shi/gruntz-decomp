// ActReg4.cpp - the RegisterType registrar (0x3e300) for the fourth per-class
// activation registry (R4 @ g_actReg4 0x6446d8, whose default-range registrar is
// OrphanLeaves' Register6446d8Range 0x3e120). Same archetype as CProjActObj /
// CKitchenSlime / CProjectile RegisterType: assign the class a type-id via the
// global bute-tree, record the name in the shared type-name table, then store the
// activation handler (0x4040a2) into the R4 table at that id. Placeholder names;
// only OFFSETS + code bytes are load-bearing.
#include <Bute/ButeMgr.h> // CButeTree
#include <rva.h>

// The shared type-name registry (R1 @0x6bf650) - identical to the other registrars.
struct CTypeColl {
    i32 Find(i32 key, i32 z); // 0x16da80
};
struct CTypeColl2 {
    void Insert(void* coll, void* item, i32 n); // 0x16d850
};
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
extern i32 g_typeCounter;
DATA(0x002bf464)
extern void* g_projActCache;
DATA(0x002bf428)
extern void* g_projActAllocResult;
extern "C" i32 ProjActAlloc(); // 0x16d990
DATA(0x002bf620)
extern CButeTree g_buteTree;

struct CStringNode {
    void* m_0;
    void Free(); // 0x1b9b93
};
struct CTypeNameEntryView {
    void Assign(const char* name); // 0x1b9e74
};

// The R4 per-class activation table (g_actReg4 @0x6446d8 is the collection; the
// lo/hi/base/cur/stride/scratch fields are separate DATA-pinned BSS globals).
struct CActReg4 {
    i32 Find(i32 coord, i32 z); // 0x16da80
};
DATA(0x002446d8)
extern CActReg4 g_actReg4;
extern struct CActReg4Coll2* g_actReg4Coll2;
extern i32 g_actReg4Lo;
extern i32 g_actReg4Hi;
extern char* g_actReg4Base;
extern struct R4Entry* g_actReg4Cur;
extern i32 g_actReg4Stride;
extern i32 g_actReg4Scratch;
struct CActReg4Coll2 {
    void Insert(void* coll, void* item, i32 n); // 0x16d850
};
struct R4Entry {
    void* m_fn;
};

// The R4 handler stored into the per-class table (LAB_004040a2, an ILT thunk).
extern "C" void ActReg4Handler(); // 0x4040a2

static inline CTypeNameEntry* TypeLookup(i32 key) {
    g_typeCount = 0;
    if (key >= g_typeLo && key <= g_typeHi) {
        return (CTypeNameEntry*)(g_typeBase + (key - g_typeLo) * g_typeStride);
    }
    if (g_typeColl.Find(key, 0)) {
        return (CTypeNameEntry*)(g_typeBase + (key - g_typeLo) * g_typeStride);
    }
    void* item = g_projActCache;
    g_projActAllocResult = (void*)ProjActAlloc();
    g_typeColl2->Insert(&g_typeColl, item, 0xc);
    return g_typeCur;
}

static inline R4Entry* R4Lookup(i32 coord) {
    g_actReg4Scratch = 0;
    if (coord >= g_actReg4Lo && coord <= g_actReg4Hi) {
        return (R4Entry*)(g_actReg4Base + (coord - g_actReg4Lo) * g_actReg4Stride);
    }
    if (g_actReg4.Find(coord, 0)) {
        return (R4Entry*)(g_actReg4Base + (coord - g_actReg4Lo) * g_actReg4Stride);
    }
    void* item = g_projActCache;
    g_projActAllocResult = (void*)ProjActAlloc();
    g_actReg4Coll2->Insert(&g_actReg4, item, 0xc);
    return g_actReg4Cur;
}

// @early-stop
// ~91%: every operation/offset/string/call is byte-correct; the residual is the
// SAME regalloc + count-down-induction wall the other RegisterTypes carry (the
// node-free loop's `ecx=cnt; eax=cnt-1; lea ebp,[eax+1]` strength-reduced idiom +
// the type-id register coloring). Not source-steerable; deferred to the final sweep.
RVA(0x0003e300, 0x18d)
void ActReg4RegisterType() {
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
    *(void**)R4Lookup(id) = (void*)&ActReg4Handler;
}

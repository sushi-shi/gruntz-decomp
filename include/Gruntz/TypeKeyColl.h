#ifndef GRUNTZ_GRUNTZ_TYPEKEYCOLL_H
#define GRUNTZ_GRUNTZ_TYPEKEYCOLL_H

#include <rva.h>

#include <Wap32/ZVec.h> // the REAL container hierarchy: zErrHandling <- _zvec <- _zdvec

class CAnimNameRecord {
public:
    char* m_name; // +0x00
};
SIZE_UNKNOWN();
SIZE_UNKNOWN();

class zDArray : public _zdvec {
public:
    zDArray(i32 stride, i32 lo, i32 hi, void* scratch); // 0x16dda0
    // The implicit destructor overrides _zdvec::~_zdvec. Retail emits the derived
    // scalar-deleting slot thunk at 0x16dde0, but no separate ??1zDArray body.
    // (the grow-on-miss the lookups drive is _zvec::GrowTo @0x16da80, reached via a
    //  (_zvec*)&coll cast at the call sites - the former zDArray::Find fake is gone.)
    zDArray* Construct(i32 lo, i32 hi); // 0x8710 (def in ZDArrayDerived.cpp: the
    // non-ctor two-phase registry build - stride 4 / scratch 1 over `this`)
    zDArray* BaseConstruct(i32 stride, i32 lo, i32 hi, void* scratch); // alias view
    // of the ctor (0x16dda0) - Construct's call spelling; a real ctor call would
    // relocate the base-ctor + derived stamp (vtable-realization boundary keep)
    void SetAtGrow(i32 id, const char* key); // grow + assign (inlined in retail)

    // ---- grunt anim-name registry views (folded from Grunt.h's CAnimNameResolver,
    // which was a duplicate view of THIS global; g_typeColl @0x6bf650 IS the anim
    // registry). Each anim-set node resolves to a CAnimNameRecord whose +0 is the
    // anim-name char*. All declared-only (reloc-masked); zDArray is a real
    // (RTTI) class, so these are legitimate unreconstructed callees, not phantoms.
    char** GetNameRecord(void* node);            // thunk_FUN_004310f0 (ret 4)
    CAnimNameRecord* GetNameRecords(void* node); // thunk_FUN_004312a0 (ret 4)
    CAnimNameRecord* ScratchResolve(void* node); // thunk 0x403864

    // NO OWN FIELDS: the "m_cursor @+0x1c / m_count @+0x20" this class used to declare
    // ARE the inherited _zvec::m_alloc / _zvec::m_grown at those exact offsets - the ctor
    // seeds them with the fresh band base and its slot count, which is precisely what an
    // initial allocation leaves in the vector's alloc/grown pair.
};
SIZE_UNKNOWN();

extern zDArray g_typeColl;

extern i32 g_typeCounter;

// --- the TU's extern surface (moved out of the .cpp; addresses/thunk
// VAs are reloc-masked at use) ---
extern "C" i32 g_recCount23;
extern "C" void Format_18d0f0(char* buf, i32 value, i32 cap); // 0x18d0f0
#include <Bute/ButeTree.h>                                    // CButeTree (for the extern below)
extern CButeTree g_buteTree;

extern "C" i32 g_helperRefCount; // 0x2bf400 owner def in TypeKeyColl.cpp (C linkage)
#endif                           // GRUNTZ_GRUNTZ_TYPEKEYCOLL_H

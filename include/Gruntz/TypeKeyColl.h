// TypeKeyColl.h - CTypeKeyColl (RTTI CTypeKeyColl, @0x6bf650) and its zDArray-style
// allocating base hierarchy CZArrayRoot <- CZArray2D <- CTypeKeyColl, the growable
// key collection that backs the per-class type registries (CKitchenSlime /
// CProjectile / CWarlord model the *registry* side in their own TUs; the
// collection's ctors / binary-search / global dynamic-init live in TypeKeyColl.cpp).
//
// The global g_typeColl @0x6bf650 is reached from many TUs, each of which used to
// re-declare its own one-method CTypeKeyColl view (SetAtGrow / IndexToPtr / Resolve
// / Find). This is the single shared shape; the view methods are declared-only so
// their __thiscall call rel32 reloc-masks. Only offsets + code bytes are load-
// bearing; field roles that are unproven keep m_<hex>.
#ifndef GRUNTZ_GRUNTZ_TYPEKEYCOLL_H
#define GRUNTZ_GRUNTZ_TYPEKEYCOLL_H

#include <rva.h>

#include <Wap32/ZVec.h> // the REAL container hierarchy: CContainerErr <- _zvec <- zDArray

// The CZErrSink / CZArrayRoot / CZArray2D trio that used to live here is DISSOLVED -
// it was a second model of the canonical WAP32 container hierarchy:
//   CZErrSink   == CVariantSlot   (the +0x04 error sink; <Bute/ButeTree.h>)
//   CZArrayRoot == CContainerErr  (ctor 0x16d9c0, 1-slot vtable 0x1f04cc; <Wap32/zBitVec.h>)
//   CZArray2D   == zDArray        (ctor 0x16de30, vtable 0x1f04d4, ??1 0x16df40; <Wap32/ZVec.h>)
// Offsets agree field-for-field (m_buf==_zvec::m_base @+0x10, m_buf2==m_spare @+0x14,
// m_owner==CContainerErr::m_errSink @+0x04), and BOTH models' vtable annotations named
// the SAME two rvas - so the two RELOC_VTBL aliases here were the "one class, two names"
// tell, not a wall. The emitted vptr stamp in the 0x16de30 ctor now binds to the real
// ??_7zDArray instead of reloc-masking it under a fabricated ??_7CZArray2D.

// The anim-name record the registry resolves an anim-set node to: its +0 is the
// anim-name char* the grunt dispatch machines strcmp against the type codes.
// (folded here from Grunt.h's CAnimNameResolver view; a data-only struct.)
SIZE_UNKNOWN(CAnimNameRecord);
class CAnimNameRecord {
public:
    char* m_name; // +0x00
};

// The growable key collection itself (@0x6bf650, 0x16dda0 ctor). Find probes the
// sorted node array; the ctor forwards to the base and derives cursor + count. The
// remaining methods are the per-TU one-method views of the shared global g_typeColl:
// SetAtGrow (grow + assign, inlined in retail), IndexToPtr (thunk 0x403864 -> name
// node), Resolve (thunk 0x437c -> interned anim-code name). Declared-only ->
// reloc-masked.
VTBL(CTypeKeyColl, 0x001f04d0); // leaf ??_7CTypeKeyColl @0x5f04d0 (1-slot dtor vtable)
class CTypeKeyColl : public zDArray {
public:
    CTypeKeyColl(i32 stride, i32 lo, i32 hi, void* scratch); // 0x16dda0
    virtual ~CTypeKeyColl() OVERRIDE;                        // [0] ??_G 0x16dde0 (external)
    // (the grow-on-miss the lookups drive is _zvec::GrowTo @0x16da80, reached via a
    //  (_zvec*)&coll cast at the call sites - the former CTypeKeyColl::Find fake is gone.)
    void Construct(i32 lo, i32 hi);          // 0x408710 (reloc-masked; build registry)
    void RegisterRange(i32 lo, i32 hi);      // 0x408710 (same slot, alt view name)
    void SetAtGrow(i32 id, const char* key); // grow + assign (inlined in retail)

    // ---- grunt anim-name registry views (folded from Grunt.h's CAnimNameResolver,
    // which was a duplicate view of THIS global; g_typeColl @0x6bf650 IS the anim
    // registry). Each anim-set node resolves to a CAnimNameRecord whose +0 is the
    // anim-name char*. All declared-only (reloc-masked); CTypeKeyColl is a real
    // (RTTI) class, so these are legitimate unreconstructed callees, not phantoms.
    char** GetNameRecord(void* node);            // thunk_FUN_004310f0 (ret 4)
    CAnimNameRecord* GetNameRecords(void* node); // thunk_FUN_004312a0 (ret 4)
    CAnimNameRecord* ScratchResolve(void* node); // thunk 0x403864
    i32 Probe(i32 a, i32 b);                     // 0x16da80 (_zvec::GrowTo base grow)
    i32 Reserve(CAnimNameRecord* rec, i32 n);    // 0x034960
    i32 MapCellIndex(i32 coord, i32 flag);       // 0x56da80 (ret 8)
    i32 MapCellRecord(i32 base, i32 size);       // 0x034960 (ret 8; block-1 fallback)
    i32 MapCellRecord2(i32 base, i32 size);      // 0x56d850 (ret 0xc; block-2 fallback)
    i32 PinCellIndex();                          // 0x56d990 (ret 0; pop/push/ret stub)

    // NO OWN FIELDS: the "m_cursor @+0x1c / m_count @+0x20" this class used to declare
    // ARE the inherited _zvec::m_alloc / _zvec::m_grown at those exact offsets - the ctor
    // seeds them with the fresh band base and its slot count, which is precisely what an
    // initial allocation leaves in the vector's alloc/grown pair.
};

// The per-type id counter the type registry hands out (seeded 2000, ++'d on each new
// game-object-type registration keyed into the collection above). ?g_typeCounter@@3HA
// @0x61aea8; DEFINED + DATA-pinned in src/Gruntz/KitchenSlime.cpp. Declared here beside
// the registry it counts for so consumers reach it via <Gruntz/TypeKeyColl.h> instead
// of a per-TU extern.
extern i32 g_typeCounter;

// The two 1-char registration keys the type/act register thunks feed to
// g_buteTree.Insert(key, (void*)g_typeCounter): s_codeA = "A" (the type-code node,
// ?s_codeA@@3PADA @0x60a454, DEFINED in src/Gruntz/Projectile.cpp) and s_actKeyB = "B"
// (the activation node, ?s_actKeyB@@3PADA @0x60d1bc, DEFINED in src/Gruntz/Wormhole.cpp).
// Declared here beside g_typeCounter (the counter they register with) so consumers reach
// them via <Gruntz/TypeKeyColl.h> instead of per-TU externs.
extern char s_codeA[];
extern char s_actKeyB[];

// --- vtable catalog (reduced-view classes share their base vtable rva) ---

// --- vtable catalog (view/base classes bound to their unit vtable rva) ---

#endif // GRUNTZ_GRUNTZ_TYPEKEYCOLL_H

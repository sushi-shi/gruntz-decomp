// RezSyncViews.h - matching scaffolding for src/Rez/RezSync.cpp. Included only by that TU.
//
// The global-namespace ::CGameMgr placeholder is the byte-necessary out-of-line emitter of
// the ??1CGameMgr@WAP32@@ dtor at 0x85540 (which restamps the base vtable then tail-calls
// Close). @identity-recovered: this IS WAP32::CGameMgr - but it cannot be folded onto the
// real class because WAP32::CGameMgr's dtor is a USER-DECLARED INLINE dtor (`~CGameMgr() {
// Close(); }`, load-bearing so CGruntzMgr's derived dtor inlines the base-subobject teardown
// exactly as retail), and making this out-of-line dtor a real WAP32::CGameMgr method would
// collide with that inline dtor - the inline-XOR-out-of-line dtor wall (same class of MSVC5
// codegen limitation as the LogicWorkerHandlersA ctor-wall scaffolding). RELOC_VTBL records
// that the placeholder's emitted vptr restamp IS the retail vtable at 0x1e9b8c
// (== ??_7CGameMgr@WAP32@@6B@), so the store reference binds to the right rva while the
// symbol_names row stays the real WAP32 name. Dtor body in src/Rez/RezSync.cpp.
#ifndef REZ_REZSYNCVIEWS_H
#define REZ_REZSYNCVIEWS_H

#include <rva.h>

struct CGameMgr {
    virtual ~CGameMgr(); // 0x85540 (slot 0): implicit base-vtable restamp + Close
    virtual void s1();
    virtual void s2();
    virtual void s3();
    virtual void s4();
    virtual void s5();
};
SIZE_UNKNOWN(CGameMgr);
RELOC_VTBL(CGameMgr, 0x001e9b8c); // == ??_7CGameMgr@WAP32@@6B@ (the real base vtable)

#endif // REZ_REZSYNCVIEWS_H

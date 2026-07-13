// ObjListBase.h - CObjListBase, the abstract 1-slot list-interface grand-base of
// the engine's small intrusive-list family (retail vtable 0x1ef760, one __purecall
// slot). It is the DESTRUCTION vtable every derived list's inlined dtor chain
// re-stamps (the derived own-vtable store is dead-store-eliminated into this base
// stamp by /O2): CSymParser's +0x10 CObjList member (SymParser.h) and CRezDir's
// two CRezDirList members (RezMgr.h) both restamp 0x5ef760 on teardown.
//
// Extracted from SymParser.h so the Rez headers can derive it without pulling the
// whole parser (RezList.h's CObjList model would clash with SymParser's in TUs
// that include both).
#ifndef GRUNTZ_BUTE_OBJLISTBASE_H
#define GRUNTZ_BUTE_OBJLISTBASE_H

#include <Ints.h>
#include <rva.h>

SIZE(CObjListBase, 0x4);
VTBL(CObjListBase, 0x001ef760);
struct CObjListBase {
    virtual void V0() = 0; // slot 0 (__purecall)
};

#endif // GRUNTZ_BUTE_OBJLISTBASE_H

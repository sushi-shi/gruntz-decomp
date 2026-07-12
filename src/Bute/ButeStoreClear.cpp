// ButeStoreClear.cpp - the OUT-OF-LINE CButeStore::Reset (0x212a0), the non-inlined
// twin CChatBoxOwner::ProcessCheatInput calls (C:\Proj\Bute). The real CButeStore::Reset
// is INLINE in <Bute/ButeStore.h> (folds into CButeMgr::Parse - MUST stay inline; a
// decl-only Reset craters Parse -22%), so a member is inline XOR out-of-line: this
// out-of-line twin keeps a distinct host view (CButeStore212a0) beside its class. The
// recursive free (ClearRecursive @0x16e070) is the REAL CButeStore method (merged
// container obj src/Gruntz/TypeKeyColl.cpp), reached by casting `this` to CButeStore so
// its call binds to the retail RVA (byte-neutral: CButeStore's primary base is at +0).
#include <Bute/ButeMgr.h> // shared CButeStore / CButeStoreNode
#include <rva.h>

struct CButeStore212a0 {
    char m_pad0[0x14];
    void* m_14; // +0x14
    i32 m_18;   // +0x18  tree root
    char m_pad1c[0x28 - 0x1c];
    i32 m_28; // +0x28
    void Reset();
};
SIZE_UNKNOWN(CButeStore212a0);
RVA(0x000212a0, 0x21)
void CButeStore212a0::Reset() {
    ((CButeStore*)this)->ClearRecursive(0); // real 0x16e070 (this+0 == CButeStore primary base)
    m_18 = 0;
    m_28 = 0;
    m_14 = 0;
}

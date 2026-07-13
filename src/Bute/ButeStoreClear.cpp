// ButeStoreClear.cpp - the copy of CButeStore::Reset the linker placed at 0x212a0.
//
// Same phenomenon as the store's destructor (three copies, see <Bute/ButeStoreLeafDtors.h>):
// Reset is an INLINE member (<Bute/ButeStore.h>), and MSVC5 without /Gy emits an inline
// member as a per-object-file static, so a standalone copy exists here as well as inlined
// into CButeMgr::Parse. It is anchored on a thin subclass that adds nothing, so the inline
// Reset expands into it verbatim: ClearRecursive(0), then zero the root / the +0x28 field /
// the node count.
//
// The former CButeStore212a0 stand-in (a hand-laid `char m_pad0[0x14]` shell that reached
// the real method by casting `this` to CButeStore) is gone - that cast existed only because
// the shell was not actually a CButeStore.
#include <Bute/ButeStore.h> // the canonical CButeStore (real bases; ClearRecursive; Reset)
#include <rva.h>

// The 0x212a0 copy of the inline Reset (the one CChatBoxOwner::ProcessCheatInput calls).
struct CButeStoreResetCopy212a0 : public CButeStore {
    void ResetCopy(); // 0x212a0
};
SIZE(CButeStoreResetCopy212a0, 0x2c); // adds nothing to CButeStore

RVA(0x000212a0, 0x21)
void CButeStoreResetCopy212a0::ResetCopy() {
    Reset();
}

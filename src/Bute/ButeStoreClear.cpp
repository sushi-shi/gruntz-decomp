// ButeStoreClear.cpp - the butestoreclear copy of CButeStore::Reset the linker placed at
// 0x212a0 (the one CChatBoxOwner::ProcessCheatInput calls). Reset is an INLINE member and
// MSVC5 (no /Gy) emits it as a per-object-file static; the anchor + the why-not-out-of-line
// rationale live in the shared <Bute/ButeStoreDtorCopies.h>. The inline Reset expands into
// the anchor verbatim: ClearRecursive(0), then zero the root / the +0x28 field / node count.
#include <Bute/ButeStoreDtorCopies.h> // the shared COMDAT-copy anchors (CButeStore == zPTree)
#include <rva.h>

RVA(0x000212a0, 0x21)
void CButeStoreResetCopyClear::ResetCopy() {
    Reset();
}

#include <rva.h>

#include <Bute/ButeStoreResetCopy.h>

// 0x212a0 IS `zPTree::Reset()` emitted out of line: its body is exactly the
// inline (ClearRecursive(0) + the three zero stores at +0x18/+0x28/+0x14), and
// its only caller is CChatBoxOwner::ProcessCheatInput, which calls it for
// m_tree48/m_tree74 after expanding the same inline for m_tree.  The COMDAT
// therefore belongs to chatboxowner.obj; it cannot be pinned there until that
// TU stops inlining all three Reset()s, so the placeholder host stays for now.
RVA(0x000212a0, 0x21)
void CButeStoreResetCopyClear::ResetCopy() {
    Reset();
}

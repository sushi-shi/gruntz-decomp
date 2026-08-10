#include <rva.h>

#include <Bute/PTreeNode.h>

// 0x212a0 IS `zPTree::Reset()` emitted out of line: its body is exactly the
// inline (ClearRecursive(0) + the three zero stores at +0x18/+0x28/+0x14), and
// its only caller is CChatBoxOwner::ProcessCheatInput, which calls it for
// m_tree48/m_tree74 after expanding the same inline for m_tree.
RVA(0x000212a0, 0x21)
void zPTree::ResetCopy() {
    Reset();
}

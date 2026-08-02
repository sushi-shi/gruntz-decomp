#include <rva.h>

#include <Bute/ButeTree.h>

#include <Ints.h>

RVA(0x00193340, 0x61)
void zPTree::Walk(
    void(__cdecl* fn)(char* key, void* value, void* ctx),
    void* ctx,
    CButeTreeNode* node
) {
    while (1) {
        if (node == 0) {
            node = m_root;
            if (node == 0) {
                return;
            }
        }
        fn(node->m_key, node->m_value, ctx);
        CButeTreeNode* l = node->m_child[0];
        if (l != 0 && l->m_bit > node->m_bit) {
            Walk(fn, ctx, l);
        }
        CButeTreeNode* r = node->m_child[1];
        if (r == 0 || r->m_bit <= node->m_bit) {
            return;
        }
        node = r;
    }
}

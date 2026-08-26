#include <rva.h>

#include <Bute/ButeTree.h>

#include <Ints.h>

#include <stddef.h>
#include <string.h>

RVA(0x00193340, 0x61)
void zPTree::Walk(
    void(__cdecl* fn)(char* key, void* value, void* ctx),
    void* ctx,
    CButeTreeNode* node
) {
    while (true) {
        if (node == NULL) {
            node = m_root;
            if (node == NULL) {
                return;
            }
        }
        fn(node->m_key, node->m_value, ctx);
        CButeTreeNode* l = node->m_child[0];
        if (l != NULL && l->m_bit > node->m_bit) {
            Walk(fn, ctx, l);
        }
        CButeTreeNode* r = node->m_child[1];
        if (r == NULL || r->m_bit <= node->m_bit) {
            return;
        }
        node = r;
    }
}

RVA(0x001933b0, 0x28f)
void* zPTree::FindOrInsert(const char* key, void* value) {
    i32 path[32];
    i32* p = path;
    m_lookupPending = false;
    if (key == NULL || value == NULL) {
        char* msg = g_errNullArg;
        g_retAddrBreadcrumb = GetCallerRetAddr();
        m_errSink->Set(this, msg, 0x16);
        return NULL;
    }

    i32 nbits = static_cast<i32>((strlen(key) * PTREE_BITS_PER_BYTE));
    m_candidateLeaf = NULL;
    m_descentCursor = m_root;
    m_keyBitLength = nbits;

    i32 sbit = nbits + PTREE_BYTE_BIT_MASK;
    i32 dir;
    if (m_descentCursor != NULL) {
        do {
            dir = sbit;
            CButeTreeNode* node = m_descentCursor;
            if (node->m_bit > sbit) {
                m_candidateLeaf = m_descentCursor;
                break;
            }
            i32 b = node->m_bit;
            dir = (1 << (b & PTREE_BYTE_BIT_MASK))
                  & static_cast<i32>(static_cast<signed char>(key[b >> PTREE_BYTE_BIT_SHIFT]));
            *p++ = dir;
            CButeTreeNode** slot = &node->m_child[1];
            if (!dir) {
                slot = &node->m_child[0];
            }
            CButeTreeNode* child = *slot;
            m_candidateLeaf = child;
            if (child == NULL) {
                break;
            }
            if (child->m_bit <= m_descentCursor->m_bit) {
                if (strcmp(key, child->m_key) == 0) {
                    return child->m_value;
                }
                break;
            }
            m_descentCursor = child;
        } while (m_descentCursor != NULL);
    }

    i32 critbit;
    if (m_candidateLeaf != NULL) {
        critbit = FirstDiffBit(key, m_candidateLeaf->m_key);
    } else {
        critbit = nbits - 1;
    }
    CButeTreeNode* nn = new CButeTreeNode;
    if (nn == NULL) {
        char* msg = g_errOutOfMem;
        g_retAddrBreadcrumb = GetCallerRetAddr();
        m_errSink->Set(this, msg, 0xc);
        return NULL;
    }
    nn->m_bit = critbit;
    nn->m_value = static_cast<char*>(value);
    char* kb = new char[(m_keyBitLength >> PTREE_BYTE_BIT_SHIFT) + 1];
    nn->m_key = kb;
    if (kb == NULL) {
        char* msg = g_errOutOfMem;
        g_retAddrBreadcrumb = GetCallerRetAddr();
        m_errSink->Set(this, msg, 0xc);
        return NULL;
    }
    strcpy(kb, key);

    i32 selfdir =
        (1 << (critbit & PTREE_BYTE_BIT_MASK))
        & static_cast<i32>(static_cast<signed char>(key[critbit >> PTREE_BYTE_BIT_SHIFT]));
    if (selfdir) {
        nn->m_child[1] = nn;
    } else {
        nn->m_child[0] = nn;
    }

    if (m_descentCursor == NULL) {
        m_root = nn;
    } else if (critbit < m_descentCursor->m_bit) {
        m_descentCursor = NULL;
        m_candidateLeaf = m_root;
        i32* pp = path;
        if (m_candidateLeaf->m_bit <= critbit) {
            do {
                i32 d = *pp++;
                m_descentCursor = m_candidateLeaf;
                CButeTreeNode** down = m_candidateLeaf->m_child;
                if (d) {
                    ++down;
                }
                m_candidateLeaf = *down;
            } while (m_candidateLeaf->m_bit <= critbit);
        }
        if (m_descentCursor == NULL) {
            m_root = nn;
        } else {
            CButeTreeNode** s = m_descentCursor->m_child;
            if (pp[-1]) {
                ++s;
            }
            *s = nn;
        }
    } else {
        CButeTreeNode** s = m_descentCursor->m_child;
        if (dir) {
            ++s;
        }
        *s = nn;
    }

    CButeTreeNode** other = &nn->m_child[1];
    if (selfdir) {
        other = &nn->m_child[0];
    }
    *other = m_candidateLeaf;
    m_nodeCount++;
    return value;
}

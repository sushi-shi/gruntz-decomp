#include <rva.h>

#include <Bute/ButeTree.h>

#include <Ints.h>

#include <stddef.h>
#include <string.h>

RVA(0x00193340, 0x61)
void zPTree::_trav(stvf_t fn, void* supplementary, zPTreeNode* node) {
    while (true) {
        if (node == NULL) {
            node = m_root;
            if (node == NULL) {
                return;
            }
        }
        fn(node->m_symbol, node->m_body, supplementary);
        zPTreeNode* l = node->m_left;
        if (l != NULL && l->m_index > node->m_index) {
            _trav(fn, supplementary, l);
        }
        zPTreeNode* r = node->m_right;
        if (r == NULL || r->m_index <= node->m_index) {
            return;
        }
        node = r;
    }
}

RVA(0x001933b0, 0x28f)
void* zPTree::insert(const char* key, void* value) {
    i32* bp;
    i32 newbranch;
    i32 stack[32];
    i32 dp;
    i32 branch;
    zPTreeNode* t;

    m_preview = false;
    if (key == NULL || value == NULL) {
        handle(g_errNullArg, 0x16);
        return NULL;
    }

    m_sbits = static_cast<i32>((strlen(key) * PTREE_BITS_PER_BYTE));
    m_p = m_root;
    m_q = NULL;
    bp = stack;

    while (m_p != NULL) {
        if (m_p->m_index > m_sbits + PTREE_BYTE_BIT_MASK) {
            m_q = m_p;
            break;
        }
        branch = bit(key, m_p->m_index);
        *bp++ = branch;
        m_q = m_p->ptr(branch);
        if (m_q == NULL) {
            break;
        }
        if (m_q->m_index <= m_p->m_index) {
            if (strcmp(key, m_q->m_symbol) == 0) {
                return m_q->m_body;
            }
            break;
        }
        m_p = m_q;
    }

    newbranch = m_q != NULL ? diffpos(key, m_q->m_symbol) : m_sbits - 1;
    t = new zPTreeNode;
    if (t == NULL) {
        handle(g_errOutOfMem, 0xc);
        return NULL;
    }
    t->m_index = newbranch;
    t->m_body = value;
    t->m_symbol = new char[(m_sbits >> PTREE_BYTE_BIT_SHIFT) + 1];
    if (t->m_symbol == NULL) {
        handle(g_errOutOfMem, 0xc);
        return NULL;
    }
    strcpy(t->m_symbol, key);

    dp = bit(key, newbranch);
    t->ptr(dp) = t;

    if (m_p != NULL) {
        if (newbranch >= m_p->m_index) {
            m_p->ptr(branch) = t;
        } else {
            m_q = m_root;
            m_p = NULL;
            bp = stack;
            while (m_q->m_index <= newbranch) {
                m_p = m_q;
                branch = *bp++;
                m_q = m_q->ptr(branch);
            }
            if (m_p != NULL) {
                --bp;
                m_p->ptr(*bp) = t;
            } else {
                m_root = t;
            }
        }
    } else {
        m_root = t;
    }

    t->ptr(!dp) = m_q;
    incc();
    return value;
}

#include <rva.h>

#include <Bute/ButeTree.h>

#include <Ints.h>

#include <stddef.h>
#include <string.h>

RVA(0x00193340, 0x61)
void zPTree::Walk(
    void(__cdecl* fn)(char* key, void* value, void* ctx),
    void* ctx,
    zPTreeNode* node
) {
    while (true) {
        if (node == NULL) {
            node = root;
            if (node == NULL) {
                return;
            }
        }
        fn(node->symbol, node->body, ctx);
        zPTreeNode* l = node->left;
        if (l != NULL && l->index > node->index) {
            Walk(fn, ctx, l);
        }
        zPTreeNode* r = node->right;
        if (r == NULL || r->index <= node->index) {
            return;
        }
        node = r;
    }
}

RVA(0x001933b0, 0x28f)
void* zPTree::FindOrInsert(const char* key, void* value) {
    i32* bp;
    i32 newbranch;
    i32 stack[32];
    i32 dp;
    i32 branch;
    zPTreeNode* t;

    preview = false;
    if (key == NULL || value == NULL) {
        handle(g_errNullArg, 0x16);
        return NULL;
    }

    sbits = static_cast<i32>((strlen(key) * PTREE_BITS_PER_BYTE));
    p = root;
    q = NULL;
    bp = stack;

    while (p != NULL) {
        if (p->index > sbits + PTREE_BYTE_BIT_MASK) {
            q = p;
            break;
        }
        branch = bit(key, p->index);
        *bp++ = branch;
        q = p->ptr(branch);
        if (q == NULL) {
            break;
        }
        if (q->index <= p->index) {
            if (strcmp(key, q->symbol) == 0) {
                return q->body;
            }
            break;
        }
        p = q;
    }

    newbranch = q != NULL ? diffpos(key, q->symbol) : sbits - 1;
    t = new zPTreeNode;
    if (t == NULL) {
        handle(g_errOutOfMem, 0xc);
        return NULL;
    }
    t->index = newbranch;
    t->body = value;
    t->symbol = new char[(sbits >> PTREE_BYTE_BIT_SHIFT) + 1];
    if (t->symbol == NULL) {
        handle(g_errOutOfMem, 0xc);
        return NULL;
    }
    strcpy(t->symbol, key);

    dp = bit(key, newbranch);
    t->ptr(dp) = t;

    if (p != NULL) {
        if (newbranch >= p->index) {
            p->ptr(branch) = t;
        } else {
            q = root;
            p = NULL;
            bp = stack;
            while (q->index <= newbranch) {
                p = q;
                branch = *bp++;
                q = q->ptr(branch);
            }
            if (p != NULL) {
                --bp;
                p->ptr(*bp) = t;
            } else {
                root = t;
            }
        }
    } else {
        root = t;
    }

    t->ptr(!dp) = q;
    m_nodeCount++;
    return value;
}

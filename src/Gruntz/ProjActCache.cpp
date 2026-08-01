#include <Bute/ButeTree.h>
#include <Gruntz/ProjActCache.h>

#include <string.h>

#pragma intrinsic(strlen, strcmp, memcpy)

// @early-stop
RVA(0x001933b0, 0x28f)
CButeNode* CProjActMap::Insert(const char* key, CButeNode* value) {
    i32 path[28];
    m_28 = 0;
    if (key == 0 || value == 0) {
        char* msg = g_errNullArg;
        g_retAddrBreadcrumb = GetCallerRetAddr();
        m_4->Set(this, msg, 0x16);
        return 0;
    }

    i32 nbits = static_cast<i32>((strlen(key) * 8));
    m_20 = 0;
    m_1c = m_18;
    m_24 = nbits;

    i32 sbit = nbits + 7;
    i32 dir;
    i32* p = path;
    if (m_18 != 0) {
        dir = sbit;
        for (;;) {
            CTrieNode* node = m_1c;
            if (node->m_8 > sbit) {
                m_20 = m_1c;
                break;
            }
            i32 b = node->m_8;
            dir = (1 << (b & 7)) & static_cast<i32>(static_cast<signed char>(key[b >> 3]));
            *p++ = dir;
            CTrieNode* child = dir ? node->m_child[1] : node->m_child[0];
            m_20 = child;
            if (child == 0) {
                break;
            }
            if (child->m_8 <= m_1c->m_8) {
                if (strcmp(key, child->m_c) == 0) {
                    return child->m_10;
                }
                break;
            }
            m_1c = child;
        }
    }

    i32 critbit;
    if (m_20 == 0) {
        critbit = nbits - 1;
    } else {
        critbit = FirstDiffBit(key, m_20->m_c);
    }
    CTrieNode* nn = static_cast<CTrieNode*>(RezAlloc(0x14));
    if (nn != 0) {
        nn->m_8 = critbit;
        nn->m_10 = value;
        char* kb = static_cast<char*>(RezAlloc((m_24 >> 3) + 1));
        nn->m_c = kb;
        if (kb != 0) {
            memcpy(kb, key, strlen(key) + 1);

            i32 selfdir = (1 << (critbit & 7))
                          & static_cast<i32>(static_cast<signed char>(key[critbit >> 3]));
            if (selfdir) {
                nn->m_child[1] = nn;
            } else {
                nn->m_child[0] = nn;
            }

            if (m_1c == 0) {
                m_18 = nn;
            } else if (critbit < m_1c->m_8) {
                m_1c = 0;
                m_20 = m_18;
                i32* pp = path;
                if (m_20->m_8 <= critbit) {
                    do {
                        i32 d = *pp++;
                        m_1c = m_20;
                        m_20 = d ? m_20->m_child[1] : m_20->m_child[0];
                    } while (m_20->m_8 <= critbit);
                }
                if (m_1c == 0) {
                    m_18 = nn;
                } else {

                    CTrieNode** s = &m_1c->m_child[0];
                    if (pp[-1]) {
                        s = &m_1c->m_child[1];
                    }
                    *s = nn;
                }
            } else if (dir) {
                m_1c->m_child[1] = nn;
            } else {
                m_1c->m_child[0] = nn;
            }

            CTrieNode** other = &nn->m_child[1];
            if (selfdir) {
                other = &nn->m_child[0];
            }
            *other = m_20;
            m_14++;
            return value;
        }
    }

    char* msg = g_errOutOfMem;
    g_retAddrBreadcrumb = GetCallerRetAddr();
    m_4->Set(this, msg, 0xc);
    return 0;
}

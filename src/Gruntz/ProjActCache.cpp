// ProjActCache.cpp - zBitVec::zBitVec (0x16d790): construct the zErrHandling
// error-tracking base (cl re-stamps the implicit most-derived ??_7zBitVec vptr after
// it returns - real polymorphic shape, no manual stamp), size the bit-vector to cover
// `idx`, then set bit `idx`; on a sizing failure record the caller return address
// and fire the error sink. The destructible polymorphic base forces the /GX frame.
#include <Bute/ButeTree.h> // the real CVariantSlot (m_errSink->Set)
#include <Gruntz/ProjActCache.h>

#include <stdlib.h> // realloc (0x125180), malloc (0x120b60)
#include <string.h> // memset (rep stos)

// memset stays the rep-stos intrinsic; memcpy is intrinsic (rep movs) for the trie
// insert below but forced out-of-line (call) for EnsureSize - the #pragma toggles
// per function.
#pragma intrinsic(strlen, strcmp, memcpy)

// (zBitVec::zBitVec(i32,i32) @0x16d790 lives in its retail TU, the merged
//  container obj src/Gruntz/TypeKeyColl.cpp - wave2-H.)

// ===========================================================================
// CProjActMap::Insert  (0x1933b0) - find-or-insert a string key into the crit-bit
// trie. Reject null key/value. Walk the trie testing the key's crit bits; if an
// existing leaf's key matches, return its stored value. Otherwise allocate a new
// 20-byte node + owned key copy, splice it at the crit bit (recomputed via
// FirstDiffBit against the nearest leaf, or key-length-1 for an empty trie),
// linking the new node's own bit slot to itself and the other to the displaced
// subtree. On any allocation failure, record the caller return address and fire
// the container error sink, returning 0.
// ===========================================================================
// @early-stop
// whole-function regalloc divergence (~58%): the control flow, the crit-bit
// descent + inline strcmp, the FirstDiffBit/RezAlloc node+key-copy allocation, the
// self/displaced child splice, the re-descent over the stored path[] and the two
// cross-jumped OOM-report tails are all logically byte-faithful. But cl assigns the
// hot pointers to different callee-saved registers than retail (this->ebx vs ebp,
// key->edi vs esi, zero->ebp vs ebx) and spills 4 fewer dwords (0x78 vs 0x88
// frame); the mismatch cascades through nearly every instruction. A register-
// assignment coin-flip with no source lever (zero-register-pinning.md family); not
// steerable here. Deferred to the final sweep.
RVA(0x001933b0, 0x28f)
void* CProjActMap::Insert(const char* key, void* value) {
    i32 path[28];
    m_28 = 0;
    if (key == 0 || value == 0) {
        void* name = g_projActName;
        g_retAddrBreadcrumb = GetCallerRetAddr();
        m_4->Set(this, reinterpret_cast<i32>(name), 0x16);
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
            CTrieNode* child = dir ? node->m_4 : node->m_0;
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

            i32 selfdir = (1 << (critbit & 7)) & static_cast<i32>(static_cast<signed char>(key[critbit >> 3]));
            if (selfdir) {
                nn->m_4 = nn;
            } else {
                nn->m_0 = nn;
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
                        m_20 = d ? m_20->m_4 : m_20->m_0;
                    } while (m_20->m_8 <= critbit);
                }
                if (m_1c == 0) {
                    m_18 = nn;
                } else if (pp[-1]) {
                    m_1c->m_4 = nn;
                } else {
                    m_1c->m_0 = nn;
                }
            } else if (dir) {
                m_1c->m_4 = nn;
            } else {
                m_1c->m_0 = nn;
            }

            if (selfdir) {
                nn->m_0 = m_20;
            } else {
                nn->m_4 = m_20;
            }
            m_14++;
            return value;
        }
    }

    void* cache = g_projActCache;
    g_retAddrBreadcrumb = GetCallerRetAddr();
    m_4->Set(this, reinterpret_cast<i32>(cache), 0xc);
    return 0;
}

// ===========================================================================
// zBitVec::Or  (0x193680) - OR the words of `o` into this, growing first if `o`
// covers more bits (EnsureSize; on failure leave this as-is). Walks the smaller of
// the two SBO/heap word bands (`o`'s bit count) OR-combining word-by-word.
// ===========================================================================
RVA(0x00193680, 0x5e)
zBitVec* zBitVec::Or(zBitVec* o) {
    if (static_cast<u32>(o->m_capacity) > static_cast<u32>(m_capacity)) {
        if (!EnsureSize(o->m_capacity)) {
            return this;
        }
    }
    i32 nwords = static_cast<i32>((static_cast<u32>((o->m_capacity + 1)) >> 5));
    u32* obuf = static_cast<u32>(o->m_capacity) > 0x20 ? o->m_words : reinterpret_cast<u32*>(&o->m_words);
    u32* tbuf = static_cast<u32>(m_capacity) > 0x20 ? m_words : reinterpret_cast<u32*>(&m_words);
    for (i32 i = 0; i < nwords; i++) {
        tbuf[i] |= obuf[i];
    }
    return this;
}

// ===========================================================================
// zBitVec::EnsureSize  (0x1936e0)
// ===========================================================================
#pragma function(memcpy)
// Grow the word band to cover `nbits` bits, preserving existing words. When the
// band is already on the heap (capacity > 32) realloc and zero only the grown
// tail; from the inline 4-byte SBO buffer malloc fresh, zero it, and copy the
// inline word in. On allocation failure record the caller return address and
// fire the container error sink, returning 0.
RVA(0x001936e0, 0xd3)
i32 zBitVec::EnsureSize(i32 nbits) {
    u32 ndwords = ((nbits & 0x1f) != 0 ? 1 : 0) + (static_cast<u32>(nbits) >> 5);
    void* nbuf;
    if (static_cast<u32>(m_capacity) > 0x20) {
        nbuf = realloc(m_words, ndwords * 4);
        if (!nbuf) {
            goto fail;
        }
        u32 oldn = static_cast<u32>(m_capacity) >> 5;
        memset(reinterpret_cast<u32*>(nbuf) + oldn, 0, (ndwords - oldn) * 4);
    } else {
        nbuf = malloc(ndwords * 4);
        if (!nbuf) {
            goto fail;
        }
        memset(nbuf, 0, ndwords * 4);
        memcpy(nbuf, &m_words, 4);
    }
    m_words = static_cast<u32*>(nbuf);
    m_capacity = ndwords * 32;
    return 1;
fail:
    void* cache = g_projActCache;
    g_retAddrBreadcrumb = GetCallerRetAddr();
    m_errSink->Set(this, reinterpret_cast<i32>(cache), 0xc);
    return 0;
}

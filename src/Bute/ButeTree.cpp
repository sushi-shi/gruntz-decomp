// ButeTree.cpp - CButeTree, the engine's string-keyed crit-bit (PATRICIA) trie.
//
// The shared keyed store the whole .att/.bute config layer (and the game-object
// type registries: g_buteTree @0x6bf620) funnel lookups through. A leaf node is
// 20 bytes: two child links, the crit-bit index, an owned key copy, and the
// stored value. The classic Sedgewick PATRICIA back-edge invariant detects a leaf
// when a followed link's bit index stops increasing.
//
//   CButeTree::Find   0x16d190  (descend by crit-bit, strcmp the leaf key)
//   CButeTree::Insert 0x16db90  (allocate a leaf, splice at the divergence bit)
//
// External engine leaves are reloc-masked (call rel32 / global DIR32): the
// _ReturnAddress-style alloc-context recorder (0x16e0f0), the engine heap alloc
// (_RezAlloc 0x1b9b46), the first-differing-bit helper (0x16e480), the
// CVariantSlot error sink (0x16d850), and the alloc-context globals. The CRT
// strlen/strcmp/strcpy lower to the /O2 repne-scas / sbb idiom / rep-movs inlines.
//
// Field names are placeholders; only the OFFSETS + emitted bytes are load-bearing.
#include <Bute/ButeTree.h> // canonical CButeTree / CVariantSlot / CButeTreeNode (one shape)
#include <Ints.h>
#include <rva.h>
#include <Globals.h>

#include <string.h> // strlen/strcmp/strcpy - MSVC5 /O2 lowers these in place

// _ReturnAddress()-style helper (0x16e0f0: mov eax,[ebp+4]; ret) - records where
// a failing call originated. A frameless 4-byte naked leaf with no plain-C++ form;
// carved out (config/library_labels.csv) so Find's `call` reloc-masks it as an
// external symbol rather than transcribing raw instruction bytes here.
void* GetCallerRetAddr(); // 0x16e0f0

// The engine heap allocator (NAFXCW operator-new replacement, __cdecl).
extern "C" void* RezAlloc(u32 size); // 0x1b9b46

// The first-differing-bit (crit-bit index) of two keys (__cdecl). The name
// matches the delinker's symbol for 0x16e480 so the `call` reloc pairs.
i32 FirstDiffBit(const char* a, const char* b); // 0x16e480

// Alloc-context diagnostic cells (.data; DATA-pinned so the loads/stores
// reloc-mask). Shared with the projectile/type registries.
extern void* g_retAddrBreadcrumb; // 0x6bf428
DATA(0x002bf464)
extern void* g_projActCache; // 0x6bf464

// CVariantSlot (error sink @+0x04, Set 0x16d850), CButeTreeNode (20-byte leaf) and
// CButeTree (the crit-bit trie) all come from the canonical <Bute/ButeTree.h>.

// ===========================================================================
// CButeTree::Find (0x16d190) - descend the trie by the key's crit bits, then
// strcmp the reached leaf's stored key; return the leaf value on a hit, else 0.
// Records the descent cursor / candidate so a following Insert can splice in.
// ===========================================================================
// @early-stop
// regalloc-coloring wall (~90.6%): structure, every offset, the inline
// strlen/strcmp idioms, the null-path global-load hoist and the GetCallerRetAddr
// helper are byte-exact. Residual is one global coloring decision: retail pins the
// descent bit `b` in edx (so the cursor load lands in eax and the child stays in
// eax across the loop), whereas cl colors `b` into eax (strlen leaves eax=0) -
// cascading a symmetric ebp<->ebx (key/mask) + eax/ecx (child) transposition and a
// couple of member reloads. Not source-steerable (tried slot-form, node-reuse,
// mask-local, name-hoist). See docs/patterns/zero-register-pinning.md. Final sweep.
RVA(0x0016d190, 0x101)
void* CButeTree::Find(const char* key) {
    if (key == 0) {
        void* name = g_projActName;
        g_retAddrBreadcrumb = GetCallerRetAddr();
        m_errorSink->Set(this, (i32)name, 0x16);
        return 0;
    }
    CButeTreeNode* root = m_root;
    m_descentCursor = root;
    m_candidateLeaf = 0;
    m_lookupPending = 1;
    i32 bitmax = (i32)strlen(key) * 8 + 7;
    m_keyBitLength = bitmax;
    if (root == 0) {
        return 0;
    }
    i32 b = root->m_bit;
    while (b <= bitmax) {
        CButeTreeNode** slot = m_descentCursor->m_child;
        if (key[b >> 3] & (1 << (b & 7))) {
            ++slot;
        }
        CButeTreeNode* child = *slot;
        m_candidateLeaf = child;
        if (child == 0) {
            return 0;
        }
        if (child->m_bit <= b) {
            if (strcmp(key, child->m_key) == 0) {
                m_lookupPending = 0;
                return m_candidateLeaf->m_value;
            }
            return 0;
        }
        m_descentCursor = child;
        b = child->m_bit;
    }
    m_candidateLeaf = m_descentCursor;
    return 0;
}

// ===========================================================================
// CButeTree::Insert (0x16db90) - splice a new leaf for `key`/`value` at the
// crit-bit where it diverges from the candidate the preceding Find recorded.
// Allocates the 20-byte node + an owned key copy, sets the node's self-link at
// its crit bit, walks to the insertion point (from the Find cursor, or from the
// root when the cursor sits below the divergence bit), and links the node's other
// child to the displaced subtree. Reports a fatal failure (no prior Find / null
// arg / OOM) through the +0x04 error sink.
// ===========================================================================
// @early-stop
// regalloc-coloring + block-layout wall (~53%) on a 518-byte crit-bit splice. The
// frame is byte-exact (the `push ecx` critbit local appears once Insert is typed
// void* to keep `value` live for the trailing return load), the error paths, the
// alloc pair, the inline strlen+rep-movs strcpy and the KeyPrefixBits/RezAlloc/Set
// calls all match. Residue: retail colors `newbit`/candidate into ecx/eax (cl picks
// the transpose), emits `add reg,-7` where cl picks `sub reg,7`, keeps `node` in esi
// across the splice with address-merge stores, and tail-merges the two `m_root=node`
// (cursor==0 / cur2==0) exits into one cold block - none reliably source-steerable
// on a body this size. Complete + correct logic; deferred to the final sweep.
// docs/patterns/zero-register-pinning.md, const-materialize-into-reg-vs-immediate.md.
RVA(0x0016db90, 0x206)
void* CButeTree::Insert(const char* key, void* value) {
    if (m_lookupPending == 0) {
        g_retAddrBreadcrumb = GetCallerRetAddr();
        m_errorSink->Set(this, (i32) "No prior lookup", 0x16);
        return 0;
    }
    i32 newbit = m_keyBitLength - 7;
    m_lookupPending = 0;
    m_keyBitLength = newbit;
    if (key == 0 || value == 0) {
        void* name = g_projActName;
        g_retAddrBreadcrumb = GetCallerRetAddr();
        m_errorSink->Set(this, (i32)name, 0x16);
        return 0;
    }

    i32 critbit;
    if (m_candidateLeaf != 0) {
        critbit = FirstDiffBit(key, m_candidateLeaf->m_key);
    } else {
        critbit = newbit - 1;
    }

    CButeTreeNode* node = (CButeTreeNode*)RezAlloc(0x14);
    if (node != 0) {
        node->m_value = value;
        node->m_bit = critbit;
        char* keybuf = (char*)RezAlloc((m_keyBitLength >> 3) + 1);
        node->m_key = keybuf;
        if (keybuf != 0) {
            strcpy(keybuf, key);

            // The node's crit-bit child points back at itself (the leaf back-edge).
            i32 dir = key[critbit >> 3] & (1 << (critbit & 7));
            if (dir) {
                node->m_child[1] = node;
            } else {
                node->m_child[0] = node;
            }

            // Find where critbit fits and re-point the parent at the new node.
            CButeTreeNode* cursor = m_descentCursor;
            i32 d2 = dir;
            if (cursor == 0) {
                m_root = node;
            } else if (critbit < cursor->m_bit) {
                // The Find cursor is below the divergence bit: walk from the root.
                CButeTreeNode* p = m_root;
                m_descentCursor = 0;
                m_candidateLeaf = p;
                if (p->m_bit <= critbit) {
                    CButeTreeNode* c;
                    do {
                        p = m_candidateLeaf;
                        m_descentCursor = p;
                        d2 = key[p->m_bit >> 3] & (1 << (p->m_bit & 7));
                        CButeTreeNode** s = p->m_child;
                        if (d2) {
                            ++s;
                        }
                        c = *s;
                        m_candidateLeaf = c;
                    } while (c->m_bit <= critbit);
                }
                CButeTreeNode* cur2 = m_descentCursor;
                if (cur2 == 0) {
                    m_root = node;
                } else {
                    CButeTreeNode** s2 = cur2->m_child;
                    if (d2) {
                        ++s2;
                    }
                    *s2 = node;
                }
            } else {
                CButeTreeNode** s1 = cursor->m_child;
                if (key[cursor->m_bit >> 3] & (1 << (cursor->m_bit & 7))) {
                    ++s1;
                }
                *s1 = node;
            }

            // Link the node's other child to the displaced subtree.
            if (dir) {
                node->m_child[0] = m_candidateLeaf;
            } else {
                node->m_child[1] = m_candidateLeaf;
            }
            m_nodeCount++;
            return value;
        }
    }

    void* cache = g_projActCache;
    g_retAddrBreadcrumb = GetCallerRetAddr();
    m_errorSink->Set(this, (i32)cache, 0xc);
    return 0;
}

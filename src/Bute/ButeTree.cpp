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
#include <Ints.h>
#include <rva.h>

// Inline CRT string intrinsics (MSVC5 /O2 lowers these in place).
extern "C" u32 strlen(const char* s);
extern "C" i32 strcmp(const char* a, const char* b);
extern "C" char* strcpy(char* d, const char* s);

// _ReturnAddress()-style helper (0x16e0f0: mov eax,[ebp+4]; ret) - records where
// a failing call originated. A 4-byte leaf with no frame; emitted via a naked
// stub so its `call` reloc names the real symbol (vs the delinker's FUN_<rva>).
void* GetCallerRetAddr(); // 0x16e0f0

// The engine heap allocator (NAFXCW operator-new replacement, __cdecl).
extern "C" void* RezAlloc(u32 size); // 0x1b9b46

// The first-differing-bit (crit-bit index) of two keys (__cdecl). The name
// matches the delinker's symbol for 0x16e480 so the `call` reloc pairs.
i32 KeyPrefixBits_16e480(const char* a, const char* b); // 0x16e480

// Alloc-context diagnostic cells (.data; DATA-pinned so the loads/stores
// reloc-mask). Shared with the projectile/type registries.
DATA(0x002bf428)
extern void* g_projActAllocResult; // 0x6bf428
DATA(0x002bf464)
extern void* g_projActCache; // 0x6bf464
DATA(0x002bf454)
extern void* g_projActName; // 0x6bf454 (bad-arg diagnostic record cell)

// The +0x04 error sink the trie reports a fatal failure through. __thiscall(this;
// obj, a, b); the delinker names 0x16d850 ?Set@CVariantSlot@@QAEXPAXHH@Z.
class CVariantSlot {
public:
    void Set(void* obj, i32 a, i32 b); // 0x16d850
};

// One crit-bit trie node (20 bytes).
struct CButeNode {
    CButeNode* child[2]; // +0x00 / +0x04
    i32 bit;             // +0x08  crit-bit index
    char* key;           // +0x0c  owned key copy
    void* value;         // +0x10  stored value
};

class CButeTree {
public:
    void* Find(const char* key);
    void* Insert(const char* key, void* value);

    void* m_vptr;            // +0x00
    CVariantSlot* m_4;       // +0x04  error sink
    char m_pad8[0x14 - 0x8]; // +0x08
    i32 m_14;                // +0x14  node count
    CButeNode* m_18;         // +0x18  root
    CButeNode* m_1c;         // +0x1c  descent cursor
    CButeNode* m_20;         // +0x20  candidate / found leaf
    i32 m_24;                // +0x24  key bit-length (strlen*8 + 7)
    i32 m_28;                // +0x28  "lookup pending" flag
};

// ===========================================================================
// GetCallerRetAddr (0x16e0f0) - return the caller's saved return address (the
// alloc-context recorder). A frameless 4-byte leaf: `mov eax,[ebp+4]; ret`.
// ===========================================================================
RVA(0x0016e0f0, 0x4)
__declspec(naked) void* GetCallerRetAddr() {
    __asm {
        mov eax, dword ptr [ebp + 4]
        ret
    }
}

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
        g_projActAllocResult = GetCallerRetAddr();
        m_4->Set(this, (i32)name, 0x16);
        return 0;
    }
    CButeNode* root = m_18;
    m_1c = root;
    m_20 = 0;
    m_28 = 1;
    i32 bitmax = (i32)strlen(key) * 8 + 7;
    m_24 = bitmax;
    if (root == 0) {
        return 0;
    }
    i32 b = root->bit;
    while (b <= bitmax) {
        CButeNode** slot = m_1c->child;
        if (key[b >> 3] & (1 << (b & 7))) {
            ++slot;
        }
        CButeNode* child = *slot;
        m_20 = child;
        if (child == 0) {
            return 0;
        }
        if (child->bit <= b) {
            if (strcmp(key, child->key) == 0) {
                m_28 = 0;
                return m_20->value;
            }
            return 0;
        }
        m_1c = child;
        b = child->bit;
    }
    m_20 = m_1c;
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
// across the splice with address-merge stores, and tail-merges the two `m_18=node`
// (cursor==0 / cur2==0) exits into one cold block - none reliably source-steerable
// on a body this size. Complete + correct logic; deferred to the final sweep.
// docs/patterns/zero-register-pinning.md, const-materialize-into-reg-vs-immediate.md.
RVA(0x0016db90, 0x206)
void* CButeTree::Insert(const char* key, void* value) {
    if (m_28 == 0) {
        g_projActAllocResult = GetCallerRetAddr();
        m_4->Set(this, (i32) "No prior lookup", 0x16);
        return 0;
    }
    i32 newbit = m_24 - 7;
    m_28 = 0;
    m_24 = newbit;
    if (key == 0 || value == 0) {
        void* name = g_projActName;
        g_projActAllocResult = GetCallerRetAddr();
        m_4->Set(this, (i32)name, 0x16);
        return 0;
    }

    i32 critbit;
    if (m_20 != 0) {
        critbit = KeyPrefixBits_16e480(key, m_20->key);
    } else {
        critbit = newbit - 1;
    }

    CButeNode* node = (CButeNode*)RezAlloc(0x14);
    if (node != 0) {
        node->value = value;
        node->bit = critbit;
        char* keybuf = (char*)RezAlloc((m_24 >> 3) + 1);
        node->key = keybuf;
        if (keybuf != 0) {
            strcpy(keybuf, key);

            // The node's crit-bit child points back at itself (the leaf back-edge).
            i32 dir = key[critbit >> 3] & (1 << (critbit & 7));
            if (dir) {
                node->child[1] = node;
            } else {
                node->child[0] = node;
            }

            // Find where critbit fits and re-point the parent at the new node.
            CButeNode* cursor = m_1c;
            i32 d2 = dir;
            if (cursor == 0) {
                m_18 = node;
            } else if (critbit < cursor->bit) {
                // The Find cursor is below the divergence bit: walk from the root.
                CButeNode* p = m_18;
                m_1c = 0;
                m_20 = p;
                if (p->bit <= critbit) {
                    CButeNode* c;
                    do {
                        p = m_20;
                        m_1c = p;
                        d2 = key[p->bit >> 3] & (1 << (p->bit & 7));
                        CButeNode** s = p->child;
                        if (d2) {
                            ++s;
                        }
                        c = *s;
                        m_20 = c;
                    } while (c->bit <= critbit);
                }
                CButeNode* cur2 = m_1c;
                if (cur2 == 0) {
                    m_18 = node;
                } else {
                    CButeNode** s2 = cur2->child;
                    if (d2) {
                        ++s2;
                    }
                    *s2 = node;
                }
            } else {
                CButeNode** s1 = cursor->child;
                if (key[cursor->bit >> 3] & (1 << (cursor->bit & 7))) {
                    ++s1;
                }
                *s1 = node;
            }

            // Link the node's other child to the displaced subtree.
            if (dir) {
                node->child[0] = m_20;
            } else {
                node->child[1] = m_20;
            }
            m_14++;
            return value;
        }
    }

    void* cache = g_projActCache;
    g_projActAllocResult = GetCallerRetAddr();
    m_4->Set(this, (i32)cache, 0xc);
    return 0;
}

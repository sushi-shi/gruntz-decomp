#include <Gruntz/UserLogic.h> // complete CUserLogic (ProjTypeXfer drives its [3]/[4]/[5] virtually)
#include <Mfc.h>
#include <iostream.h>       // the REAL istream the config reader is (operator>> @0x191fe0/0x191f30)
#include <Bute/ButeTree.h>  // canonical CButeTree / CVariantSlot / CButeTreeNode (one shape)
#include <Bute/PTreeNode.h> // zErrHandling / CButeNodeEntry / zPTree (the .bute node family)
#include <Bute/ButeStore.h> // zPTree / CButeTreeNode (the keyed-store family)
#include <Wap32/zBitVec.h>  // zErrHandling / zBitVec + the container-error globals
#include <Gruntz/UserBaseLink.h> // CUserBaseLink (the +0x18 link sub-object; embeds a zBitVec)
#include <rva.h>
#include <ctype.h>  // isspace (0x12f8a0) / isdigit (0x12f840) - as FUNCTION calls
#include <stdlib.h> // malloc (0x120b60) / realloc (0x125180) / free (0x120c30)
#include <string.h> // memset (rep stos) / inline strcpy / strchr / memmove / memcpy

#undef isspace
#undef isdigit
#pragma function(memcpy)

#include <Gruntz/StringNode.h>    // the type-name teardown slot
#include <Gruntz/TypeKeyColl.h>   // CZErrSink/CZArrayRoot/CZArray2D/zDArray (one shape)
#include <Gruntz/TypeKeyCollStr.h> // s_out_of_memory (owner-only decl header)
#include <Gruntz/TypeNameEntry.h> // the shared type-name-registry record (CString m_name)
#include <Gruntz/XferArchive.h>   // canonical CXferArchive/CXferField (ProjTypeXfer arg)
#include <Wap32/ZVec.h>

DATA(0x002bf454)
void* g_projActName;

DATA(0x002bf428)
void* g_retAddrBreadcrumb;

DATA(0x002bf400)
i32 g_helperRefCount; // owner def (zero-init .bss; C linkage via TypeKeyColl.h decl)

// g_typeColl (0x002bf650): zDArray - no provable static init (the type has no
// default ctor / is runtime-Init'd), so the datum is named by symbol.
DATA_SYMBOL(0x002bf650, 0x0, ?g_typeColl@@3VzDArray@@A)

VTBL(zBitVec, 0x001f04c8);
VTBL(zDArray, 0x001f04d0); // leaf ??_7CTypeKeyColl @0x5f04d0 (1-slot dtor vtable)
VTBL(_zdvec, 0x001f04d4); // ~_zdvec-entry vtable (0x5f04d4)

VTBL(CButeNodeEntry, 0x001f04d8); // the entry member's own (base) vtable
DATA(0x002bf468)
u8 g_zArrayTag; // 0x6bf468 (owner-TU def; the CZArrayRoot base-tag byte, &g_zArrayTag)
// @identity-TODO INTERIOR-OFFSET CLUSTER - do NOT "fix" these by defining them.
// g_typeColl (0x6bf650) and the eight scalars around it are ONE object, not nine globals:
//     g_typeColl +0x00   g_typeColl.m_errSink +0x04  g_typeColl.m_lo  +0x08  g_typeColl.m_hi    +0x0c
//     g_typeColl.m_base +0x10   g_typeColl.m_spare   +0x14  g_typeColl.m_stride +0x18  g_typeColl.m_alloc +0x1c
//     g_typeColl.m_grown +0x20
// which is EXACTLY <Gruntz/ActReg.h>'s CActReg field map (m_coll/m_coll2/m_lo/m_hi/m_base/
// m_cur/m_stride/pad1c/m_scratch), and GruntVoice.cpp's ActNameLookup is CActReg::ResolveEntry
// hand-inlined over them, statement for statement. So the activation-NAME registry at
// 0x6bf650 is a CActReg like every other one.
// I promoted five of these to DEFINITIONS in this batch to clear them off the undefined-DATA
// list, and that was WRONG - it fabricates five globals at interior offsets of a real object.
// Reverted. The correct fix is to SUBSUME them onto one CActReg (as done for the two
// registries in GruntVoice.cpp), but they are referenced by ~20 TUs, so that is its own pass -
// not a drive-by. Left undefined and honest until then.

DATA(0x002bf45c)
void* g_projActName2; // 0x6bf45c

// g_containerName (0x2bf408, char[] in <Wap32/zBitVec.h>) - the zErrHandling base-ctor
// name arg the zBitVec ctors pass. Bound via DATA_SYMBOL (the char[] extern mangles
// `?g_containerName@@3PADA` under cl; scanned per-.cpp so it lives here, not the header).
// g_defaultProjActSize (0x21ad28, i32 in zBitVec.h) - the fallback capacity the
// default/HH zBitVec ctors size to.
DATA_SYMBOL(0x002bf408, 0x0, ?g_containerName@@3PADA)
DATA_SYMBOL(0x0021ad28, 0x0, ?g_defaultProjActSize@@3HA)

DATA(0x0021adf4)
const char s_out_of_memory[] = "out of memory"; // decl in <Gruntz/TypeKeyColl.h>

i32 FirstDiffBit(const char* a, const char* b); // 0x16e480

DATA(0x002bf498)
TypeKeyRec g_recs23[32];
DATA_SYMBOL(0x002bf618, 0x4, _g_recCount23)

// ===========================================================================
// 0x16d000 - config field loader.  __cdecl(reader, data): pulls 29 doubles and
// one int out of the reader into the data block at fixed offsets via the stream
// extraction operators, returning the reader.  The reader is the global (pre-std)
// C++ `istream`: 0x191fe0 = istream::operator>>(double&) and 0x191f30 =
// istream::operator>>(int&) (both LIBCIMT carve-outs). ReadDouble/ReadInt are our
// reloc-mask names for those library operators - EXEMPT via config/library_labels.csv
// (reloc-alias rows). @identity-TODO: fold CConfigReader onto the real istream once a
// safe include path exists (renaming LoadConfigFields' RVA-tracked mangled name).
// ===========================================================================
// The reader is the CRT's `istream`, not a class of ours: 0x191fe0 is
// istream::operator>>(double&) and 0x191f30 is istream::operator>>(int&). The
// `CConfigReader` view that used to stand in for it declared two methods
// (ReadDouble / ReadInt) that exist nowhere in the tree or in any library - 32
// guaranteed unresolved externals. Using the real istream makes every one of them the
// real, statically-linked library operator.
// (`d` stays a byte cursor: the 0x108-byte config record it walks has an irregular
//  layout - doubles at 0x00..0x50 and 0x70..0x100 with an int at 0xb8 - and its field
//  identities are not recovered yet. @identity-TODO: model the record.)
RVA(0x0016d000, 0x189)
istream* LoadConfigFields(istream* r, char* d) {
    *r >> *reinterpret_cast<double*>((d + 0x00));
    *r >> *reinterpret_cast<double*>((d + 0x08));
    *r >> *reinterpret_cast<double*>((d + 0x10));
    *r >> *reinterpret_cast<double*>((d + 0x18));
    *r >> *reinterpret_cast<double*>((d + 0x20));
    *r >> *reinterpret_cast<double*>((d + 0x28));
    *r >> *reinterpret_cast<double*>((d + 0x30));
    *r >> *reinterpret_cast<double*>((d + 0x38));
    *r >> *reinterpret_cast<double*>((d + 0x40));
    *r >> *reinterpret_cast<double*>((d + 0x48));
    *r >> *reinterpret_cast<double*>((d + 0x50));
    *r >> *reinterpret_cast<double*>((d + 0x70));
    *r >> *reinterpret_cast<double*>((d + 0x78));
    *r >> *reinterpret_cast<double*>((d + 0x80));
    *r >> *reinterpret_cast<double*>((d + 0x88));
    *r >> *reinterpret_cast<double*>((d + 0x90));
    *r >> *reinterpret_cast<double*>((d + 0x98));
    *r >> *reinterpret_cast<double*>((d + 0xa0));
    *r >> *reinterpret_cast<double*>((d + 0xa8));
    *r >> *reinterpret_cast<double*>((d + 0xb0));
    *r >> *reinterpret_cast<int*>((d + 0xb8));
    *r >> *reinterpret_cast<double*>((d + 0xc0));
    *r >> *reinterpret_cast<double*>((d + 0xc8));
    *r >> *reinterpret_cast<double*>((d + 0xd0));
    *r >> *reinterpret_cast<double*>((d + 0xd8));
    *r >> *reinterpret_cast<double*>((d + 0xe0));
    *r >> *reinterpret_cast<double*>((d + 0xe8));
    *r >> *reinterpret_cast<double*>((d + 0xf0));
    *r >> *reinterpret_cast<double*>((d + 0xf8));
    *r >> *reinterpret_cast<double*>((d + 0x100));
    return r;
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
        g_retAddrBreadcrumb = GetCallerRetAddr();
        m_errSink->Set(this, reinterpret_cast<i32>(name), 0x16);
        return 0;
    }
    CButeTreeNode* root = m_root;
    m_descentCursor = root;
    m_candidateLeaf = 0;
    m_lookupPending = 1;
    i32 bitmax = static_cast<i32>(strlen(key)) * 8 + 7;
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

RVA(0x0016d2a0, 0x26)
zBitVec::~zBitVec() {
    if (static_cast<u32>(m_capacity) > 0x20) {
        free(m_words);
    }
}

// zBitVec::operator= (0x16d2f0) - deep-copy the DWORD band. If the capacities differ,
// release this band (RezFree) and reallocate to the source's word count (or OOM), then
// memcpy m_capacity/8 bytes from the source (heap band or inline SBO word).
// @early-stop
// regalloc wall: 1 instruction of 172 differs - `m_capacity = that.m_capacity` on the
// capacities-differ path picks ecx in cl vs eax in retail (retail reuses the freed
// malloc-result register). 99.84%, no source lever for the scratch register.
RVA(0x0016d2f0, 0xac)
zBitVec& zBitVec::operator=(const zBitVec& that) {
    if (this != &that) {
        if (m_capacity != that.m_capacity) {
            if (static_cast<u32>(m_capacity) > 0x20) {
                ::operator delete(m_words);
            }
            if (static_cast<u32>(that.m_capacity) > 0x20) {
                m_words = static_cast<u32*>(malloc((static_cast<u32>(that.m_capacity) >> 5) * 4));
                if (!m_words) {
                    void* cache = g_projActCache;
                    g_retAddrBreadcrumb = GetCallerRetAddr();
                    m_errSink->Set(this, reinterpret_cast<i32>(cache), 0xc);
                    m_capacity = 0x20;
                    return *this;
                }
            }
            m_capacity = that.m_capacity;
        }
        const u32* src = (static_cast<u32>(that.m_capacity) > 0x20) ? that.m_words : reinterpret_cast<const u32*>(&that.m_words);
        u32* dst = (static_cast<u32>(m_capacity) > 0x20) ? m_words : reinterpret_cast<u32*>(&m_words);
        memcpy(dst, src, static_cast<u32>(m_capacity) >> 3);
    }
    return *this;
}

// zBitVec(const char* tokens, int minSize) - the 836B whitespace/','/'-' number-list
// PARSER ctor (0x16d3a0). Pass 1 validates the token syntax and finds the max index
// (to size the bit-set to max(minSize, maxIndex)); pass 2 re-scans and sets each
// listed bit, expanding "N-M" ranges. Bad arg / bad char fire the container error sink.
// @early-stop
// regalloc wall (zero-register-pinning.md family): the two-pass logic, the
// isspace/isdigit/strchr(" ,-") validation, the max-index sizing, the SBO bit-set and
// the "N-M" range expansion are all byte-faithful, but cl pins the pass-1 cursor in
// edi where retail keeps it in esi (retail: esi=cursor, ebp=value, edi=sawSep flag).
// That swap spills the flag to an extra [esp+0x10] slot and cascades esi/edi through
// every [cursor] access. Splitting the pass-2 cursor into its own var did not move the
// pass-1 assignment; no source lever reaches the allocator here. ~79.8%, logic
// complete; deferred to the final sweep.
RVA(0x0016d3a0, 0x344)
zBitVec::zBitVec(const char* tokens, i32 minSize) : zErrHandling(g_containerName) {
    i32 maxv = 0;
    const char* start;
    const char* q;
    if (tokens == 0) {
        void* name = g_projActName;
        g_retAddrBreadcrumb = GetCallerRetAddr();
        m_errSink->Set(this, reinterpret_cast<i32>(name), 0x16);
        return;
    }
    if (minSize == 0) {
        minSize = g_defaultProjActSize;
    }

    const char* p = tokens;
    if (isspace(*p)) {
        do {
            ++p;
        } while (isspace(*p));
    }
    if (*p == 0) {
        if (!SetSize(minSize)) {
            goto oom;
        }
        return;
    }
    if (!isdigit(*p)) {
        goto badchar;
    }

    // ---- pass 1: validate + find max index ----
    start = p;
    while (*p != 0) {
        i32 v = 0;
        i32 sawSep = 0;
        while (isdigit(*p)) {
            v = v * 10 + (*p - '0');
            ++p;
        }
        if (static_cast<u32>(v) > static_cast<u32>(maxv)) {
            maxv = v;
        }
        while (*p != 0) {
            if (isdigit(*p)) {
                break;
            }
            if (sawSep && *p != ' ') {
                goto badchar;
            }
            if (strchr(" ,-", *p) == 0) {
                goto badchar;
            }
            if (*p != ' ') {
                sawSep = 1;
            }
            ++p;
        }
    }

    if (static_cast<u32>(minSize) > static_cast<u32>(maxv)) {
        maxv = minSize;
    }
    if (!SetSize(maxv)) {
        goto oom;
    }

    // ---- pass 2: set the bits ----
    q = start;
    while (*q != 0) {
        i32 v = 0;
        while (isdigit(*q)) {
            v = v * 10 + (*q - '0');
            ++q;
        }
        {
            u32* band = (static_cast<u32>(m_capacity) > 0x20) ? m_words : reinterpret_cast<u32*>(&m_words);
            band[static_cast<u32>(v) >> 5] |= 1u << (v & 0x1f);
        }
        if (*q == 0) {
            break;
        }
        if (isspace(*q)) {
            while (isspace(q[1])) {
                ++q;
            }
        }
        char sep = *q;
        ++q;
        if (isspace(*q)) {
            while (isspace(q[1])) {
                ++q;
            }
        }
        if (sep == '-') {
            i32 v2 = 0;
            while (isdigit(*q)) {
                v2 = v2 * 10 + (*q - '0');
                ++q;
            }
            if (v > v2) {
                i32 t = v;
                v = v2;
                v2 = t;
            }
            for (i32 b = v + 1; b <= v2; ++b) {
                u32* band = (static_cast<u32>(m_capacity) > 0x20) ? m_words : reinterpret_cast<u32*>(&m_words);
                band[static_cast<u32>(b) >> 5] |= 1u << (b & 0x1f);
            }
            while (*q != 0 && !isdigit(*q)) {
                ++q;
            }
        }
    }
    return;

oom: {
    void* cache = g_projActCache;
    g_retAddrBreadcrumb = GetCallerRetAddr();
    m_errSink->Set(this, reinterpret_cast<i32>(cache), 0xc);
    return;
}
badchar: {
    void* name = g_projActName2;
    g_retAddrBreadcrumb = GetCallerRetAddr();
    m_errSink->Set(this, reinterpret_cast<i32>(name), 0x16);
    return;
}
}

inline zBitVec::zBitVec() : zErrHandling(g_containerName) {
    if (!SetSize(g_defaultProjActSize)) {
        void* cache = g_projActCache;
        g_retAddrBreadcrumb = GetCallerRetAddr();
        m_errSink->Set(this, reinterpret_cast<i32>(cache), 0xc);
    }
}

RVA(0x0016d710, 0x76)
CUserBaseLink::CUserBaseLink() {}

// ===========================================================================
// zBitVec(idx, sizehint) (0x16d790, ex ProjActCache.cpp) - construct the
// zErrHandling error-tracking base (cl re-stamps the implicit most-derived
// ??_7zBitVec vptr after it returns), size the bit-vector to cover `idx`, then
// set bit `idx`; on a sizing failure record the caller return address and fire
// the error sink. The destructible polymorphic base forces the /GX frame.
// ===========================================================================
// @early-stop
// /GX EH-epilogue + RMW-fusion wall (topic:eh topic:regalloc; see docs/patterns/
// identical-return-epilogue-tailmerge.md). Logic, recovered types (zErrHandling/
// zBitVec/CVariantSlot), the const-char* base ctor, the unsigned size compares,
// the SBO bit-buffer select and the inverted branch layout (failure inline /
// success out-of-line) are all byte-faithful, but two MSVC5 /O2 choices diverge
// with no source lever:
//   (a) retail SHARES one /GX teardown epilogue (the failure path `jmp`s to it,
//       success falls through); our cl pops edi early in the success bitset (idx
//       dies after the address calc) so the two exit epilogues are NOT identical
//       and cl duplicates them instead of merging.
//   (b) the bit set is `or [eax],edx` (RMW) in retail; cl emits load/or/store,
//       +3 bytes that shift the success tail.
// ~77.8%, logic complete; deferred to the final sweep.
RVA(0x0016d790, 0xb1)
zBitVec::zBitVec(i32 idx, i32 sizehint) : zErrHandling(g_containerName) {
    u32 n = static_cast<u32>(sizehint);
    if (n == 0) {
        n = static_cast<u32>(g_defaultProjActSize);
    }
    if (static_cast<u32>(idx) >= n) {
        n = static_cast<u32>(idx) + 1;
    }
    if (!SetSize(static_cast<i32>(n))) {
        void* cache = g_projActCache;
        g_retAddrBreadcrumb = GetCallerRetAddr();
        m_errSink->Set(this, reinterpret_cast<i32>(cache), 0xc);
    } else {
        u32* base = (static_cast<u32>(m_capacity) > 0x20) ? m_words : reinterpret_cast<u32*>(&m_words);
        u32* slot = base + (static_cast<u32>(idx) >> 5);
        *slot |= 1u << (idx & 0x1f);
    }
}

// ===========================================================================
// 0x16d850 - variant/property setter (the HOT helper, ret 0xc).  Switches on the
// slot's type tag (m_0c): 4 -> store the low word directly; otherwise probe the
// global index table (when enabled) and, by tag 2/1, either dispatch through the
// resolved table entry's fn / word slot, or (unresolved) format the slot's label
// + invoke the slot's own +0x00 callback / store the word.  The index probe is
// CVariantSlot::Find (0x16e1d0, defined below); the table + gate are globals.
// @early-stop
// 99.85% - reloc-typing / entropy-tail artifact only: the full switch dispatch,
// indexed-table paths, inline strcpy + Format_18d0f0 and the +0x00 callback are
// all byte-exact; the residual is the DIR32-vs-REL32 scoring on the named
// g_varTable / g_varProbeEnabled / Find externs (docs/matching-patterns.md fuzzy%).
// ===========================================================================
// CVariantSlot (the +0x00 callback / probe-index / word / type-tag / label slot)
// is the canonical one in <Bute/ButeTree.h>; Set (0x16d850) is defined here.

RVA(0x0016d850, 0x11e)
void CVariantSlot::Set(void* key, i32 arg2, i32 arg3) {
    if (m_typeTag == 4) {
        m_valueWord = static_cast<u16>(arg3);
        return;
    }
    i32 idx;
    if (g_recCount23 != 0) {
        idx = this->Find(reinterpret_cast<i32>(key));
    } else {
        idx = -1;
    }
    if (idx == -1) {
        if (m_typeTag == 2) {
            char buf[0x94];
            strcpy(buf, m_label);
            Format_18d0f0(buf, arg2, 0x4f);
            m_callback(buf, arg3);
        } else if (m_typeTag == 1) {
            m_valueWord = static_cast<u16>(arg3);
        }
    } else {
        if (m_typeTag == 2) {
            (static_cast<void(__cdecl*)(i32, i32)>(g_recs23[idx].m_4))(arg2, arg3);
        } else if (m_typeTag == 1) {
            g_recs23[idx].m_8 = static_cast<short>(arg3);
        }
    }
}

RVA(0x0016da60, 0x12)
zErrHandling::~zErrHandling() {
    // 0x16e360 is CVariantSlot::Add (the cursor's insert/update/remove op; val==0 removes),
    // called on the error sink to unregister this handler. m_errSink is a CVariantSlot* -
    // no cast (the ex CKeyFinder view of it is dissolved).
    m_errSink->Add(this, 0);
}

// ===========================================================================
// _zvec::GrowTo(idx, at) (0x16da80, ex ZVec.cpp) - realloc the element band so
// index `idx` (relative to the `at` anchor) becomes addressable, shift/zero-
// fill, update bounds.
// ===========================================================================
// @early-stop
// regalloc wall (~80%): retail pins idx in ebx / this in esi / realloc result
// in ebp (arg-before-this); our recompile assigns idx->ebp / this->ebx, which
// cascades into the two-block branch distances. `#pragma function(memcpy)`
// recovered the out-of-line memcpy call (62%->80%); the residual is the
// register assignment, not source-steerable. Logic exact.
RVA(0x0016da80, 0x10b)
void* _zvec::GrowTo(i32 idx, i32 at) {
    void* p;
    if (idx < m_lo) {
        p = realloc(m_base, (m_hi - (idx - at) + 1) * m_stride);
        if (!p) {
            g_retAddrBreadcrumb = GetCallerRetAddr();
            m_errSink->Set(static_cast<void*>(this), reinterpret_cast<u32>(s_out_of_memory), 0x22);
            return 0;
        }
        i32 oldbytes = (m_hi - m_lo + 1) * m_stride;
        i32 shift = m_lo - (idx - at);
        m_grown = shift;
        m_alloc = static_cast<char*>(p);
        memcpy(m_alloc + shift * m_stride, p, oldbytes);
        memset(m_alloc, 0, m_grown * m_stride);
        m_lo = idx - at;
        m_base = static_cast<char*>(p);
        return p;
    }
    i32 hinew = idx + at;
    p = realloc(m_base, (hinew - m_lo + 1) * m_stride);
    if (!p) {
        g_retAddrBreadcrumb = GetCallerRetAddr();
        m_errSink->Set(static_cast<void*>(this), reinterpret_cast<u32>(s_out_of_memory), 0x22);
        return 0;
    }
    i32 oldbytes = (m_hi - m_lo + 1) * m_stride;
    char* fill = reinterpret_cast<char*>(p) + oldbytes;
    m_grown = hinew - m_hi;
    m_alloc = fill;
    memset(fill, 0, m_grown * m_stride);
    m_hi = hinew;
    m_base = static_cast<char*>(p);
    return p;
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
        m_errSink->Set(this, reinterpret_cast<i32>("No prior lookup"), 0x16);
        return 0;
    }
    i32 newbit = m_keyBitLength - 7;
    m_lookupPending = 0;
    m_keyBitLength = newbit;
    if (key == 0 || value == 0) {
        void* name = g_projActName;
        g_retAddrBreadcrumb = GetCallerRetAddr();
        m_errSink->Set(this, reinterpret_cast<i32>(name), 0x16);
        return 0;
    }

    i32 critbit;
    if (m_candidateLeaf != 0) {
        critbit = FirstDiffBit(key, m_candidateLeaf->m_key);
    } else {
        critbit = newbit - 1;
    }

    CButeTreeNode* node = static_cast<CButeTreeNode*>(::operator new(0x14));
    if (node != 0) {
        node->m_value = value;
        node->m_bit = critbit;
        char* keybuf = static_cast<char*>(::operator new((m_keyBitLength >> 3) + 1));
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
    m_errSink->Set(this, reinterpret_cast<i32>(cache), 0xc);
    return 0;
}

RVA(0x0016dda0, 0x3c)
zDArray::zDArray(i32 stride, i32 lo, i32 hi, void* scratch)
    : _zdvec(stride, lo, hi, scratch) {
    m_alloc = m_base;          // +0x1c  the fresh band base (was the m_cursor view)
    m_grown = m_hi - m_lo + 1; // +0x20  its slot count (was the m_count view)
}

// ===========================================================================
// _zdvec::_zdvec (0x16de30) - the allocating vector ctor (this body was modelled
// as `CZArray2D::CZArray2D` until the fold: CZArray2D IS _zdvec, one class under two
// names - its vtable 0x1f04d4 is the datum VTBL(_zdvec) binds and its ??1 0x16df40 is
// ~_zdvec, both right here in this TU). Builds the zErrHandling base (0x16d9c0, was
// the duplicate `CZArrayRoot` model), records the [lo, hi] bounds + element stride,
// allocates the (hi-lo+1)*stride element band (+ a scratch element when none was
// supplied), and reports a fatal "Inconsistent bounds" / "out of memory" through the
// inherited error sink. cl emits the implicit ??_7zDArray vptr stamp and the /GX unwind
// frame (the partially-built zErrHandling subobject must be destroyed if a later
// allocation throws).
//
// @early-stop
// vptr-position wall (~96%, up from 67% as a plain method). Modeling this as a
// real ctor over a destructible error-sink base recovered the whole /GX state
// frame (push -1 / push handler / fs:0 chain / trylevel write) that the plain
// method could not emit - the bulk of the old gap. Residue: cl schedules the
// implicit ??_7 stamp BEFORE the m_lo/m_hi/m_base/m_stride stores, but retail
// sinks it AFTER them, plus a minor regalloc swap in the lo/hi/stride/scratch
// load sequence. Not source-steerable; deferred to the final sweep.
RVA(0x0016de30, 0xe7)
_zdvec::_zdvec(i32 stride, i32 lo, i32 hi, void* scratch)
    : _zvec(&g_zArrayTag) { // -> the zErrHandling base ctor @0x16d9c0
    m_spare = static_cast<char*>(scratch); // +0x14  scratch element (was the m_buf2 view)
    m_lo = lo;
    m_hi = hi;
    m_base = 0; // +0x10  element band (was the m_buf view)
    m_stride = stride;
    if (lo > hi) {
        g_retAddrBreadcrumb = GetCallerRetAddr();
        m_errSink->Set(static_cast<void*>(this), reinterpret_cast<i32>("Inconsistent bounds"), 0x16);
        return;
    }
    i32 total = (hi - lo + 1) * stride;
    void* buf = malloc(total);
    m_base = static_cast<char*>(buf);
    if (buf != 0) {
        memset(buf, 0, total);
        if (m_spare != 0) {
            return;
        }
        m_spare = static_cast<char*>(malloc(m_stride));
        if (m_spare != 0) {
            return;
        }
    }
    g_retAddrBreadcrumb = GetCallerRetAddr();
    m_errSink->Set(static_cast<void*>(this), reinterpret_cast<i32>("out of memory"), 0xc);
}

RVA(0x0016df40, 0x22)
_zdvec::~_zdvec() {
    char* p = m_base;
    if (p) {
        free(p);
    }
    // ~_zvec() base destructor is chained in by the compiler (mov ecx,esi; call).
}

// ===========================================================================
// CButeNodeEntry ctor (0x16df70, ex ButeNode.cpp): __thiscall(this, n, desc).
// cl auto-stamps the ??_7CButeNodeEntry vptr@+0, then stores desc@+4, (WORD)n@+8,
// 0@+0xc. Clean leaf ctor, called out-of-line by the zPTree ctor's member-init.
// @early-stop
// vptr-position wall (~82.9%, was 100% hand-rolled): real polymorphism sinks the
// implicit ??_7 vptr stamp to FIRST; the hand-rolled last-store cannot be sunk in
// MSVC5 (same mechanism as CZArray2D). Converted per the ALL-VTABLES mandate.
RVA(0x0016df70, 0x22)
CButeNodeEntry::CButeNodeEntry(i32 n, void(__cdecl* teardown)(void*)) {
    m_teardown = teardown;
    m_kind = static_cast<i16>(n);
    m_nodeCount = 0;
}

RVA(0x0016dfc0, 0x7)
CButeNodeEntry::~CButeNodeEntry() {}

// zPTree ctor (0x16dff0, ex ButeNode.cpp): run the zErrHandling primary base
// ctor + the CButeNodeEntry second-base ctor, then cl auto-stamps the two
// most-derived vptrs (??_7zPTree @+0 = 0x5e94ac, and the second-base-in-derived
// vtable @+8 = 0x5e949c) and zeroes the two child links. /GX unwind frame from
// the two destructible base sub-objects.
// @early-stop
// vptr-position wall (~96.1%, was 100% hand-rolled): real MI polymorphism
// auto-stamps both vptrs FIRST, shifting the stamp schedule vs the hand-rolled
// last-stores. Logic byte-faithful; converted per the ALL-VTABLES mandate.
RVA(0x0016dff0, 0x73)
zPTree::zPTree(void(__cdecl* teardown)(void*), i32 n)
    : zErrHandling(&g_buteNodeErrMsg), CButeNodeEntry(n, teardown) {
    m_root = 0;
    m_lookupPending = 0;
}

RVA(0x0016e070, 0x7b)
void zPTree::ClearRecursive(CButeTreeNode* node) {
    CButeTreeNode* n = node;
    if (n == 0) {
        n = m_root;
        if (n == 0) {
            return;
        }
    }
    if (n->m_child[0] != 0 && n->m_child[0]->m_bit > n->m_bit) {
        ClearRecursive(n->m_child[0]);
    }
    if (n->m_child[1] != 0 && n->m_child[1]->m_bit > n->m_bit) {
        ClearRecursive(n->m_child[1]);
    }
    ::operator delete(n->m_key);
    if (m_kind & 2) {
        m_teardown(n->m_value);
        ::operator delete(n->m_value);
    }
    ::operator delete(n);
}

RVA(0x0016e0f0, 0x4)
__declspec(naked) void* GetCallerRetAddr() {
    __asm {
        mov eax, [ebp + 4]
        ret
    }
}

// ===========================================================================
// zBitVec::SetSize(nbits) (0x16e100, ex EngStr.cpp) - round the requested bit
// count up to whole 32-bit words, allocate + zero-fill the word band, and report
// the realized bit capacity (nwords*32). A request of <=32 bits leaves no band
// and reports 32.
// @early-stop
// one-instruction sar/shr wall (signed-shift-cast-reschedules.md): retail's
// `nbits >> 5` lowers to `shr` (the bit count is unsigned); our `int nbits`
// gives the byte-identical body EXCEPT that one shift as `sar`. Casting the
// operand to unsigned flips it to `shr` but reschedules the whole round-up
// block (this->esi vs arg->eax flow, lea vs shl for *4): 1 diff -> 11. So the
// signed `int` form (the closest, single-opcode miss) is kept.
RVA(0x0016e100, 0x7f)
i32 zBitVec::SetSize(i32 nbits) {
    if (static_cast<u32>(nbits) > 0x20) {
        i32 nwords = (nbits >> 5) + ((nbits & 0x1f) != 0 ? 1 : 0);
        m_capacity = nwords;
        u32* band = static_cast<u32*>(malloc(nwords * 4));
        m_words = band;
        if (!band) {
            return 0;
        }
        memset(band, 0, m_capacity << 2);
        m_capacity = m_capacity << 5;
        return 1;
    }
    m_words = 0;
    m_capacity = 0x20;
    return 1;
}

RVA(0x0016e1a0, 0x23)
CVariantSlot::CVariantSlot(void* owner) {
    m_typeTag = 2;
    m_10 = 2;
    // +0x00 (m_callback) is intentionally left unset here (the register path seeds it).
    m_valueWord = 0;
    m_label = static_cast<char*>(owner);
}

RVA(0x0016e1d0, 0x4b)
i32 CVariantSlot::Find(i32 key) {
    i32 hi = g_recCount23 - 1;
    i32 lo = 0;
    if (hi >= 0) {
        do {
            i32 mid = (lo + hi) / 2;
            m_04 = mid;
            i32 d = g_recs23[mid].m_key - key;
            if (d < 0) {
                lo = mid + 1;
            } else if (d <= 0) {
                return mid;
            } else {
                hi = mid - 1;
            }
        } while (lo <= hi);
    }
    m_04 = hi + 1;
    return -1;
}

// ===========================================================================
// TmErrorHandler (0x16e220) - the "C++ Tools error handler": the default callback
// seeded into the CVariantSlot +0x00 slot (the g_keyFinderVtbl fn-ptr;
// invoked by CVariantSlot::Set as m_callback(label, value) when a slot is unresolved).
// Builds "<prefix> - error #<errNum> Caller IP = <hhhh>\n" into a bounded stack buffer
// (the low 16 bits of the g_retAddrBreadcrumb caller-IP breadcrumb, 4 hex digits, and
// clears the breadcrumb), MessageBeep(0), pops a MessageBoxA titled "C++ Tools error
// handler", then FatalAppExitA + exit(1). __cdecl(prefix, errNum). The string operands
// + the three Win32 imports (MessageBeep/MessageBoxA/FatalAppExitA) reloc-mask.
// (re-homed from src/Stub/GapFunctions.cpp.)
// ===========================================================================
// @early-stop
// 98.2%, frame-slot-displacement residue: every instruction MATCHES (the 0x60 frame,
// the decimal itoa via idiv %10 + magic /10, the bounded prefix copy, the do-while
// 4-digit hex loop with the ebp old-index save, and the three Win32 fatal-report calls
// are all byte-faithful). The only difference is where MSVC5 seats the shared decimal/hex
// scratch + the message buffer within the (matched-size) frame - retail puts them at
// [esp+0x15]/[esp+0x18], this source at [esp+0x1b]/[esp+0x1c], shifting two lea/mov
// displacement bytes. A pure stack-slot-assignment coin-flip (buffer size / decl-order
// variations do not move it; the permuter finds no fix). Deferred to the final sweep.
RVA(0x0016e220, 0x139)
void TmErrorHandler(char* prefix, i32 errNum) {
    char tmp[16];
    char* np = &tmp[15];
    *np = 0;
    if (errNum != 0) {
        do {
            *--np = static_cast<char>((errNum % 10)) + '0';
            errNum = errNum / 10;
        } while (errNum != 0);
    }

    char msg[0x50];
    char* q = msg;
    while (0 != *prefix) {
        if (q >= &msg[0x40]) {
            break;
        }
        *q++ = *prefix++;
    }
    const char* s;
    s = " - error #";
    while (*s != 0) {
        *q++ = *s++;
    }
    while (*np != 0) {
        *q++ = *np++;
    }
    s = " Caller IP = ";
    while (*s != 0) {
        *q++ = *s++;
    }

    u32 v = 0xffff & reinterpret_cast<u32>(g_retAddrBreadcrumb);
    char* hp = &tmp[15];
    *hp = 0;
    i32 i;
    i = 7;
    do {
        i32 d = v & 0xf;
        *--hp = static_cast<char>((d > 9 ? d + 0x37 : d + 0x30));
        v >>= 4;
        if (4 == i) {
            break;
        }
    } while (i-- != 0);
    g_retAddrBreadcrumb = reinterpret_cast<void*>(v);
    while (*hp != 0) {
        *q++ = *hp++;
    }
    *q++ = '\n';
    *q = 0;

    MessageBeep(0);
    MessageBoxA(0, msg, "C++ Tools error handler", 0x2010);
    FatalAppExitA(0, "The error handler terminated the application");
    exit(1);
}

// ===========================================================================
// CVariantSlot::Add (0x16e360, the ex-Reg23 facet) - fixed-capacity (32) keyed
// record insert/update/remove over the global table at the cursor's m_04.
// ===========================================================================
// @early-stop
// Structure fixed 2026-07-21 (18% -> 88%): the INSERT path (idx==-1) is INLINE and the
// UPDATE/REMOVE path (idx!=-1) is OUT-OF-LINE (retail `cmp idx,-1; jne <update-far>`),
// and BOTH array shifts are `memcpy` (not memmove). The reconstruction had them reversed +
// memmove. Residual (~12%) is the genuine callee-saved register-preference wall: MSVC pins
// val->ebx/key->edi where retail uses key->ebx/val->edi, cascading through the stores.
// Not source-steerable; deferred.
RVA(0x0016e360, 0x11a)
void* CVariantSlot::Add(void* key, void* val) {
    int count = g_recCount23;
    if (val != 0 && count >= 0x20) {
        return 0;
    }
    int idx;
    if (count != 0) {
        idx = Find(reinterpret_cast<i32>(key));
    } else {
        idx = -1;
    }
    if (idx == -1) {
        if (val == 0) {
            return 0;
        }
        if (g_recCount23 != 0) {
            memcpy(
                &g_recs23[m_04 + 1],
                &g_recs23[m_04],
                (g_recCount23 - m_04) * sizeof(TypeKeyRec)
            );
        }
        g_recs23[m_04].m_4 = val;
        g_recs23[m_04].m_key = reinterpret_cast<i32>(key);
        g_recCount23 = g_recCount23 + 1;
        g_recs23[m_04].m_8 = 0;
        return 0;
    }
    void* old = g_recs23[idx].m_4;
    if (val != 0) {
        g_recs23[idx].m_4 = val;
        return old;
    }
    memcpy(
        &g_recs23[m_04],
        &g_recs23[m_04 + 1],
        (g_recCount23 - m_04 - 1) * sizeof(TypeKeyRec)
    );
    g_recCount23 = g_recCount23 - 1;
    return old;
}

RVA(0x0016e480, 0x3e)
i32 FirstDiffBit(const char* a, const char* b) {
    i32 n = 0;
    while (*a == *b) {
        n += 8;
        ++a;
        ++b;
    }
    i32 x = *a;
    x ^= *b;
    i32 c = 0;
    while (!(x & 1)) {
        x >>= 1;
        ++c;
    }
    return c + n;
}

static inline char* TypeResolve(i32 key) {
    g_typeColl.m_grown = 0;
    if (key >= g_typeColl.m_lo && key <= g_typeColl.m_hi) {
        return reinterpret_cast<char*>((g_typeColl.m_base + (key - g_typeColl.m_lo) * g_typeColl.m_stride));
    }
    if (reinterpret_cast<i32>((static_cast<_zvec*>(&g_typeColl))->GrowTo(key, 0))) {
        return reinterpret_cast<char*>((g_typeColl.m_base + (key - g_typeColl.m_lo) * g_typeColl.m_stride));
    }
    void* item = g_projActCache;
    g_retAddrBreadcrumb = GetRetAddr();
    g_typeColl.m_errSink->Set(&g_typeColl, reinterpret_cast<i32>(item), 0xc);
    return reinterpret_cast<char*>(g_typeColl.m_spare);
}

static inline void FreeNodes() {
    CStringNode* nodes = reinterpret_cast<CStringNode*>(g_typeColl.m_alloc);
    i32 cnt = g_typeColl.m_grown;
    if (cnt != 0) {
        do {
            if (nodes != 0) {
                (reinterpret_cast<CString*>(nodes))->~CString();
            }
            ++nodes;
        } while (--cnt);
    }
}

// @early-stop
// register-scheduling wall: the two inlined TypeResolve copies + the node-free
// count-down loops + the interleaved archive slot dispatches pin the saved regs in
// a spill order MSVC reproduces only for one allocation; logic + offsets + the
// inlined lookup + the slot conventions are byte-faithful. Deferred to the final sweep.
RVA(0x0016e4f0, 0x19b)
i32 ProjTypeXfer(CUserLogic* ar) {
    CTypeNameEntry* entry =
        reinterpret_cast<CTypeNameEntry*>(TypeResolve(reinterpret_cast<i32>(ar->m_objAux->m_1c)));
    FreeNodes();
    ar->XferName(entry->m_name.GetBuffer(0)); // 0x1ba11c ?GetBuffer@CString@@QAEPADH@Z
    ar->FireActivation(reinterpret_cast<i32>(ar->m_objAux->m_1c));

    entry = reinterpret_cast<CTypeNameEntry*>(TypeResolve(reinterpret_cast<i32>(ar->m_objAux->m_1c)));
    FreeNodes();
    ar->FinalizeStep(reinterpret_cast<i32>(entry->m_name.GetBuffer(0)));
    return 1;
}

// ===========================================================================
// `dynamic initializer for g_buteTree' (0x16e6a0) - construct the global bute
// tree (deeper base ctor 0x16dff0) then stamp its runtime vtables.
// ===========================================================================
// g_buteTree is the canonical CButeTree (crit-bit trie), now `CButeTree : public zPTree`
// (<Bute/ButeTree.h>). Construct (0x16dff0 == the zPTree base ctor above) runs the base
// ctor; the +0x00 / +0x08 runtime vptr re-stamps (0x5f04e0/0x5f04dc, CButeTree's most-
// derived vtables) are dropped here - they emit only when g_buteTree is DEFINED (not
// extern) as a real global (see the ??_GCButeTree @emission-TODO below). Kept extern +
// hand-written this pass; @early-stop until the coupled emission lands.
// @early-stop
// hand-written construct (~71%): the CButeTree runtime vptr re-stamps are elided (they
// need g_buteTree defined so cl auto-emits the ctor+vtables). Deferred to the coupled
// emission (docs at ??_GCButeTree below).
// Canonical name->RVA pin for the g_buteTree singleton (?g_buteTree@@3VCButeTree@@A
// @0x6bf620): this is its construction owner (DynInitButeTree below). The type-complete
// decl lives in <Bute/ButeTree.h> (included above); consumers reach it there. DATA() in
// a header is ignored, so the reloc-name binding is pinned here, on the owner TU's decl.
DATA_SYMBOL(0x002bf620, 0x2c, ?g_buteTree@@3VCButeTree@@A)

void ButeTreeNopFree(void*);

RVA(0x0016e6a0, 0x26)
void DynInitButeTree() {
    g_buteTree.Construct(static_cast<void*>(&ButeTreeNopFree), 0);
}

// @early-stop
// `atexit destructor for g_buteTree' (??__F, 0x16e6e0) - homed from GapFunctions.cpp
// (matcher-5); the teardown thunk MSVC registers via atexit for the DynInit above. It
// restamps g_buteTree's dtor vptrs (0x5e94ac / 0x5e949c), runs the member-dtor chain
// (0x16e070 on &g_buteTree, 0x16cfc0 on the +8 subobject via the `p?&p->m8:0` neg/sbb/and
// idiom) and tail-jmps the base dtor 0x16ca60. A COMPILER-GENERATED atexit thunk: emitting
// it byte-exact needs g_buteTree defined (not extern) with the real ~CButeTree chain, which
// conflicts with the hand-written DynInit; homed pending that restructure. The
// name states the proven ??__F identity; RVA_COMPGEN(0x16e6e0, 0x3e,
// ??__Fg_buteTree@@YAXXZ) is the target model, MISSING until that emission lands.
RVA(0x0016e6e0, 0x3e)
i32 ButeTreeAtexitDtor(void) {
    return 0;
}

inline void* operator new(u32, void* p) {
    return p;
}

// ===========================================================================
// `dynamic initializer for g_typeColl' (0x16e730) - construct the shared key
// collection with the [0x7d0, 0x7da] id range, stamp its runtime vtable, then
// free the (initially stale) node array. The construct is a placement-new of the
// real zDArray ctor (0x16dda0) over the global; the runtime re-stamp swaps in
// the live g_typeCollRunVtbl over the just-built construction vtable.
// ===========================================================================
// @early-stop
// placement-new null-guard + count-down-induction wall (~70%). Two residues:
// (1) constructing g_typeColl now goes through the REAL zDArray ctor via a
// placement-new (the only way to invoke a ctor on the pre-pinned extern global);
// MSVC5 emits the placement null-guard `mov eax,&g_typeColl; test eax,eax; je`
// that retail's direct in-place build lacks - intrinsic to placement-new of a
// non-trivial ctor, not source-steerable. (2) the node-free loop: retail
// materializes the counter via the `ecx=cnt; eax=cnt-1; lea edi,[eax+1]`
// strength-reduced idiom and shrink-wraps `push edi`; cl loads the count plainly.
// Same not-source-steerable idiom as CKitchenSlime/CProjectile::RegisterType.
// Deferred to the final sweep.
RVA(0x0016e730, 0x51)
void DynInitTypeColl() {
    new (&g_typeColl) zDArray(4, 0x7d0, 0x7da, reinterpret_cast<void*>(1));
    CStringNode* nodes = reinterpret_cast<CStringNode*>(g_typeColl.m_alloc);
    // vptr install dropped -> compiler-emitted vtable (% ok per drive-to-0)
    if (nodes != 0) {
        i32 cnt = g_typeColl.m_grown;
        if (cnt != 0) {
            do {
                if (nodes != 0) {
                    (reinterpret_cast<CString*>(nodes))->~CString();
                }
                ++nodes;
            } while (--cnt);
        }
    }
}

// @early-stop
// `atexit destructor for g_typeColl' (??__F, 0x16e7a0) - homed from GapFunctions.cpp
// (matcher-5); the teardown thunk atexit-registered for DynInitTypeColl above. It restamps
// g_typeColl's runtime vptr (0x5f04e4), frees the node array (the same count-down loop
// calling the element dtor 0x1b8cde), then drains g_typeColl @0x6bf650 via 0x16cf40. A
// COMPILER-GENERATED atexit thunk (same restructure blocker as ??__Fg_buteTree above);
// homed pending g_typeColl defined with its real ~zDArray chain.
RVA(0x0016e7a0, 0x48)
i32 TypeCollAtexitDtor(void) {
    return 0;
}

// ===========================================================================
// CButeTree::`scalar deleting destructor' ??_GCButeTree (0x16e9c0) - the compiler-
// synthesized slot-0 scalar-deleting dtor of CButeTree's virtual destructor (declared
// in <Bute/ButeTree.h>): cl inlines the dtor teardown (dtor-phase vptr re-stamps
// 0x5e94ac @+0 / 0x5e949c @+8, ClearRecursive(0) @0x16e070, the second-base restore
// @0x16dfc0 on the masked this+8, then the primary BaseDtor @0x16da60), then
// ::operator delete when bit0 of the deleting-flag is set; returns this. It is NOT
// dev source (a compiler ??_G thunk), so it is pinned by mangled name here, not
// written as a method. (zPTree==CButeTree, the same 0x2c-byte class.)
//
// DEFERRED EMISSION (reloc): this RVA_COMPGEN binds only once cl EMITS ??_GCButeTree in
// THIS obj, which needs g_buteTree defined (not extern) as a real global CButeTree so
// its compiler-generated ctor/dtor/vtable emit here. That is BLOCKED by a dual-vtable
// identity (the butenode-dual-model, OUT OF SCOPE for this pass), PROVEN by disasm:
//   * construction (DynInitButeTree 0x16e6a0) stamps ??_7CButeTree     @0x5f04e0 /
//     ??_7CButeStore@@6BCButeStoreSecond @0x5f04dc   (the CButeTree identity);
//   * destruction (0x16e9c0 ??_G AND 0x16e6e0 atexit) stamps ??_7zPTree @0x5e94ac /
//     ??_7CButeStore@@6BCButeNodeEntry  @0x5e949c    (the zPTree/zPTree identity).
// CORRECTION (RTTI-proven): the split is NOT irreducible. It is exactly what
// `class CButeTree : public zPTree` (now modeled, <Bute/ButeTree.h>) with an EMPTY
// ~CButeTree produces - the ctor re-stamps CButeTree's most-derived 0x5f04e0/0x5f04dc
// after the zPTree base ctor; the empty derived dtor elides its protective re-stamp so
// ??_G goes straight to ~zPTree (which stamps zPTree's 0x5e94ac/0x5e949c). Real classes
// are zErrHandling(@0), zPtrColl(fabricated "CButeNodeEntry", @8), zPTree : those.
// DEFERRED EMISSION (COUPLED, not this pass): binding 0x16e9c0 needs g_buteTree DEFINED so
// cl emits ??_G/??__E/??__F/the CButeTree vtables. Requires as one atomic unit: (1) an
// INLINE ~zPTree { ClearRecursive(0); } + ClearRecursive as a zPTree method (0x16e070),
// which forces zPTree : zPTree so its Reset/leaf-dtor callers still bind faithfully;
// (2) DATA_SYMBOL on the emitted ??_7CButeTree@@6BCButeNodeEntry@@ @0x5f04dc (the
// fabricated ??_7CButeStore@@6BCButeStoreSecond@@ name won't auto-match) + butemgr
// dropping that binding; (3) RVA_COMPGEN pins for the cl-mangled init/atexit thunks. Gate on
// ClearRecursive/~CButeMgr staying byte-exact (the +0x10 store-flag is a BYTE read;
// zPTree's m_kind is i16 - a real byte-risk to the 100% ClearRecursive).
RVA_COMPGEN(0x0016e9c0, 0x45, ??_GCButeTree@@UAEPAXI@Z)

RVA(0x0016ea10, 0x1)
void ButeTreeNopFree(void*) {}

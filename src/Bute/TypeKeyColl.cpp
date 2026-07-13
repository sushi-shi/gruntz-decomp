// TypeKeyColl.cpp - the shared container/type-registry engine TU at retail
// .text [0x16d000 .. 0x16e7e8] (C++ Tools container library + the type-key
// registries; C:\Proj\incs).
//
// ONE original TU (wave2-H merge): the former typekeycoll + butetree(interval
// fns) + userbaselink(interval fns) units were a WOVEN single interval
// (TU_MIGRATION 0x16d000, weave 0.31; typekeycoll's own 7-frag init run
// @0x16d6f0 sits MID-interval between userbaselink's 0x16d3a0 and 0x16d710 -
// one obj), plus the seven in-interval strays proven by first-link contiguity:
// zBitVec(i32,i32) (ex ProjActCache.cpp), _zvec::GrowTo + ~zDArray (ex
// ZVec.cpp), CButeNodeEntry/zPTree ctors (ex ButeNode.cpp),
// CButeStore::ClearRecursive (ex ButeStoreClear.cpp), zBitVec::SetSize +
// ~CContainerErr (ex EngStr.cpp), and Reg23::Add (ex Registry23.cpp - Reg23 IS
// CKeyFinder: its Find was the same 0x16e1d0, its m_4 the same +0x04 index;
// folded onto the one class here).
//
// STILL ELSEWHERE (documented walls): the CContainerErr/zErrHandling ctor
// (0x16d9c0) stays in GameText.cpp - it needs the deliberately NON-virtual
// (the old "GameText.h and zBitVec.h never coexist" wall is DEAD - GameText.h had one
//  includer; its duplicate CContainerErr view is folded onto zBitVec.h's)
// in one TU). The CZArrayRoot/zErrHandling/CContainerErr and CZArray2D/zDArray
// dual models are a known pending dedup (one real class each).
//
// All callees into the deeper engine (the base ctor, the error/insert helper,
// the CRT alloc/free) are external no-body so their call rel32 / the vtable +
// global DIR32 stores reloc-mask in objdiff. Field names are placeholders; only
// OFFSETS + code bytes are load-bearing (campaign doctrine).
#include <Mfc.h>
#include <iostream.h>       // the REAL istream the config reader is (operator>> @0x191fe0/0x191f30)
#include <Bute/ButeTree.h>  // canonical CButeTree / CVariantSlot / CButeTreeNode (one shape)
#include <Bute/PTreeNode.h> // zErrHandling / CButeNodeEntry / zPTree (the .bute node family)
#include <Bute/ButeStore.h> // CButeStore / CButeStoreNode (the keyed-store family)
#include <Wap32/zBitVec.h>  // CContainerErr / zBitVec + the container-error globals
#include <Gruntz/UserBaseLink.h> // CUserBaseLink (the +0x18 link sub-object; embeds a zBitVec)
#include <rva.h>
#include <ctype.h>  // isspace (0x12f8a0) / isdigit (0x12f840) - as FUNCTION calls
#include <stdlib.h> // malloc (0x120b60) / realloc (0x125180) / free (0x120c30)
#include <string.h> // memset (rep stos) / inline strcpy / strchr / memmove / memcpy

// The zBitVec parser calls the isspace/isdigit LIBRARY functions (retail does, not
// the table macro), and operator=/GrowTo call memcpy out-of-line; force all three.
#undef isspace
#undef isdigit
#pragma function(memcpy)

#include <Gruntz/StringNode.h>    // the type-name teardown slot
#include <Gruntz/TypeKeyColl.h>   // CZErrSink/CZArrayRoot/CZArray2D/CTypeKeyColl (one shape)
#include <Gruntz/TypeNameEntry.h> // the shared type-name-registry record (CString m_name)
#include <Gruntz/XferArchive.h>   // canonical CXferArchive/CXferField (ProjTypeXfer arg)
#include <Globals.h>
#include <Wap32/ZVec.h>
#include <Wap32/ZDArrayDerived.h>

// ===========================================================================
// Vtables (UNMATCHED engine tables - stamped by address, reloc-masked DIR32).
//
// The CZArrayRoot/CZArray2D/CTypeKeyColl vtables (0x5f04cc/0x5f04d4/0x5f04d0) are
// NO LONGER externs: that 1-slot-each construction hierarchy is now modeled as
// REAL polymorphic C++ (virtual dtor per level), so cl emits the implicit ??_7
// vptr stamp in each ctor (reloc-masked) instead of a manual stamp. Likewise
// ~zDArray below auto-emits ??_7zDArray (bound at the retail dtor-vtable RVA).
// ===========================================================================
// g_keyFinderVtbl is NOT a vtable: 0x16e220 is a FUNCTION in .text (the default
// callback the CKeyFinder/CVariantSlot +0x00 slot is seeded with) - stored as a
// plain fn-ptr field init, not a polymorphic vptr, so it stays a manual store.

// The live zDArray<...> vtable Destroy() re-stamps lives with Destroy in
// ZVec.cpp; ~zDArray's emitted ??_7zDArray is bound at the retail dtor-entry
// vtable RVA here (the annotation moved with the dtor).
VTBL(zDArray, 0x001f04d4); // ~zDArray-entry vtable (0x5f04d4)

// ===========================================================================
// The registry globals (BSS / .data; DATA-pinned so the loads reloc-mask).
// (g_retAddrBreadcrumb / g_projActCache / g_projActName / g_containerName /
// g_defaultProjActSize + GetCallerRetAddr come from <Wap32/zBitVec.h>.)
// ===========================================================================
DATA(0x002bf468)
u8 g_zArrayTag; // 0x6bf468 (owner-TU def; the CZArrayRoot base-tag byte, &g_zArrayTag)
DATA(0x002bf658)
extern i32 g_typeLo;
DATA(0x002bf65c)
extern i32 g_typeHi;
// @identity-TODO INTERIOR-OFFSET CLUSTER - do NOT "fix" these by defining them.
// g_typeColl (0x6bf650) and the eight scalars around it are ONE object, not nine globals:
//     g_typeColl +0x00   g_typeColl2 +0x04  g_typeLo  +0x08  g_typeHi    +0x0c
//     g_typeBase +0x10   g_typeCur   +0x14  g_typeStride +0x18  g_typeNodes +0x1c
//     g_typeCount +0x20
// which is EXACTLY <Gruntz/ActReg.h>'s CActReg field map (m_coll/m_coll2/m_lo/m_hi/m_base/
// m_cur/m_stride/pad1c/m_scratch), and GruntVoice.cpp's ActNameLookup is CActReg::ResolveEntry
// hand-inlined over them, statement for statement. So the activation-NAME registry at
// 0x6bf650 is a CActReg like every other one.
// I promoted five of these to DEFINITIONS in this batch to clear them off the undefined-DATA
// list, and that was WRONG - it fabricates five globals at interior offsets of a real object.
// Reverted. The correct fix is to SUBSUME them onto one CActReg (as done for the two
// registries in GruntVoice.cpp), but they are referenced by ~20 TUs, so that is its own pass -
// not a drive-by. Left undefined and honest until then.
DATA(0x002bf660)
extern char* g_typeBase;
DATA(0x002bf668)
extern i32 g_typeStride;
DATA(0x002bf670)
extern i32 g_typeCount;
DATA(0x002bf66c)
extern void* g_typeNodes;

// The bad-argument / bad-character diagnostic name cell the zBitVec parser reports
// through (distinct from g_projActName @0x6bf454; this one @0x6bf45c). Reloc-masked.
extern void* g_projActName2; // 0x6bf45c

// g_containerName (0x2bf408, char[] in <Wap32/zBitVec.h>) - the CContainerErr base-ctor
// name arg the zBitVec ctors pass. Bound via @data-symbol (the char[] extern mangles
// `?g_containerName@@3PADA` under cl; scanned per-.cpp so it lives here, not the header).
// g_defaultProjActSize (0x21ad28, i32 in zBitVec.h) - the fallback capacity the
// default/HH zBitVec ctors size to.
// @data-symbol: ?g_containerName@@3PADA 0x002bf408
// @data-symbol: ?g_defaultProjActSize@@3HA 0x0021ad28

// The container OOM message the _zvec grow path reports (0x61adf4).
DATA(0x0021adf4)
extern const char s_out_of_memory[]; // 0x61adf4

// The deeper-base ctor argument (a data tag global at 0x6bf468).

// The "Inconsistent bounds" / "out of memory" message strings are emitted as
// literals (their DIR32 references reloc-mask against the retail $SG symbols).

// ===========================================================================
// External engine leaves (no body - call rel32 reloc-masks).
// ===========================================================================
// CZErrSink (the fatal-alloc/bounds error sink stored at +0x04) is the shared
// <Gruntz/TypeKeyColl.h> shape.
// The CKSlimeColl2 dispatcher the type-name lookup grows the collection through.
DATA(0x002bf654)
extern CVariantSlot* g_typeColl2;

// The zDArray construction hierarchy CZArrayRoot <- CZArray2D <- CTypeKeyColl and
// the leaf ??_7CTypeKeyColl @0x5f04d0 are the shared <Gruntz/TypeKeyColl.h> shape.
DATA(0x002bf650)
extern CTypeKeyColl g_typeColl; // 0x6bf650

// The engine heap alloc/free ARE the global operator new (0x1b9b46 = ??2) / operator
// delete (0x1b9b82 = ??3); called as ::operator new/delete (no decl needed).
extern void* GetRetAddr(); // 0x16d990 (the _ReturnAddress breadcrumb capture)

// The first-differing-bit (crit-bit index) of two keys (__cdecl). The name
// matches the delinker's symbol for 0x16e480 so the `call` reloc pairs; defined
// below at its retail RVA.
i32 FirstDiffBit(const char* a, const char* b); // 0x16e480

// ===========================================================================
// CKeyFinder - the binary-search cursor over the sorted global key table (the
// 12-byte-stride record array @0x6bf498 with its count @0x6bf618). Its ctor
// (0x16e1a0) seeds the two small constant fields and stows the owner; Find
// (0x16e1d0) bisects for `key`, recording the found index (or the insertion
// point) into +0x04; Add (0x16e360, the ex-"Reg23" facet - same class, same
// +0x04 slot, same Find) inserts/updates/removes a record at the cursor.
// ===========================================================================
SIZE_UNKNOWN(CKeyFinder);
struct CKeyFinder {
    char _vft0[4];           // +0x00 foreign/base object vptr (reduced view; not owned/dispatched)
    i32 m_index;             // +0x04  found index / insertion point (the ex-Reg23 m_4)
    u16 m_08;                // +0x08
    u16 m_0a;                // +0x0a  (padding)
    i32 m_0c;                // +0x0c  = 2
    i32 m_10;                // +0x10  = 2
    void* m_owner;           // +0x14
    CKeyFinder(void* owner); // 0x16e1a0
    i32 Find(i32 key);       // 0x16e1d0
    void* Add(void* key, void* val); // 0x16e360
};

// The sorted key table: 12-byte records {key, value/fn, flag-word} @0x6bf498.
// Find reads it as a flat i32[] (stride 3); Set dispatches through the value/fn
// member at +4 (the g_varTable alias @0x6bf49c = &g_recs23[0].m_4); the count
// @0x6bf618 doubles as Set's probe-enable gate (three names, one datum each).
SIZE_UNKNOWN(Rec23);
struct Rec23 {
    i32 m_key; // +0x00  the key (CKeyFinder::Find subtracts the probe key from it)
    void* m_4; // +0x04  value
    short m_8; // flag
    short m_a;
};
DATA(0x002bf498)
extern Rec23 g_recs23[];
// The live record count @0x6bf618. It had THREE names for the one datum (g_keyCount,
// g_recCount23, g_varProbeEnabled) - and the DATA() sat on an `extern`, i.e. a
// declaration, so none of them was actually a defined global. Defined once, here, in the
// TU that owns the table; every reader uses this name. (CVariantSlot::Set's "probe
// enabled" gate is just `count != 0`.)
extern "C" {
    DATA(0x002bf618)
    i32 g_recCount23;
}
// (The `g_keyArray` / `g_keyCount` aliases are GONE. They were a second, flat i32-view of
//  these very globals - declared, never defined, so ?g_keyArray@@3PAHA / ?g_keyCount@@3HA
//  were two guaranteed unresolved externals. CKeyFinder::Find walks the real records: its
//  stride-3 i32 scan IS g_recs23[mid].m_key, the array's 12-byte stride.)

// The slot's resolved-index dispatch table (12-byte stride): a __cdecl fn at +0,
// a word slot at +4. == (char*)g_recs23 + 4. Reloc-masked DATA extern.
SIZE_UNKNOWN(CVarTableEntry);
struct CVarTableEntry {
    void(__cdecl* fn)(i32 a, i32 b); // +0x00
    u16 w;                           // +0x04
    char m_pad06[12 - 6];            // 12-byte stride
};
DATA(0x002bf49c)
extern CVarTableEntry g_varTable[]; // 0x6bf49c
DATA(0x002bf618)

// The slot label formatter (__cdecl(buf, value, cap)).
extern "C" void Format_18d0f0(char* buf, i32 value, i32 cap); // 0x18d0f0

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
    *r >> *(double*)(d + 0x00);
    *r >> *(double*)(d + 0x08);
    *r >> *(double*)(d + 0x10);
    *r >> *(double*)(d + 0x18);
    *r >> *(double*)(d + 0x20);
    *r >> *(double*)(d + 0x28);
    *r >> *(double*)(d + 0x30);
    *r >> *(double*)(d + 0x38);
    *r >> *(double*)(d + 0x40);
    *r >> *(double*)(d + 0x48);
    *r >> *(double*)(d + 0x50);
    *r >> *(double*)(d + 0x70);
    *r >> *(double*)(d + 0x78);
    *r >> *(double*)(d + 0x80);
    *r >> *(double*)(d + 0x88);
    *r >> *(double*)(d + 0x90);
    *r >> *(double*)(d + 0x98);
    *r >> *(double*)(d + 0xa0);
    *r >> *(double*)(d + 0xa8);
    *r >> *(double*)(d + 0xb0);
    *r >> *(int*)(d + 0xb8);
    *r >> *(double*)(d + 0xc0);
    *r >> *(double*)(d + 0xc8);
    *r >> *(double*)(d + 0xd0);
    *r >> *(double*)(d + 0xd8);
    *r >> *(double*)(d + 0xe0);
    *r >> *(double*)(d + 0xe8);
    *r >> *(double*)(d + 0xf0);
    *r >> *(double*)(d + 0xf8);
    *r >> *(double*)(d + 0x100);
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
        m_errSink->Set(this, (i32)name, 0x16);
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
// zBitVec::~zBitVec() (0x16d2a0) - free the heap band when out of SBO range,
// then chain ~CContainerErr (implicit).
// ===========================================================================
RVA(0x0016d2a0, 0x26)
zBitVec::~zBitVec() {
    if ((u32)m_capacity > 0x20) {
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
            if ((u32)m_capacity > 0x20) {
                ::operator delete(m_words);
            }
            if ((u32)that.m_capacity > 0x20) {
                m_words = (u32*)malloc(((u32)that.m_capacity >> 5) * 4);
                if (!m_words) {
                    void* cache = g_projActCache;
                    g_retAddrBreadcrumb = GetCallerRetAddr();
                    m_errSink->Set(this, (i32)cache, 0xc);
                    m_capacity = 0x20;
                    return *this;
                }
            }
            m_capacity = that.m_capacity;
        }
        const u32* src = ((u32)that.m_capacity > 0x20) ? that.m_words : (const u32*)&that.m_words;
        u32* dst = ((u32)m_capacity > 0x20) ? m_words : (u32*)&m_words;
        memcpy(dst, src, (u32)m_capacity >> 3);
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
zBitVec::zBitVec(const char* tokens, i32 minSize) : CContainerErr(g_containerName) {
    i32 maxv = 0;
    const char* start;
    const char* q;
    if (tokens == 0) {
        void* name = g_projActName;
        g_retAddrBreadcrumb = GetCallerRetAddr();
        m_errSink->Set(this, (i32)name, 0x16);
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
        if ((u32)v > (u32)maxv) {
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

    if ((u32)minSize > (u32)maxv) {
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
            u32* band = ((u32)m_capacity > 0x20) ? m_words : (u32*)&m_words;
            band[(u32)v >> 5] |= 1u << (v & 0x1f);
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
                u32* band = ((u32)m_capacity > 0x20) ? m_words : (u32*)&m_words;
                band[(u32)b >> 5] |= 1u << (b & 0x1f);
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
    m_errSink->Set(this, (i32)cache, 0xc);
    return;
}
badchar: {
    void* name = g_projActName2;
    g_retAddrBreadcrumb = GetCallerRetAddr();
    m_errSink->Set(this, (i32)name, 0x16);
    return;
}
}

// ===========================================================================
// zBitVec() default ctor - build the base, size to g_defaultProjActSize, no bit
// set. INLINE so it folds into CUserBaseLink::CUserBaseLink() (0x16d710), the
// only site that default-constructs the link's zBitVec name.
// ===========================================================================
inline zBitVec::zBitVec() : CContainerErr(g_containerName) {
    if (!SetSize(g_defaultProjActSize)) {
        void* cache = g_projActCache;
        g_retAddrBreadcrumb = GetCallerRetAddr();
        m_errSink->Set(this, (i32)cache, 0xc);
    }
}

// The +0x18 link: a zBitVec name field, default-constructed (0x16d710; can throw ->
// the /GX EH frame every leaf ctor inherits).
RVA(0x0016d710, 0x76)
CUserBaseLink::CUserBaseLink() {}

// ===========================================================================
// zBitVec(idx, sizehint) (0x16d790, ex ProjActCache.cpp) - construct the
// CContainerErr error-tracking base (cl re-stamps the implicit most-derived
// ??_7zBitVec vptr after it returns), size the bit-vector to cover `idx`, then
// set bit `idx`; on a sizing failure record the caller return address and fire
// the error sink. The destructible polymorphic base forces the /GX frame.
// ===========================================================================
// @early-stop
// /GX EH-epilogue + RMW-fusion wall (topic:eh topic:regalloc; see docs/patterns/
// identical-return-epilogue-tailmerge.md). Logic, recovered types (CContainerErr/
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
zBitVec::zBitVec(i32 idx, i32 sizehint) : CContainerErr(g_containerName) {
    u32 n = (u32)sizehint;
    if (n == 0) {
        n = (u32)g_defaultProjActSize;
    }
    if ((u32)idx >= n) {
        n = (u32)idx + 1;
    }
    if (!SetSize((i32)n)) {
        void* cache = g_projActCache;
        g_retAddrBreadcrumb = GetCallerRetAddr();
        m_errSink->Set(this, (i32)cache, 0xc);
    } else {
        u32* base = ((u32)m_capacity > 0x20) ? m_words : (u32*)&m_words;
        u32* slot = base + ((u32)idx >> 5);
        *slot |= 1u << (idx & 0x1f);
    }
}

// ===========================================================================
// 0x16d850 - variant/property setter (the HOT helper, ret 0xc).  Switches on the
// slot's type tag (m_0c): 4 -> store the low word directly; otherwise probe the
// global index table (when enabled) and, by tag 2/1, either dispatch through the
// resolved table entry's fn / word slot, or (unresolved) format the slot's label
// + invoke the slot's own +0x00 callback / store the word.  The index probe is
// CKeyFinder::Find (0x16e1d0, defined below); the table + gate are globals.
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
    if (m_0c == 4) {
        m_08 = (u16)arg3;
        return;
    }
    i32 idx;
    if (g_recCount23 != 0) {
        idx = ((CKeyFinder*)this)->Find((i32)key);
    } else {
        idx = -1;
    }
    if (idx == -1) {
        if (m_0c == 2) {
            char buf[0x94];
            strcpy(buf, m_14);
            Format_18d0f0(buf, arg2, 0x4f);
            m_callback(buf, arg3);
        } else if (m_0c == 1) {
            m_08 = (u16)arg3;
        }
    } else {
        if (m_0c == 2) {
            g_varTable[idx].fn(arg2, arg3);
        } else if (m_0c == 1) {
            g_varTable[idx].w = (u16)arg3;
        }
    }
}

// ===========================================================================
// CContainerErr::~CContainerErr() (0x16da60, ex EngStr.cpp) - the compiler
// auto-stamps ??_7CContainerErr at dtor entry (matching retail's stamp-first
// order), then unregisters the error handler. Real-polymorphic (manual
// vptr-field stamp drained): cl's implicit dtor-entry store lands stamp-first,
// exactly as retail does here, so this is byte-exact. (The CTOR at 0x16d9c0
// stays in GameText.cpp - it needs the vptr-LAST non-virtual dual-view.)
// ===========================================================================
RVA(0x0016da60, 0x12)
CContainerErr::~CContainerErr() {
    // 0x16e360 is CKeyFinder::Add (the cursor's insert/update/remove facet), reached on
    // the error sink; cast at the call (the sink is a CKeyFinder cursor over the key table).
    ((CKeyFinder*)m_errSink)->Add(this, 0);
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
        p = realloc((void*)m_base, (m_hi - (idx - at) + 1) * m_stride);
        if (!p) {
            g_retAddrBreadcrumb = GetCallerRetAddr();
            m_errSink->Set((void*)this, (u32)s_out_of_memory, 0x22);
            return 0;
        }
        i32 oldbytes = (m_hi - m_lo + 1) * m_stride;
        i32 shift = m_lo - (idx - at);
        m_grown = shift;
        m_alloc = (i32)p;
        memcpy((char*)p + shift * m_stride, p, oldbytes);
        memset((char*)m_alloc, 0, m_grown * m_stride);
        m_lo = idx - at;
        m_base = (i32)p;
        return p;
    }
    i32 hinew = idx + at;
    p = realloc((void*)m_base, (hinew - m_lo + 1) * m_stride);
    if (!p) {
        g_retAddrBreadcrumb = GetCallerRetAddr();
        m_errSink->Set((void*)this, (u32)s_out_of_memory, 0x22);
        return 0;
    }
    i32 oldbytes = (m_hi - m_lo + 1) * m_stride;
    char* fill = (char*)p + oldbytes;
    m_grown = hinew - m_hi;
    m_alloc = (i32)fill;
    memset(fill, 0, m_grown * m_stride);
    m_hi = hinew;
    m_base = (i32)p;
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
        m_errSink->Set(this, (i32) "No prior lookup", 0x16);
        return 0;
    }
    i32 newbit = m_keyBitLength - 7;
    m_lookupPending = 0;
    m_keyBitLength = newbit;
    if (key == 0 || value == 0) {
        void* name = g_projActName;
        g_retAddrBreadcrumb = GetCallerRetAddr();
        m_errSink->Set(this, (i32)name, 0x16);
        return 0;
    }

    i32 critbit;
    if (m_candidateLeaf != 0) {
        critbit = FirstDiffBit(key, m_candidateLeaf->m_key);
    } else {
        critbit = newbit - 1;
    }

    CButeTreeNode* node = (CButeTreeNode*)::operator new(0x14);
    if (node != 0) {
        node->m_value = value;
        node->m_bit = critbit;
        char* keybuf = (char*)::operator new((m_keyBitLength >> 3) + 1);
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
    m_errSink->Set(this, (i32)cache, 0xc);
    return 0;
}

// ===========================================================================
// CTypeKeyColl::CTypeKeyColl (0x16dda0) - the derived ctor. Forwards the four
// arguments to the 2D-array base ctor (0x16de30), then derives the cursor (==
// the primary buffer) and the element count (hi - lo + 1). cl emits the implicit
// ??_7CTypeKeyColl vptr stamp (was `*(void**)this = &g_typeKeyCollVtbl`). No EH
// frame of its own (the base ctor owns the unwind state).
// ===========================================================================
RVA(0x0016dda0, 0x3c)
CTypeKeyColl::CTypeKeyColl(i32 stride, i32 lo, i32 hi, void* scratch)
    : zDArray(stride, lo, hi, scratch) {
    m_alloc = m_base;              // +0x1c  the fresh band base (was the m_cursor view)
    m_grown = m_hi - m_lo + 1;     // +0x20  its slot count (was the m_count view)
}

// ===========================================================================
// zDArray::zDArray (0x16de30) - the allocating vector ctor (this body was modelled
// as `CZArray2D::CZArray2D` until the fold: CZArray2D IS zDArray, one class under two
// names - its vtable 0x1f04d4 is the datum VTBL(zDArray) binds and its ??1 0x16df40 is
// ~zDArray, both right here in this TU). Builds the CContainerErr base (0x16d9c0, was
// the duplicate `CZArrayRoot` model), records the [lo, hi] bounds + element stride,
// allocates the (hi-lo+1)*stride element band (+ a scratch element when none was
// supplied), and reports a fatal "Inconsistent bounds" / "out of memory" through the
// inherited error sink. cl emits the implicit ??_7zDArray vptr stamp and the /GX unwind
// frame (the partially-built CContainerErr subobject must be destroyed if a later
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
zDArray::zDArray(i32 stride, i32 lo, i32 hi, void* scratch)
    : _zvec(&g_zArrayTag) { // -> the CContainerErr base ctor @0x16d9c0
    m_spare = (i32)scratch;         // +0x14  scratch element (was the m_buf2 view)
    m_lo = lo;
    m_hi = hi;
    m_base = 0; // +0x10  element band (was the m_buf view)
    m_stride = stride;
    if (lo > hi) {
        g_retAddrBreadcrumb = GetCallerRetAddr();
        m_errSink->Set((void*)this, (i32) "Inconsistent bounds", 0x16);
        return;
    }
    i32 total = (hi - lo + 1) * stride;
    void* buf = malloc(total);
    m_base = (i32)buf;
    if (buf != 0) {
        memset(buf, 0, total);
        if (m_spare != 0) {
            return;
        }
        m_spare = (i32)malloc(m_stride);
        if (m_spare != 0) {
            return;
        }
    }
    g_retAddrBreadcrumb = GetCallerRetAddr();
    m_errSink->Set((void*)this, (i32) "out of memory", 0xc);
}

// ===========================================================================
// zDArray::~zDArray() (0x16df40, ex ZVec.cpp) - cl auto-stamps the derived dtor
// vtable (??_7zDArray) at entry, then frees the band and chains to the base
// dtor. Real-polymorphic: the manual dtor-vtable stamp is drained (cl's
// implicit dtor-entry stamp replaces it, reloc-masked); byte-exact.
// ===========================================================================
RVA(0x0016df40, 0x22)
zDArray::~zDArray() {
    i32 p = m_base;
    if (p) {
        free((void*)p);
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
    m_kind = (i16)n;
    m_nodeCount = 0;
}

// ===========================================================================
// CButeNodeEntry::~CButeNodeEntry (0x16dfc0) - the +0x08 second base's virtual
// destructor: an EMPTY body, so all cl emits is the implicit re-stamp of the class's
// own vptr and a return (`mov [ecx],offset ??_7CButeNodeEntry@@6B@ (0x5f04d8); ret` -
// exactly the 7 retail bytes). Its scalar-deleting ??_G sits at 0x16dfa0.
//
// This is the REAL identity of the `CButeNodeSecondBase` phantom: the store/config-node
// destructors (0x174d70 butenode, 0x21310 / 0x21570 butemgr) all fold their +0x08 base
// through THIS dtor with the MI `this ? this+8 : 0` adjust. It had no definition
// anywhere in the tree, so every one of those calls dangled; defining it here - in the
// TU that owns the class's ctor and whose RVA band contains 0x16dfc0 (between
// ~zDArray @0x16df40 and the zPTree ctor @0x16dff0) - binds them all.
// ===========================================================================
RVA(0x0016dfc0, 7)
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
    : CContainerErr(&g_buteNodeErrMsg), CButeNodeEntry(n, teardown) {
    m_root = 0;
    m_lookupPending = 0;
}

// ===========================================================================
// CButeStore::ClearRecursive (0x16e070, ex ButeStoreClear.cpp) - the derived
// keyed-store's recursive node-free (C:\Proj\Bute). The store holds a binary
// tree at +0x18 keyed by each node's +0x08. The walk post-order frees: it
// recurses into a child only when that child's key is GREATER than the current
// node's (the heap-ordered owned-subtree invariant), frees the node's name
// string (+0x0c), then - when the store's +0x10 flag has bit 2 - runs the
// store's per-value callback (+0x0c fn-ptr) on the node's value (+0x10) and
// frees it, and finally frees the node itself. No destructible local, so no /GX
// frame even under this TU's eh flags.
// ===========================================================================
RVA(0x0016e070, 0x7b)
void CButeStore::ClearRecursive(CButeTreeNode* node) {
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

// GetCallerRetAddr (0x16e0f0): return the caller's return address, read from the
// caller's frame at [ebp+4]. Naked leaf - a 4-byte `mov eax,[ebp+4]; ret` (an
// intrinsic emits a different encoding, so inline asm is the only faithful form, like
// GetRetAddr @0x16d990). In this TU's own .text band; was a GAME-ASM carve-out.
RVA(0x0016e0f0, 4)
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
    if ((u32)nbits > 0x20) {
        i32 nwords = (nbits >> 5) + ((nbits & 0x1f) != 0 ? 1 : 0);
        m_capacity = nwords;
        u32* band = (u32*)malloc(nwords * 4);
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
CKeyFinder::CKeyFinder(void* owner) {
    m_0c = 2;
    m_10 = 2;
    // vptr install dropped -> compiler-emitted vtable (% ok per drive-to-0)
    m_08 = 0;
    m_owner = owner;
}

RVA(0x0016e1d0, 0x4b)
i32 CKeyFinder::Find(i32 key) {
    i32 hi = g_recCount23 - 1;
    i32 lo = 0;
    if (hi >= 0) {
        do {
            i32 mid = (lo + hi) / 2;
            m_index = mid;
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
    m_index = hi + 1;
    return -1;
}

// ===========================================================================
// TmErrorHandler (0x16e220) - the "C++ Tools error handler": the default callback
// seeded into the CKeyFinder/CVariantSlot +0x00 slot (the g_keyFinderVtbl fn-ptr;
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
            *--np = (char)(errNum % 10) + '0';
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

    u32 v = 0xffff & (u32)g_retAddrBreadcrumb;
    char* hp = &tmp[15];
    *hp = 0;
    i32 i;
    i = 7;
    do {
        i32 d = v & 0xf;
        *--hp = (char)(d > 9 ? d + 0x37 : d + 0x30);
        v >>= 4;
        if (4 == i) {
            break;
        }
    } while (i-- != 0);
    g_retAddrBreadcrumb = (void*)v;
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
// CKeyFinder::Add (0x16e360, the ex-Reg23 facet) - fixed-capacity (32) keyed
// record insert/update/remove over the global table at the cursor's m_index.
// ===========================================================================
// @early-stop
// Control flow + 12-byte-stride index arithmetic + memmove shifts + global stores are
// all faithful, but MSVC pins val->ebx / key->edi where retail uses key->ebx / val->edi
// (callee-saved register-preference wall, docs/patterns/pin-local-for-callee-saved-reg.md);
// the ebx<->edi swap is not source-steerable and cascades through every store. Deferred.
RVA(0x0016e360, 0x11a)
void* CKeyFinder::Add(void* key, void* val) {
    int count = g_recCount23;
    if (val != 0 && count >= 0x20) {
        return 0;
    }
    int idx;
    if (count != 0) {
        idx = Find((i32)key);
    } else {
        idx = -1;
    }
    if (idx != -1) {
        void* old = g_recs23[idx].m_4;
        if (val != 0) {
            g_recs23[idx].m_4 = val;
            return old;
        }
        memmove(
            &g_recs23[m_index],
            &g_recs23[m_index + 1],
            (g_recCount23 - m_index - 1) * sizeof(Rec23)
        );
        g_recCount23 = g_recCount23 - 1;
        return old;
    }
    if (val == 0) {
        return 0;
    }
    if (g_recCount23 != 0) {
        memmove(
            &g_recs23[m_index + 1],
            &g_recs23[m_index],
            (g_recCount23 - m_index) * sizeof(Rec23)
        );
    }
    g_recs23[m_index].m_4 = val;
    g_recs23[m_index].m_key = (i32)key;
    g_recCount23 = g_recCount23 + 1;
    g_recs23[m_index].m_8 = 0;
    return 0;
}

// ===========================================================================
// FirstDiffBit (0x16e480) - the crit-bit index (bit-level common-prefix length)
// of two byte keys: 8 per matching leading byte, plus the trailing-zero-bit count
// of the first differing pair's xor. __cdecl free helper; Insert (above) and
// CProjActMap::Insert (projactcache) both call it. Folded from Stub/DiscoveredSmall.
// ===========================================================================
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

// ===========================================================================
// ProjTypeXfer (0x16e4f0) - serialize the type-name table entry resolved from an
// archive record through the archive's slot dispatches. Resolves the entry id
// (ar->m_14->m_1c) to its CTypeKeyColl entry (the inlined fast-range / Find /
// grow lookup), frees the stale node array, then xfers the entry name (slot
// +0x0c) and id (slot +0x10); a second identical resolve xfers the name through
// slot +0x14. Returns 1.
// ===========================================================================
// The archive record (`ar`) the serializer drives is the canonical CXferArchive
// (<Gruntz/XferArchive.h>, included above).
DATA(0x002bf664)
extern CTypeNameEntry* g_typeCur;

// The inlined type-id -> entry resolution (== KitchenSlime/Projectile TypeLookup).
static inline char* TypeResolve(i32 key) {
    g_typeCount = 0;
    if (key >= g_typeLo && key <= g_typeHi) {
        return g_typeBase + (key - g_typeLo) * g_typeStride;
    }
    if ((i32)((_zvec*)&g_typeColl)->GrowTo(key, 0)) {
        return g_typeBase + (key - g_typeLo) * g_typeStride;
    }
    void* item = g_projActCache;
    g_retAddrBreadcrumb = GetRetAddr();
    g_typeColl2->Set(&g_typeColl, (i32)item, 0xc);
    return (char*)g_typeCur;
}

// Free the stale node array (g_typeCount slots, walking g_typeNodes).
static inline void FreeNodes() {
    CStringNode* nodes = (CStringNode*)g_typeNodes;
    i32 cnt = g_typeCount;
    if (cnt != 0) {
        do {
            if (nodes != 0) {
                ((CString*)nodes)->~CString();
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
i32 ProjTypeXfer(CXferArchive* ar) {
    CTypeNameEntry* entry = (CTypeNameEntry*)TypeResolve(ar->m_14->m_1c);
    FreeNodes();
    ar->Xfer0c(entry->m_name.GetBuffer(0)); // 0x1ba11c ?GetBuffer@CString@@QAEPADH@Z
    ar->Xfer10(ar->m_14->m_1c);

    entry = (CTypeNameEntry*)TypeResolve(ar->m_14->m_1c);
    FreeNodes();
    ar->Xfer14(entry->m_name.GetBuffer(0));
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
DATA(0x002bf620)
extern CButeTree g_buteTree;

RVA(0x0016e6a0, 0x26)
void DynInitButeTree() {
    g_buteTree.Construct(&g_buteTreeArg, 0);
}

// @early-stop
// `atexit destructor for g_buteTree' (??__F, 0x16e6e0) - homed from GapFunctions.cpp
// (matcher-5); the teardown thunk MSVC registers via atexit for the DynInit above. It
// restamps g_buteTree's dtor vptrs (0x5e94ac / 0x5e949c), runs the member-dtor chain
// (0x16e070 on &g_buteTree, 0x16cfc0 on the +8 subobject via the `p?&p->m8:0` neg/sbb/and
// idiom) and tail-jmps the base dtor 0x16ca60. A COMPILER-GENERATED atexit thunk: emitting
// it byte-exact needs g_buteTree defined (not extern) with the real ~CButeTree chain, which
// conflicts with the hand-written DynInit; homed pending that restructure.
RVA(0x0016e6e0, 0x3e)
i32 Gap_16e6e0(void) {
    return 0;
}

// Placement new (construct g_typeColl in place; no allocation, so it just runs the
// CTypeKeyColl ctor on the existing global, exactly as the retail in-place build).
inline void* operator new(u32, void* p) {
    return p;
}

// g_typeColl's RUNTIME-PHASE type. After DynInitTypeColl builds g_typeColl through
// the CTypeKeyColl ctor (construction vtable ??_7CTypeKeyColl @0x5f04d0), retail
// re-stamps this live 1-slot runtime vtable @0x5f04e4 over it - slot 0 is 0x16ea20,
// the CContainerErr error-drain tail the collection shares by COMDAT fold. Because
// CTypeKeyColl is single-inheritance, cl emits NO ??_7 for this phase (no secondary),
// so the runtime type is modeled as its own minimal real class and its vtable named
// directly with VTBL - the only binding mechanism available for a symbol-less datum
// (chosen over a DATA() manual-vtbl or leaving it in AnalysisVtables).
struct CTypeCollRuntime {
    virtual void Slot00_16ea20(); // [0] 0x16ea20 (shared CContainerErr drain tail)
};
SIZE_UNKNOWN(CTypeCollRuntime);
VTBL(CTypeCollRuntime, 0x001f04e4); // g_typeColl runtime-phase vtable @0x5f04e4

// ===========================================================================
// `dynamic initializer for g_typeColl' (0x16e730) - construct the shared key
// collection with the [0x7d0, 0x7da] id range, stamp its runtime vtable, then
// free the (initially stale) node array. The construct is a placement-new of the
// real CTypeKeyColl ctor (0x16dda0) over the global; the runtime re-stamp swaps in
// the live g_typeCollRunVtbl over the just-built construction vtable.
// ===========================================================================
// @early-stop
// placement-new null-guard + count-down-induction wall (~70%). Two residues:
// (1) constructing g_typeColl now goes through the REAL CTypeKeyColl ctor via a
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
    new (&g_typeColl) CTypeKeyColl(4, 0x7d0, 0x7da, (void*)1);
    CStringNode* nodes = (CStringNode*)g_typeNodes;
    // vptr install dropped -> compiler-emitted vtable (% ok per drive-to-0)
    if (nodes != 0) {
        i32 cnt = g_typeCount;
        if (cnt != 0) {
            do {
                if (nodes != 0) {
                    ((CString*)nodes)->~CString();
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
// homed pending g_typeColl defined with its real ~CTypeKeyColl chain.
RVA(0x0016e7a0, 0x48)
i32 Gap_16e7a0(void) {
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
// written as a method. (CButeStore==CButeTree, the same 0x2c-byte class.)
//
// @reloc-TODO / emission: this @rva-symbol binds only once cl EMITS ??_GCButeTree in
// THIS obj, which needs g_buteTree defined (not extern) as a real global CButeTree so
// its compiler-generated ctor/dtor/vtable emit here. That is BLOCKED by a dual-vtable
// identity (the butenode-dual-model, OUT OF SCOPE for this pass), PROVEN by disasm:
//   * construction (DynInitButeTree 0x16e6a0) stamps ??_7CButeTree     @0x5f04e0 /
//     ??_7CButeStore@@6BCButeStoreSecond @0x5f04dc   (the CButeTree identity);
//   * destruction (0x16e9c0 ??_G AND 0x16e6e0 atexit) stamps ??_7zPTree @0x5e94ac /
//     ??_7CButeStore@@6BCButeNodeEntry  @0x5e949c    (the zPTree/CButeStore identity).
// CORRECTION (RTTI-proven): the split is NOT irreducible. It is exactly what
// `class CButeTree : public zPTree` (now modeled, <Bute/ButeTree.h>) with an EMPTY
// ~CButeTree produces - the ctor re-stamps CButeTree's most-derived 0x5f04e0/0x5f04dc
// after the zPTree base ctor; the empty derived dtor elides its protective re-stamp so
// ??_G goes straight to ~zPTree (which stamps zPTree's 0x5e94ac/0x5e949c). Real classes
// are zErrHandling(@0), zPtrColl(fabricated "CButeNodeEntry", @8), zPTree : those.
// @emission-TODO (COUPLED, not this pass): binding 0x16e9c0 needs g_buteTree DEFINED so
// cl emits ??_G/??__E/??__F/the CButeTree vtables. Requires as one atomic unit: (1) an
// INLINE ~zPTree { ClearRecursive(0); } + ClearRecursive as a zPTree method (0x16e070),
// which forces CButeStore : zPTree so its Reset/leaf-dtor callers still bind faithfully;
// (2) @data-symbol on the emitted ??_7CButeTree@@6BCButeNodeEntry@@ @0x5f04dc (the
// fabricated ??_7CButeStore@@6BCButeStoreSecond@@ name won't auto-match) + butemgr
// dropping that binding; (3) @rva-symbols for the cl-mangled init/atexit thunks. Gate on
// ClearRecursive/~CButeMgr staying byte-exact (the +0x10 store-flag is a BYTE read;
// zPTree's m_kind is i16 - a real byte-risk to the 100% ClearRecursive).
// @rva-symbol: ??_GCButeTree@@UAEPAXI@Z 0x0016e9c0 0x45

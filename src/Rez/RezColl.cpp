// RezColl.cpp - the rez/sym/hash pocket utility TU (C:\Proj\...): ONE original obj
// [0x1848b0..0x184b5d] (wave1-E; interval dossier 0x1832d0 engine-util pocket). The
// RezNode/RezColl iterators, the CSymList array-list ctor and the CHashBase/CHashSlot
// slot machinery interleave fn-by-fn (rezcoll @0x1848b0/0x184ae0 INSIDE the sym/hash
// runs) - impossible across objs at first link, so the three families are one file.
// @identity-TODO: the original file name is unrecovered (the pocket block carries no
// __FILE__ anchor, no init fragment and - per the private-globals oracle - no data
// references at all); some rez/sym/hash utility file. Strict retail-RVA order.
//
// RezColl::First / RezNode::Next walk a hash-bucket array (each bucket is 16 B with
// a chain-head link at +8). A stored link points at the successor node's +4 link
// field, so the node is recovered as (link - 4); a null link stays null. The bucket
// array + count live at the collection's +4 / +0; a node caches its owning
// collection (+0xc) and its bucket index (+0x10) so Next can advance to the following
// occupied bucket. Leaf pointer-walks, no callees. Only the offsets + the (link - 4)
// recovery are load-bearing; names are placeholders.
#include <rva.h>

#include <Ints.h>
#include <Rez/RezColl.h>
#include <Bute/Hash.h>              // CHashBase/CHashSlot/CHashElement + Tm_DestroyArray/RezFree
#include <Dsndmgr/SoundVoiceList.h> // DSoundList::InsertHead/Unlink (the intrusive chain ops)

// The bucket stride is proven 0x10 from the iterators' `shl eax,0x4` (index * 16)
// / `add ecx,0x10` bucket-walk in Next (0x1848d5 / 0x1848ee) and First's
// `add edx,0x10` (0x184af7).
SIZE(RezBucket, 0x10);

// A chain link stores the address of the *successor's* +4 field (an interior
// pointer), so recovering the owning node is container_of(link, RezNode, +4) =
// (char*)link - 4. The char*/RezNode* cast pair is the language-forced
// container_of reinterpret (`add eax,0xfffffffc` in the disasm); it cannot be
// expressed without a cast.

// The next node: follow this node's chain link, else scan the collection's later
// buckets for the next occupied chain head.
RVA(0x001848b0, 0x47)
RezNode* RezNode::Next() {
    RezNode* n = m_nextLink ? (RezNode*)((char*)m_nextLink - 4) : 0;
    if (n == 0) {
        u32 i = m_bucketIdx + 1;
        RezColl* coll = m_coll;
        u32 count = coll->m_count;
        if (i < count) {
            RezBucket* b = coll->m_buckets;
            do {
                void* link = b[i].m_chainHead;
                n = link ? (RezNode*)((char*)link - 4) : 0;
                if (n) {
                    break;
                }
                i++;
            } while (i < count);
        }
    }
    return n;
}

// @identity-TODO (matcher-5): 0x184900 is a hash reverse-iterator "current/last" method:
// if the cached chain link @this+0x08 resolves (CHashBase::FromLink, container_of -4) return
// its element; else scan the table (CHashBase* @this+0x0c, its m_buckets @+0x04) from the
// highest bucket index (count @this+0x10) down, returning the tail element
// (FromLink(slot.m_chain.m_tail)) of the first non-empty bucket, or 0. The receiver is a
// 2-level iterator (NOT a bare 8-byte CHashBase) and is a ZERO-REF ORPHAN - no rel32/vtable/
// data-ref caller anywhere in the image (full-binary VA byte-scan), so its class is
// unrecoverable. Homed as a stub rather than fabricate a per-TU view of an un-xref-able
// receiver (no-fake-view rule); the CHashBase::Insert/Last SIB coin-flip wall applies once
// the real iterator class surfaces.
RVA(0x00184900, 0x43)
i32 Gap_184900(void) {
    return 0;
}

// ---------------------------------------------------------------------------
// CSymList::Construct (0x184960) - the sym-subsystem's array-backed list container
// ctor, re-homed from src/Stub/MallocConstructors. op-news (count<<4)+4 bytes (an
// int header + count 16-byte elements), stores count at +0x00 and the data ptr at
// +0x04, element-constructs via 0x11f5a0. xref (gruntz.analysis.xref): built by the
// CSymTab entry ctors (0x139bf0/0x139c80), CSymTab::CSymTab (0x139de0),
// CSymParser::CSymParser (0x13ab00) and CSymParseConfig (0x13aa10) - i.e. the
// list members throughout the symbol table/parser. Modeled as a plain shell;
// reconstruction deferred.
struct CSymList {
    CSymList* Construct(int count); // 0x184960
};
SIZE(CSymList, 0x8); // array-backed list container { count, data }
// @confidence: high
// @source: xref
// @stub
RVA(0x00184960, 0x70)
CSymList* CSymList::Construct(int count) {
    return this;
}

// ---------------------------------------------------------------------------
// CHashSlot - the 16-byte bucket slot (ctor + its vector deleting destructor).
// Homed from src/Stub/GapFunctions.cpp (matcher-5); the gap tool had merged the
// two into one 0x5b-byte span (0x1849d0 dtor 0x50 + 0x184a20 ctor 0xb).
// ---------------------------------------------------------------------------

// @early-stop
// 0x1849d0 = CHashSlot's `vector deleting destructor' (??_ECHashSlot@@QAEPAXI@Z): the
// COMPILER-GENERATED array-delete helper (flags&2 -> ehvec over the array with the no-op
// element dtor 0x584a30 + RezFree the cookie; else run the element dtor + flags&1 free).
// It is a ZERO-REF orphan COMDAT (full-binary VA byte-scan: no caller anywhere; CHashBase::
// RemoveAll @0x184a40 inlines its OWN ehvec rather than call this). MSVC only emits ??_E
// from a live `delete[] CHashSlot` site, which this TU does not have (RemoveAll uses a
// direct Tm_DestroyArray, matching retail), so there is no source construct to regenerate
// it at this RVA without spuriously reshaping RemoveAll. Homed as a stub (not a hand-written
// method masquerading as the compiler thunk).
RVA(0x001849d0, 0x50)
i32 Gap_1849d0(void) {
    return 0;
}

// CHashSlot ctor (0x184a20): zero the {head,tail} chain head, return this.
RVA(0x00184a20, 0xb)
CHashSlot::CHashSlot() {
    m_chain.m_head = 0;
    m_chain.m_tail = 0;
}

// RemoveAll (0x184a40): array-delete the bucket array. The count cookie sits one
// word before m_buckets (the RezAlloc header); each 16-byte slot's dtor is a
// no-op; RezFree the cookie. The cookie read is a deliberate alloc-header offset.
RVA(0x00184a40, 0x27)
void CHashBase::RemoveAll() {
    if (m_buckets) {
        u32* cookie = (u32*)((char*)m_buckets - 4);
        Tm_DestroyArray(m_buckets, 0x10, *cookie, &CHashSlot_Dtor);
        RezFree(cookie);
    }
}

// Insert (0x184a70): ask the element for its bucket index (the slot-0 virtual
// hash), stamp the owning table (+0xc) and the bucket (+0x10), then splice the
// chain node (element+4) into the bucket's chain. The `?:` keeps the null-check
// `lea ecx,[esi+4]/xor ecx,ecx` even though the engine never feeds a null node.
// @early-stop
// SIB base/index coin-flip (99.55%): retail `lea [eax+ecx+8]` (idx<<4 as base) vs
// cl `lea [ecx+eax+8]` (m_buckets as base); operand-typing/reorder do not flip it.
RVA(0x00184a70, 0x34)
void CHashBase::Insert(CHashElement* node) {
    node->m_owner = this;
    u32 idx = node->Hash();
    node->m_bucket = idx;
    CHashLink* biased = node ? &node->m_link : 0;
    ((DSoundList*)&m_buckets[idx].m_chain)->InsertHead((DSoundLink*)biased);
}

// Remove (0x184ab0): unlink `entry` (its chain node = &entry->m_link) from the
// owning bucket's intrusive {head,tail} chain.
RVA(0x00184ab0, 0x25)
void CHashBase::Remove(CHashElement* entry) {
    CHashLink* node = entry ? &entry->m_link : 0;
    ((DSoundList*)&m_buckets[entry->m_bucket].m_chain)->Unlink((DSoundLink*)node);
}

// The first node: the chain head of the first occupied bucket.
RVA(0x00184ae0, 0x24)
RezNode* RezColl::First() {
    u32 i = 0;
    RezNode* n;
    do {
        void* link = m_buckets[i].m_chainHead;
        n = link ? (RezNode*)((char*)link - 4) : 0;
        i++;
    } while (n == 0 && i < m_count);
    return n;
}

// Last (0x184b10): reverse iteration - scan the bucket array from the highest index
// down, return the tail element of the first non-empty bucket (or 0 when empty).
// Re-homed from src/Stub/GapFunctions.cpp (matcher-2; CHashBase-owned - bracketed by
// Remove/Lookup, scans the 16-byte CHashSlot[] reading m_chain.m_tail @+0x0c).
// @early-stop
// SAME SIB base/index coin-flip wall as CHashBase::Insert (above): retail folds the
// bucket address into `mov ecx,[ecx+4]; lea ecx,[ecx+eax+0xc]` (m_buckets reuses the
// `this` register), while cl loads m_buckets into eax -> `push esi; add eax,esi; lea
// ecx,[eax+0xc]`. The loop body is byte-identical; the base-register pick is not
// source-steerable (operand-typing/reorder do not flip it, per the Insert note).
RVA(0x00184b10, 0x29)
CHashElement* CHashBase::Last() {
    u32 i = m_count - 1;
    CHashSlot* p = &m_buckets[i];
    CHashElement* e;
    for (;;) {
        e = FromLink(p->m_chain.m_tail);
        if (i == 0) {
            break;
        }
        --i;
        --p;
        if (e != 0) {
            break;
        }
    }
    return e;
}

// Lookup (0x184b40): chain head for bucket `idx`, biased back to the element, or 0.
// @early-stop
// SIB base/index coin-flip (99%; was 100% in the pre-merge Hash.cpp TU with identical
// source): the pocket-TU merge flipped `mov eax,[eax+ecx+0x8]` to the ecx-base form +
// the FromLink -4 add to the imm8 encoding. Same non-steerable family as Insert/Last
// (local-slot spelling doesn't flip it back).
RVA(0x00184b40, 0x1d)
CHashElement* CHashBase::Lookup(u32 idx) {
    return FromLink(m_buckets[idx].m_chain.m_head);
}

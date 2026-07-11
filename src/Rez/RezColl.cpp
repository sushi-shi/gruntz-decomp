// RezColl.cpp - the rez/sym/hash pocket utility TU (C:\Proj\...): ONE original obj
// [0x1848b0..0x184b5d] (wave1-E; interval dossier 0x1832d0 engine-util pocket). The
// hash-table iterators, the sized-table ctor and the CHashBase/CHashSlot slot
// machinery interleave fn-by-fn - impossible across objs at first link, so the
// families are one file.
//
// wave5-F1: the five-way hash-table class conflation is folded onto the canonical
// CHashBase (base) / CHashElement (24-byte intrusive node) / CHash+CHashB (the two
// key-typed template instantiations) from <Bute/Hash.h>. The former per-TU views
// (RezColl/RezNode/RezBucket here, CHashTable/CHashTableEntry in symtab, CSymList
// everywhere) were divergent names for the SAME physical class - now ONE model:
//   0x1848b0  RezNode::Next       -> CHashElement::Next
//   0x184950  (unhomed)           -> CHash::CHash()        (default/empty-table ctor)
//   0x184960  CSymList::Construct -> CHashBase::Construct  (the sized "constructor")
//   0x184ae0  RezColl::First      -> CHashBase::First
// @identity-TODO: the original file name is unrecovered (the pocket block carries no
// __FILE__ anchor / init fragment / data references at all); some rez/sym/hash TU.
// Strict retail-RVA order.
//
// CHashBase::First / CHashElement::Next walk a hash-bucket array (each bucket is a
// 16-byte CHashSlot with the {head,tail} chain at +8). A stored chain link points at
// the successor element's +4 link field, so the element is recovered as (link - 4);
// a null link stays null. The element caches its owning table (+0xc) and bucket index
// (+0x10) so Next can advance to the following occupied bucket. Leaf pointer-walks.
#include <rva.h>

#include <Ints.h>
#include <Bute/Hash.h>              // CHashBase/CHashSlot/CHashElement + Tm_*Array/RezFree
#include <Dsndmgr/SoundVoiceList.h> // DSoundList::InsertHead/Unlink (the intrusive chain ops)

// The bucket stride is proven 0x10 from the iterators' `shl eax,0x4` (index * 16)
// / `add ecx,0x10` bucket-walk in Next (0x1848d5 / 0x1848ee) and First's
// `add edx,0x10` (0x184af7). CHashSlot (Hash.h) carries the 0x10-byte layout.

// A chain link stores the address of the *successor's* +4 field (an interior
// pointer), so recovering the owning element is container_of(link, CHashElement,
// m_link) = (char*)link - 4. The char*/CHashElement* cast pair is the language-forced
// container_of reinterpret (`add eax,0xfffffffc` in the disasm); it cannot be
// expressed without a cast.

// The next element: follow this element's chain link, else scan the owning table's
// later buckets for the next occupied chain head.
RVA(0x001848b0, 0x47)
CHashElement* CHashElement::Next() {
    CHashElement* n = m_link.m_next ? (CHashElement*)((char*)m_link.m_next - 4) : 0;
    if (n == 0) {
        u32 i = m_bucket + 1;
        CHashBase* coll = m_owner;
        u32 count = coll->m_count;
        if (i < count) {
            CHashSlot* b = coll->m_buckets;
            do {
                void* link = b[i].m_chain.m_head;
                n = link ? (CHashElement*)((char*)link - 4) : 0;
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
// CHash::CHash() (0x184950) - the default (empty-table) constructor: zero the count
// and bucket-array pointer. OUT-OF-LINE (CSymRec's 3-arg ctor `call`s it for
// m_keyTable). Re-homed here (was an unhomed body; wave5-F1). It lives on the
// leaf/value instantiation CHash (the only default-constructed table); the base
// CHashBase stays trivially-constructible so the sized tables' Construct-only
// member-init is not preceded by a base-ctor zeroing pass.
// ---------------------------------------------------------------------------
RVA(0x00184950, 0x10)
CHash::CHash() {
    m_count = 0;
    m_buckets = 0;
}

// ---------------------------------------------------------------------------
// CHashBase::Construct (0x184960) - the sized "constructor": store `count`, allocate
// a (count<<4)+4-byte block (an int cookie one word before the array, then `count`
// 16-byte CHashSlot buckets), ehvec-construct the slots, and store the array pointer.
// Returns this. A plain method (retail mangles ?Construct@... returning `this`), not a
// C++ ctor; the derived CHash/CHashB sized ctors delegate to it. Was the CSymList::
// Construct stub (wave5-F1). xref: built by every symbol-table/parser hash member
// (CSymRec 0x139bf0/0x139c80, CSymTab 0x139de0, CSymParser 0x13ab00/0x13aa10).
// @early-stop
// /GX-frame wall (61.5%, was a `return this;` stub): the alloc + cookie store + the
// ehvec constructor-iterator call + the m_buckets store are all byte-faithful. The
// residual is the implicit /GX EH try-frame retail wraps the throwing ehvec in
// (push -1/handler/mov fs:0 at entry + fs:0 restore/add esp,0xc at exit) plus the
// frame-relative [esp+N] shift it induces - cl emits that frame ONLY for a real
// `new CHashSlot[count]` array-new construct, not for this hand-written ehvec extern
// call. Reproducing the array-new would require homing CHashSlot's rollback dtor at
// the cross-unit RVA 0x584a30 (currently the free CHashSlot_Dtor RemoveAll shares) -
// out of this obj's band. Logic complete; parked for the final sweep.
RVA(0x00184960, 0x70)
CHashBase* CHashBase::Construct(i32 count) {
    m_count = count;
    void* buf = RezAlloc((count << 4) + 4);
    CHashSlot* buckets = 0;
    if (buf) {
        *(i32*)buf = count;
        buckets = (CHashSlot*)((char*)buf + 4);
        Tm_ConstructArray(buckets, 0x10, count, &CHashSlot_Ctor, &CHashSlot_Dtor);
    }
    m_buckets = buckets;
    return this;
}

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

// The first element: the chain head of the first occupied bucket.
RVA(0x00184ae0, 0x24)
CHashElement* CHashBase::First() {
    u32 i = 0;
    CHashElement* n;
    do {
        void* link = m_buckets[i].m_chain.m_head;
        n = link ? (CHashElement*)((char*)link - 4) : 0;
        i++;
    } while (n == 0 && i < m_count);
    return n;
}

// Last (0x184b10): reverse iteration - scan the bucket array from the highest index
// down, return the tail element of the first non-empty bucket (or 0 when empty).
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

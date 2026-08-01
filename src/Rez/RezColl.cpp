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

// byte-forced: the chain threads the element's +4 link field, so the element is
// recovered as (link - 4). One seam per walk site; there is no member to name.
RVA(0x001848b0, 0x47)
CHashElement* CHashElement::Next() {
    // retail 0x1848b0: `mov eax,[ecx+4]; test eax,eax; je +; add eax,-4` - the chain
    // stores the address of the SUCCESSOR's +4 link field, so the element is recovered
    // through the table's one container-of seam.
    CHashElement* n = CHashBase::FromLink(m_link.m_next);
    if (n == 0) {
        u32 i = m_bucket + 1;
        CHashBase* coll = m_owner;
        u32 count = coll->m_count;
        if (i < count) {
            CHashSlot* b = coll->m_buckets;
            do {
                n = CHashBase::FromLink(b[i].m_chain.m_head);
                if (n) {
                    break;
                }
                i++;
            } while (i < count);
        }
    }
    return n;
}

// CHashElement::Prev (0x184900) - the exact reverse mirror of Next above. IDENTITY
// RESOLVED (the old @identity-TODO called the receiver an unrecoverable "2-level
// iterator"): the three offsets it reads are this class's own members. Retail reads
// [ecx+0x08] = m_link.m_prev (DSoundLink is {m_next+0x00, m_prev+0x04} and m_link sits
// at CHashElement+0x04), [ecx+0x0c] = m_owner, [ecx+0x10] = m_bucket - the same three
// fields Next uses, only the backward halves. It then walks buckets m_bucket-1 DOWN to
// 0 reading [slot+0x0c] = m_chain.m_tail (Next walks UP reading +0x08 = m_chain.m_head).
// So it is CHashElement::Prev, and it pairs with CHashBase::Last exactly as Next pairs
// with First. Zero-ref is expected: Last (0x184b10) is equally uncalled - retail emitted
// the whole reverse-iteration family and the game only ever iterates forward.
// @early-stop
// ONE byte left (99.66%): the SIB base/index coin-flip this whole CHashBase family sits
// on (Insert 0x184a70 99.55 / Remove 0x184ab0 99.23 / Lookup 0x184b40 99.00). Every
// register is coloured exactly as retail; only the roles inside the preheader lea are
// swapped - retail `lea ecx,[eax+ecx+0xc]` (SIB base = m_buckets), cl `[ecx+eax+0xc]`
// (SIB base = i<<4). docs/patterns/sib-base-index-follows-local-decl-order.md says local
// declaration order is the lever; it is NOT one here (measured: counter-first, pointer-
// first, and routing the subscript through `coll->m_buckets[i]` instead of a `b` local
// all produce the same byte - the last of those additionally recolours m_buckets into
// ecx, so it is strictly worse). The pointer comes from a MEMBER, which is exactly the
// sub-family that doc records as still open.
// 2026-07-29, the real mechanism: this byte is decided by TU-CUMULATIVE COMPILER STATE,
// not by the victim's own source. Two controlled A/B compiles prove it - dropping /GX
// flips Insert's SIB (retail's form) with a byte-identical instruction stream otherwise,
// and MOVING Insert to the top of the file flips it too. Deleting the fabricated
// Gap_1849d0 stub (one preceding COMDAT) flipped Lookup and nothing else. /GX is not
// available as a lever (Construct's EH frame needs it) and the RVA order is fixed, so
// there is no legal knob left for these four - but stop looking for a local spelling.
RVA(0x00184900, 0x43)
CHashElement* CHashElement::Prev() {
    CHashElement* e = CHashBase::FromLink(m_link.m_prev);
    if (e == 0) {
        // `> 0` on a u32 is the load-bearing spelling: retail gates with `jbe` and
        // closes the loop with `ja`, both unsigned.
        if (m_bucket > 0) {
            // Plain index walk, NOT a hand-rolled cursor: /O2 strength-reduces
            // &b[i].m_chain.m_tail into retail's `lea ecx,[buckets+i*16+0xc]` seeded at
            // the PRE-decrement i, then steps it with `sub ecx,0x10`. Writing the cursor
            // by hand (`t -= 4`) instead emits `add ecx,0xfffffff0` - the induction
            // variable has to be the compiler's for the `sub` encoding to appear.
            CHashSlot* b = m_owner->m_buckets;
            u32 i = m_bucket;
            do {
                --i;
                e = CHashBase::FromLink(b[i].m_chain.m_tail);
            } while (e == 0 && i > 0);
        }
    }
    return e;
}

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
// (The former "/GX-frame wall" @early-stop here is RETIRED: it said the implicit EH frame
// "cl emits ONLY for a real `new CHashSlot[count]` array-new construct" could not be had
// because CHashSlot's rollback dtor was not homed. It is now - see ~CHashSlot in Hash.h -
// so the array-new is written directly below and cl emits the frame itself.)
RVA(0x00184960, 0x70)
CHashBase* CHashBase::Construct(i32 count) {
    m_count = count;
    // The real array-new. cl emits exactly retail's sequence: operator new(count*0x10 + 4),
    // store the count cookie at [0], then the CRT `'eh vector constructor iterator'` ??_P
    // (0x11f5a0) over the elements with &CHashSlot::CHashSlot (0x184a20) and the rollback
    // &CHashSlot::~CHashSlot (0x184a30) - and, because the iterator can throw, the /GX EH
    // try-frame retail wraps it in. It used to be hand-written as a call to three fabricated
    // symbols (Tm_ConstructArray / CHashSlot_Ctor / CHashSlot_Dtor), none of which anything
    // defines. The old @early-stop said this could not be regenerated without "homing
    // CHashSlot's rollback dtor at 0x584a30" - that is now done (CHashSlot has its real
    // ~CHashSlot, RVA_COMPGEN-pinned below), so the construct IS expressible.
    m_buckets = new CHashSlot[count];
    return this;
}

// 0x1849d0 = CHashSlot's `vector deleting destructor' (??_ECHashSlot@@QAEPAXI@Z): the
// COMPILER-GENERATED array-delete helper (flags&2 -> ehvec over the array with the no-op
// element dtor 0x584a30 + RezFree the cookie; else run the element dtor + flags&1 free).
// A ZERO-REF orphan COMDAT (RemoveAll @0x184a40 inlines its OWN ehvec rather than call it)
// - but cl emits it ANYWAY from the `delete[] m_buckets` in RemoveAll, byte-for-byte, so
// there is nothing to hand-write. The old `Gap_1849d0` stub was a fabricated duplicate of
// a COMDAT the compiler already produces; RVA_COMPGEN names the real one at this RVA (same
// device as ??_EBucketHead in WwdGrid.cpp).
RVA_COMPGEN(0x001849d0, 0x50, ??_ECHashSlot@@QAEPAXI@Z)

RVA(0x00184a20, 0xb)
CHashSlot::CHashSlot() {} // m_chain empties itself (DSoundList's own default ctor)

RVA(0x00184a30, 0x1)
CHashSlot::~CHashSlot() {}

RVA(0x00184a40, 0x27)
void CHashBase::RemoveAll() {
    delete[] m_buckets;
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
    DSoundLink* biased = node ? &node->m_link : 0;
    m_buckets[idx].m_chain.InsertHead(biased);
}

RVA(0x00184ab0, 0x25)
void CHashBase::Remove(CHashElement* entry) {
    DSoundLink* node = entry ? &entry->m_link : 0;
    m_buckets[entry->m_bucket].m_chain.Unlink(node);
}

RVA(0x00184ae0, 0x24)
CHashElement* CHashBase::First() {
    u32 i = 0;
    CHashElement* n;
    do {
        n = FromLink(m_buckets[i].m_chain.m_head);
        i++;
    } while (n == 0 && i < m_count);
    return n;
}

// Last (0x184b10): reverse iteration - scan the bucket array from the highest index
// down, return the tail element of the first non-empty bucket (or 0 when empty).
// (ex-wall note, RETIRED 2026-07-29: this is now 100% EXACT. It used to carry the same
// SIB base/index @early-stop as Insert - `mov ecx,[ecx+4]; lea ecx,[ecx+eax+0xc]` vs a
// `push esi; add eax,esi; lea ecx,[eax+0xc]` recompile. The tail-anchored-cursor spelling
// below closed it; the text is history, not a current claim.)
RVA(0x00184b10, 0x29)
CHashElement* CHashBase::Last() {
    u32 i = m_count - 1;
    DSoundLink** t = &m_buckets[i].m_chain.m_tail; // tail-anchored cursor (one lea)
    CHashElement* e;
    for (;;) {
        e = FromLink(*t);
        if (i <= 0) {
            break;
        }
        --i;
        t -= 4; // one CHashSlot (0x10) back, link-typed stride
        if (e != 0) {
            break;
        }
    }
    return e;
}

// Lookup (0x184b40): chain head for bucket `idx`, biased back to the element, or 0.
// (ex-wall, RETIRED 2026-07-29 - EXACT. The old note called the SIB base/index byte a
// "non-steerable" coin-flip that "local-slot spelling doesn't flip back". It is not a
// coin-flip: it is TU-CUMULATIVE STATE. Deleting the fabricated `Gap_1849d0` stub above
// - which duplicated the ??_E COMDAT cl already emits - flipped this one function back
// to retail's `[eax+ecx+0x8]` with no change to its own source at all. The remaining
// four in the family (Prev/Insert/Remove/Last) did not move.)
RVA(0x00184b40, 0x1d)
CHashElement* CHashBase::Lookup(u32 idx) {
    return FromLink(m_buckets[idx].m_chain.m_head);
}

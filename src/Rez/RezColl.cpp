// @identity-TODO
// No file anchor, initializer, or data reference proves the original TU name.

#include <rva.h>

#include <Bute/Hash.h>
#include <Dsndmgr/IntrusiveList.h>
#include <Enums.h>
#include <Ints.h>

#include <stddef.h>

RVA(0x001848b0, 0x47)
CHashElement* CHashElement::Next() {

    CHashElement* n = CHashBase::FromLink(m_link.m_next);
    if (n == NULL) {
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

// @early-stop
// One SIB byte, pointer-from-a-member sub-family.
// docs/patterns/sib-base-index-follows-local-decl-order.md
// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00184900, 0x43)
CHashElement* CHashElement::Prev() {
    CHashElement* e = CHashBase::FromLink(m_link.m_prev);
    if (e == NULL) {

        if (m_bucket > 0) {

            CHashSlot* b = m_owner->m_buckets;
            u32 i = m_bucket;
            do {
                --i;
                e = CHashBase::FromLink(b[i].m_chain.m_tail);
            } while (e == NULL && i > 0);
        }
    }
    return e;
}

RVA(0x00184950, 0x10)
CHash::CHash() {
    m_count = 0;
    m_buckets = NULL;
}

RVA(0x00184960, 0x70)
CHashBase::CHashBase(i32 count) {
    m_count = count;

    m_buckets = new CHashSlot[count];
}

RVA_COMPGEN(0x001849d0, 0x50, ??_ECHashSlot@@QAEPAXI@Z)

RVA(0x00184a20, 0xb)
CHashSlot::CHashSlot() {}

RVA(0x00184a30, 0x1)
CHashSlot::~CHashSlot() {}

RVA(0x00184a40, 0x27)
void CHashBase::RemoveAll() {
    delete[] m_buckets;
}

// @early-stop
// One SIB byte, pointer-from-a-member sub-family.
// docs/patterns/sib-base-index-follows-local-decl-order.md
RVA(0x00184a70, 0x34)
void CHashBase::Insert(CHashElement* node) {
    node->m_owner = this;
    u32 idx = node->Hash();
    node->m_bucket = idx;
    IntrusiveLink* biased = node ? &node->m_link : NULL;
    m_buckets[idx].m_chain.InsertHead(biased);
}

RVA(0x00184ab0, 0x25)
void CHashBase::Remove(CHashElement* entry) {
    IntrusiveLink* node = entry ? &entry->m_link : NULL;
    m_buckets[entry->m_bucket].m_chain.Unlink(node);
}

RVA(0x00184ae0, 0x24)
CHashElement* CHashBase::First() {
    u32 i = 0;
    CHashElement* n;
    do {
        n = FromLink(m_buckets[i].m_chain.m_head);
        i++;
    } while (n == NULL && i < m_count);
    return n;
}

// @early-stop
// The only differing byte is the commuted scale-1 SIB encoding of the tail
// address: [eax+ecx+0xc] here and [ecx+eax+0xc] in retail.
// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00184b10, 0x29)
CHashElement* CHashBase::Last() {
    u32 i = m_count - 1;
    IntrusiveLink** t = &m_buckets[i].m_chain.m_tail;
    CHashElement* e;
    for (;;) {
        e = FromLink(*t);
        if (i <= 0) {
            break;
        }
        --i;
        t -= 4;
        if (e != NULL) {
            break;
        }
    }
    return e;
}

// @early-stop
// The only differing byte is the commuted scale-1 SIB encoding of the bucket
// address: [ecx+eax+8] here and [eax+ecx+8] in retail. A natural bucket-pointer
// local is byte-flat.
RVA(0x00184b40, 0x1d)
CHashElement* CHashBase::Lookup(u32 idx) {
    return FromLink(m_buckets[idx].m_chain.m_head);
}

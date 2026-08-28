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

    CHashElement* next = CHashBase::FromLink(m_next);
    if (next == NULL) {
        u32 bucketIndex = m_bucketIndex + 1;
        CHashBase* hash = m_hash;
        u32 bucketCount = hash->m_bucketCount;
        if (bucketIndex < bucketCount) {
            CHashSlot* buckets = hash->m_buckets;
            do {
                next = CHashBase::FromLink(buckets[bucketIndex].m_chain.m_head);
                if (next) {
                    break;
                }
                bucketIndex++;
            } while (bucketIndex < bucketCount);
        }
    }
    return next;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00184900, 0x43)
CHashElement* CHashElement::Prev() {
    CHashElement* previous = CHashBase::FromLink(m_prev);
    if (previous == NULL) {
        if (m_bucketIndex > 0) {
            CHashSlot* buckets = m_hash->m_buckets;
            u32 bucketIndex = m_bucketIndex;
            do {
                --bucketIndex;
                previous = CHashBase::FromLink(buckets[bucketIndex].m_chain.m_tail);
            } while (previous == NULL && bucketIndex > 0);
        }
    }
    return previous;
}

RVA(0x00184950, 0x10)
CRezEntryIdHash::CRezEntryIdHash() {
    m_bucketCount = 0;
    m_buckets = NULL;
}

RVA(0x00184960, 0x70)
CHashBase::CHashBase(i32 bucketCount) {
    m_bucketCount = bucketCount;

    m_buckets = new CHashSlot[bucketCount];
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

RVA(0x00184a70, 0x34)
void CHashBase::Insert(CHashElement* node) {
    node->m_hash = this;
    u32 bucketIndex = node->Hash();
    node->m_bucketIndex = bucketIndex;
    m_buckets[bucketIndex].m_chain.InsertHead(node);
}

RVA(0x00184ab0, 0x25)
void CHashBase::Remove(CHashElement* entry) {
    m_buckets[entry->m_bucketIndex].m_chain.Unlink(entry);
}

RVA(0x00184ae0, 0x24)
CHashElement* CHashBase::First() {
    u32 bucketIndex = 0;
    CHashElement* first;
    do {
        first = FromLink(m_buckets[bucketIndex].m_chain.m_head);
        bucketIndex++;
    } while (first == NULL && bucketIndex < m_bucketCount);
    return first;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00184b10, 0x29)
CHashElement* CHashBase::Last() {
    u32 bucketIndex = m_bucketCount - 1;
    CHashElement* last;
    do {
        last = FromLink(m_buckets[bucketIndex].m_chain.m_tail);
        if (bucketIndex > 0) {
            bucketIndex--;
        } else {
            break;
        }
    } while (last == NULL);
    return last;
}

RVA(0x00184b40, 0x1d)
CHashElement* CHashBase::Lookup(u32 bucketIndex) {
    return FromLink(m_buckets[bucketIndex].m_chain.m_head);
}

// RezColl.cpp - the Rez subsystem's hash-bucket collection iterators
// (C:\Proj\...\Rez).
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

struct RezNode;

// One hash bucket: 16 bytes, chain-head link at +8.
struct RezBucket {
    char m_pad0[8];
    void* m_chainHead; // +0x08  points at first node's +4 link, or 0
    char m_padc[4];
};
SIZE_UNKNOWN(RezBucket);

// The bucket collection: count at +0, bucket array at +4 (0x10 bytes total).
struct RezColl {
    u32 m_count;          // +0x00  bucket count
    RezBucket* m_buckets; // +0x04  bucket array
    char m_pad8[8];       // +0x08..+0x0f
    RezNode* First();
};

// A node: next link at +4, owning collection at +0xc, bucket index at +0x10.
struct RezNode {
    void* m_0;
    void* m_nextLink; // +0x04  points at the successor's +4, or 0
    void* m_8;
    RezColl* m_coll; // +0x0c  owning collection
    u32 m_bucketIdx; // +0x10  bucket index
    RezNode* Next();
};

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

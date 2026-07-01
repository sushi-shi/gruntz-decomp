// RezColl.cpp - the Rez subsystem's hash-bucket collection iterators
// (C:\Proj\...\Rez). Re-homed from src/Stub/RezColl.cpp + src/Stub/RezNode.cpp.
//
// RezColl::First / RezNode::Next walk a hash-bucket array (each bucket is 16 B
// with a chain-head link at +8). A stored link points at the successor node's +4
// link field, so the node is recovered as (link - 4); a null link stays null.
// The bucket array + count live at the collection's +4 / +0; a node caches its
// owning collection (+0xc) and its bucket index (+0x10) so Next can advance to the
// following occupied bucket. Leaf pointer-walks, no callees. Only the offsets +
// the (link - 4) recovery are load-bearing; names are placeholders.
#include <rva.h>

#include <Ints.h>

struct RezNode;

// One hash bucket: 16 bytes, chain-head link at +8.
struct RezBucket {
    char m_pad0[8];
    void* m_8; // +0x08  chain head (points at first node's +4 link, or 0)
    char m_padc[4];
};
SIZE_UNKNOWN(RezBucket);

// The bucket collection: count at +0, bucket array at +4 (0x10 bytes total).
struct RezColl {
    u32 m_0;        // +0x00  bucket count
    RezBucket* m_4; // +0x04  bucket array
    char m_pad8[8]; // +0x08..+0x0f
    RezNode* First();
};

// A node: next link at +4, owning collection at +0xc, bucket index at +0x10.
struct RezNode {
    void* m_0;
    void* m_4; // +0x04  next link (points at the successor's +4, or 0)
    void* m_8;
    RezColl* m_c; // +0x0c  owning collection
    u32 m_10;     // +0x10  bucket index
    RezNode* Next();
};

// 0x1848b0 - the next node: follow this node's chain link, else scan the
// collection's later buckets for the next occupied chain head.
RVA(0x001848b0, 0x47)
RezNode* RezNode::Next() {
    RezNode* n = m_4 ? (RezNode*)((char*)m_4 - 4) : 0;
    if (n == 0) {
        u32 i = m_10 + 1;
        RezColl* coll = m_c;
        u32 count = coll->m_0;
        if (i < count) {
            RezBucket* b = coll->m_4;
            do {
                void* link = b[i].m_8;
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

// 0x184ae0 - the first node: the chain head of the first occupied bucket.
RVA(0x00184ae0, 0x24)
RezNode* RezColl::First() {
    u32 i = 0;
    RezNode* n;
    do {
        void* link = m_4[i].m_8;
        n = link ? (RezNode*)((char*)link - 4) : 0;
        i++;
    } while (n == 0 && i < m_0);
    return n;
}

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
#include <Rez/RezColl.h>

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

// RezColl.h - the ONE shared definition of the engine's hash-bucket collection
// (RezColl) and its intrusive chain node (RezNode), plus the 16-byte RezBucket.
//
// These placeholder types name the collection/node iterated by the two physical
// engine functions RezColl::First (0x184ae0) and RezNode::Next (0x1848b0) - the
// canonical bodies live in src/Rez/RezColl.cpp. The SAME collection is embedded
// in CRezDirNode (RezMgr.h, the archive directory tree) and reinterpret-cast over
// the CSymTab hash tables (Bute/SymTab.h). Previously each of those three TUs
// carried its own divergent RezColl/RezNode; this header is the merged truth so
// there is exactly one layout.
//
// Layout recovered from the two functions' disassembly (llvm-objdump -dr):
//   Next reads node+0x04 (chain link), node+0x0c (owning collection), node+0x10
//   (bucket index); the collection at coll+0x00 (count) and coll+0x04 (bucket
//   array, 0x10-byte buckets, chain head at bucket+0x08). A stored link points at
//   the successor node's +4 field, so the node is recovered as (link - 4); a null
//   link stays null. The node's +0x14 slot holds the resolved payload (a CSymTab*
//   child scope / CSymRec* leaf record, or a CRezDirNode* sub-dir) that the
//   iterators return; the two physical functions never touch +0x14 themselves.
//
// The collection is 8 bytes {count, buckets}: CSymTab embeds two of these tables
// at +0x38 and +0x40 (8 bytes apart), which fixes the size. Only the OFFSETS +
// the (link - 4) recovery are load-bearing; names are placeholders.
#ifndef INCLUDE_REZ_REZCOLL_H
#define INCLUDE_REZ_REZCOLL_H

#include <Ints.h>
#include <rva.h> // SIZE()

struct RezNode;

// One hash bucket: 16 bytes, chain-head link at +8.
struct RezBucket {
    char m_pad0[8];
    void* m_chainHead; // +0x08  points at the first node's +4 link, or 0
    char m_padc[4];
};

// The hash-bucket collection (8 bytes): count at +0, bucket array at +4.
struct RezColl {
    u32 m_count;          // +0x00  bucket count
    RezBucket* m_buckets; // +0x04  bucket array

    // The first node: the chain head of the first occupied bucket (0x184ae0).
    RezNode* First();
};
SIZE(RezColl, 0x8); // { count, buckets }

// A node/entry in a bucket chain: chain link at +4, owning collection at +0xc,
// bucket index at +0x10, resolved payload at +0x14.
struct RezNode {
    void* m_0;        // +0x00
    void* m_nextLink; // +0x04  points at the successor's +4, or 0
    void* m_8;        // +0x08
    RezColl* m_coll;  // +0x0c  owning collection
    u32 m_bucketIdx;  // +0x10  bucket index
    void* m_14;       // +0x14  resolved payload (CSymTab*/CSymRec*/CRezDirNode*)

    // The next node: follow this node's chain link, else scan the collection's
    // later buckets for the next occupied chain head (0x1848b0).
    RezNode* Next();
};
SIZE(RezNode, 0x18); // { .., nextLink @0x04, coll @0x0c, bucketIdx @0x10, payload @0x14 }

#endif // INCLUDE_REZ_REZCOLL_H

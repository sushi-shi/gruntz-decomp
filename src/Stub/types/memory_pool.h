#ifndef UTILS_MEMORY_POOL_H
#define UTILS_MEMORY_POOL_H

/*
 * Utils::MemoryPool<T> — a fixed-block free-list allocator over a single
 * contiguous char[] block. Each node is { Node* pNext; T data; }; free nodes form
 * a singly-linked list; the pool always leaves the last node free (a sentinel).
 *
 * CGruntzMgr::memory_pool is the only instantiation we know: MemoryPool<Pair>
 * (see ../game/cgruntzmgr.h, where Pair {int a; int b;} is declared).
 *
 * LAYOUT PORTED FROM tomalla (refs/tomalla-gruntz/utils/memory_pool.h).
 * @approx tomalla 1.0.1.77 — field OFFSETS are version-independent. tomalla notes
 * it could not match any ATL/MFC allocator (closest was CFixedAllocNoSync, but
 * fields/signatures differ); this is a bespoke pool. Function addresses (mostly
 * inlined) are deferred to the re-anchor and NOT recorded here.
 *
 * Provenance: not in RTTI; "Utils::MemoryPool" is a tomalla-invented name for a
 * real binary class with matched fields.
 */

namespace Utils
{
    template <typename T>
    class MemoryPool
    {
    private:
        struct Node
        {
            Node* pNext;   // free-list link (also data-offset base; see m_dataOffset)
            T     data;
        };

    public:
        MemoryPool();
        ~MemoryPool();

        // Allocates the backing block of `nodesCount` nodes and threads the
        // free-list. @bug (faithfully reproduced from tomalla's reading): the
        // null-check after `new` can never fire, and double-alloc is unguarded.
        bool AllocateBlock(unsigned int nodesCount);
        // Pops the next free node's data (returns 0 when only the sentinel remains).
        T* Get();
        // Pushes a node back onto the free-list (uses m_dataOffset to recover Node*).
        void Free(T* pData);

    private:
        char* m_pBlock;             // +0x00  the single contiguous backing block
        Node* m_pNextFreeNode;      // +0x04  head of the free-list
        unsigned int m_nodesCount;  // +0x08
        unsigned int m_dataOffset;  // +0x0c  offset from Node* to its `data`
    };                              // 0x10 bytes
}

// MemoryPool_Pair — the concrete Utils::MemoryPool<Pair> instantiation that
// CGruntzMgr::memory_pool uses (see ../game/cgruntzmgr.h). The template above is
// never instantiated in these comprehension TUs, so clang emits no record layout
// for it; this is a plain struct mirror of its 0x10-byte layout (same field set,
// element-type independent) so `gruntz structs` emits a "MemoryPool_Pair" record.
// @approx tomalla 1.0.1.77 (offsets version-independent).
struct MemoryPool_Pair
{
    char*        m_pBlock;        // +0x00  the single contiguous backing block
    void*        m_pNextFreeNode; // +0x04  head of the free-list (a Node*)
    unsigned int m_nodesCount;    // +0x08
    unsigned int m_dataOffset;    // +0x0c  offset from Node* to its `data`
};                                // 0x10

#endif /* UTILS_MEMORY_POOL_H */

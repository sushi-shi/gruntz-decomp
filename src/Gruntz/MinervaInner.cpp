// MinervaInner.cpp - the teardown of an engine object that owns an intrusive list
// at +0x94 (re-homed from src/Stub/MinervaInner.cpp). The list head + each node's
// next link store (node + 4), so a node is recovered as (link - 4); null stays
// null.
//
// Free walks the +0x94 list (following each node's +0x04 next link, NOT re-reading
// the head), destroys each node's embedded sub-object at +0x6c, then runs the
// final cleanup. The sub-object destroy (0x137f00) / Cleanup (0x136de0) are
// external __thiscall helpers, modeled with no body so their `call rel32`
// displacements reloc-mask. Only the offsets + the (link - 4) recovery are
// load-bearing; names are placeholders.
#include <rva.h>

#include <Ints.h>

// The per-node embedded sub-object destroyed during the walk (node + 0x6c).
struct MinervaSub {
    void Destroy(); // 0x137f00
};

struct MinervaNode {
    void* m_0;
    void* m_4; // +0x04  next link (stores node + 4)
    char m_pad8[0x6c - 0x8];
    MinervaSub m_6c; // +0x6c  embedded sub-object
};

class MinervaInner {
public:
    void Free();
    void Cleanup(); // 0x136de0  final teardown

    char m_pad00[0x94];
    void* m_94; // +0x94  list head (stores node + 4)
};

// destroy every node's +0x6c sub-object down the +0x94 list, then run
// the final cleanup.
RVA(0x00137a80, 0x3d)
void MinervaInner::Free() {
    MinervaNode* node = m_94 ? (MinervaNode*)((char*)m_94 - 4) : 0;
    while (node != 0) {
        node->m_6c.Destroy();
        void* nx = node->m_4;
        node = nx ? (MinervaNode*)((char*)nx - 4) : 0;
    }
    Cleanup();
}

SIZE_UNKNOWN(MinervaNode);
SIZE_UNKNOWN(MinervaSub);

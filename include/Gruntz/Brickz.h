// Brickz.h - trace-discovered "CBrickz" cluster (placeholder name).
//
// The trace tool (gruntz.analysis.this_cluster) grouped these __thiscall methods
// by a shared `this`/ecx pointer and labeled them CBrickz. The RTTI class actually
// reached through the 0x113c0 ctor's vftable (0x5e705c) is CUserLogic, and the
// 0x9f* run operates on a SELF-CONTAINED graph/grid container sub-object with a
// different `this` shape (links via +0x14/+0x18, free/active lists at +0x30/+0x40,
// a column table at +0x08 and a width/height at +0x0c/+0x10). The true class name
// is undetermined; only the OFFSETS + code bytes are load-bearing here.
//
// These methods are pure pointer-shuffle over the container's node graph, so they
// match by shape independent of the (placeholder) class name. We model the node
// and container faithfully from the field stores; field names are placeholders.
#ifndef GRUNTZ_BRICKZ_H
#define GRUNTZ_BRICKZ_H

#include <Ints.h>

// A graph/list node. The container threads nodes through several intrusive links;
// the same physical node is reached as different "views" depending on the list.
struct BrickzNode {
    // m_0/m_4 are a (key1,key2) pair when the node is in the lookup list, and a
    // (child-ptr, free-list back-ptr) pair when the node is a cell-bucket node;
    // typed int here (Find compares them as ints), reinterpreted in Reset.
    i32 m_0;         // +0x00  key1 / child ptr
    i32 m_4;         // +0x04  key2 / back-link (free list)
    BrickzNode* m_8; // +0x08  fwd-link (free/active list) / bucket next
    char m_padc[0x10 - 0x0c];
    i32 m_10;         // +0x10  sort key
    BrickzNode* m_14; // +0x14  list link A (prev / next)
    BrickzNode* m_18; // +0x18  list link B (next / prev)
    char m_pad1c[0x20 - 0x1c];
    BrickzNode* m_20; // +0x20  owning bucket-node back-pointer
};

// A grid cell occupies 0x1c bytes; its bucket-list head sits at offset +0x18.
struct BrickzCell {
    char m_pad0[0x18];
    BrickzNode* m_head; // +0x18  bucket-list head
};

class CBrickz {
public:
    // ---- reconstructed in src/Gruntz/Brickz.cpp ----
    i32 Insert(BrickzNode* node);             // 0x09f370
    void PopFront();                          // 0x09f430
    void CellPush(BrickzNode* node);          // 0x09f470
    BrickzNode* Find(i32 key1, i32 key2);     // 0x09f500
    void Drain();                             // 0x09f590
    void Unlink(BrickzNode* node);            // 0x09f690
    void CellPop(BrickzNode* node, i32 flag); // 0x09f710
    void Reset();                             // 0x09f5d0

    // container fields (offsets recovered from the field stores above)
    char m_pad0[0x04];
    BrickzCell* m_4;  // +0x04  flat cell pool (width*height cells)
    BrickzCell** m_8; // +0x08  column table  (m_8[row] -> cell row base)
    u32 m_c;          // +0x0c  height
    u32 m_10;         // +0x10  width
    char m_pad14[0x18 - 0x14];
    BrickzNode* m_18; // +0x18  sorted/lookup list head
    char m_pad1c[0x20 - 0x1c];
    i32 m_20; // +0x20  (container; node->m_20 is the per-node back-ptr)
    char m_pad24[0x30 - 0x24];
    BrickzNode* m_30; // +0x30  list head (active)
    char m_pad34[0x40 - 0x34];
    BrickzNode* m_40; // +0x40  free list head
};

#endif // GRUNTZ_BRICKZ_H

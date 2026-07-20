#ifndef GRUNTZ_BRICKZ_H
#define GRUNTZ_BRICKZ_H
#include <rva.h>

class CBattlezData; // folded BrickzSerObj
struct tagRECT;     // Win32 RECT (CBrickzGrid::Clip arg)

#include <Ints.h>
#include <Gruntz/MapMgr.h> // CBrickzGrid IS CMapMgr (see the fold note below)

struct BrickzNode {
    // m_0/m_4 are a (key1,key2) pair when the node is in the lookup list, and a
    // (child-ptr, free-list back-ptr) pair when the node is a cell-bucket node;
    // typed int here (Find compares them as ints), reinterpreted in Reset.
    i32 m_0;          // +0x00  key1 / col / child ptr
    i32 m_4;          // +0x04  key2 / row / back-link (free list)
    BrickzNode* m_8;  // +0x08  fwd-link (free/active list) / bucket next / g cost
    i32 m_c;          // +0x0c  priority key
    i32 m_10;         // +0x10  sort key / total f
    BrickzNode* m_14; // +0x14  list link A (prev / next)
    BrickzNode* m_18; // +0x18  list link B (next / prev)
    i32 m_1c;         // +0x1c  payload (search data)
    BrickzNode* m_20; // +0x20  owning bucket-node / parent back-pointer
};

struct BrickzCell {
    // +0x00 is read BOTH ways in retail: as a dword of packed terrain flags, and as a single
    // BYTE at +3 (CGrunt's tracked-coord scan tests `byte [cell+3] & 0x20` - the "stepped"
    // bit - with a byte load, not a dword test of 0x20000000). An anonymous union models both
    // without a cast; it is the same 4 bytes. (This is what the CScanCell view in
    // GruntCombat.cpp existed to express.)
    union {
        i32 m_0;           // +0x00  packed terrain flags
        u8 m_flagBytes[4]; //        byte view; [3] & 0x20 = the stepped/visited bit
    };
    i32 m_4;            // +0x04  per-cell edge/id payload
    BrickzCell* m_8;    // +0x08  per-cell link (CGruntzMapMgr::LoadAttributes zeroes it)
    i32 m_c;            // +0x0c  id3 payload (ComputeCellFlags)
    i32 m_10;           // +0x10  bute type code (ComputeCellFlags)
    i32 m_count;        // +0x14  per-cell open-list reference count
    BrickzNode* m_head; // +0x18  bucket-list head
};

#endif // GRUNTZ_BRICKZ_H

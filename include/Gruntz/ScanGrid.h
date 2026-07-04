// ScanGrid.h - the shared board grid view (g_mgrSettings->m_tileGrid, CBrickz-shape):
// m_8 row table, m_c/m_10 dims, m_60 dirty rect, m_70/m_74 its size. Scan TUs that
// only read the dims still adopt the full shape (extra members are unreferenced).
// The grid's cell type (CScanCell, 0x1c B) is the single shared definition here;
// the CGrunt tile/arrival/target-scan TUs all fold onto it. Placeholder names;
// offsets are load-bearing.
#ifndef GRUNTZ_GRUNTZ_CSCANGRID_H
#define GRUNTZ_GRUNTZ_CSCANGRID_H

#include <rva.h>

#include <Win32.h> // RECT

// One occupancy cell of the tile-scan grid (0x1c B / 7 dwords). m_flags carries
// the per-cell flag bits (bit 0x40 reserved, 0x4000/0x8000 blocked, 0x10000
// claimed); m_type is the tile type/code word (0x97/0x98/0x99 sentinel tiles).
// Cells that only read m_flags (the CTileScan gate) share this same shape.
struct CScanCell {
    i32 m_flags; // +0x00  flag bits
    char _04[0x10 - 0x4];
    i32 m_type; // +0x10  tile type/code
    char _14[0x1c - 0x14];
};

struct CScanGrid {
    char _00[8];
    CScanCell** m_8; // +0x08 row table
    i32 m_c, m_10;   // +0x0c width, +0x10 height
    char _14[0x60 - 0x14];
    RECT m_60;      // +0x60 dirty rect
    i32 m_70, m_74; // +0x70/0x74 its size
};

// Path-node coordinate pair {col,row} that CScanNode324::m_8 points at. Shared by
// the CGrunt tile/arrival scan TUs (was locally redeclared per-TU).
struct CScanCoord {
    i32 x, y;
};

// The grunt's current path node (grunt->m_324): +0x08 -> the {col,row} coord pair.
struct CScanNode324 {
    char _00[8];
    CScanCoord* m_8; // +0x08 -> {col,row}
};

// A pending-coord list node (grunt->m_320 chain): +0x08 is the recycled coord-node
// handle fed to the coord recycle pool's Drop.
struct CScanListNode {
    CScanListNode* m_next; // +0x00
    i32 _04;
    i32 m_8; // +0x08 recycled coord-node handle
};

// The grunt's +0x10 sub-object: screen x/y at +0x5c/+0x60.
struct CScanSub10 {
    char _00[0x5c];
    i32 m_5c, m_60; // +0x5c screen x, +0x60 screen y
};
SIZE_UNKNOWN(CScanSub10); // tree-wide tag (was hosted in GruntPathScan.cpp before the CGrunt fold)

#endif // GRUNTZ_GRUNTZ_CSCANGRID_H

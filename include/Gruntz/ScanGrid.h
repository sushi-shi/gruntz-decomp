#ifndef GRUNTZ_GRUNTZ_CSCANGRID_H
#define GRUNTZ_GRUNTZ_CSCANGRID_H

#include <rva.h>

#include <Win32.h> // RECT

struct CScanCell {
    i32 m_flags; // +0x00  flag bits
    char _04[0x10 - 0x4];
    i32 m_type; // +0x10  tile type/code
    char _14[0x1c - 0x14];
};
SIZE(0x1c); // stride proven by every scan walk (cell++ steps 0x1c)

struct CScanGrid {
    char _00[8];
    CScanCell** m_8;       // +0x08 row table
    i32 m_width, m_height; // +0x0c width, +0x10 height (names shared with CMapMgr
                           // so the grid macros work on both)
    char _14[0x60 - 0x14];
    RECT m_60;      // +0x60 dirty rect
    i32 m_70, m_74; // +0x70/0x74 its size
};
SIZE_UNKNOWN();

struct CGameRegistry; // CTileScan::m_4 (the registry whose +0x150 m_focusSlots[] the scan probes)
class CGrunt;         // CTileScan::Scan arg (the scanned grunt)

// @identity-TODO: the 3x3 tile-region scan owner (TileScan.cpp @0x35f10) is an orphan
// COMDAT - no caller / new-site / RTTI / vtable-dispatch (all attribution techniques
// dead-end). Its members are typed from their proven roles: m_4 the CGameRegistry (the
// scan indexes its +0x150 m_focusSlots[] by the grunt's slot id), m_c the CScanGrid tile
// board, m_c8 the per-frame dwell threshold. Homed here (its shape belongs in the shared
// scan header, not the .cpp); Scan's body is in TileScan.cpp.
struct CTileScan {
    char _00[4];
    class CGruntzMgr* m_4; // +0x04  the manager singleton (its +0x150 m_options[] the scan probes)
    char _08[0xc - 8];
    CScanGrid* m_c; // +0x0c  tile board (dims + row table)
    char _10[0xc8 - 0x10];
    i32 m_c8;              // +0xc8  dwell threshold
    i32 Scan(CGrunt* arg); // 0x35f10
};
SIZE_UNKNOWN();

// @identity-TODO: the 10x10 tile-region scan owner (GruntTileScan.cpp
// CScanMgr::ScanRegion @0x32ce0) - a sibling orphan scan-owner of CTileScan (both
// own a CScanGrid at +0x0c) whose only reference is an ILT thunk-band that dead-ends;
// kept a DISTINCT shape (its threshold is at +0xcc vs CTileScan's +0xc8, and it adds the
// +0xf4/+0xf8 goal table) pending proof they are the same class. Homed here (shape belongs
// in the shared scan header, not the .cpp); the scan body lives in GruntTileScan.cpp.
struct CScanGoal { // CScanMgr::m_f4[] element (a {x,y} goal point)
    i32 m_0, m_4;
};
SIZE_UNKNOWN();
struct CScanMgr {
    // Fire the per-cell trigger (msg 0xd87) on a flagged tile. No body -> the __thiscall
    // through the 0x1fb9 ILT thunk (-> 0x300c0) reloc-masks.
    i32 DoTrigger1fb9(CGrunt* g, i32 x, i32 y, i32 msg, i32 c, i32 d); // 0x1fb9
    char _00[0xc];
    CScanGrid* m_c; // +0x0c  tile board
    char _10[0xcc - 0x10];
    u32 m_cc; // +0xcc  idle threshold
    char _d0[0xf4 - 0xd0];
    CScanGoal** m_f4; // +0xf4  goal table
    i32 m_f8;         // +0xf8  goal count
    i32 ScanRegion(CGrunt* g); // 0x32ce0
};
SIZE_UNKNOWN();

struct CScanSub10 {
    char _00[0x5c];
    i32 m_5c, m_60; // +0x5c screen x, +0x60 screen y
};
SIZE_UNKNOWN(); // tree-wide tag (was hosted in GruntPathScan.cpp before the CGrunt fold)

#endif // GRUNTZ_GRUNTZ_CSCANGRID_H

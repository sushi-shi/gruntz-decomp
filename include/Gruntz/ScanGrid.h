#ifndef GRUNTZ_GRUNTZ_CSCANGRID_H
#define GRUNTZ_GRUNTZ_CSCANGRID_H

#include <rva.h>

#include <Win32.h> // RECT

struct CGameRegistry; // CTileScan::m_4 (the registry whose +0x150 m_focusSlots[] the scan probes)
class CGrunt;         // CTileScan::Scan arg (the scanned grunt)
class CMapMgr;        // canonical tile board (the former CScanGrid view)

// @identity-TODO: the 3x3 tile-region scan owner (TileScan.cpp @0x35f10) is an orphan
// COMDAT - no caller / new-site / RTTI / vtable-dispatch (all attribution techniques
// dead-end). Its members are typed from their proven roles: m_4 the CGameRegistry (the
// scan indexes its +0x150 m_focusSlots[] by the grunt's slot id), m_c the CMapMgr tile
// board, m_c8 the per-frame dwell threshold. Homed here (its shape belongs in the shared
// scan header, not the .cpp); Scan's body is in TileScan.cpp.
struct CTileScan {
    char _00[4];
    class CGruntzMgr* m_4; // +0x04  the manager singleton (its +0x150 m_options[] the scan probes)
    char _08[0xc - 8];
    CMapMgr* m_c; // +0x0c  tile board (dims + row table)
    char _10[0xc8 - 0x10];
    i32 m_c8;              // +0xc8  dwell threshold
    i32 Scan(CGrunt* arg); // 0x35f10
};
SIZE_UNKNOWN();

// CBattlezMapConfig::ScanRegion's m_0f0 elements are {x,y} goal points.
struct CScanGoal {
    i32 m_0, m_4;
};
SIZE_UNKNOWN();

struct CScanSub10 {
    char _00[0x5c];
    i32 m_5c, m_60; // +0x5c screen x, +0x60 screen y
};
SIZE_UNKNOWN(); // tree-wide tag (was hosted in GruntPathScan.cpp before the CGrunt fold)

#endif // GRUNTZ_GRUNTZ_CSCANGRID_H

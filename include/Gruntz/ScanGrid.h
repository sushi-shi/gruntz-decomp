#ifndef GRUNTZ_GRUNTZ_CSCANGRID_H
#define GRUNTZ_GRUNTZ_CSCANGRID_H

#include <rva.h>

#include <Win32.h> // RECT

class CGrunt;  // scan args (the scanned grunt)
class CMapMgr; // canonical tile board (the former CScanGrid view)

// The ex-CTileScan view (3x3 tile-region scan owner, TileScan.cpp @0x35f10) is
// DISSOLVED: its sole retail caller is CBattlezMapConfig::StepRowUnits @0x267c0
// (m_2d8==0xb dispatch arm) calling on ITS `this`, and every field aligned
// (m_4==m_ctx, m_c==m_board, m_c8==m_0c8). Scan is CBattlezMapConfig::Scan now
// (<Gruntz/BattlezMapConfig.h>); the body stays in TileScan.cpp (retail placement).

// (CScanGoal DISSOLVED 2026-07-27: ScanRegion's m_0f0 elements are the SAME
// GruntCoord {x,y} records CBattlezMapConfig::coordData() already names - one
// member, one element type, two spellings. Use coordData().)

struct CScanSub10 {
    char _00[0x5c];
    i32 m_5c, m_60; // +0x5c screen x, +0x60 screen y
};
SIZE_UNKNOWN(); // tree-wide tag (was hosted in GruntPathScan.cpp before the CGrunt fold)

#endif // GRUNTZ_GRUNTZ_CSCANGRID_H

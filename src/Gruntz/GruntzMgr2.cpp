// GruntzMgr2.cpp - the second CGruntzMgr retail object. CGruntzMgr's methods are
// compiled across several retail objects; the bulk lives in GruntzMgr.cpp
// (0x083030..0x093ce7) but CGruntzMgr::SetCellHeight is a lone contribution far out
// at 0x111ec0 (interleaved with the tile-trigger-switch-logic .text block), a
// separate retail object. Split here (same class, same "eh" flags) so each src TU
// maps to one contiguous retail .text region. Byte-neutral TU cut.
#include <Gruntz/GruntzMgr.h>
#include <Rez/RezAlloc.h> // RezAlloc/RezFree
#include <Gruntz/GameRegistry.h> // CSpriteFactoryHolder (m_world's real class)
#include <Gruntz/GameLevel.h> // CGameLevel (m_world->m_24) + CLevelPlane // CGruntzMgr / CWorldZ / CGameLevel (m_world->m_24) / CGruntzMapMgr
#include <Wwd/WwdFile.h> // CPlaneRender (the world plane; m_tileGrid / m_colOffsets height grid)
#include <rva.h>

// operator-delete wrapper (RezFree, __cdecl; reloc-masked).

// -------------------------------------------------------------------------
// CGruntzMgr::SetCellHeight (0x111ec0; ret 0xc). Writes value into the loaded
// world's height grid: idx = view->m_24[col] + row; view->m_20[idx] = value.
// Then forwards (row, col, value) to the +0x70 notify object (reloc-masked).
RVA(0x00111ec0, 0x37)
void CGruntzMgr::SetCellHeight(i32 row, i32 col, i32 value) {
    CPlaneRender* grid = (CPlaneRender*)m_world->m_24->m_mainPlane;
    i32 idx = grid->m_colOffsets[col] + row;
    grid->m_tileGrid[idx] = value;
    RezFree((void*)m_tileGrid);
}

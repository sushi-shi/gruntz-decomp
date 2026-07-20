#include <Gruntz/GruntzMgr.h>
#include <Rez/RezAlloc.h>        // RezAlloc/RezFree
#include <Gruntz/GameRegistry.h> // CDDrawSurfaceMgr (m_world's real class)
#include <Gruntz/GameLevel.h> // CGameLevel (m_world->m_level) + CLevelPlane // CGruntzMgr / CWorldZ / CGameLevel (m_world->m_level) / CGruntzMapMgr
#include <Wwd/WwdFile.h> // CPlaneRender (the world plane; m_tileGrid / m_colOffsets height grid)
#include <rva.h>

RVA(0x00111ec0, 0x37)
void CGruntzMgr::SetCellHeight(i32 row, i32 col, i32 value) {
    CPlaneRender* grid = m_world->m_level->m_mainPlane;
    i32 idx = grid->m_colOffsets[col] + row;
    grid->m_tileGrid[idx] = value;
    RezFree(static_cast<void*>(m_tileGrid));
}

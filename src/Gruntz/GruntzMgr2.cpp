#include <rva.h>

#include <Gruntz/GameLevel.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GruntzMgr.h>
#include <Wwd/WwdFile.h>

RVA(0x00111ec0, 0x37)
void CGruntzMgr::SetCellHeight(i32 row, i32 col, i32 value) {
    CDDrawWorkerHost* grid = m_world->m_level->m_mainPlane;
    i32 idx = grid->m_colOffsets[col] + row;
    grid->m_tileGrid[idx] = value;

    m_tileGrid->ComputeCellFlags(row, col, value);
}

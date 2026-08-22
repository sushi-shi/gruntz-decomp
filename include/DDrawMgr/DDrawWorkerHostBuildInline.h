#ifndef GRUNTZ_CDDRAWWORKERHOSTBUILDINLINE_H
#define GRUNTZ_CDDRAWWORKERHOSTBUILDINLINE_H

#include <DDrawMgr/DDrawWorkerHost.h>
#include <Wap32/CoordUnset.h>

#define APPLY_WORKER_HOST_BOUNDS(coords)                                                           \
    if (coords->left != COORD_UNSET) {                                                             \
        LevelCoordRect local;                                                                      \
        CopyRect((&local), (coords));                                                              \
        m_bounds50 = local;                                                                        \
        m_viewW = m_bounds50.right - m_bounds50.left + 1;                                          \
        m_viewH = m_bounds50.bottom - m_bounds50.top + 1;                                          \
        m_anchorX = m_viewW / 2;                                                                   \
        m_anchorY = m_viewH / 2;                                                                   \
        RecomputePlaneCoords();                                                                    \
    }

#define SET_TILE_SIZE_FROM_IMAGE(image) SetTileSize((image)->m_width, (image)->m_height)

#endif // GRUNTZ_CDDRAWWORKERHOSTBUILDINLINE_H

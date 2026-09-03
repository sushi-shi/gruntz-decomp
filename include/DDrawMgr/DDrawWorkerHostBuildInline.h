#ifndef GRUNTZ_CDDRAWWORKERHOSTBUILDINLINE_H
#define GRUNTZ_CDDRAWWORKERHOSTBUILDINLINE_H

#include <DDrawMgr/DDrawWorkerHost.h>
#include <Wap32/CoordUnset.h>

#define APPLY_WORKER_HOST_BOUNDS(coords)                                                           \
    if (coords->left != COORD_UNSET) {                                                             \
        LevelCoordRect local;                                                                      \
        CopyRect((&local), (coords));                                                              \
        m_viewportRect = local;                                                                    \
        m_viewportSize = CRect(m_viewportRect).Size() + CSize(1, 1);                               \
        m_viewHalfSize = CSize(m_viewportSize.cx / 2, m_viewportSize.cy / 2);                      \
        UpdatePlaneViewRect();                                                                     \
    }

#define SET_TILE_SIZE_FROM_IMAGE(image) SetTileSize((image)->m_width, (image)->m_height)

#endif // GRUNTZ_CDDRAWWORKERHOSTBUILDINLINE_H

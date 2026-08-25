#ifndef GRUNTZ_CDDRAWWORKERHOSTBUILDINLINE_H
#define GRUNTZ_CDDRAWWORKERHOSTBUILDINLINE_H

#include <DDrawMgr/DDrawWorkerHost.h>
#include <Wap32/CoordUnset.h>

#define APPLY_WORKER_HOST_BOUNDS(coords)                                                           \
    if (coords->left != COORD_UNSET) {                                                             \
        LevelCoordRect local;                                                                      \
        CopyRect((&local), (coords));                                                              \
        m_viewportRect = local;                                                                    \
        m_viewportWidth = m_viewportRect.right - m_viewportRect.left + 1;                          \
        m_viewportHeight = m_viewportRect.bottom - m_viewportRect.top + 1;                         \
        m_viewHalfWidth = m_viewportWidth / 2;                                                     \
        m_viewHalfHeight = m_viewportHeight / 2;                                                   \
        UpdatePlaneViewRect();                                                                     \
    }

#define SET_TILE_SIZE_FROM_IMAGE(image) SetTileSize((image)->m_width, (image)->m_height)

#endif // GRUNTZ_CDDRAWWORKERHOSTBUILDINLINE_H

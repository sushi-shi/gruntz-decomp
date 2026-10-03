#ifndef GRUNTZ_DDRAWMGR_DDRAWWORKERHOSTDRAWMACROS_H
#define GRUNTZ_DDRAWMGR_DDRAWWORKERHOSTDRAWMACROS_H

#include <DDrawMgr/DDrawWorkerHost.h>
#include <DDrawMgr/DDSurface.h>
#include <Image/CImage.h>
#include <MakeRect.h>
#include <Wwd/WwdTileHandle.h>

#define DRAW_CELL(handle, xp, yp, srcp)                                                            \
    do {                                                                                           \
        u32 h_ = static_cast<u32>(handle);                                                         \
        if (h_ == UNINIT_FILL) {                                                                   \
            dr.left = (xp);                                                                        \
            dr.top = (yp);                                                                         \
            dr.right = (xp) + ((srcp)->right - (srcp)->left);                                      \
            dr.bottom = (yp) + ((srcp)->bottom - (srcp)->top);                                     \
            surf->BltEx(&dr, 0, 0, DDBLT_WAIT | DDBLT_COLORFILL, &m_fillFx);                       \
        } else if (h_ != static_cast<u32>(s_tileClear)) {                                          \
            CDDrawWorker* fr_ = ImageSetAt(h_ >> 16);                                              \
            i32 idx_ = static_cast<i32>(h_ & WWD_TILE_IMAGE_SET_INDEX_MASK);                       \
            CImage* e_ = fr_->GetAt(idx_);                                                         \
            surf->BltFast((xp), (yp), e_->m_surface, (srcp), e_->m_bltFastFlags);                  \
        }                                                                                          \
    } while (0)

static inline void
DrawCell(CDDrawWorkerHost* host, CDDSurface* surface, i32 handle, i32 x, i32 y, RECT* source) {
    u32 tileHandle = static_cast<u32>(handle);
    if (tileHandle == UNINIT_FILL) {
        CRect destination =
            MakeRect(x, y, x + (source->right - source->left), y + (source->bottom - source->top));
        surface->BltEx(&destination, NULL, NULL, DDBLT_WAIT | DDBLT_COLORFILL, &host->m_fillFx);
    } else if (tileHandle != static_cast<u32>(WWD_TILE_CLEAR)) {
        CDDrawWorker* frames = host->ImageSetAt(tileHandle >> 16);
        i32 index = static_cast<i32>(tileHandle & WWD_TILE_IMAGE_SET_INDEX_MASK);
        CImage* image = frames->GetAt(index);
        surface->BltFast(x, y, image->m_surface, source, image->m_bltFastFlags);
    }
}

#endif // GRUNTZ_DDRAWMGR_DDRAWWORKERHOSTDRAWMACROS_H

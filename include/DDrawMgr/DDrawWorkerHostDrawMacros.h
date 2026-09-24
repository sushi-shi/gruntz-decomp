#ifndef GRUNTZ_DDRAWMGR_DDRAWWORKERHOSTDRAWMACROS_H
#define GRUNTZ_DDRAWMGR_DDRAWWORKERHOSTDRAWMACROS_H

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

#endif // GRUNTZ_DDRAWMGR_DDRAWWORKERHOSTDRAWMACROS_H

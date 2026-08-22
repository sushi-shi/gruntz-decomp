#ifndef GRUNTZ_DDRAWMGR_DDRAWSUBMGRPAGESINLINE_H
#define GRUNTZ_DDRAWMGR_DDRAWSUBMGRPAGESINLINE_H

#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDSurface.h>

inline void FlipFrontAndRestoreOverlay(CDDrawSubMgrPages* pages) {
    pages->m_frontPair->m_surface->Flip(0);
    pages->m_backPair->m_surface
        ->BltFast(0, 0, pages->m_overlayPair->m_surface, &pages->m_overlayPair->m_srcRect, 0x10);
}

#endif // GRUNTZ_DDRAWMGR_DDRAWSUBMGRPAGESINLINE_H

#ifndef GRUNTZ_DDRAWMGR_DDRAWSUBMGRPAGESINLINE_H
#define GRUNTZ_DDRAWMGR_DDRAWSUBMGRPAGESINLINE_H

#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDSurface.h>

#include <stddef.h>

inline void FlipFrontAndRestoreOverlay(CDDrawSubMgrPages* pages) {
    pages->m_frontSurface->GetSurface()->Flip(NULL);
    pages->m_backPair->GetSurface()->BltFast(
        0,
        0,
        pages->m_overlayPair->GetSurface(),
        &pages->m_overlayPair->m_srcRect,
        DDBLTFAST_WAIT
    );
}

#endif // GRUNTZ_DDRAWMGR_DDRAWSUBMGRPAGESINLINE_H

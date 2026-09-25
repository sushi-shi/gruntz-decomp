#ifndef GRUNTZ_DDRAWMGR_DRAWSUBWORKERINLINE_H
#define GRUNTZ_DDRAWMGR_DRAWSUBWORKERINLINE_H

#include <MfcWin.h>

#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDSurface.h>
#include <Ints.h>

#include <ddraw.h>
#include <stddef.h>

inline void CDrawSubWorker::BlitDirtyRect(CDDrawSurfacePair* other, i32* pos, i32* size) {
    CRect rc(CPoint(pos[0], pos[1]), CSize(size[0], size[1]));
    m_surface->BltEx(&rc, other->m_surface, &rc, DDBLT_WAIT, NULL);
}

#endif // GRUNTZ_DDRAWMGR_DRAWSUBWORKERINLINE_H

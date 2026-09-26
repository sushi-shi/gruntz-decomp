#ifndef GRUNTZ_DDRAWMGR_DRAWSUBWORKERINLINE_H
#define GRUNTZ_DDRAWMGR_DRAWSUBWORKERINLINE_H

#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDSurface.h>
#include <Ints.h>

#include <ddraw.h>
#include <stddef.h>

inline void
CDrawSubWorker::BlitDirtyRect(CDDrawSurfacePair* other, const POINT& pos, const SIZE& size) {
    CRect rc(CPoint(pos.x, pos.y), CSize(size.cx, size.cy));
    m_surface->BltEx(&rc, other->GetSurface(), &rc, DDBLT_WAIT, NULL);
}

#endif // GRUNTZ_DDRAWMGR_DRAWSUBWORKERINLINE_H

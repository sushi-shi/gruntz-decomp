#ifndef GRUNTZ_DDRAWMGR_DRAWSUBWORKERINLINE_H
#define GRUNTZ_DDRAWMGR_DRAWSUBWORKERINLINE_H

#include <MfcWin.h>

#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDSurface.h>
#include <Ints.h>

#include <ddraw.h>
#include <stddef.h>

inline void
CDrawSubWorker::BlitDirtyRect(CDDrawSurfacePair* other, const POINT& position, const SIZE& size) {
    CRect rc(position, size);
    m_surface->BltEx(&rc, other->m_surface, &rc, DDBLT_WAIT, NULL);
}

#endif // GRUNTZ_DDRAWMGR_DRAWSUBWORKERINLINE_H

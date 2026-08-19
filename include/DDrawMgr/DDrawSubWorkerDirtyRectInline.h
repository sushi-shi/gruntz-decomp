#ifndef GRUNTZ_DDRAWMGR_CDDRAWSUBWORKERDIRTYRECTINLINE_H
#define GRUNTZ_DDRAWMGR_CDDRAWSUBWORKERDIRTYRECTINLINE_H

#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDSurface.h>
#include <MakeRect.h>

inline void CDrawSubWorker::BlitDirtyRect(CDDrawSurfacePair* other, i32* pos, i32* size) {
    RECT rc;
    rc = MakeRect(pos[0], pos[1], pos[0] + size[0], pos[1] + size[1]);
    m_surface->BltEx(&rc, other->m_surface, &rc, 0x1000000, 0);
}

#endif // GRUNTZ_DDRAWMGR_CDDRAWSUBWORKERDIRTYRECTINLINE_H

#ifndef GRUNTZ_DDRAWMGR_DDSURFACECAPS_H
#define GRUNTZ_DDRAWMGR_DDSURFACECAPS_H

#include <DDrawMgr/DDSurface.h>

inline DWORD SurfaceCaps(CDDSurface* surface, DWORD mask) {
    DDSCAPS caps;
    if (surface->m_ddSurface->GetCaps(&caps) == 0) {
        return caps.dwCaps & mask;
    }
    return 0;
}

#endif // GRUNTZ_DDRAWMGR_DDSURFACECAPS_H

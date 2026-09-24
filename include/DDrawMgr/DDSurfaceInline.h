#ifndef GRUNTZ_DDRAWMGR_DDSURFACEINLINE_H
#define GRUNTZ_DDRAWMGR_DDSURFACEINLINE_H

#include <DDrawMgr/DDSurface.h>

static inline i32 PixOffset(const CDDSurface* surface, i32 x, i32 y) {
    return y * surface->m_apiDesc.lPitch + x * surface->m_bytesPerPixel;
}

#endif // GRUNTZ_DDRAWMGR_DDSURFACEINLINE_H

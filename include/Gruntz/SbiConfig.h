#ifndef GRUNTZ_SBICONFIG_H
#define GRUNTZ_SBICONFIG_H

#include <rva.h>
#include <Gruntz/GameRegistry.h>      // CDDrawSurfaceMgr - the real config-host class
#include <DDrawMgr/DDrawSurfaceMgr.h> // CImageRegistry (host->m_10) + its m_10map
#include <Image/ImageSet.h>           // CImageSet - the real class the lookup yields

SIZE_UNKNOWN(CSbiSurfacePool);
struct CSbiSurfacePool {};

#endif // GRUNTZ_SBICONFIG_H

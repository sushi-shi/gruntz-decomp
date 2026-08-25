#ifndef GRUNTZ_GRUNTZ_SHAPEFADERCONFIG_H
#define GRUNTZ_GRUNTZ_SHAPEFADERCONFIG_H

#include <rva.h>

#include <Mfc.h>

#include <Gruntz/FaderConfig.h>

GZ_ENUM_FORWARD(FaderMode);

class CShapeFaderConfig : public CFaderConfig {
public:
    CShapeFaderConfig();
    class CDDSurface* m_warpSourceSurface;
    i32 m_halfWidth;
    GZ_ENUM_STORAGE(FaderMode, u32) m_mode;
    i32 m_stripCopy;
    i32 m_useLut;
    class CShadeTable* m_shadeTable;
    CString m_shadeTablePath;
    class CDDPalette* m_palette;
};

#endif // GRUNTZ_GRUNTZ_SHAPEFADERCONFIG_H

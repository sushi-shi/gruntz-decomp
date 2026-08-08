#ifndef GRUNTZ_FXMODET1_H
#define GRUNTZ_FXMODET1_H

#include <rva.h>

#include <Mfc.h>

#include <Gruntz/FxModeDesc.h>

GZ_ENUM_FORWARD(FaderMode);

class CFxModeT1 : public CFxModeDesc {
public:
    CFxModeT1();
    class CDDSurface* m_warpSourceSurface;
    i32 m_halfWidth;
    GZ_ENUM_STORAGE(FaderMode, u32) m_mode;
    i32 m_stripCopy;
    i32 m_useLut;
    class CShadeTable* m_shadeTable;
    CString m_shadeTablePath;
    class CDDPalette* m_palette;
};
SIZE(0x2c);

#endif // GRUNTZ_FXMODET1_H

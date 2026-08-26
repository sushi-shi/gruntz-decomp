#ifndef GRUNTZ_GRUNTZ_FADERCONFIG_H
#define GRUNTZ_GRUNTZ_FADERCONFIG_H

#include <rva.h>

#include <Gruntz/FaderConfigKind.h>
#include <Ints.h>

class CFaderConfig {
public:
    CFaderConfig();

    FaderConfigKind m_kind;
    class CDDSurface* m_targetSurface;
    class CDDSurface* m_sourceSurface;
};

class CLightFaderConfig : public CFaderConfig {
public:
    CLightFaderConfig();
    class CDDPalette* m_palette;
    b32 m_clearMode;
    i32 m_spanCount;
    i32 m_centerX;
    i32 m_centerY;
    class CShadeTable* m_shadeTable;
};

class CSineFaderConfig : public CFaderConfig {
public:
    CSineFaderConfig();
    b32 m_clearToBlack;
    i32 m_intensityPercent;
};

class CRadialFaderConfig : public CFaderConfig {
public:
    CRadialFaderConfig();
    i32 m_unusedOption;
    class CDDPalette* m_palette;
    class CShadeTable* m_shadeTable;
};

class CFlatFaderConfig : public CFaderConfig {
public:
    CFlatFaderConfig();
    i32 m_unusedOption;
    i32 m_durationPercent;
    i32 m_splitPercent;
};

class CMeshFaderConfig : public CFaderConfig {
public:
    CMeshFaderConfig();
    class CDDSurface* m_primeSource;
    class CDDSurface* m_flipTarget;
    b32 m_reverseOrder;
    i32 m_unusedOption;
    i32 m_cols;
    i32 m_rows;
};

#endif // GRUNTZ_GRUNTZ_FADERCONFIG_H

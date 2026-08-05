#ifndef GRUNTZ_CFXMODEDESC_H
#define GRUNTZ_CFXMODEDESC_H

#include <rva.h>

#include <Gruntz/FxModeKind.h>
#include <Ints.h>

class CFxModeDesc {
public:
    CFxModeDesc();

    FxModeKind m_type;
    class CDDSurface* m_targetSurface;
    class CDDSurface* m_sourceSurface;
};
SIZE(0xc);

class CFxModeT2 : public CFxModeDesc {
public:
    CFxModeT2();
    class CDDPalette* m_palette;
    i32 m_clearMode;
    i32 m_spanCount;
    i32 m_centerX;
    i32 m_centerY;
    class CShadeTable* m_shadeTable;
};
SIZE(0x24);

class CFxModeT3 : public CFxModeDesc {
public:
    CFxModeT3();
    i32 m_clearToBlack;
    i32 m_intensityPercent;
};
SIZE(0x14);

class CFxModeT4 : public CFxModeDesc {
public:
    CFxModeT4();
    i32 m_param0c;
    class CDDPalette* m_palette;
    class CShadeTable* m_shadeTable;
};
SIZE(0x18);

class CFxModeT5 : public CFxModeDesc {
public:
    CFxModeT5();
    i32 m_param0c;
    i32 m_durationPercent;
    i32 m_splitPercent;
};
SIZE(0x18);

class CFxModeT6 : public CFxModeDesc {
public:
    CFxModeT6();
    class CDDSurface* m_primeSource;
    class CDDSurface* m_flipTarget;
    i32 m_reverseOrder;
    i32 m_param18;
    i32 m_cols;
    i32 m_rows;
};
SIZE(0x24);

#endif // GRUNTZ_CFXMODEDESC_H

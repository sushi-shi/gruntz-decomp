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

class CFxModeT3 : public CFxModeDesc {
public:
    CFxModeT3();
    i32 m_clearToBlack;
    i32 m_intensityPercent;
};

class CFxModeT4 : public CFxModeDesc {
public:
    CFxModeT4();
    i32 m_unusedOption;
    class CDDPalette* m_palette;
    class CShadeTable* m_shadeTable;
};

class CFxModeT5 : public CFxModeDesc {
public:
    CFxModeT5();
    i32 m_unusedOption;
    i32 m_durationPercent;
    i32 m_splitPercent;
};

class CFxModeT6 : public CFxModeDesc {
public:
    CFxModeT6();
    class CDDSurface* m_primeSource;
    class CDDSurface* m_flipTarget;
    i32 m_reverseOrder;
    i32 m_unusedOption;
    i32 m_cols;
    i32 m_rows;
};

#endif // GRUNTZ_CFXMODEDESC_H

#ifndef GRUNTZ_CFXMODEDESC_H
#define GRUNTZ_CFXMODEDESC_H

#include <Ints.h>
#include <rva.h>

class CFxModeDesc {
public:
    CFxModeDesc();

    i32 m_type;
    class CDDSurface* m_04;
    class CDDSurface* m_08;

    union {
        i32 m_0c;
        class CDDPalette* m_pal0c;
        class CDDSurface* m_surf0c;
    };
    union {
        i32 m_10;
        class CDDPalette* m_pal10;
        class CDDSurface* m_surf10;
    };
};
SIZE_UNKNOWN();

class CFxModeT2 : public CFxModeDesc {
public:
    CFxModeT2();
    i32 m_14;
    i32 m_18;
    i32 m_1c;
    class CShadeTable* m_20;
};
SIZE_UNKNOWN();

class CFxModeT3 : public CFxModeDesc {
public:
    CFxModeT3();
};
SIZE_UNKNOWN();

class CFxModeT4 : public CFxModeDesc {
public:
    CFxModeT4();
    class CShadeTable* m_14;
};
SIZE_UNKNOWN();

class CFxModeT5 : public CFxModeDesc {
public:
    CFxModeT5();
    i32 m_14;
};
SIZE_UNKNOWN();

class CFxModeT6 : public CFxModeDesc {
public:
    CFxModeT6();
    i32 m_14;
    i32 m_18;
    i32 m_1c;
    i32 m_20;
};
SIZE_UNKNOWN();

#endif // GRUNTZ_CFXMODEDESC_H

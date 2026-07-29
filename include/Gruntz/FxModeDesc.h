#ifndef GRUNTZ_CFXMODEDESC_H
#define GRUNTZ_CFXMODEDESC_H

#include <Ints.h>
#include <rva.h>

class CFxModeDesc {
public:
    CFxModeDesc(); // 0x17e7b0 base init (type = 0)

    i32 m_type;             // +0x00 discriminator: it selects the union arms below
    class CDDSurface* m_04; // the mode's source surface
    class CDDSurface* m_08; // the mode's dest/alt surface
    // +0x0c/+0x10 are a real TAGGED UNION, so they are modelled as one instead of
    // punned at every reader (the device WaveFormatX/BrickzCell already use):
    // CFaderLight reads +0x0c as a CDDPalette*, the flip fader reads +0x0c/+0x10 as
    // CDDSurface*, the shade fader reads +0x10 as a CDDPalette*, and the box fader
    // reads both as plain ints. All arms are 4 bytes, so the layout is unchanged.
    union {
        i32 m_0c;                   // box-fader parameter
        class CDDPalette* m_pal0c;  // CFaderLight's palette
        class CDDSurface* m_surf0c; // the flip fader's prime source
    };
    union {
        i32 m_10;                   // box-fader parameter / light gate / percent
        class CDDPalette* m_pal10;  // the shade fader's palette
        class CDDSurface* m_surf10; // the flip fader's flip target
    };
}; // 0x14 - the shared base; the upper fields (m_14..m_20) belong to whichever
SIZE_UNKNOWN();

class CFxModeT2 : public CFxModeDesc {
public:
    CFxModeT2(); // 0x17e840
    i32 m_14;
    i32 m_18;
    i32 m_1c;
    class CShadeTable* m_20; // the fader's shade table (null = none)
}; // 0x24
SIZE_UNKNOWN();

class CFxModeT3 : public CFxModeDesc {
public:
    CFxModeT3(); // 0x17e880
}; // 0x14
SIZE_UNKNOWN();

class CFxModeT4 : public CFxModeDesc {
public:
    CFxModeT4();             // 0x17e8b0
    class CShadeTable* m_14; // the fader's shade table (null = none)
}; // 0x18
SIZE_UNKNOWN();

class CFxModeT5 : public CFxModeDesc {
public:
    CFxModeT5(); // 0x17e8e0
    i32 m_14;
}; // 0x18
SIZE_UNKNOWN();

class CFxModeT6 : public CFxModeDesc {
public:
    CFxModeT6(); // 0x17e910
    i32 m_14;
    i32 m_18;
    i32 m_1c;
    i32 m_20;
}; // 0x24
SIZE_UNKNOWN();

#endif // GRUNTZ_CFXMODEDESC_H

#ifndef GRUNTZ_GRUNTZ_CFADERSUBTYPES_H
#define GRUNTZ_GRUNTZ_CFADERSUBTYPES_H

#include <rva.h>

#include <Mfc.h>

#include <DDrawMgr/DDSurface.h>
#include <Enums.h>
#include <Gruntz/Fader.h>
#include <Gruntz/FxModeDesc.h>
#include <Ints.h>
#include <Rez/RezBufferObject.h>

GZ_ENUM_FORWARD(FaderMode);

class CDDSurface;
struct CDDPalette;

struct CFaderRadialCell {
    float m_vx;
    float m_vy;
    float m_radius;
    i32 m_pixel;
};
SIZE(0x10);

class CFaderMesh : public CFader {
public:
    virtual ~CFaderMesh() OVERRIDE;
    CFaderMesh();
    virtual void RenderFrame(i32 f) OVERRIDE;
    virtual i32 GetFrameCount() OVERRIDE;

    i32 ApplyInit(CFxModeDesc* src);

    CDDSurface* m_bltSrc;
    CDDSurface* m_dstSurface;
    CDDSurface* m_primeSrc;
    CDDSurface* m_flipTarget;
    i32 m_desc18; // from CFxModeT6::m_param18; never read
    i32 m_recOrderFlag;
    i32 m_cols;
    i32 m_rows;
    CRezBufferObject m_meshBuf;
};
SIZE(0x6c);

class CFaderSine : public CFader {
public:
    // Monolith's GetRandomNumber, in-class (implicitly inline, no keyword) so the
    // local static is emitted COMMON with this class in its mangled name - which
    // is what gives this module its own guard/seed pair. See <Gruntz/GameRand.h>.
    i32 GetRandomNumber() {
        static long holdrand = timeGetTime();
        return (((holdrand = holdrand * 214013L + 2531011L) >> 16) & 0x7fff);
    }

    i32 FxRand(i32 range) {
        return GetRandomNumber() % range;
    }
    CFaderSine();
    virtual ~CFaderSine() OVERRIDE;
    virtual void RenderFrame(i32 f) OVERRIDE;
    virtual i32 GetFrameCount() OVERRIDE;

    i32 ApplyInit(CFxModeDesc* src);

    CDDSurface* m_srcBox;
    CDDSurface* m_dstBox;
    i32 m_boxParam;

    u8* m_srcBits;
    u8* m_dstBits;
    i32 m_frameCount;
    i32 m_elemCount;
    i32 m_scaledMag;
    i32 m_intensity;
    i32 m_arr0[2000];
    i32 m_arr1[2000];

    float m_arr2[2000];
    i32 m_arr3[2000];
};
SIZE(0x7d5c);

class CFaderFlat : public CFader {
public:
    virtual ~CFaderFlat() OVERRIDE;
    CFaderFlat();
    virtual void RenderFrame(i32 f) OVERRIDE;
    virtual i32 GetFrameCount() OVERRIDE;

    i32 ApplyInit(CFxModeDesc* src);

    CDDSurface* m_desc04;
    CDDSurface* m_src;
    i32 m_desc0c;
    i32 m_percent;
    i32 m_desc14;
    i32* m_frames;
};
SIZE(0x50);

class CFaderLight : public CFader {
public:
    virtual void BeginFade() OVERRIDE;
    virtual void EndFade() OVERRIDE;
    virtual ~CFaderLight() OVERRIDE;
    CFaderLight();
    virtual void RenderFrame(i32 f) OVERRIDE;
    virtual i32 GetFrameCount() OVERRIDE;

    i32 ApplyInit(CFxModeDesc* src);
    void SubFree();

    void Render(i32 row0, i32 radiusSq, i32 radius, u8* lut, u8* srcBits, u8* dstBits);

    CDDSurface* m_surface;
    CDDSurface* m_dstSurface;

    CDDSurface* m_overlay;
    CDDPalette* m_palette;
    i32 m_lightGate;
    i32 m_centerX;
    i32 m_centerY;

    u8* m_srcBits;
    u8* m_dstBits;
    i32 m_frameCount;
    i32 m_spanStarts[1024];
    i32 m_spanEnds[1024];
    i32 m_spanCount;
    i32 m_surfWidth;
    i32 m_surfHeight;
};
SIZE(0x206c);

class CFaderRadial : public CFader {
public:
    virtual ~CFaderRadial() OVERRIDE;
    CFaderRadial();
    virtual void RenderFrame(i32 f) OVERRIDE;
    virtual i32 GetFrameCount() OVERRIDE;

    i32 ApplyInit(CFxModeDesc* src);
    void FreeBuffer();

    CDDSurface* m_srcSurface;
    CDDSurface* m_dstSurface;
    i32 m_reserved40;
    i32 m_maxRadius;
    i32 m_reserved48;
    float m_fadeDivisor;

    CFaderRadialCell* m_cells;
    i32 m_centerX;
    i32 m_centerY;
};
SIZE(0x5c);

class CFaderShape : public CFader {
public:
    CFaderShape();
    virtual ~CFaderShape() OVERRIDE;
    virtual void RenderFrame(i32 f) OVERRIDE;
    virtual i32 GetFrameCount() OVERRIDE;

    i32 ApplyInit(CFxModeDesc* src);

    void RenderTile(i32 baseRow, i32 leadWidth);
    void RenderWarpTile(i32 baseRow, i32 leadWidth);

    CDDSurface* m_surfA;
    CDDSurface* m_surfB;
    CDDSurface* m_surfC;
    i32* m_rowOfsA;
    i32* m_rowOfsB;
    i32* m_rowOfsC;
    GZ_ENUM_STORAGE(FaderMode, u32) m_mode;
    i32 m_stripCopy;
    i32 m_halfWidth;
    i32 m_useLut;
    i32 m_span;
    i32 m_rowCount;
    i32 m_spanB;
    i32 m_rowCountB;
    i32 m_spanC;
    i32 m_rowCountC;
    char _pad78[0x478 - 0x78];
    i32* m_warpTable;
    u8* m_dstBase;
    u8* m_straightBase;
    u8* m_gatherBase;
    u8* m_lineBuf;
    u8* m_shadeRamp;

    i32 m_reserved490;
};
SIZE(0x494);

#endif // GRUNTZ_GRUNTZ_CFADERSUBTYPES_H

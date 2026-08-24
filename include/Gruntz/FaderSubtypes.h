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

// m_pixel is a BYTE, not an i32: CFaderRadial::RenderFrame reads it with
// `mov al,BYTE PTR [ebx+0xc]` (0x17fd2a) and ApplyInit writes it with a byte
// store (0x17fbc9).  The three trailing pad bytes are why ApplyInit's cell
// write is a 4-dword struct copy - it carries the padding too.
struct CFaderRadialCell {
    float m_vx;
    float m_vy;
    float m_radius;
    u8 m_pixel;
};

class CFaderMesh : public CFader {
public:
    virtual ~CFaderMesh() OVERRIDE;
    CFaderMesh();
    virtual void RenderFrame(i32 frame) OVERRIDE;
    virtual i32 GetFrameCount() OVERRIDE;

    i32 ApplyInit(CFxModeDesc* src);

    CDDSurface* m_sourceSurface;
    CDDSurface* m_dstSurface;
    CDDSurface* m_primeSurface;
    CDDSurface* m_flipTarget;
    i32 m_unusedOption;
    i32 m_reverseOrder;
    i32 m_cols;
    i32 m_rows;
    CRezBufferObject m_meshBuf;
};

class CFaderSine : public CFader {
public:
    // The fader module's own revision of Monolith's GetRandomNumber. In-class so
    // the static's mangled name differs from the game header's - three distinct
    // names are the only mechanism for retail's three guard/seed pairs (same-name
    // COMMONs fold, `static __inline` layout disproven):
    // docs/patterns/header-inline-local-static-three-copies.md. This module's
    // GetRandom below builds on it where the game's builds on rand() - the
    // diverged-revision proof.
    i32 GetRandomNumber() {
        static long holdrand = timeGetTime();
        return (((holdrand = holdrand * 214013L + 2531011L) >> 16) & 0x7fff);
    }

    // Monolith's inclusive-range companion. Retail's three call sites all show the
    // `dec` of the loaded bound before the seed guard and the matching `inc` before
    // `idiv` - i.e. `hi - lo + 1` with a folded-away `lo == 0`.
    i32 GetRandom(i32 lo, i32 hi) {
        return lo + GetRandomNumber() % (hi - lo + 1);
    }
    CFaderSine();
    virtual ~CFaderSine() OVERRIDE;
    virtual void RenderFrame(i32 frame) OVERRIDE;
    virtual i32 GetFrameCount() OVERRIDE;

    i32 ApplyInit(CFxModeDesc* src);

    CDDSurface* m_targetSurface;
    CDDSurface* m_restoreSurface;
    i32 m_clearToBlack;

    u8* m_targetBits;
    u8* m_restoreBits;
    i32 m_height;
    i32 m_width;
    i32 m_fadeRowCount;
    i32 m_intensityPercent;
    i32 m_appliedCounts[2000];
    i32 m_sampleCursors[2000];

    float m_fractionalCounts[2000];
    i32 m_sampleOrder[2000];
};

class CFaderFlat : public CFader {
public:
    virtual ~CFaderFlat() OVERRIDE;
    CFaderFlat();
    virtual void RenderFrame(i32 frame) OVERRIDE;
    virtual i32 GetFrameCount() OVERRIDE;

    i32 ApplyInit(CFxModeDesc* src);
    void PrepareFrame();
    void FinishFrame();

    CDDSurface* m_dstSurface;
    CDDSurface* m_srcSurface;
    i32 m_unusedOption;
    i32 m_durationPercent;
    i32 m_splitPercent;
    i32* m_rowStates;
};

class CFaderLight : public CFader {
public:
    virtual void BeginFade() OVERRIDE;
    virtual void EndFade() OVERRIDE;
    virtual ~CFaderLight() OVERRIDE;
    CFaderLight();
    virtual void RenderFrame(i32 frame) OVERRIDE;
    virtual i32 GetFrameCount() OVERRIDE;

    i32 ApplyInit(CFxModeDesc* src);
    void ReleaseBuffers();

    void Render(i32 row0, i32 radiusSq, i32 radius, u8* lut, u8* srcBits, u8* dstBits);

    CDDSurface* m_targetSurface;
    CDDSurface* m_restoreSurface;

    CDDSurface* m_overlay;
    CDDPalette* m_palette;
    i32 m_clearMode;
    i32 m_centerX;
    i32 m_centerY;

    u8* m_targetBits;
    u8* m_restoreBits;
    i32 m_frameCount;
    i32 m_spanStarts[1024];
    i32 m_spanEnds[1024];
    i32 m_spanCount;
    i32 m_width;
    i32 m_height;
};

class CFaderRadial : public CFader {
public:
    virtual ~CFaderRadial() OVERRIDE;
    CFaderRadial();
    virtual void RenderFrame(i32 frame) OVERRIDE;
    virtual i32 GetFrameCount() OVERRIDE;

    i32 ApplyInit(CFxModeDesc* src);
    void FreeBuffer();

    CDDSurface* m_srcSurface;
    CDDSurface* m_dstSurface;
    i32 m_unusedZero;
    i32 m_maxRadius;
    i32 m_unusedOne;
    float m_fadeDivisor;

    CFaderRadialCell* m_cells;
    i32 m_centerX;
    i32 m_centerY;
};

class CFaderShape : public CFader {
public:
    CFaderShape();
    virtual ~CFaderShape() OVERRIDE;
    virtual void RenderFrame(i32 frame) OVERRIDE;
    virtual i32 GetFrameCount() OVERRIDE;

    i32 ApplyInit(CFxModeDesc* src);

    void RenderTile(i32 baseRow, i32 leadWidth);
    void RenderWarpTile(i32 baseRow, i32 leadWidth);

    CDDSurface* m_targetSurface;
    CDDSurface* m_sourceSurface;
    CDDSurface* m_warpSourceSurface;
    i32* m_targetRowOffsets;
    i32* m_sourceRowOffsets;
    i32* m_warpRowOffsets;
    GZ_ENUM_STORAGE(FaderMode, u32) m_mode;
    i32 m_stripCopy;
    i32 m_halfWidth;
    i32 m_useLut;
    i32 m_targetWidth;
    i32 m_targetHeight;
    i32 m_sourceWidth;
    i32 m_sourceHeight;
    i32 m_warpWidth;
    i32 m_warpHeight;
    char _pad78[0x478 - 0x78];
    i32* m_warpTable;
    u8* m_dstBase;
    u8* m_straightBase;
    u8* m_gatherBase;
    u8* m_lineBuf;
    u8* m_shadeRamp;

    i32 m_unusedTail;
};

#endif // GRUNTZ_GRUNTZ_CFADERSUBTYPES_H

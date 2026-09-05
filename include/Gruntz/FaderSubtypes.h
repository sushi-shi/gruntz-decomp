#ifndef GRUNTZ_GRUNTZ_CFADERSUBTYPES_H
#define GRUNTZ_GRUNTZ_CFADERSUBTYPES_H

#include <rva.h>

#include <Mfc.h>

#include <DDrawMgr/DDSurface.h>
#include <Enums.h>
#include <Gruntz/Fader.h>
#include <Gruntz/FaderConfig.h>
#include <Ints.h>
#include <Rez/RezBufferObject.h>

GZ_ENUM_FORWARD(FaderMode);

class CDDSurface;
struct CDDPalette;

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

    i32 ApplyInit(CFaderConfig* src);

    CDDSurface* m_sourceSurface;
    CDDSurface* m_dstSurface;
    CDDSurface* m_primeSurface;
    CDDSurface* m_flipTarget;
    i32 m_unusedOption;
    b32 m_reverseOrder;
    i32 m_cols;
    i32 m_rows;
    CRezBufferObject m_meshBuf;
};

class CFaderSine : public CFader {
public:
    i32 AccumulateSampleCount(i32 row, i32 delta, float step);
    i32 AdvanceSampleCursor(i32 row);

    CFaderSine();
    virtual ~CFaderSine() OVERRIDE;
    virtual void RenderFrame(i32 frame) OVERRIDE;
    virtual i32 GetFrameCount() OVERRIDE;

    i32 ApplyInit(CFaderConfig* src);

    CDDSurface* m_targetSurface;
    CDDSurface* m_restoreSurface;
    b32 m_clearToBlack;

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

    i32 ApplyInit(CFaderConfig* src);
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

    i32 ApplyInit(CFaderConfig* src);
    void ReleaseBuffers();

    inline void ComputeSpan(i32 row, i32 radiusSq, i32 edgeOffset, i32& right, i32& left);
    inline void Render(i32 row0, i32 radiusSq, i32 radius, u8* lut, u8* srcBits, u8* dstBits);

    CDDSurface* m_targetSurface;
    CDDSurface* m_restoreSurface;

    CDDSurface* m_overlay;
    CDDPalette* m_palette;
    b32 m_clearMode;
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

    i32 ApplyInit(CFaderConfig* src);
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

    i32 ApplyInit(CFaderConfig* src);

    void RenderTile(i32 baseRow, i32 leadWidth);
    void RenderWarpTile(i32 baseRow, i32 leadWidth);

    CDDSurface* m_targetSurface;
    CDDSurface* m_sourceSurface;
    CDDSurface* m_warpSourceSurface;
    i32* m_targetRowOffsets;
    i32* m_sourceRowOffsets;
    i32* m_warpRowOffsets;
    GZ_ENUM_STORAGE(FaderMode, u32) m_mode;
    b32 m_stripCopy;
    i32 m_halfWidth;
    b32 m_useLut;
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

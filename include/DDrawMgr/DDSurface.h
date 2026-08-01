#ifndef DDRAWMGR_CDDSURFACE_H
#define DDRAWMGR_CDDSURFACE_H

#include <Mfc.h>
#include <Ints.h>
#include <ddraw.h>
#include <rva.h>

struct CDDPalette;
class CDDrawPtrCollections;
struct PcxHeader;

union BltFxWords {
    DDBLTFX m_fx;
    i32 m_words[0x19];
};

struct ClipRect16 {
    i32 a, b, c, d;
};
SIZE(0x10);
SIZE(0x10);

enum PidFlags {

    PID_TRANSPARENCY = 0x01,

    PID_VIDEO_MEMORY = 0x02,

    PID_SYSTEM_MEMORY = 0x04,

    PID_GRAMMAR_SKIPRUN = 0x20,

    PID_SRC_8BPP_SHADE = 0x40,

    PID_EMBEDDED_PALETTE = 0x80,

    PID_FILL_IS_WORD = 0x100,

    PID_SRC_8BPP = 0x200,
};

struct PidHeader {

    u32 formatTag;
    u32 flags;
    i32 width;
    i32 height;
    i32 offsetX;
    i32 offsetY;
    u32 fill;
    u32 unk1;

    u8 pixels[1];
};
SIZE(0x20);

enum FileImageFormat {
    FMT_BMP = 1,
    FMT_PCX = 2,
    FMT_PID = 4,
};

typedef enum DDSurfacePoolKind {
    POOLKIND_PLAIN = 0,
    POOLKIND_MODE = 1,
    POOLKIND_FILEIMAGE = 2,
    POOLKIND_BLIT7 = 3,
    POOLKIND_BLIT47 = 4,
} DDSurfacePoolKind;

class CDDSurface {
public:
    CDDSurface();

    virtual ~CDDSurface();
    virtual i32 Refresh(IDirectDrawSurface* surf);

    virtual i32 CreateFromDesc(CDDrawPtrCollections* h, const DDSURFACEDESC* desc);
    virtual i32 BlitSurf(void* surf, i32 width, i32 height, i32 bitDepth, i32 caps);
    virtual void FreeSurfaces();
    virtual i32 IsValid();

    virtual i32 GetPoolKind();
    virtual i32 RestoreLost();
    virtual i32 BlitIntoDesc(void* a);

    void* Lock(void* rect);
    i32 SetPalette(CDDPalette* pal, i32 unused);
    i32 Restore(void* info, i32 mode);
    i32 Flip(CDDSurface* target);

    void ReloadImageCache();
    void* GetElementAt(i32 i);
    i32 SetColorKey(u32 flags, void* key);

    i32 SetColorKeyVal(u32 flags, u32 key);
    i32 SetColorKeyRange(u32 flags, u32 lo, u32 hi);
    i32 SetDestColorKey(u32 key);

    void WaitFlip();
    i32 Blt(CDDSurface* src);
    i32 BltEx(void* dstRect, CDDSurface* src, void* srcRect, u32 flags, void* fx);
    i32 BltFast(u32 x, u32 y, CDDSurface* src, void* srcRect, u32 trans);
    void Tile(CDDSurface* src, i32 useColorKey);
    void DumpSurfaceInfo(i32 detailed);
    i32 ShadeBlt(struct tagRECT* dstRect, CDDSurface* src, struct tagRECT* srcRect, i32 shade);
    i32 GetColorKey();

    i32 Fill(u32 color);
    i32 GetWidth();
    i32 GetHeight();
    i32 Scale(i32 n);
    void UnlockThunk();

    i32 SaveFile(char* buf, i32 type, void* pal, i32 flag);
    i32 SaveDispatch(char* path, void* pal, i32 flag);
    void Clear(i32 white);

    i32 SaveBmp(const char* path, void* pal, i32 mode);
    i32 SaveRle16(void* file, void* pal, i32 flag);
    i32 SaveTga(const char* path, void* pal, i32 mode);

    i32 Resolve(class CDDrawPtrCollections* pal, void* buf, i32 type, u32 size, u32 colorKey);

    i32 DecodeBmp(class CDDrawPtrCollections* pal, void* buf, u32 size);
    i32 DecodePcx(class CDDrawPtrCollections* pal, struct PcxHeader* hdr, u32 size);
    i32 DecodePid(class CDDrawPtrCollections* pal, PidHeader* hdr, u32 size, u32 colorKey);
    i32 DecodePcxData(class CDDrawPtrCollections* dst, PidHeader* hdr, i32 size, i32 caps, u32 key);

    i32 LoadBmp(class CDDrawPtrCollections* pal, char* path);
    i32 LoadPcx(class CDDrawPtrCollections* pal, char* path);
    i32 LoadPid(class CDDrawPtrCollections* pal, char* path, u32 colorKey);

    i32 MakeImageKey(class CDDrawPtrCollections* pal, char* name, u32 colorKey);
    i32 DecodePcxEx(class CDDrawPtrCollections* pal, char* path, i32 caps, u32 key);

    i32 DecodeRun(CDDrawPtrCollections* info, void* src, i32 a, i32 b);
    i32 Decode(CDDrawPtrCollections* info, PcxHeader* src, i32 len, i32 mode);

    void FlipVertical();

    i32 RotateBlit(
        CDDSurface* src,
        i32* pivot,
        i32 destX,
        i32 destY,
        float scale,
        i32 mode,
        i32 colorkey
    );
    i32 ScaleBlit(
        CDDSurface* src,
        i32* pivot,
        i32 destX,
        i32 destY,
        float angle,
        i32 mode,
        i32 colorkey
    );
    i32 RotateScaleBlit(
        CDDSurface* src,
        i32* pivot,
        i32 destX,
        i32 destY,
        float angle,
        float scale,
        i32 mode,
        i32 colorkey
    );

    i32 StretchBlit(CDDSurface* src, RECT* srcRect, RECT* dstRect, i32 mode, i32 colorkey);

    void DecodeThunk(i32 x0, i32 y0, i32 x1, i32 y1, i32 halfWidth, i16 color, RECT clip);
    i32 LoadFile2(CDDrawPtrCollections* info, const char* path, i32 mode);
    i32 LoadFile(CDDrawPtrCollections* info, const char* path, i32 mode);
    i32 Load(CDDrawPtrCollections* a, char* name, i32 c);

    i32 Blit(void* src, i32 bitcount, void* palette, i32 mode);
    i32 BlitDirect(void* src, i32 mode);
    i32 DecodeRun8(void* dst);
    i32 DecodeRun24(void* dst);
    i32 RunDecode1(void* dst, void* src, i32 width, i32 height);
    i32 RunDecode3(void* dst, void* src, i32 width, i32 height);
    void FillPalette(u32 key);
    i32 ShadeRect(i32 pct, RECT* clip);

    i32 Blit248(void* src, void* palette, i32 mode);
    i32 Blit2416(void* src, i32 mode);
    i32 Blit1624(void* src, i32 mode);
    i32 Blit168(void* src, void* palette, i32 mode);
    i32 Blit824(void* src, void* palette, i32 mode);
    i32 Blit816(void* src, void* palette, i32 mode);

    POSITION m_pos;
    IDirectDrawSurface* m_ddSurface;
    IDirectDrawSurface* m_ddSurfaceBack;

    union {
        DDSURFACEDESC m_apiDesc;
        i32 m_descWords[0x6c / 4];
        struct {
            i32 m_descSize;
            i32 m_descFlags;
            i32 m_height;
            i32 m_width;
            i32 m_pitch;
            i32 m_backBufferCount;
            i32 m_mipMapCount;
            i32 m_alphaBitDepth;
            i32 m_descReserved;
            u8* m_lockBits;
            char m_colorKeys[0x20];
            i32 m_pixelFormatSize;
            i32 m_pixelFormatFlags;
            i32 m_pixelFormatFourCC;
            i32 m_srcBitDepth;
            i32 m_rMask;
            i32 m_gMask;
            i32 m_bMask;
            i32 m_alphaMask;
            i32 m_surfaceCaps;
        };
    };
    i32 m_dontOwn;

    RECT m_fullRect;
    i32 m_imageBytes;
    CPtrArray m_elements;

    i32 m_bitDepth;
    i32 m_bytesPerRow;
    i32 m_bytesPerPixel;
    i32 m_pixelsPerRow;

    i32(__cdecl* m_b8)(CDDSurface*);
    i32 m_hasColorKey;
};
SIZE(0xc0);
SIZE(0xc0);
SIZE_UNKNOWN();

inline CDDSurface::CDDSurface() {
    m_ddSurface = 0;
    m_ddSurfaceBack = 0;
    m_pos = 0;
    m_dontOwn = 0;
    m_bitDepth = 0;
    m_b8 = 0;
}

extern "C" const GUID IID_IDirectDrawSurface3;

extern u8 g_clut[];
extern u16 g_lut16[256];

void* operator new(u32);

extern CPtrArray g_imageCache;

HRESULT __stdcall EnumSurfacesCallback(IDirectDrawSurface* surf, DDSURFACEDESC* desc, void* ctx);

#endif // DDRAWMGR_CDDSURFACE_H

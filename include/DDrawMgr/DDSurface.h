#ifndef DDRAWMGR_CDDSURFACE_H
#define DDRAWMGR_CDDSURFACE_H

#include <rva.h>

#include <Mfc.h>

#include <DDrawMgr/ColorDepth.h>
#include <DDrawMgr/DDSurface.h>
#include <DDrawMgr/RasterRowOrder.h>
#include <Enums.h>
#include <Ints.h>

#include <ddraw.h>

struct CDDPalette;
class CDDrawDeviceManager;
class CFileImagePal;
struct PcxHeader;
struct BmpFileImage;

union BltFxWords {
    DDBLTFX m_fx;
    i32 m_words[0x19];
};

struct ClipRect16 {
    i32 a, b, c, d;
};

GZ_ENUM_FLAGS_BEGIN(PidFlags, u32)

    PID_TRANSPARENCY = 0x01,

    PID_VIDEO_MEMORY = 0x02,

    PID_SYSTEM_MEMORY = 0x04,

    PID_GRAMMAR_SKIPRUN = 0x20,

    PID_SRC_8BPP_SHADE = 0x40,

    PID_EMBEDDED_PALETTE = 0x80,

    PID_FILL_IS_WORD = 0x100,

    PID_SRC_8BPP = 0x200
GZ_ENUM_FLAGS_END(PidFlags, u32)
GZ_ENUM_FLAGS_OPS(PidFlags)

struct PidHeader {

    u32 formatTag;
    PidFlags flags;
    i32 width;
    i32 height;
    i32 offsetX;
    i32 offsetY;
    u32 fill;
    u32 reserved1c;

    u8 pixels[1];
};

// The format CDDSurface::Resolve decodes. CImage picks it from the REZ entry
// tag: IMGTAG_PMB('BMP')->FMT_BMP, IMGTAG_XCP('PCX')->FMT_PCX,
// IMGTAG_DIR('RID')->FMT_RID, IMGTAG_DIP('PID')->FMT_PID.
GZ_ENUM_BEGIN(FileImageFormat)
    FMT_BMP = 1,
    FMT_PCX = 2,
    FMT_RID = 3,
    FMT_PID = 4
GZ_ENUM_END(FileImageFormat)

GZ_ENUM_BEGIN(DDSurfacePoolKind)
    POOLKIND_PLAIN = 0,
    POOLKIND_MODE = 1,
    POOLKIND_FILEIMAGE = 2,
    POOLKIND_OVERLAY = 3,
    POOLKIND_ZBUFFER = 4
GZ_ENUM_END(DDSurfacePoolKind)

class CDDSurface {
public:
    CDDSurface();

    virtual ~CDDSurface();
    virtual i32 Refresh(IDirectDrawSurface* surface);

    virtual i32 CreateFromDesc(CDDrawDeviceManager* manager, const DDSURFACEDESC* desc);
    virtual i32
    BlitSurf(CDDrawDeviceManager* manager, i32 width, i32 height, ColorDepth bitDepth, i32 caps);
    virtual void FreeSurfaces();
    virtual i32 IsValid();

    virtual DDSurfacePoolKind GetPoolKind();
    virtual i32 RestoreLost();
    virtual i32 BlitIntoDesc(CDDrawDeviceManager* manager);

    void* Lock(RECT* rect);
    u8 GetPixel(i32 x, i32 y);
    void PutPixel(i32 x, i32 y, u8 color);
    i32 SetPalette(CDDPalette* palette, i32 unused);
    i32 Restore(RECT* dstRect, i32 fillColor);
    i32 Flip(CDDSurface* target);

    void ReloadImageCache();
    CDDSurface* GetElementAt(i32 i);
    i32 SetColorKey(u32 flags, DDCOLORKEY* key);

    i32 SetColorKeyVal(u32 flags, u32 key);
    i32 SetColorKeyRange(u32 flags, u32 lo, u32 hi);
    i32 SetDestColorKey(u32 key);

    void WaitFlip();
    i32 Blt(CDDSurface* src);
    i32 BltEx(RECT* dstRect, CDDSurface* src, RECT* srcRect, u32 flags, DDBLTFX* fx);
    i32 BltFast(u32 x, u32 y, CDDSurface* src, RECT* srcRect, u32 trans);
    void Tile(CDDSurface* src, i32 useColorKey);
    void DumpSurfaceInfo(i32 detailed);
    i32 ShadeBlt(struct tagRECT* dstRect, CDDSurface* src, struct tagRECT* srcRect, i32 shade);
    i32 GetColorKey();

    i32 Fill(u32 color);
    i32 GetWidth();
    i32 GetHeight();
    i32 Scale(i32 n);
    void UnlockThunk();

    i32 SaveFile(char* buf, FileImageFormat type, CFileImagePal* pal, i32 flag);
    i32 SaveDispatch(char* path, CFileImagePal* pal, i32 flag);
    void Clear(i32 white);

    i32 SaveBmp(const char* path, CFileImagePal* pal, i32 mode);
    i32 SaveRle16(char* path, CFileImagePal* pal, i32 flag);
    i32 SaveTga(const char* path, CFileImagePal* pal, i32 mode);

    i32 Resolve(
        class CDDrawDeviceManager* manager,
        void* data,
        FileImageFormat format,
        u32 dataSize,
        u32 colorKey
    );

    i32 DecodeBmp(class CDDrawDeviceManager* manager, BmpFileImage* image, u32 dataSize);
    i32 DecodePcx(class CDDrawDeviceManager* manager, struct PcxHeader* image, u32 dataSize);
    i32 DecodePid(class CDDrawDeviceManager* manager, PidHeader* image, u32 dataSize, u32 colorKey);
    i32 DecodePcxData(
        class CDDrawDeviceManager* manager,
        PidHeader* image,
        i32 dataSize,
        i32 surfaceCaps,
        u32 colorKey
    );

    i32 LoadBmp(class CDDrawDeviceManager* manager, char* path);
    i32 LoadPcx(class CDDrawDeviceManager* manager, char* path);
    i32 LoadPid(class CDDrawDeviceManager* manager, char* path, u32 colorKey);

    i32 MakeImageKey(class CDDrawDeviceManager* manager, char* path, u32 colorKey);
    i32 DecodePcxEx(class CDDrawDeviceManager* manager, char* path, i32 surfaceCaps, u32 colorKey);

    i32 DecodeRun(CDDrawDeviceManager* manager, BmpFileImage* image, i32 dataSize, i32 surfaceCaps);
    i32 Decode(CDDrawDeviceManager* manager, PcxHeader* image, i32 dataSize, i32 surfaceCaps);

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
    i32 LoadFile2(CDDrawDeviceManager* manager, const char* path, i32 surfaceCaps);
    i32 LoadFile(CDDrawDeviceManager* manager, const char* path, i32 surfaceCaps);
    i32 Load(CDDrawDeviceManager* manager, char* resourceName, i32 surfaceCaps);

    i32 Blit(u8* src, ColorDepth bitcount, PALETTEENTRY* palette, RasterRowOrder rowOrder);
    i32 BlitDirect(u8* src, RasterRowOrder rowOrder);
    i32 DecodeRun8(u8* src);
    i32 DecodeRun24(u8* src);
    i32 RunDecode1(u8* dst, u8* src, i32 width, i32 height);
    i32 RunDecode3(u8* dst, u8* src, i32 width, i32 height);
    void FillPalette(u32 key);
    i32 ShadeRect(i32 pct, RECT* clip);

    i32 Blit248(u8* src, PALETTEENTRY* palette, RasterRowOrder rowOrder);
    i32 Blit2416(u8* src, RasterRowOrder rowOrder);
    i32 Blit1624(u8* src, RasterRowOrder rowOrder);
    i32 Blit168(u8* src, PALETTEENTRY* palette, RasterRowOrder rowOrder);
    i32 Blit824(u8* src, PALETTEENTRY* palette, RasterRowOrder rowOrder);
    i32 Blit816(u8* src, PALETTEENTRY* palette, RasterRowOrder rowOrder);

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
            ColorDepth m_srcBitDepth;
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

    ColorDepth m_bitDepth;
    i32 m_bytesPerRow;
    i32 m_bytesPerPixel;
    i32 m_pixelsPerRow;

    i32(__cdecl* m_restoreCallback)(CDDSurface*);
    i32 m_hasColorKey;
};

inline u8 CDDSurface::GetPixel(i32 x, i32 y) {
    u8* bits = static_cast<u8*>(Lock(0));
    if (bits != NULL) {
        u8 color = bits[m_bytesPerPixel * x + m_pitch * y];
        m_ddSurface->Unlock(NULL);
        return color;
    }
    return 0;
}

inline void CDDSurface::PutPixel(i32 x, i32 y, u8 color) {
    u8* bits = static_cast<u8*>(Lock(0));
    if (bits != NULL) {
        bits[m_bytesPerPixel * x + m_pitch * y] = color;
        m_ddSurface->Unlock(NULL);
    }
}

inline CDDSurface::CDDSurface() {
    m_ddSurface = NULL;
    m_ddSurfaceBack = NULL;
    m_pos = NULL;
    m_dontOwn = 0;
    m_bitDepth = BPP_UNSET;
    m_restoreCallback = NULL;
}

inline CDDSurface::~CDDSurface() {
    FreeSurfaces();
}

extern u16 g_lut16[256];

extern CPtrArray g_imageCache;

HRESULT __stdcall EnumSurfacesCallback(IDirectDrawSurface* surf, DDSURFACEDESC* desc, void* ctx);

#endif // DDRAWMGR_CDDSURFACE_H

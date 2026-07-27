#ifndef DDRAWMGR_CDDSURFACE_H
#define DDRAWMGR_CDDSURFACE_H

#include <Mfc.h> // real MFC CPtrArray (the +0x94 element array, value member) + POSITION
#include <Ints.h>
#include <ddraw.h> // DDSURFACEDESC + IDirectDrawSurface (the embedded descriptor/COM boundary)
#include <rva.h>

struct CDDPalette;          // fwd (SetPalette takes a wrapper ptr; PAUCDDPalette => struct)
class CDDrawPtrCollections; // fwd (the display/pool manager passed as the palette/init context)
class CFileImageSrc;        // fwd (Decode's run-length source header; full def in <Image/Image.h>)

struct ClipRect16 {
    i32 a, b, c, d;
};
SIZE(0x10); // 16-byte by-value rect/clip record
SIZE(0x10); // 16-byte by-value rect/clip record

enum PidFlags {
    PID_TRANSPARENCY = 0x01,     // bit0  install the transparent colour key
    PID_VIDEO_MEMORY = 0x02,     // bit1  "VID"
    PID_SYSTEM_MEMORY = 0x04,    // bit2  "SYS"
    PID_COMPRESSION = 0x20,      // bit5  "RLE" - skip/fill RLE pixel stream
    PID_EMBEDDED_PALETTE = 0x80, // bit7  trailing 768-byte VGA palette at EOF
};

struct PidHeader {
    u32 fileDesc; // +0x00  id / file descriptor
    u32 flags;    // +0x04  PidFlags
    i32 width;    // +0x08
    i32 height;   // +0x0c
    i32 offsetX;  // +0x10  draw anchor X
    i32 offsetY;  // +0x14  draw anchor Y
    u32 fill;     // +0x18  fill colour (masked to low word when flags & 0x100)
    u32 unk1;     // +0x1c
    // +0x20: the RLE/uncompressed 8bpp pixel stream begins here.
};
SIZE(0x20);

enum FileImageFormat {
    FMT_BMP = 1,
    FMT_PCX = 2,
    FMT_PID = 4,
};

typedef enum DDSurfacePoolKind {
    POOLKIND_PLAIN = 0,     // CDDSurface base default        (0x141300: xor eax,eax)
    POOLKIND_MODE = 1,      // CPoolItemAB8 device/mode page  (0x143cd0)
    POOLKIND_FILEIMAGE = 2, // CFileImageSurface              (0x143cc0)
    POOLKIND_BLIT7 = 3,     // CPoolItemA88 "Blit7" pocket    (0x143cb0)
    POOLKIND_BLIT47 = 4,    // CPoolItemAE8 "Blit47" pocket   (0x143ce0)
} DDSurfacePoolKind;

class CDDSurface {
public:
    // The pool factories (CDDrawPtrCollections::Create*) construct this via `new`; the
    // ctor zeroes the scalar fields (the CPtrArray member + vptr are the compiler's job).
    CDDSurface();

    // --- the 9-slot shared surface vtable (VA 0x5ef7f0) -----------------------
    // Slots 3/4 (BlitSurf / FreeSurfaces) carry real bodies in Image.cpp; the others are
    // declared-only (their bodies live in sibling TUs: Refresh 0x13e140 in DirectDrawMgr,
    // the slot-2 init 0x13e0a0 in BoundaryUpper, etc.) so the emitted vtable's DIR32 slot
    // relocs mask. Declaring these real virtuals makes the slot-5 IsValid / slot-3
    // BlitSurf / slot-8 BlitIntoDesc dispatch sites genuine virtual calls on `this`.
    virtual ~CDDSurface(); // slot 0  0x141350 (??_G 0x141330; implicit vptr stamp lands stamp-first)
    virtual i32
    Refresh(IDirectDrawSurface* surf); // slot 1  0x13e140  (GetSurfaceDesc-driven re-cache)
    virtual i32 Init1(CDDrawPtrCollections* h, i32 a); // slot 2  0x13e0a0
    virtual i32 BlitSurf(
        void* surf,
        i32 width,
        i32 height,
        i32 a4,
        i32 a5
    ); // slot 3  0x13e0d0 (DecodePcxData dest setup / "BeginDecode")
    virtual void
    FreeSurfaces();        // slot 4  0x13e4d0  (releases m_8/m_c, empties + destroys m_elements)
    virtual i32 IsValid(); // slot 5  0x1412d0  (surface present + positive w/h)
    // slot 6 - the per-class POOL-KIND tag: every body is `mov eax,<kind>; ret`
    // (base 0x141300 -> 0; the pool subclasses return 1..4 - see DDSurfacePoolKind).
    virtual i32 GetPoolKind();         // slot 6  0x141300 (POOLKIND_PLAIN)
    virtual i32 RestoreLost();         // slot 7  0x13f960  (restore-this-lost-surface retry)
    virtual i32 BlitIntoDesc(void* a); // slot 8  0x13e2e0  (the surface's own blit-into-desc)

    // --- non-virtual __thiscall DirectDraw thunks (DIRSURF.CPP) ----------------
    // The held-surface COM ops; each dispatches m_8/m_c (IDirectDrawSurface) and retries
    // on SURFACELOST via RestoreLost (slot 7). Bodies in DirectDrawMgr.cpp.
    // (`Init0` used to be declared here at 0x53edb0 = RVA 0x13edb0 - a SECOND, never-
    // defined declaration of `Clear` below. Its one caller (CLightFxRender::AllocSurface)
    // now calls Clear(0) directly; the fabricated symbol is gone.)
    void* Lock(void* rect);                      // 0x13e6d0  the locked bits (m_lockBits)
    i32 SetPalette(CDDPalette* pal, i32 unused); // 0x13e690
    i32 Restore(void* arg1, i32 arg2);           // 0x13e7d0 (BoundaryUpper2.cpp)
    i32 Flip(CDDSurface* target);                // 0x13e850
    void* GetElementAt(i32 i);                   // 0x13ea70  m_elements[i] (bounds-checked)
    i32 SetColorKey(u32 flags, void* key);       // 0x13eaa0
    // Convenience SetColorKey overloads that build a DDCOLORKEY on the stack + forward.
    i32 SetColorKeyVal(u32 flags, u32 key);          // 0x13eae0  key={v,v}
    i32 SetColorKeyRange(u32 flags, u32 lo, u32 hi); // 0x13eb10  key={lo,hi}
    i32 SetDestColorKey(u32 key);                    // 0x13eb80  SetColorKey(DDCKEY_DESTBLT,{v,v})
    // 0x13ee30 - spin until the held surface's pending flip retires
    // (IDirectDrawSurface::GetFlipStatus(DDGFS_ISFLIPDONE) != DDERR_WASSTILLDRAWING).
    void WaitFlip();                                                               // 0x13ee30
    i32 Blt(CDDSurface* src);                                                      // 0x13ee60
    i32 BltEx(void* dstRect, CDDSurface* src, void* srcRect, u32 flags, void* fx); // 0x13eef0
    i32 BltFast(u32 x, u32 y, CDDSurface* src, void* srcRect, u32 trans);          // 0x13ef90
    void Tile(CDDSurface* src, i32 useColorKey); // 0x13f990 (tile src across this via BltFast)
    void DumpSurfaceInfo(i32 detailed); // 0x140770 (GetSurfaceDesc + TRACE the geometry/caps)
    i32 ShadeBlt(
        struct tagRECT* dstRect,
        CDDSurface* src,
        struct tagRECT* srcRect,
        i32 shade
    );                 // 0x13f020 (16bpp shade-LUT blend blit)
    i32 GetColorKey(); // 0x13fa60

    // The colour-fill / geometry accessors (DIRSURF.CPP; some external no-body/reloc-
    // masked, some carry real bodies in Image.cpp).
    i32 Fill(u32 color); // 0x13e760  colour-fill blt (real body, Image.cpp)
    i32 GetWidth();      // 0x141310  (returns m_width)
    i32 GetHeight();     // 0x141320  (returns m_height)
    i32 Scale(i32 n);    // 0x1413c0  (returns m_pitch * n)
    void UnlockThunk();  // 0x1413b0  (m_8->vtbl[0x80](m_8, 0))

    // --- the surface SAVE/export path (DIRSURF.CPP) ---------------------------
    // SaveFile validates the surface + args, SaveDispatch picks the per-bit-depth writer
    // by m_bitDepth (8/16/24). Clear blanks the surface.
    i32 SaveFile(char* buf, i32 type, void* a3, void* a4); // 0x13f910 (ret 0x10)
    i32 SaveDispatch(char* a1, void* a2, void* a3);        // 0x144350 (ret 0xc)
    void Clear(i32 white);                                 // 0x13edb0 (ret 4)

    // The per-bit-depth file writers SaveDispatch delegates to (ret 0xc = 3 args). SaveBmp
    // (0x1443b0) writes the 8bpp palettized BMP, SaveTga (0x144900) the 24bpp TGA,
    // SaveRle16 (0x144640) the 16bpp->24bpp BMP. SaveBmp/SaveTga are in FileImage.cpp.
    i32 SaveBmp(const char* path, void* pal, i32 mode); // 0x1443b0 (8bpp)
    i32 SaveRle16(void* a1, void* a2, void* a3);        // 0x144640 (16bpp)
    i32 SaveTga(const char* path, void* pal, i32 mode); // 0x144900 (24bpp)

    // --- format dispatchers (Image.cpp). __thiscall on CDDSurface --------------
    // Resolve picks the BMP/PCX/PID decoder by `type` (1/2/4) for the file path.
    // (ResolveEx/LoadByExt/LoadKeyed/UpdateOverlay moved to their REAL owners: xref
    // proves each body is reached ONLY through a DERIVED vtable slot - CFileImageSurface
    // slots 9/10/11, CPoolItemA88 slot 10 - never by a direct call on this base.)
    i32 Resolve(class CDDrawPtrCollections* pal, void* buf, i32 type, u32 size, void* surf2); // 0x13e550 (ret 0x14)

    // Per-format decoders (Image.cpp). __thiscall on CDDSurface. arg1 is the source-palette
    // surface (downcast to CDDSurface* in each body); the class passes surfaces as void*.
    i32 DecodeBmp(class CDDrawPtrCollections* pal, void* buf, u32 size);
    i32 DecodePcx(class CDDrawPtrCollections* pal, struct PcxHeader* hdr, u32 size);
    i32 DecodePid(class CDDrawPtrCollections* pal, PidHeader* hdr, u32 size, void* surf2);
    i32 DecodePcxData(class CDDrawPtrCollections* dst, PidHeader* hdr, i32 size, i32 a4, i32 a5);

    // The file-backed BMP/PCX/PID loaders (Image.cpp): construct a stack CFile, open the
    // file, slurp it into an `operator new` buffer and call the matching decoder (the
    // CFile stack object forces a C++ EH frame -> /GX).
    i32 LoadBmp(class CDDrawPtrCollections* pal, char* path);
    i32 LoadPcx(class CDDrawPtrCollections* pal, char* path);
    i32 LoadPid(class CDDrawPtrCollections* pal, char* path, void* a3);
    // Extension-dispatch resource loader (0x13e5d0): strrchr the ext, _strcmpi
    // .BMP/.PCX/.PID, forward to the matching LoadBmp/LoadPcx/LoadPid on this.
    i32 MakeImageKey(class CDDrawPtrCollections* pal, char* name, void* arg3);
    i32 DecodePcxEx(class CDDrawPtrCollections* pal, char* path, void* a3, void* a4);

    // The surface-blit decoders ResolveEx dispatches to (ret 0x10 = 4 args). DecodeRun ==
    // the former DecodeBmpData @0x143cf0; Decode == DecodePcxData2 @0x144b30. Reconstructed
    // in FileImage.cpp; `info` is the CDDrawPtrCollections display manager (palette context
    // - source bpp / palette / have-palette), NOT a 2nd surface.
    i32 DecodeRun(CDDrawPtrCollections* info, void* src, i32 a, i32 b); // 0x143cf0 (BMP run)
    i32
    Decode(CDDrawPtrCollections* info, CFileImageSrc* src, i32 len, i32 mode); // 0x144b30 (PCX run)

    // The file-load + export path (FileImage.cpp == the DIRSURF.CPP surface). LoadFile2/
    // LoadFile slurp a .BMP/.PCX file into a heap buffer then run DecodeRun/Decode;
    // LoadByExt (FileImageLoadByExt.cpp) picks the loader by file extension; Load (0x144270,
    // ApiCallers TU) is the default RT_BITMAP loader. FlipVertical row-flips the locked
    // surface; DecodeThunk forwards to Run (the inner blit/decode worker, external/reloc-
    // masked, taking a by-value ClipRect16).
    void FlipVertical(); // 0x13ebb0

    // Rotated-blit forwarders onto ImageRotateBlit (0x145f60): thin arg-reorder
    // thunks passing `this` as the destination. RotateBlit fixes rotation=0.0f;
    // ScaleBlit fixes scale=1.0f; RotateScaleBlit passes both. Each param is typed
    // by the ImageRotateBlit (float rot/scale, int mode/colorkey) slot it feeds so
    // the pushes match retail. (Orphan copies.)
    i32 RotateBlit(
        i32 rect,
        i32 pivot,
        i32 a1,
        i32 a2,
        float scale,
        i32 mode,
        i32 colorkey
    ); // 0x141040
    i32 ScaleBlit(
        i32 rect,
        i32 pivot,
        i32 a1,
        i32 a2,
        float angle,
        i32 mode,
        i32 colorkey
    ); // 0x141200
    i32 RotateScaleBlit(
        i32 rect,
        i32 pivot,
        i32 a1,
        i32 a2,
        float angle,
        float scale,
        i32 mode,
        i32 colorkey
    ); // 0x141240

    void DecodeThunk(
        i32 a1,
        i32 a2,
        i32 a3,
        i32 a4,
        i32 a5,
        i32 a6,
        i32 r0,
        i32 r1,
        i32 r2,
        i32 r3
    );                                                                     // 0x141280
    i32 LoadFile2(CDDrawPtrCollections* info, const char* path, i32 mode); // 0x143e60
    i32 LoadFile(CDDrawPtrCollections* info, const char* path, i32 mode);  // 0x144d80
    i32 Load(i32 a, char* name, i32 c);                                    // 0x144270

    // The surface blitters + raw run-decoders the decoders delegate to (external no-body,
    // reloc-masked). Blit does a palette-remap copy (ret 0x10 = 4 args), BlitDirect a
    // straight copy (ret 8 = 2 args); DecodeRun8/DecodeRun24 RLE-expand one plane (ret 4 =
    // 1 arg); RunDecode1/RunDecode3 emit a decoded scanline run (ret 0x10 = 4 args);
    // FillPalette installs the transparency colour (ret 4 = 1 arg).
    i32 Blit(void* src, i32 bitcount, void* palette, i32 mode);  // 0x13faa0
    i32 BlitDirect(void* src, i32 mode);                         // 0x13ece0
    i32 DecodeRun8(void* dst);                                   // 0x140aa0
    i32 DecodeRun24(void* dst);                                  // 0x140c50
    i32 RunDecode1(void* dst, void* src, i32 width, i32 height); // 0x145270
    i32 RunDecode3(void* dst, void* src, i32 width, i32 height); // 0x1453f0
    void FillPalette(u32 key);                                   // 0x13eb40
    i32 ShadeRect(i32 pct, RECT* clip);                          // LutShadeRect.cpp

    // The per-(dest-bpp,src-bpp) blit specializations Blit dispatches to (external no-body,
    // reloc-masked; some real in FileImageBlit.cpp). The trailing digits encode dest_src
    // bit depths: 248 = dest 24bpp / src 8bpp, etc.
    i32 Blit248(void* src, void* palette, i32 mode); // 0x13fe60 (ret 0xc)
    i32 Blit2416(void* src, i32 mode);               // 0x13ff80 (ret 8)
    i32 Blit1624(void* src, i32 mode);               // 0x13fce0 (ret 8)
    i32 Blit168(void* src, void* palette, i32 mode); // 0x13fbb0 (ret 0xc)
    i32 Blit824(void* src, void* palette, i32 mode); // 0x140110 (ret 0xc)
    i32 Blit816(void* src, void* palette, i32 mode); // 0x140420 (ret 0xc)

    // --- layout (0xc0 bytes; the OFFSETS are load-bearing) ---------------------
    // vptr @+0x00 (implicit, polymorphic; the compiler emits the ctor/dtor vptr stamp).
    POSITION m_pos; // +0x04  cached CPtrList POSITION (the pool-A item slot); pad otherwise
    IDirectDrawSurface* m_ddSurface;     // +0x08  held DirectDraw surface (released via Release)
    IDirectDrawSurface* m_ddSurfaceBack; // +0x0c  held back/secondary surface (also released)
    // +0x10..+0x7c: one embedded DDSURFACEDESC scratch (0x6c), with two authentic
    // roles. DirectDraw receives the SDK arm; game arithmetic uses the signed arm
    // because retail performs signed comparisons/divisions on width, height and
    // pitch. A plain DDSURFACEDESC member changed those operations to unsigned.
    union {
        DDSURFACEDESC m_apiDesc; // +0x10  official COM-boundary representation
        i32 m_descWords[0x6c / 4]; // +0x10  signed word view used by retail clear loops
        struct {
            i32 m_descSize;             // +0x10  dwSize
            i32 m_descFlags;            // +0x14  dwFlags
            i32 m_height;               // +0x18  dwHeight
            i32 m_width;                // +0x1c  dwWidth
            i32 m_pitch;                // +0x20  lPitch
            i32 m_backBufferCount;      // +0x24  dwBackBufferCount
            i32 m_mipMapCount;          // +0x28  dwMipMapCount / dwZBufferBitDepth
            i32 m_alphaBitDepth;        // +0x2c  dwAlphaBitDepth
            i32 m_descReserved;         // +0x30  dwReserved
            i32 m_lockBits;             // +0x34  lpSurface (32-bit pointer carrier)
            char m_colorKeys[0x20];      // +0x38  four DDCOLORKEY records
            i32 m_pixelFormatSize;       // +0x58  ddpfPixelFormat.dwSize
            i32 m_pixelFormatFlags;      // +0x5c  ddpfPixelFormat.dwFlags
            i32 m_pixelFormatFourCC;     // +0x60  ddpfPixelFormat.dwFourCC
            i32 m_srcBitDepth;           // +0x64  ddpfPixelFormat.dwRGBBitCount
            i32 m_rMask;                 // +0x68  ddpfPixelFormat.dwRBitMask
            i32 m_gMask;                 // +0x6c  ddpfPixelFormat.dwGBitMask
            i32 m_bMask;                 // +0x70  ddpfPixelFormat.dwBBitMask
            i32 m_alphaMask;             // +0x74  ddpfPixelFormat.dwRGBAlphaBitMask
            i32 m_surfaceCaps;           // +0x78  ddsCaps.dwCaps
        };
    };
    i32 m_dontOwn; // +0x7c  don't-own flag (bit0 => surfaces not released)
    // +0x80  the full-surface blit RECT {left=0, top=0, right=width, bottom=height};
    // Refresh caches it, Blt hands &m_fullRect to IDirectDrawSurface::Blt, IsValid
    // reads right/bottom as the width/height.
    RECT m_fullRect;      // +0x80..+0x8f
    i32 m_imageBytes;     // +0x90  bytes-per-row * height
    CPtrArray m_elements; // +0x94  owned element array (m_pData@+0x98 / m_nSize@+0x9c);
                          //         FreeSurfaces scalar-dtor-deletes each then RemoveAll
    i32 m_bitDepth;       // +0xa8  raw bit depth (8/16/24; the SaveDispatch selector)
    i32 m_bytesPerRow;    // +0xac  bytes-per-row factor
    i32 m_bytesPerPixel;  // +0xb0  pixels-per-unit divisor
    i32 m_pixelsPerRow;   // +0xb4  lPitch/divisor
    // +0xb8  per-surface restore callback (__cdecl fn-ptr taking `this`); RestoreLost
    // (slot 7) tail-dispatches through it. A fn-ptr is 4 bytes = layout-identical to the
    // former i32 slot; cleared by the surface teardown.
    i32(__cdecl* m_b8)(CDDSurface*);
    i32 m_hasColorKey; // +0xbc  cleared
};
SIZE(0xc0);
SIZE(0xc0); // DIRSURF.CPP surface item (both surface ctors 0x13e9a0/0x1421a0
SIZE_UNKNOWN();

inline CDDSurface::CDDSurface() {
    m_ddSurface = 0;
    m_ddSurfaceBack = 0;
    m_pos = 0;
    m_dontOwn = 0;
    m_bitDepth = 0;
    m_b8 = 0;
}

// --- the TU's extern surface (moved out of the .cpp; addresses/thunk
// VAs are reloc-masked at use) ---
extern "C" const GUID IID_IDirectDrawSurface3; // 0x5ef888

extern u8 g_clut[];
extern u16 g_lut16[256];

// File-scope prototypes moved from the .cpp (external linkage
// belongs in the owner header).
void* operator new(u32); // engine allocator (reloc-masked rel32)

#endif // DDRAWMGR_CDDSURFACE_H

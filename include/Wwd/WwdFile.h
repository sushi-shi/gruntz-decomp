#include <Mfc.h> // real MFC CString (the WwdFile members take it by value)
#include <Ints.h>
#include <rva.h>
#include <DDrawMgr/DDrawWorkerHost.h> // CDDrawWorkerHost (+ CDDrawWorkerHost/CDDrawWorkerHost/
#ifndef SRC_WWD_WWDFILE_H
#define SRC_WWD_WWDFILE_H

class CDDrawWorker; // CDDrawWorker IS CDDrawWorker (<DDrawMgr/DDrawWorker.h>);

typedef u8 Bytef;
typedef u32 uLong;
typedef u32 uLongf;

struct WwdHeader {
    u32 wwdSignature; // +0x000  == 1524 (header size)
    u32 field_4;      // +0x004
    u32 flags;        // +0x008  bit0 (0x1) USE_Z_COORDS, bit1 (0x2) COMPRESS
    u8 pad_c[0x10 - 0x0c];
    char levelName[0x2d0 - 0x10]; // +0x010  (name/author/paths block)
    i32 startX;                   // +0x2D0
    i32 startY;                   // +0x2D4
    u32 pad_2d8;                  // +0x2D8
    u32 numPlanes;                // +0x2DC
    u32 planesOffset;             // +0x2E0
    u32 tileDescriptionsOffset;   // +0x2E4
    u32 mainBlockLength;          // +0x2E8  inflated main-block size
    u32 checksum;                 // +0x2EC
    u8 pad_2f0[0x5f4 - 0x2f0];
};
SIZE(0x5f4); // on-disk WWD header (RE'd 0x5F4 bytes)

// The on-disk WWD PLANE record: a fixed 0xa0 header, `numPlanes` of them packed at
// WwdHeader::planesOffset (CGameLevel::LoadWwd walks them with a 0xa0 stride). Every
// field below is proven by CDDrawWorkerHost::Read (0x161640), which is the reader:
// it guards on headerSize == 0xa0 and then copies each field into the plane object.
// The three trailing dwords no reader touches stay padding.
struct WwdPlaneHeader {
    u32 headerSize;         // +0x00  == 0xa0 (the Read guard + the LoadWwd stride)
    u32 field_04;           // +0x04
    u32 flags;              // +0x08  -> CDDrawWorkerHost::m_flags (bit0 origin-fixed,
                            //         bit1 hidden, bit2/3 wrap X/Y, bit4 tile size from
                            //         the first image set)
    u32 field_0c;           // +0x0c
    char name[0x50 - 0x10]; // +0x10  plane name (strcpy'd into CDDrawWorkerHost::m_name)
    i32 pixelWidth;         // +0x50
    i32 pixelHeight;        // +0x54
    i32 tilePixelWidth;     // +0x58  -> m_tilePxW (SetTileSize arg)
    i32 tilePixelHeight;    // +0x5c  -> m_tilePxH
    i32 tilesWide;          // +0x60  -> m_gridW
    i32 tilesHigh;          // +0x64  -> m_gridH
    i32 scrollX;            // +0x68  initial scroll origin -> m_scaledX
    i32 scrollY;            // +0x6c  -> m_scaledY
    i32 movementXPercent;   // +0x70  -> m_94 (m_scaleX = movementXPercent * 0.01f)
    i32 movementYPercent;   // +0x74  -> m_98
    u32 fillColor;          // +0x78  -> the plane DDBLTFX dwFillColor
    u32 imageSetsCount;     // +0x7c  tokens in the imageSetsOffset name list
    i32 objectsCount;       // +0x80  records at objectsOffset
    u32 tilesOffset;        // +0x84  dword tile-handle grid (tilesWide*tilesHigh)
    u32 imageSetsOffset;    // +0x88  the NUL/punctuation-separated image-set name list
    u32 objectsOffset;      // +0x8c  the serialized PlaneObjectRecord stream (0 = none)
    i32 zCoord;             // +0x90  -> m_zBound
    u8 pad_94[0xa0 - 0x94]; // +0x94  three dwords no reader touches
};
SIZE(0xa0); // the LoadWwd plane-loop stride + the Read guard constant

class WwdInputStream {
public:
    WwdInputStream();
    ~WwdInputStream();
    i32 Open(const char* name, i32 mode, void* errSink);
    i32 Read(void* buf, i32 len);

private:
    // +0x00 is the engine vtable pointer of this EXTERNAL binary-file-stream class
    // (its ctor/dtor/Open/Read are all unmatched engine code, reloc-masked). Our
    // code never touches this slot; it is a pure layout placeholder so m_handle
    // lands at +0x04. Modeling it as C++ virtuals would make cl emit a WRONG,
    // incomplete vtable in this TU (its real virtuals are unmodeled) and regress
    // the neighbouring Save -> kept as an opaque leading word.
    // authentic: opaque foreign slots - m_00 is the EXTERNAL stream class's engine
    // vptr (its virtuals are unmatched; declaring them would emit a wrong ??_7 and
    // regress Save), m_handle is a Win32 HANDLE, m_name the engine CString buffer.
    // None are dereferenced here; they are pure layout placeholders. Kept void*.
    char _vft0[4];   // +0x00  engine vptr (reduced view; not dispatched)
    HANDLE m_handle; // +0x04  Win32 HANDLE (-1 when closed); never dereferenced here
    i32 m_open;      // +0x08  open/refcount flag
    char* m_name;    // +0x0C  CString filename buffer (the CString body pointer)
};
SIZE(0x10); // 16-byte file-stream object (full layout to +0xc)

class CDDSurface;

// (CPlaneFrame + CPlaneTile DISSOLVED 2026-07-28: they were pad-and-offset views of
// CDDrawWorker (<DDrawMgr/DDrawWorker.h>) and CImage (<Image/CImage.h>). CPlaneFrame's
// m_frames@+0x14 IS CDDrawWorker::m_items.m_pData and m_lo/m_hi@+0x64/+0x68 ARE
// m_minIndex/m_maxIndex; CPlaneTile's m_trans/m_src@+0x28/+0x2c ARE CImage's
// m_loadResult/m_surface. Proof: the plane's +0x9c array is filled by
// CDDrawWorkerHost::Read out of the image-set registry map (CDDrawWorkerRegistry::
// m_10map, "the name -> worker/sprite hash table"), and Read's `flags & 0x10` arm
// open-codes CDDrawWorker::GetAt's [m_minIndex, m_maxIndex] bounds check on that very
// element, then feeds the result's +0x10/+0x14 to SetTileSize as width/height -
// i.e. CImage::m_width/m_height, exactly like CDDrawWorkerHost::SetTileSizeFromImageSet.)

struct CPlaneDrawCtx {
    u8 pad_0[0x2c];
    CDDSurface* m_surface; // +0x2c  the blit target surface
};
SIZE_UNKNOWN();

// (CPlanePalOwner + CPlanePalArr DISSOLVED 2026-07-27 - the @identity-TODO closed.
// The palette tail hanging off CDDrawWorkerMapSmall::m_cachedWorker (+0x64) is
// CAniRecordBase2::m_buf (+0x10, a CDDPalette*) -> CDDPalette::m_cacheA (+0x0c, the
// live 0x400-byte 256-entry PALETTEENTRY table). The recorded LEAD was right - every
// m_map1 value is already deleted as a CAniRecordBase2 - and the byte that was
// missing is CDDPalette's own layout: m_cacheA sits at exactly +0x0c and a
// PALETTEENTRY is 4 bytes {R,G,B,flags}, which is precisely the [i*4+0..2] stride the
// plane's ResolveColorKey reads. m_cachedWorker is typed CAniRecordBase2* now.)
class CFileMemBase;

extern void* operator new(u32 size);
extern void operator delete(void* p);

extern "C" i32 uncompress(Bytef* dest, uLongf* destLen, const Bytef* source, uLong sourceLen);

extern "C" Bytef* __stdcall WwdFile_InflateMainBlock(WwdHeader* src, Bytef* dest, u32 destLen);

i32 __stdcall WwdFile_CheckHeader(const char* name, void* headerOut);

class WwdFile {
public:
    static i32 ValidateMainBlock(CString name);
    // GetMapBaseName (0x3bb50, static __cdecl, returns CString by value): given a
    // map file path, return the filename portion (after the last '\\') with the
    // 4-char extension dropped, via the shared 0x62c010 scratch buffer. Empty or
    // <= 4-char paths come back unchanged.
    static CString GetMapBaseName(CString path);
    // (RebuildPlanes @0x1628f0 + ReadPlaneObjects @0x162af0 are GONE from here: their
    // `this` IS the plane - they read m_mapData@+0x0c, m_wrapW/m_wrapH@+0x30/+0x34 and
    // the spatial worker @+0xb0 off it - so they are CDDrawWorkerHost methods now. This
    // class keeps only its two genuinely-static WWD helpers.)
};
SIZE_UNKNOWN(); // namespace-class (method-only)

#endif // SRC_WWD_WWDFILE_H

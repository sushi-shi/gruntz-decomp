// WwdFile.h - Gruntz WWD level-file loader (Monolith-faithful reconstruction).
//
// On-disk WWD header layout is pinned in src/Stub/types/wwd.h; this header
// reproduces the load-bearing fields the loaders actually touch.
#ifndef SRC_WWD_WWDFILE_H
#define SRC_WWD_WWDFILE_H

#include <Mfc.h> // real MFC CString (the WwdFile members take it by value)
#include <Ints.h>
#include <rva.h>

typedef u8 Bytef;
typedef u32 uLong;
typedef u32 uLongf;

// WWD file header (1524 = 0x5F4 bytes on disk). Only the fields the loaders
// read are named; the rest is padding to preserve offsets. See formats/wwd.h.
struct WwdHeader {
    u32 wwdSignature; // +0x000  == 1524 (header size)
    u32 field_4;      // +0x004
    u32 flags;        // +0x008  bit1 (0x2) = COMPRESS
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

// ---------------------------------------------------------------------------
// Engine binary file stream (16 bytes). The validators construct one on the
// stack, Open() a file, Read() the header, then let the destructor (Close) run.
// All four members are UNMATCHED engine functions (ctor, dtor, Open, Read);
// their call bytes are reloc-masked in objdiff.
// Layout: vtable@+0, HANDLE@+4, openFlag@+8, CString filename@+0xc.
//   - ctor sets up the vtable + CString and leaves HANDLE = -1.
//   - Open(name, mode, errSink) opens via CreateFileA; returns nonzero on
//     success, 0 on failure (the engine's reversed sense).
//   - Read(buf, len) ReadFiles len bytes into buf; returns the count read.
//   - the destructor CloseHandle()s an open handle (HANDLE != -1).
// ---------------------------------------------------------------------------
class WwdInputStream {
public:
    WwdInputStream();
    ~WwdInputStream();
    i32 Open(const char* name, i32 mode, void* errSink);
    i32 Read(void* buf, i32 len);

private:
    void* m_vtbl;   // +0x00
    void* m_handle; // +0x04  HANDLE (-1 when closed)
    i32 m_open;     // +0x08  open/refcount flag
    void* m_name;   // +0x0C  CString filename buffer
};

// ---------------------------------------------------------------------------
// CPlane - the in-memory plane object ReadPlane constructs (one per WWD plane).
// UNMATCHED engine class; modeled as an external shell so ReadPlane's calls to
// it reloc-mask. Size 0x158 (344 B); ctor with three
// args, two-phase vtable. ReadPlane uses
// only the vtable: slot 0xA (offset 0x28) = the plane-block reader (reads one
// WwdPlaneHeader at 0xA0 stride + fans out to tile/imageset/object sub-readers);
// slot 0x1 (offset 0x4) = the scalar-deleting destructor. m_flags@+0x8 carries
// the plane flags (bit0 = MAIN). None of these have bodies here.
// ---------------------------------------------------------------------------
class CPlane {
public:
    // Construct from (a, planeIndex, c); reloc-masked engine ctor.
    CPlane(void* a, i32 planeIndex, void* c);
    // vtable: +0x28 reader(planeData, blockBase, outCtx) -> nonzero on success.
    virtual i32 dummy0();
    virtual void dtor(i32 flags); // +0x04  scalar-deleting dtor
    virtual i32 dummy2();         // +0x08
    virtual i32 dummy3();         // +0x0c
    virtual i32 dummy4();         // +0x10
    virtual i32 dummy5();         // +0x14
    virtual i32 dummy6();         // +0x18
    virtual i32 dummy7();         // +0x1c
    virtual i32 dummy8();         // +0x20
    // +0x24  the object-plane block reader: 6 forwarded args, the plane context
    // (&CGameLevelPlanes::m_planeCtx) as the 7th, and a trailing 8th arg.
    virtual i32 ReadObjects(i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6, void* outCtx, i32 a7);
    virtual i32 Read(void* planeData, void* blockBase, void* outCtx); // +0x28

    u8 pad_4[0x4]; // +0x04
    u32 m_flags;   // +0x08  bit0 = MAIN/origin-fixed plane
    u8 pad_c[0x10 - 0x0c];
    float m_scaledX; // +0x10  startX (or startX * m_scaleX)
    float m_scaledY; // +0x14  startY (or startY * m_scaleY)
    float m_scaleX;  // +0x18  X parallax factor
    float m_scaleY;  // +0x1c  Y parallax factor
    u8 pad_20[0x84 - 0x20];
    i32 m_originX;           // +0x84
    i32 m_originY;           // +0x88
    u8 pad_8c[0x158 - 0x8c]; // pad to the real CPlane size (0x158)
};

// ---------------------------------------------------------------------------
// CDDSurface - the engine DirectDraw surface. UNMATCHED; modeled as an external
// shell so the plane draw methods' Blt calls reloc-mask. The plane embeds its
// scratch DDBLTFX-ish blit param at +0xF4 (passed by address into BltEx). Only
// the two blit virtuals the renderer uses are declared.
class CDDSurface {
public:
    // BltEx(srcRect, a, b, flags, blitParam) - the "fill / colorkey-clear" blit;
    // __thiscall, 5 stack args. Reloc-masked engine code.
    i32 BltEx(void* srcRect, i32 a, i32 b, i32 flags, void* blitParam);
    // BltFast(x, y, src, srcRect, blitParam) - the per-tile fast blit; __thiscall,
    // 5 stack args. Reloc-masked engine code.
    i32 BltFast(i32 x, i32 y, void* src, void* srcRect, void* blitParam);
};

// A drawable plane source-frame (one entry of the tile/image lookup the renderer
// dereferences through m_planeArray[handle>>16]). Only the fields the draw loop
// reads are pinned: the cell-bounds (+0x64/+0x68), the frame table (+0x14), and
// the per-frame blit src (+0x28 surface, +0x2c srcRect).
struct CPlaneFrame {
    u8 pad_0[0x14];
    void** m_frames; // +0x14  frame table (indexed by the low 16 bits of handle)
    u8 pad_18[0x64 - 0x18];
    i32 m_lo; // +0x64  valid handle range [m_lo, m_hi]
    i32 m_hi; // +0x68
};

// One resolved tile/sprite frame: +0x28 = the source surface, +0x2c = its srcRect.
struct CPlaneTile {
    u8 pad_0[0x28];
    void* m_surface; // +0x28
    void* m_srcRect; // +0x2c
};

// ---------------------------------------------------------------------------
// CPlaneRender - the WWD plane render view (a window onto CPlane / the level
// plane). The four draw methods below are __thiscall members. Layout (offsets
// are the load-bearing thing; this is the same plane object as PlaneGeom):
//   +0x08 flags        bit1 (0x2)=hidden, bit2 (0x4)=wrap X, bit3 (0x8)=wrap Y
//   +0x10/+0x14        scaledX / scaledY (float scroll origin)
//   +0x20 m_tileGrid   i32* tile-handle grid (row-major, stride m_gridW)
//   +0x24 m_colOffsets i32* per-column base offset (indexed by tile col)
//   +0x28/+0x2c        gridW / gridH (tile wrap counts; row-major dims)
//   +0x30/+0x34        wrapW / wrapH (pixel-wrap moduli)
//   +0x38/+0x3c        tilePxW / tilePxH (one tile's pixel size)
//   +0x40/+0x44        originX / originY      +0x48/+0x4c extentX / extentY
//   +0x50/+0x54        viewX / viewY (scroll pixel offset)
//   +0x60/+0x64/+0x68/+0x6c  default fill rect (passed as a BltFast src rect)
//   +0x8c/+0x90        log2(tilePxW) / log2(tilePxH) (shift amounts)
//   +0xa0 m_planeArray CPlaneFrame** (resolved by handle>>16)
//   +0xb0 m_scroll     the camera/scroll sub-object (SetTarget)
//   +0xf4 m_surface    embedded CDDSurface blit target/param
// ---------------------------------------------------------------------------
struct CPlaneScroll {
    u8 pad_0[0x10];
    i32 m_rectALeft, m_rectATop, m_rectARight, m_rectABottom; // +0x10
    i32 m_rectCLeft, m_rectCTop, m_rectCRight, m_rectCBottom; // +0x20
    i32 m_rectBLeft, m_rectBTop, m_rectBRight, m_rectBBottom; // +0x30
    i32 m_centerAX, m_centerAY;                               // +0x40
    i32 m_centerBX, m_centerBY;                               // +0x48
    i32 m_centerCX, m_centerCY;                               // +0x50
    u8 pad_58[0x68 - 0x58];
    i32 m_targetX, m_targetY; // +0x68

    // 0x168340 / 0x168500: SetTarget(x, y) - stores at +0x68/+0x6c when changed,
    // returns nonzero when it moved (0 when unchanged).
    i32 SetTargetA(i32 a, i32 b);
    i32 SetTargetB(i32 a, i32 b);
};

// The level map/surface source CPlaneRender renders from (this->m_mapData). Three
// chains the boundary methods walk: a pixel-format descriptor (+0x4 -> +0x10 ->
// +0x18 = bitdepth 8/16), a palette host (+0x18 -> +0x64 -> +0x10 -> +0xc =
// RGB888 array), and a plane geometry block (+0x24, with the six dim fields the
// scroll-rect setup reads). Only the offsets are load-bearing.
struct CPlaneSurfDesc {
    u8 pad_0[0x18];
    i32 m_format; // +0x18  8 or 16 (bpp)
};
struct CPlaneSurf {
    u8 pad_0[0x10];
    CPlaneSurfDesc* m_desc; // +0x10
};
struct CPlanePalArr {
    u8 pad_0[0xc];
    u8* m_rgb; // +0xc  RGB888 triples (4 bytes/entry)
};
struct CPlanePalOwner {
    u8 pad_0[0x10];
    CPlanePalArr* m_palette; // +0x10
};
struct CPlanePalHost {
    u8 pad_0[0x64];
    CPlanePalOwner* m_owner; // +0x64
};
struct CPlaneGeom {
    u8 pad_0[0xc8];
    i32 m_rectAWidth, m_rectAHeight, m_rectBWidth, m_rectBHeight, m_rectCWidth,
        m_rectCHeight; // +0xc8..+0xdc
};
struct CPlaneMapData {
    void* m_0;
    CPlaneSurf* m_surface; // +0x4  pixel-format chain
    u8 pad_8[0x18 - 0x8];
    CPlanePalHost* m_paletteHost; // +0x18  palette host
    u8 pad_1c[0x24 - 0x1c];
    CPlaneGeom* m_geometry; // +0x24  plane geometry
};

// The serialize stream CPlaneRender::Save/Load drive (a CArchive-like binary I/O
// object). Virtual Read at slot +0x2c, Write at +0x30; both __thiscall, return an
// int byte-count. UNMATCHED engine code -> reloc-masked virtual calls.
struct CWwdStream {
    virtual void v00();
    virtual void v04();
    virtual void v08();
    virtual void v0c();
    virtual void v10();
    virtual void v14();
    virtual void v18();
    virtual void v1c();
    virtual void v20();
    virtual void v24();
    virtual void v28();
    virtual i32 Read(void* buf, i32 len);  // +0x2c
    virtual i32 Write(void* buf, i32 len); // +0x30
};

// CPlaneRender's own vtable dispatch view (the +0x14 "is-loaded" virtual the tile
// validator gates on). Modeled as a polymorphic interface so the vtable call falls
// out; no vtable is emitted here (no virtual is defined in this TU).
struct CPlaneRenderPoly {
    virtual void v00();
    virtual void v04();
    virtual void v08();
    virtual void v0c();
    virtual void v10();
    virtual i32 IsLoaded(); // +0x14
};

class CPlaneRender {
public:
    void Draw(void* ctx);                   // 0x162010  the tile-grid render
    void SetTileSize(i32 tileW, i32 tileH); // 0x161f00  derive wrap dims/fill/shifts
    void WrapCoord(i32* px, i32* py);       // 0x00a000  wrap+transform a world coord
    i32 CenterScrollA();                    // 0x163300
    i32 CenterScrollB();                    // 0x163370
    void InitScrollRects();                 // 0x163420  seed the scroll sub-object rects
    i32 ValidateTiles(char* errOut);        // 0x163510  scan the tile grid for bad refs
    void ResolveColorKey();                 // 0x163670  pack the +0x144 index to RGB565
    i32 Save(CWwdStream* s);                // 0x163780  serialize out
    i32 Load(CWwdStream* s);                // 0x1638c0  serialize in
    // 0x0d53a0 (__thiscall, ret 8): index the tile-handle grid by (row, col):
    //   m_tileGrid[m_colOffsets[col] + row].
    i32 GetTileHandle(i32 row, i32 col);
    // 0x0311e0 (__thiscall, ret 0xc): snap a world (x,y) to its tile centre:
    //   out = (floor-to-tile) + half a tile, per-axis.
    void SnapToTileCenter(i32* out, i32 x, i32 y);

    u8 pad_0[0x08];
    u32 m_flags; // +0x08
    CPlaneMapData* m_mapData;
    float m_scaledX; // +0x10
    float m_scaledY; // +0x14
    i32 m_18, m_1c;
    i32* m_tileGrid;   // +0x20
    i32* m_colOffsets; // +0x24
    i32 m_gridW;       // +0x28
    i32 m_gridH;       // +0x2c
    i32 m_wrapW;       // +0x30
    i32 m_wrapH;       // +0x34
    i32 m_tilePxW;     // +0x38
    i32 m_tilePxH;     // +0x3c
    i32 m_originX;     // +0x40
    i32 m_originY;     // +0x44
    i32 m_extentX;     // +0x48
    i32 m_extentY;     // +0x4c
    i32 m_viewX;       // +0x50
    i32 m_viewY;       // +0x54
    u8 pad_58[0x60 - 0x58];
    i32 m_fillL; // +0x60
    i32 m_fillT; // +0x64
    i32 m_fillR; // +0x68
    i32 m_fillB; // +0x6c
    u8 pad_70[0x80 - 0x70];
    i32 m_80, m_84, m_88;       // +0x80..+0x88
    i32 m_shiftX;               // +0x8c
    i32 m_shiftY;               // +0x90
    i32 m_94, m_98, m_9c;       // +0x94..+0x9c
    CPlaneFrame** m_planeArray; // +0xa0
    u8 pad_a4[0xb0 - 0xa4];
    CPlaneScroll* m_scroll;   // +0xb0
    char m_name[0xf4 - 0xb4]; // +0xb4  plane name (serialized as a fixed 0x80 field)
    CDDSurface m_surface;     // +0xf4  (empty model, sizeof 1)
    u8 pad_f5[0x144 - 0xf5];
    i32 m_colorKey; // +0x144  the color-key palette index, packed in place to RGB565
};

// ---------------------------------------------------------------------------
// CGameLevel - the level-load orchestrator (a.k.a. CDDrawLevelData). ReadPlane is a
// __thiscall member on it. Only the members ReadPlane touches are pinned here
// (the rest of the class is reconstructed in src/Gruntz/GameLevel.{cpp,h}):
//   +0x0C m_field0c   - 1st CPlane ctor arg (the owning context the planes share)
//   +0x10 m_planeCtx  - &this->m_planeCtx is the 3rd arg to CPlane::Read
//   +0x34 m_planes    - MFC CArray<CPlane*> (SetAtGrow grows + stores)
//   +0x3C m_planeCount- current plane index / count (the running loop counter)
//   +0x5C m_mainPlane - cached pointer to the MAIN plane
//   +0x60 m_mainIndex - index of the MAIN plane (m_planeCount - 1 at capture)
// ---------------------------------------------------------------------------
class CGameLevelPlanes {
public:
    // ReadPlane: new CPlane; if its block reader
    // succeeds, append it to m_planes and record the MAIN plane; else delete it.
    CPlane* ReadPlane(void* planeData, void* blockBase, void* unused);
    // ReadObjectPlane (0x15d9a0): same shape as ReadPlane but the plane is built
    // via the +0x24 object-block reader (6 forwarded args + &m_planeCtx + a7).
    CPlane* ReadObjectPlane(i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6, i32 a7);

    u8 pad_0[0x0c];
    void* m_field0c;            // +0x0C
    u8 m_planeCtx[0x34 - 0x10]; // +0x10  (&m_planeCtx -> CPlane::Read)
    u8 m_planes[0x3c - 0x34];   // +0x34  CArray<CPlane*>
    i32 m_planeCount;           // +0x3C
    u8 pad_40[0x5c - 0x40];
    CPlane* m_mainPlane; // +0x5C
    i32 m_mainIndex;     // +0x60
};

// MFC CArray<CPlane*>::SetAtGrow(index, value).
// Grows the backing store to fit `index` then stores value at [index].
struct CPlanePtrArray {
    void SetAtGrow(i32 index, CPlane* value);
};

// Global operator new (NAFXCW new-handler loop) / delete.
extern void* operator new(u32 size);
extern void operator delete(void* p);

// zlib one-shot decompressor (matched, vendor/zlib-1.0.4/uncompr.c).
extern "C" i32 uncompress(Bytef* dest, uLongf* destLen, const Bytef* source, uLong sourceLen);

// Inflate the WWD main block (planes/tiles) in place into a caller buffer.
// __stdcall free function (callee cleans 12 bytes).
extern "C" i32 __stdcall WwdFile_InflateMainBlock(WwdHeader* src, Bytef* dest, u32 destLen);

// Validate a WWD file by name: open it, read the 1524-byte header into the
// caller buffer, require read == 0x5F4 and header signature <= 0x5F4.
// __stdcall free function (callee cleans 8 bytes); returns 1 / 0.
i32 __stdcall WwdFile_IsValidWwd(const char* name, void* headerBuf);

// Validate a WWD file by name into a private 1524-byte buffer, then copy that
// header into the caller buffer. __stdcall (callee cleans 8 bytes); returns 1 / 0.
i32 __stdcall WwdFile_CheckHeader(const char* name, void* headerOut);

// (real MFC CString from <Mfc.h> at the top; the WwdFile members take it by value)

// ---------------------------------------------------------------------------
// WwdFile - the WWD level-file loader namespace-class. IsValidWwd/CheckHeader/
// InflateMainBlock are exposed above as the free `WwdFile_*` symbols the engine
// actually emits; the two methods below carry the `@WwdFile@@` mangling.
//   - ValidateMainBlock: a static caller-cleaned helper (Ghidra mis-derived its
//     prototype as `void(void)`); takes a CString by value, returns an int.
//   - ReadPlaneObjects: __thiscall, reads one object record at `src` into a new
//     game object and returns the byte count consumed.
// ---------------------------------------------------------------------------
class WwdFile {
public:
    static i32 ValidateMainBlock(CString name);
    // GetMapBaseName (0x3bb50, static __cdecl, returns CString by value): given a
    // map file path, return the filename portion (after the last '\\') with the
    // 4-char extension dropped, via the shared 0x62c010 scratch buffer. Empty or
    // <= 4-char paths come back unchanged.
    static CString GetMapBaseName(CString path);
    i32 ReadPlaneObjects(const i32* src);
    // 0x1628f0: free the old +0xb0 plane-render worker, allocate+init a fresh one
    // from the level header geometry, then ReadPlaneObjects `count` times.
    i32 RebuildPlanes(i32 base, i32 count);
};

#endif // SRC_WWD_WWDFILE_H

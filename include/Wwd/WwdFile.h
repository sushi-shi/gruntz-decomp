// WwdFile.h - Gruntz WWD level-file loader (Monolith-faithful reconstruction).
//
// This header reproduces the load-bearing on-disk WWD fields the loaders
// actually touch (WwdHeader, 0x5F4); the full plane/object on-disk formats are
// not yet modeled in a matched TU.
#ifndef SRC_WWD_WWDFILE_H
#define SRC_WWD_WWDFILE_H

class CImageSet; // Image/ImageSet.h - SetTileSizeFromImageSet's frame source

#include <Mfc.h> // real MFC CString (the WwdFile members take it by value)
#include <Ints.h>
#include <rva.h>

// The plane object + its camera/scroll worker are the canonical
// CDDrawWorkerHost / CWwdSpatialMgr now (<DDrawMgr/DDrawWorkerHost.h>, included
// below); the CPlane/CPlaneRender/CPlaneScroll spellings this header used to
// define are typedefs of those canonicals.

typedef u8 Bytef;
typedef u32 uLong;
typedef u32 uLongf;

// WWD file header (1524 = 0x5F4 bytes on disk). Only the fields the loaders
// read are named; the rest is padding to preserve offsets.
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
    void* m_00;     // +0x00  engine vptr (opaque)
    void* m_handle; // +0x04  HANDLE (-1 when closed)
    i32 m_open;     // +0x08  open/refcount flag
    void* m_name;   // +0x0C  CString filename buffer
};

// ---------------------------------------------------------------------------
// CPlane - the in-memory plane object ReadPlane constructs (one per WWD plane).
// IT IS the real polymorphic CDDrawWorkerHost: the "reloc-masked engine ctor"
// ReadPlane news (0x1615a0, size 0x158 - the +0xf4 pool ends exactly at +0x158)
// stamps ??_7CDDrawWorkerHost (0x1f0270, 12 slots). The full unified class - the
// union of the CPlane/CPlaneRender/CLevelPlane facet views - plus PlaneBlitScratch
// and the CPlaneScroll==CWwdSpatialMgr scroll worker live in the canonical header:
// ---------------------------------------------------------------------------
#include <DDrawMgr/DDrawWorkerHost.h> // CDDrawWorkerHost (+ CPlane/CPlaneRender/
                                      // CLevelPlane/CPlaneScroll typedefs,
                                      // LevelCoordRect, PlaneBlitScratch)

// The real engine draw surface (BltEx/BltFast `this`); defined in
// <DDrawMgr/DDSurface.h> and reached via the Draw ctx's +0x2c.
class CDDSurface;
struct CPlaneTile;

// A drawable plane source-frame (one entry of the tile/image lookup the renderer
// dereferences through m_planeArray[handle>>16]). Only the fields the draw loop
// reads are pinned: the cell-bounds (+0x64/+0x68), the frame table (+0x14).
SIZE_UNKNOWN(CPlaneFrame);
struct CPlaneFrame {
    u8 pad_0[0x14];
    CPlaneTile** m_frames; // +0x14  frame table (indexed by the low 16 bits of handle)
    u8 pad_18[0x64 - 0x18];
    i32 m_lo; // +0x64  valid handle range [m_lo, m_hi]
    i32 m_hi; // +0x68
};

// One resolved tile/sprite frame Draw blits: +0x28 = the BltFast transparency
// key, +0x2c = the source surface. (Draw supplies the srcRect itself, per region.)
SIZE_UNKNOWN(CPlaneTile);
struct CPlaneTile {
    u8 pad_0[0x28];
    u32 m_trans;       // +0x28  BltFast transparency/colour-key flag
    CDDSurface* m_src; // +0x2c  source surface
};

// The render context Draw takes (the render visitor Sync/Draw/Hook receive):
// only the target draw surface (the BltEx/BltFast `this`) at +0x2c is used.
SIZE_UNKNOWN(CPlaneDrawCtx);
struct CPlaneDrawCtx {
    u8 pad_0[0x2c];
    CDDSurface* m_surface; // +0x2c  the blit target surface
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
    u8 pad_0[0xb0];
    // +0xb0..+0xc4: the first three geometry pairs (ex the WwdPlaneHdr view's
    // geo[0..5]); with the three rect-dim pairs below they are the SIX pairs
    // RebuildPlanes hands the spatial worker's Init.
    i32 m_pairA[2]; // +0xb0
    i32 m_pairB[2]; // +0xb8
    i32 m_pairC[2]; // +0xc0
    i32 m_rectAWidth, m_rectAHeight, m_rectBWidth, m_rectBHeight, m_rectCWidth,
        m_rectCHeight; // +0xc8..+0xdc
};
struct CPlaneMapData {
    void* m_0;
    CPlaneSurf* m_surface; // +0x4  pixel-format chain
    void* m_8;             // +0x08  worker source (RebuildPlanes hands it to the grid Init;
                           //        ex the WwdRegOwner view's m_8)
    u8 pad_c[0x18 - 0xc];
    CPlanePalHost* m_paletteHost; // +0x18  palette host
    u8 pad_1c[0x24 - 0x1c];
    CPlaneGeom* m_geometry; // +0x24  plane geometry (ex the WwdRegOwner view's m_24)
};

// The serialize stream CPlaneRender::Save/Load drive is the REAL abstract stream
// base CFileMemBase (<Io/FileMem.h>, VTBL 0x1efe68): its slot-11 (+0x2c) Read /
// slot-12 (+0x30) Write are exactly the "+0x2c Read / +0x30 Write" this header's
// former 13-slot CWwdStream view hand-rolled (and the CSerialArchive view still
// models - same identity, fold deferred). Pointer-only here; the dispatching TU
// includes the real header.
class CFileMemBase;

// (The former CPlaneRenderPoly "own vtable dispatch view" is GONE - the plane's
// real vtable is ??_7CDDrawWorkerHost (0x1f0270) and the +0x14 "is-loaded" virtual
// the tile validator gates on is its slot 5 (IsLoaded, 0x163a90), declared on the
// canonical class. The recorded "cannot be made polymorphic" wall was false: the
// class HAS been real-polymorphic in <DDrawMgr/DDrawWorkerHost.h> since the
// all-vtables-real batch.)

// (The CPlaneRender render-facet class that stood here - Draw/SetTileSize/scroll/
// serialize over the +0x08..+0x144 layout - is DISSOLVED onto the canonical
// CDDrawWorkerHost: same ctor-proven object, same offsets. Its +0x9c CObArray
// (m_frameSets - proven by CGruntzMgr::LoadMonologoSprite's SetAtGrow at
// [plane+0x9c]), the +0xb0 scroll worker and the +0xf4 blit scratch/color key all
// live on the canonical class; CPlaneRender is a typedef of it now.)

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
    CPlaneMapData* m_field0c;  // +0x0C  (the plane ctor's 1st arg - the shared map-data owner)
    LevelCoordRect m_planeCtx; // +0x10  (&m_planeCtx -> CPlane::Read / InitGeometry bounds)
    u8 pad_20[0x34 - 0x20];    // +0x20
    u8 m_planes[0x3c - 0x34];  // +0x34  CArray<CPlane*>
    i32 m_planeCount;          // +0x3C
    u8 pad_40[0x5c - 0x40];
    CPlane* m_mainPlane; // +0x5C
    i32 m_mainIndex;     // +0x60
};

// MFC CArray<CPlane*>::SetAtGrow(index, value).
// Grows the backing store to fit `index` then stores value at [index].
// authentic: the +0x34 CArray sub-object is an embedded member reached by address
// (`&m_planes`), not a typed pointer; the cast targets that embedded array's method
// set (binary-proven CArray<CPlane*> shape). A typed member here shifted GameLevel's
// ctor 89.5%->72% in a prior probe, so the by-address method view is retained.

// Global operator new (NAFXCW new-handler loop) / delete.
extern void* operator new(u32 size);
extern void operator delete(void* p);

// zlib one-shot decompressor (matched, vendor/zlib-1.0.4/uncompr.c).
extern "C" i32 uncompress(Bytef* dest, uLongf* destLen, const Bytef* source, uLong sourceLen);

// Inflate the WWD main block (planes/tiles) in place into a caller buffer.
// __stdcall free function (callee cleans 12 bytes).
extern "C" i32 __stdcall WwdFile_InflateMainBlock(WwdHeader* src, Bytef* dest, u32 destLen);

// (0x160530, "validate a WWD by name": Open -> Read the 1524-byte header -> require
// read == 0x5F4 and header signature <= 0x5F4, returns 1/0, callee cleans 8 bytes - is
// NOT a free WwdFile_* symbol: its only caller invokes it as WwdLevelInfoSrc::IsValidWwd,
// a thiscall method that ignores `this`. Modeled there - <Gruntz/CustomWorldInfoDlg.h> /
// GameLevel.cpp.)

// Validate a WWD file by name into a private 1524-byte buffer, then copy that
// header into the caller buffer. __stdcall (callee cleans 8 bytes); returns 1 / 0.
i32 __stdcall WwdFile_CheckHeader(const char* name, void* headerOut);

// (real MFC CString from <Mfc.h> at the top; the WwdFile members take it by value)

// ---------------------------------------------------------------------------
// WwdFile - the WWD level-file loader namespace-class. CheckHeader/InflateMainBlock
// are exposed above as the free `WwdFile_*` symbols the engine actually emits (the
// 0x160530 validator is instead WwdLevelInfoSrc::IsValidWwd - see above); the two
// methods below carry the `@WwdFile@@` mangling.
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
    // (RebuildPlanes @0x1628f0 + ReadPlaneObjects @0x162af0 are GONE from here: their
    // `this` IS the plane - they read m_mapData@+0x0c, m_wrapW/m_wrapH@+0x30/+0x34 and
    // the spatial worker @+0xb0 off it - so they are CDDrawWorkerHost methods now. This
    // class keeps only its two genuinely-static WWD helpers.)
};

// --- vtable catalog ---

#endif // SRC_WWD_WWDFILE_H

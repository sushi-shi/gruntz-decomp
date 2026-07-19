// WwdFile.h - Gruntz WWD level-file loader (Monolith-faithful reconstruction).
//
// This header reproduces the load-bearing on-disk WWD fields the loaders
// actually touch (WwdHeader, 0x5F4); the full plane/object on-disk formats are
// not yet modeled in a matched TU.
#ifndef SRC_WWD_WWDFILE_H
#define SRC_WWD_WWDFILE_H

class CDDrawWorker;             // CImageSet IS CDDrawWorker (<DDrawMgr/DDrawWorker.h>);
typedef CDDrawWorker CImageSet; // identical repeat of ImageSet.h's typedef - legal, and
                                // keeps this header pointer-only/include-light.

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
    void* m_00;      // +0x00  engine vptr (opaque)
    HANDLE m_handle; // +0x04  Win32 HANDLE (-1 when closed); never dereferenced here
    i32 m_open;      // +0x08  open/refcount flag
    char* m_name;    // +0x0C  CString filename buffer (the CString body pointer)
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
// ---------------------------------------------------------------------------
// THE PLANE "MAP DATA" CASCADE IS DISSOLVED (VW3 2026-07-17). CPlaneMapData was a
// facet view of the canonical CDDrawSurfaceMgr (<DDrawMgr/DDrawSurfaceMgr.h>) - the
// level's world/display root - and its four pointee "facets" were views of that
// root's real members. GameLevel.cpp already carried the identity in a comment
// ("the object IS this level's CDDrawSurfaceMgr world root"); these are the bytes
// that close it:
//
//   view field            was        IS (CDDrawSurfaceMgr member)
//   m_0        +0x00      void*      the CObject vptr
//   m_surface  +0x04      CPlaneSurf*      m_drawTarget (CDDrawSubMgrPages*)
//   m_8        +0x08      void*            m_childGroup (CDDrawChildGroup*)
//   (n/a)      +0x10                       m_imageRegistry -> m_10map  [see below]
//   (n/a)      +0x14                       m_workerCache   -> m_10     [see below]
//   m_paletteHost +0x18   CPlanePalHost*   m_workerMap  (CDDrawWorkerMapSmall*)
//   m_geometry +0x24      CPlaneGeom*      m_level      (CGameLevel*)
//
// PROOFS (each read off retail, not inferred from names):
//  * +0x10 / +0x14 ARE the registry/cache map chain. ReadPlaneObjects @0x162af0 does
//        mov eax,[ecx+0x0c]   ; the plane's m_mapData
//        mov ecx,[eax+0x14]   ; -> m_workerCache            (a DEREF at +0x14)
//        add ecx,0x10         ; -> &m_workerCache->m_10     (CMapStringToOb)
//        call 0x1b8008        ; CMapStringToOb::Lookup
//    which is exactly CDDrawSurfaceMgr::m_workerCache->m_10.Lookup. (Our source had
//    mis-transcribed this as address arithmetic `(char*)m_mapData + 0x14 + 0x10`,
//    i.e. m_mapData+0x24 with NO deref, and called CMapStringToPtr::Lookup @0x1b8438
//    - the wrong map class. Both are fixed by the fold.) The sibling chain at +0x10
//    (Helper_166040 / RegisterNamed) is m_imageRegistry->m_10map, same shape.
//  * +0x04 -> CDDrawSubMgrPages: the view read m_surface->m_desc (+0x10) ->
//    m_format (+0x18), "8 or 16". CDDrawSubMgrPages::m_frontPair IS at +0x10 and
//    CDDrawSurfacePair::m_bpp IS at +0x18, "bits-per-pixel (8/16/24/32)".
//  * +0x24 -> CGameLevel: already PROVEN in DDrawSurfaceMgr.h (Init news it with
//    new(0x6d4) + ctor 0x15ccd0 == SIZE(CGameLevel,0x6d4) + ??0CGameLevel), and the
//    ex CPlaneGeom's +0xb0..+0xdc block overlays CGameLevel's m_b0..m_dc dword for
//    dword - the block CGameLevel's own ctor seeds with (500,250)/(1000,1000)/
//    (250,125) rate pairs and the (1600,1200)/(2560,1920) extents.
//  * +0x18 -> CDDrawWorkerMapSmall: the +0x64 slot. Its canonical name was "m_64
//    entry counter cleared by the teardown", but it is a POINTER: RemoveByValue
//    @0x165c40 compares it against the CObject* worker being removed and nulls it on
//    a hit (a counter is neither compared to a worker pointer nor dereferenced), and
//    the plane's palette walk dereferences it (+0x64 -> +0x10 -> +0xc RGB triples).
//    Both readers agree it caches one of m_map1's workers -> m_cachedWorker (CObject*).
// ---------------------------------------------------------------------------

// The palette tail hanging off CDDrawWorkerMapSmall::m_cachedWorker (+0x64).
// @identity-TODO: these two are the ONLY links of the cascade still unproven - the
// cached worker is a CObject* out of the map (RemoveByValue proves that much), but
// WHICH palette-bearing worker class it is, no caller/new-site the xref DB can reach
// names (its only reader is the plane's ResolveColorKey). Kept as honest named
// records rather than a fabricated identity; the RGB888 table at +0xc is what is
// load-bearing.
//
// LEAD for whoever closes it (do not treat as proof): CDDrawWorkerMapSmall's own
// methods treat every m_map1 value as a CAniRecordBase2 (DestroyAll `delete
// ((CAniRecordBase2*)val)`, RemoveByKey's cast), and +0x64 caches a map value - so
// the cached worker is very likely a CAniRecordBase2, whose +0x10 "owned work
// buffer" would then BE this palette array. What is missing is a byte that ties
// that generic work buffer to the RGB888 layout; until one turns up this stays a
// lead, and the slot keeps the map's own element type (CObject*).
struct CPlanePalArr {
    u8 pad_0[0xc];
    u8* m_rgb; // +0xc  RGB888 triples (4 bytes/entry)
};
struct CPlanePalOwner {
    u8 pad_0[0x10];
    CPlanePalArr* m_palette; // +0x10
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

// (The former "CGameLevelPlanes" plane-reader view of CGameLevel is DISSOLVED:
// ReadPlane @0x15d8d0 / ReadObjectPlane @0x15d9a0 are real CGameLevel methods
// (<Gruntz/GameLevel.h>, bodies in GameLevel.cpp) - the view's m_field0c/
// m_planeCtx/m_planes/m_planeCount/m_mainPlane/m_mainIndex were CGameLevel's
// m_0c/+0x10/the +0x34 CObArray (m_nSize == the plane count)/+0x5c/+0x60.)

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

// DDrawWorkerHost.h - THE WWD level plane object (ctor 0x1615a0, dtor 0x163af0;
// own 12-slot vtable ??_7CDDrawWorkerHost @0x5f0270; `new` size 0x158). ONE class
// that was reconstructed as FIVE views, now unified here (name-preserving union of
// every view's proven members):
//   * CDDrawWorkerHost      - the ctor/dtor/RegisterNamed shape (this header)
//   * CPlane     (WwdFile.h)   - CGameLevelPlanes::ReadPlane's product (kept as a typedef)
//   * CPlaneRender (WwdFile.h) - the render facet (Draw/SetTileSize/scroll; typedef)
//   * CLevelPlane (GameLevel.h)- the level's typed facet (geometry/probes; typedef)
//   * "CImageSet3" grid-owner  - the ex-LevelPlane.cpp local pocket (Cleanup/Prune/
//     GetSize) - a MISATTRIBUTION: the receiver view+0x5c is the distinguished MAIN
//     plane (this class), not the 0x18-byte CImageSet3 collision record.
// Identity proof: ReadPlane's `new`(0x158) calls 0x1615a0 which stamps 0x5f0270;
// the +0xf4 pool ends exactly at +0x158; the slot map (vtable_hierarchy) ties
// IsLoaded@5/Cleanup@7/InitGeometry@9/Read@10 to the bodies the facets dispatch.
// Where two views recovered DIFFERENT readings of one retail field, both spellings
// are kept via a union and flagged for the naming pass. Only offsets + emitted
// bytes are load-bearing.
#ifndef GRUNTZ_CDDRAWWORKERHOST_H
#define GRUNTZ_CDDRAWWORKERHOST_H

#include <Ints.h>
#include <Wap32/Object.h>         // CObject (pulls <Mfc.h>/windows.h: RECT + ::CObArray)
#include <DDrawMgr/DDrawWorker.h> // CLoadable (m_frameSets is the real MFC ::CObArray)
#include <rva.h>

// The 4-int coordinate/extent record (CGameLevel+0x10; passed by pointer to the
// level-load/edit methods and embedded at plane+0x50). Half-open tile-bounds box
// [minX,maxX) x [minY,maxY); minX==0x80000000 is the "unset" sentinel. (Moved here
// from GameLevel.h - the plane embeds one.)
SIZE_UNKNOWN(LevelCoordRect);
struct LevelCoordRect {
    i32 minX, minY, maxX, maxY;
};

// PlaneBlitScratch - the DDBLTFX-ish blit-param scratch the plane embeds at +0xF4
// (its ADDRESS is passed into CDDSurface::BltEx by Draw). A zero-size marker so the
// +0xF4 field is addressable without pulling the 0xc0-byte surface into the layout.
SIZE_UNKNOWN(PlaneBlitScratch);
struct PlaneBlitScratch {};

// The camera/scroll + spatial-grid worker at plane+0xb0. ONE class, formerly two
// views: WwdFile.h's CPlaneScroll (the scroll rects/centers/target + SetTargetA/B
// @0x168340/0x168500) and this header's CWwdSpatialMgr (PruneCount/GetSize/FreeGrids
// @0x1688b0/0x168430/0x1682f0 - the same 0x168xxx method band; real class defined in
// src/Gruntz/WwdSpatialMgr.cpp). Its FreeGrids body runs on teardown, then operator
// delete frees it.
SIZE_UNKNOWN(CWwdSpatialMgr);
struct CWwdSpatialMgr {
    char pad_0[0x10];                                         // +0x00 vptr + head words
    i32 m_rectALeft, m_rectATop, m_rectARight, m_rectABottom; // +0x10
    i32 m_rectCLeft, m_rectCTop, m_rectCRight, m_rectCBottom; // +0x20
    i32 m_rectBLeft, m_rectBTop, m_rectBRight, m_rectBBottom; // +0x30
    i32 m_centerAX, m_centerAY;                               // +0x40
    i32 m_centerBX, m_centerBY;                               // +0x48
    i32 m_centerCX, m_centerCY;                               // +0x50
    char pad_58[0x68 - 0x58];
    i32 m_targetX, m_targetY; // +0x68
    void* m_baseVtbl; // +0x70  the m_iter MEMBER's vptr (CWwdGridIter : CObject) - the
                      //        ??_7CObject stamp retail emits on teardown. NOT a
                      //        secondary base: the "+0x70 => multiple inheritance"
                      //        premise behind the ex-C163a40 placeholder was WRONG.

    i32 SetTargetA(i32 a, i32 b); // 0x168340
    i32 SetTargetB(i32 a, i32 b); // 0x168500
    i32 PruneCount();             // 0x1688b0  (reloc-masked external)
    i32 GetSize();                // 0x168430  (reloc-masked external; serialized-size accessor)
    void FreeGrids();             // 0x1682f0  (reloc-masked external teardown body)
    // The out-of-line COMPLETE dtor (0x163a40; body with the canonical class in
    // WwdSpatialMgr.cpp): FreeGrids + the compiler's ~m_iter. DECLARED-ONLY - an
    // explicit `p->~CWwdSpatialMgr()` (Cleanup_161bf0) then emits the retail
    // out-of-line call, while ~CDDrawWorkerHost reproduces retail's INLINED copy
    // explicitly (a decl-only-dtor `delete` there would mis-bind to 0x163a40).
    ~CWwdSpatialMgr();
};
typedef CWwdSpatialMgr CPlaneScroll; // the WwdFile.h spelling

// Support types the method signatures reference (full defs in <Wwd/WwdFile.h> /
// <Image/ImageSet.h> / <Io/FileMem.h> - pointer-only here).
struct CPlaneMapData; // the +0x0c owner/map-data chain (pixel format / palette / geometry)
struct CPlaneDrawCtx; // Draw's render context (its +0x2c is the blit target surface)
class CImageSet;      // the 0x6c sparse CImage-frame collection
class CFileMemBase;   // the abstract serialize stream (Read @+0x2c / Write @+0x30)

class CDDrawWorkerHost : public CObject {
public:
    CDDrawWorkerHost(CPlaneMapData* mapData, i32 field04, i32 flags); // 0x1615a0
    virtual ~CDDrawWorkerHost() OVERRIDE; // slot 1 (scalar-deleting dtor) 0x163af0
    // `new CPlane` (CGameLevelPlanes::ReadPlane/ReadObjectPlane) needs an accessible
    // operator new; MFC CObject's PASCAL one is not usable under MSVC5, so forward to
    // global new (byte-identical: the same `push 0x158; call ??2@YAPAXI@Z`).
    void* operator new(size_t n) {
        return ::operator new(n);
    }

    // --- own vtable slots 5..11 (retail ??_7 @0x1f0270 is 12 slots; per-slot RTTI
    // map from `vtable_hierarchy --class CDDrawWorkerHost`). ------------------------
    virtual i32 IsLoaded();      // slot 5  (+0x14) 0x163a90 "plane loaded?" gate
                                 //         (the ValidateTiles gate; declared-only)
    virtual void VtSlot6_1c08(); // slot 6  (+0x18) 0x001c08 shared CWapObj-family default
    // slot 7 (+0x1c) 0x161bf0 - free the spatial worker (PruneCount then delete) and
    // the two owned grid buffers. (Ex the LevelPlane.cpp-local "CImageSet3" pocket.)
    virtual void Cleanup_161bf0();
    virtual void VtSlot8_163ab0(); // slot 8  (+0x20) 0x163ab0 (role unrecovered)
    // slot 9 (+0x24) 0x1619f0 - the geometry init / object-plane reader
    // CGameLevelPlanes::ReadObjectPlane dispatches: seed tile/wrap/origin/shift
    // fields from the 8 args, log2 the tile shifts, strcpy the name, alloc the tile
    // grid + column-offset table, tail-call RecomputePlaneCoords.
    virtual i32 InitGeometry_1619f0(
        i32 w,
        i32 h,
        i32 tileW,
        i32 tileH,
        i32 depthX,
        i32 depthY,
        LevelCoordRect* bounds,
        char* name
    );
    // slot 10 (+0x28) 0x161640 - the 3-arg plane-block reader
    // CGameLevelPlanes::ReadPlane dispatches (parses one WwdPlaneHeader, fans out to
    // the tile/imageset/object sub-readers). Stub body in LevelPlane.cpp (ex Gap_161640).
    virtual i32 Read(void* planeData, void* blockBase, void* bounds);
    virtual void VtSlot11_163ac0(); // slot 11 (+0x2c) 0x163ac0 (role unrecovered)

    // --- non-virtual methods (union of the facets' method sets; one decl per RVA -
    // the duplicate facet names Sync/Refresh/QueryA/QueryB/Notify are dissolved onto
    // the defined bodies below). --------------------------------------------------
    void RegisterNamed(char index, const char* key); // 0x161c50 (owner-map lookup cache)
    // 0x077dc0 (body in BrickzCellFlags_077790.cpp; ex BrickzGridDesc::SetCell - that
    // view IS this plane, proven by the parallel Brickz fold):
    // m_tileGrid[m_colOffsets[y] + x] = id.
    void SetCell(i32 x, i32 y, i32 id);
    void RecomputePlaneCoords(); // 0x161c90 wrap/clamp scaled coords
    void Build(LevelCoordRect* coords);              // 0x161e80 re-place + recompute one plane
    void SetTileSize(i32 tileW, i32 tileH);          // 0x161f00 derive wrap dims/fill/shifts
    void SetTileSizeFromImageSet(CImageSet* set);    // 0x161fa0 seed tile size from a frame
    void Draw(CPlaneDrawCtx* ctx);                   // 0x162010 the tile-grid render (ex "Sync")
    i32 Prune_1628d0();                              // 0x1628d0 forward the grid's Prune
    i32 CenterScrollA();                             // 0x163300 (ex "QueryA")
    i32 CenterScrollB();                             // 0x163370 (ex "QueryB")
    i32 GetSize_1633e0();                            // 0x1633e0 forward the grid's GetSize
    void InitScrollRects();                          // 0x163420 seed the scroll rects (ex "Notify")
    i32 ValidateTiles(char* errOut);                 // 0x163510 scan the tile grid for bad refs
    void ResolveColorKey();                        // 0x163670 pack +0x144 to RGB565 (ex "Refresh")
    i32 Save(CFileMemBase* s);                     // 0x163780 serialize out
    i32 Load(CFileMemBase* s);                     // 0x1638c0 serialize in
    void WrapCoord(i32* px, i32* py);              // 0x00a000 wrap+transform a world coord
    void SnapToTileCenter(i32* out, i32 x, i32 y); // 0x0311e0 snap world (x,y) to tile centre
    i32 GetTileHandle(i32 row, i32 col);           // 0x0d53a0 m_tileGrid[m_colOffsets[col]+row]

    // --- layout (the union of every facet's proven members; offsets load-bearing).
    // Where a union appears, TWO views recovered different readings of ONE retail
    // field - both spellings kept, reconcile in a naming pass. ---------------------
    i32 m_04;                 // +0x04  (merged CLoadable base field; ctor arg2)
    u32 m_flags;              // +0x08  bit0 = MAIN/origin-fixed plane, bit1 hidden,
                              //        bit2/3 = wrap X/Y (ctor arg3)
    CPlaneMapData* m_mapData; // +0x0c  owner/map-data chain (ctor arg1)
    float m_scaledX;          // +0x10  scroll origin X (RecomputePlaneCoords wrap target)
    float m_scaledY;          // +0x14  scroll origin Y
    float m_scaleX;           // +0x18  X parallax factor (ctor seeds 1.0f)
    float m_scaleY;           // +0x1c  Y parallax factor (ctor seeds 1.0f)
    i32* m_tileGrid;          // +0x20  tile-handle grid (row-major; owned, RezFree'd)
    i32* m_colOffsets;        // +0x24  per-column base offsets (owned, RezFree'd)
    union {
        i32 m_gridW; // +0x28  tile-grid width (the render facet's spelling)
        i32 m_width; //        (the level facet's spelling; LookupTile clamp)
    };
    union {
        i32 m_gridH; // +0x2c  tile-grid height
        i32 m_height;
    };
    i32 m_wrapW; // +0x30  tile count across (wrap/clamp modulus)
    i32 m_wrapH; // +0x34  tile count down
    union {
        i32 m_tilePxW;  // +0x38  tile pixel width (log2 -> m_shiftX)
        i32 m_tilePixW; //        (the level facet's spelling)
    };
    union {
        i32 m_tilePxH; // +0x3c  tile pixel height
        i32 m_tilePixH;
    };
    i32 m_originX; // +0x40  near tile-origin X (RecomputePlaneCoords out; Draw/wrap base)
    i32 m_originY; // +0x44  near tile-origin Y
    i32 m_extentX; // +0x48  far tile-extent X
    i32 m_extentY; // +0x4c  far tile-extent Y
    union {        // +0x50  ONE retail field, two recovered readings (reconcile):
        LevelCoordRect m_bounds50; //  level coord bounds (Build/InitGeometry copy here)
        struct {
            i32 m_viewX; //  scroll pixel offset X (Draw/WrapCoord reading; ctor seeds -1)
            i32 m_viewY; //  scroll pixel offset Y
        };
    };
    RECT m_fillRect;          // +0x60  default full-tile src rect {0,0,tilePxW,tilePxH}
                              //        (== the geometry init's zero/tile-dim quad)
    i32 m_viewW;              // +0x70  viewport tiles across (= bounds width)
    i32 m_viewH;              // +0x74  viewport tiles down (= bounds height)
    i32 m_anchorX;            // +0x78  view-anchor X (= half width)
    i32 m_anchorY;            // +0x7c  view-anchor Y (= half height)
    i32 m_zBound;             // +0x80  plane z bound (VisitVisible draws objects with z-key < this)
    i32 m_snappedX;           // +0x84  out: integer scaledX (snapped by RecomputePlaneCoords)
    i32 m_snappedY;           // +0x88  out: integer scaledY
    i32 m_shiftX;             // +0x8c  tile->pixel shift X (log2 tilePxW)
    i32 m_shiftY;             // +0x90  tile->pixel shift Y
    i32 m_94;                 // +0x94  int scaled into m_scaleX (m_scaleX = m_94 * DAT_5f02a0)
    i32 m_98;                 // +0x98  int scaled into m_scaleY
    ::CObArray m_frameSets;   // +0x9c  frame-set array (elements: CPlaneFrame*; the
                              //        draw loop indexes m_pData by handle>>16;
                              //        ctor 0x1b55e9 / ~ 0x1b561c; ex "m_obArray")
    CWwdSpatialMgr* m_scroll; // +0xb0  camera/scroll + spatial-grid worker
    char m_name[0xf4 - 0xb4]; // +0xb4  plane name (serialized as a fixed 0x80 field)
    union {                   // +0xf4  the 25-dword tail pool (ctor: memset 0, [0]=100)
        i32 m_pool[0x19];
        PlaneBlitScratch m_surface; // +0xf4  blit-param scratch (address passed to BltEx)
        struct {
            char _padColorKey[0x144 - 0xf4];
            i32 m_colorKey; // +0x144  color-key palette index, packed in place to RGB565
        };
    };
    // ENDS AT 0x158 - the ReadPlane allocation size.
};

// The facet spellings (one class, several historical view names - kept as aliases).
typedef CDDrawWorkerHost CPlane;       // the WwdFile.h loader-facet spelling
typedef CDDrawWorkerHost CPlaneRender; // the render-facet spelling
typedef CDDrawWorkerHost CLevelPlane;  // the GameLevel.h level-facet spelling

#endif // GRUNTZ_CDDRAWWORKERHOST_H

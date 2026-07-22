#ifndef GRUNTZ_CDDRAWWORKERHOST_H
#define GRUNTZ_CDDRAWWORKERHOST_H

#include <Ints.h>
#include <Wap32/WapObj.h>         // CWapObj : CObject (pulls <Mfc.h>/windows.h: RECT + ::CObArray)
#include <DDrawMgr/DDrawWorker.h> // CLoadable (m_frameSets is the real MFC ::CObArray)
#include <ddraw.h> // DDBLTFX (the +0xf4 blit-fx member; windows.h via <Mfc.h> above)
#include <rva.h>

typedef struct tagRECT LevelCoordRect;
SIZE_UNKNOWN();

struct CWwdSpatialMgr;

class CDDrawSurfaceMgr; // the +0x0c world/display root (ex the CPlaneMapData view)
struct CPlaneDrawCtx; // Draw's render context (its +0x2c is the blit target surface)
class CDDrawWorker;             // CImageSet IS CDDrawWorker (<DDrawMgr/DDrawWorker.h>);
typedef CDDrawWorker CImageSet; // identical repeat of ImageSet.h's typedef - legal, and
class CFileMemBase;   // the abstract serialize stream (Read @+0x2c / Write @+0x30)

class CDDrawWorkerHost : public CWapObj {
public:
    CDDrawWorkerHost(CDDrawSurfaceMgr* mapData, i32 field04, i32 flags); // 0x1615a0
    virtual ~CDDrawWorkerHost() OVERRIDE; // slot 1 (scalar-deleting dtor) 0x163af0
    // `new CPlane` (CGameLevel::ReadPlane/ReadObjectPlane) needs an accessible
    // operator new; MFC CObject's PASCAL one is not usable under MSVC5, so forward to
    // global new (byte-identical: the same `push 0x158; call ??2@YAPAXI@Z`).
    void* operator new(size_t n) {
        return ::operator new(n);
    }

    // --- own vtable slots 5..11 (retail ??_7 @0x1f0270 is 12 slots; per-slot RTTI
    // map from `vtable_hierarchy --class CDDrawWorkerHost`). ------------------------
    virtual i32 IsLoaded() OVERRIDE; // slot 5  (+0x14) 0x163a90 "plane loaded?" gate
                                     //         (the ValidateTiles gate; overrides CWapObj)
    // slot 6 IsReady INHERITED from CWapObj (its `return 1` default @0xd5da0, reached
    // via the 0x001c08 thunk); not redeclared (that was a phantom own "VtSlot6_1c08").
    // slot 7 (+0x1c) 0x161bf0 - free the spatial worker (PruneCount then delete) and
    // the two owned grid buffers. (Ex the LevelPlane.cpp-local "CImageSet3" pocket.)
    virtual void Cleanup();
    virtual void VtSlot8_163ab0(); // slot 8  (+0x20) 0x163ab0 (role unrecovered)
    // slot 9 (+0x24) 0x1619f0 - the geometry init / object-plane reader
    // CGameLevel::ReadObjectPlane dispatches: seed tile/wrap/origin/shift
    // fields from the 8 args, log2 the tile shifts, strcpy the name, alloc the tile
    // grid + column-offset table, tail-call RecomputePlaneCoords.
    virtual i32 InitGeometry(
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
    // CGameLevel::ReadPlane dispatches (parses one WwdPlaneHeader, fans out to
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
    void RecomputePlaneCoords();                  // 0x161c90 wrap/clamp scaled coords
    void Build(LevelCoordRect* coords);           // 0x161e80 re-place + recompute one plane
    void SetTileSize(i32 tileW, i32 tileH);       // 0x161f00 derive wrap dims/fill/shifts
    void SetTileSizeFromImageSet(CImageSet* set); // 0x161fa0 seed tile size from a frame
    void Draw(CPlaneDrawCtx* ctx);                // 0x162010 the tile-grid render (ex "Sync")
    i32 Prune();                           // 0x1628d0 forward the grid's Prune
    i32 CenterScrollA();                          // 0x163300 (ex "QueryA")
    i32 CenterScrollB();                          // 0x163370 (ex "QueryB")
    i32 GetSize();                         // 0x1633e0 forward the grid's GetSize
    void InitScrollRects();                       // 0x163420 seed the scroll rects (ex "Notify")
    i32 ValidateTiles(char* errOut);              // 0x163510 scan the tile grid for bad refs
    void ResolveColorKey();                       // 0x163670 pack +0x144 to RGB565 (ex "Refresh")
    i32 Save(CFileMemBase* s);                    // 0x163780 serialize out
    i32 Load(CFileMemBase* s);                    // 0x1638c0 serialize in
    // 0x1628f0 / 0x162af0 (ex WwdFile:: + its WwdLevelLoader `this`-view): free the old
    // spatial worker, allocate + Init a fresh one from the map-data geometry, then read
    // `count` object records. `this` IS the plane - the view read m_mapData@+0x0c and
    // (as "grid extents") m_wrapW/m_wrapH@+0x30/+0x34, and the worker slot IS m_scroll.
    i32 RebuildPlanes(i32 base, i32 count);
    i32 ReadPlaneObjects(const i32* src);
    void WrapCoord(i32* px, i32* py);              // 0x00a000 wrap+transform a world coord
    void SnapToTileCenter(i32* out, i32 x, i32 y); // 0x0311e0 snap world (x,y) to tile centre
    i32 GetTileHandle(i32 row, i32 col);           // 0x0d53a0 m_tileGrid[m_colOffsets[col]+row]

    // --- layout (the union of every facet's proven members; offsets load-bearing).
    // Where a union appears, TWO views recovered different readings of ONE retail
    // field - both spellings kept, reconcile in a naming pass. ---------------------
    i32 m_04;                 // +0x04  (merged CLoadable base field; ctor arg2)
    u32 m_flags;              // +0x08  bit0 = MAIN/origin-fixed plane, bit1 hidden,
                              //        bit2/3 = wrap X/Y (ctor arg3)
    // +0x0c  the owning world/display root (ctor arg1). Ex CPlaneMapData - a facet
    // view of THIS class; see the cascade proof in <Wwd/WwdFile.h>.
    CDDrawSurfaceMgr* m_mapData;
    float m_scaledX;          // +0x10  scroll origin X (RecomputePlaneCoords wrap target)
    float m_scaledY;          // +0x14  scroll origin Y
    float m_scaleX;           // +0x18  X parallax factor (ctor seeds 1.0f)
    float m_scaleY;           // +0x1c  Y parallax factor (ctor seeds 1.0f)
    i32* m_tileGrid;          // +0x20  tile-handle grid (row-major; owned, RezFree'd)
    i32* m_colOffsets;        // +0x24  per-column base offsets (owned, RezFree'd)
    i32 m_gridW; // +0x28  tile-grid width
    i32 m_gridH; // +0x2c  tile-grid height
    i32 m_wrapW; // +0x30  tile count across (wrap/clamp modulus)
    i32 m_wrapH; // +0x34  tile count down
    i32 m_tilePxW; // +0x38  tile pixel width (log2 -> m_shiftX)
    i32 m_tilePxH; // +0x3c  tile pixel height
    i32 m_originX; // +0x40  near tile-origin X (RecomputePlaneCoords out; Draw/wrap base)
    i32 m_originY; // +0x44  near tile-origin Y
    i32 m_extentX; // +0x48  far tile-extent X
    i32 m_extentY; // +0x4c  far tile-extent Y
    // +0x50  the plane's level-coord bounds (Build/InitGeometry copy). RECONCILED:
    // the ex-m_viewX/m_viewY "scroll pixel offset" reading was .left/.top - WrapCoord
    // translates world coords by the bounds ORIGIN; the ctor's -1 is the pre-Build
    // sentinel on .left.
    LevelCoordRect m_bounds50;
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
    // +0xf4  the plane's DDBLTFX (0x64 = 25 dwords; the ctor memsets it and seeds
    // dwSize=100 - the classic DirectDraw pattern; BltEx takes its address; the
    // ex-"m_colorKey" @+0x144 is its dwFillColor @+0x50, packed in place to RGB565).
    DDBLTFX m_bltFx;
    // ENDS AT 0x158 - the ReadPlane allocation size.
};
SIZE(0x158);

typedef CDDrawWorkerHost CPlane;       // the WwdFile.h loader-facet spelling
typedef CDDrawWorkerHost CPlaneRender; // the render-facet spelling
SIZE_UNKNOWN();
typedef CDDrawWorkerHost CLevelPlane;  // the GameLevel.h level-facet spelling

#endif // GRUNTZ_CDDRAWWORKERHOST_H

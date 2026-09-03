#ifndef SRC_GRUNTZ_GAMELEVEL_H
#define SRC_GRUNTZ_GAMELEVEL_H

#include <rva.h>

#include <Mfc.h>

#include <DDrawMgr/DDrawWorkerHost.h>
#include <Enums.h>
#include <Globals.h>
#include <Gruntz/CoordNode.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/TileCollisionKind.h>
#include <Wap32/Object.h>
#include <Wap32/WapObj.h>
#include <Wwd/WwdFile.h>
#include <Wwd/WwdTileHandle.h>

class CFileMemBase;
class CDDrawSurfacePair;
struct WwdTileImageRecord;

static const i32 TILE_CLEAR = -1;

#define PROBE_TILE(LVL, X, Y, RESULT)                                                              \
    do {                                                                                           \
        Coord pixel_;                                                                              \
        pixel_.m_y = (Y);                                                                          \
        pixel_.m_x = (X);                                                                          \
        pixel_.Max(Coord(0, 0));                                                                   \
        pixel_.Min(Coord(                                                                          \
            (LVL)->m_mainPlane->m_planePixelSize.cx - 1,                                           \
            (LVL)->m_mainPlane->m_planePixelSize.cy - 1                                            \
        ));                                                                                        \
        CDDrawWorkerHost* pl_ = (LVL)->m_mainPlane;                                                \
        Coord tile_(pixel_.m_x >> pl_->m_tileShift.m_x, pixel_.m_y >> pl_->m_tileShift.m_y);       \
        Coord cellOrigin_(tile_.m_x << pl_->m_tileShift.m_x, tile_.m_y << pl_->m_tileShift.m_y);   \
        Coord subPixel_ = pixel_ - cellOrigin_;                                                    \
        i32 idx_ = pl_->m_tileRowOffsets[tile_.m_y] + tile_.m_x;                                   \
        i32 tileHandle_ = pl_->m_tileHandles[idx_];                                                \
        if (tileHandle_ == UNINIT_FILL || tileHandle_ == TILE_CLEAR) {                             \
            (RESULT) = TILEKIND_PASSABLE;                                                          \
        } else {                                                                                   \
            CTileImageSet* set_ = static_cast<CTileImageSet*>(                                     \
                m_imageSets[tileHandle_ & WWD_TILE_IMAGE_SET_INDEX_MASK]                           \
            );                                                                                     \
            (RESULT) = set_->GetCollisionAt(subPixel_.m_x, subPixel_.m_y);                         \
        }                                                                                          \
    } while (0)

#define PROBE_TILE_VIA_HANDLE(LVL, X, Y, RESULT)                                                   \
    do {                                                                                           \
        Coord pixel_;                                                                              \
        pixel_.m_y = (Y);                                                                          \
        pixel_.m_x = (X);                                                                          \
        pixel_.Max(Coord(0, 0));                                                                   \
        pixel_.Min(Coord(                                                                          \
            (LVL)->m_mainPlane->m_planePixelSize.cx - 1,                                           \
            (LVL)->m_mainPlane->m_planePixelSize.cy - 1                                            \
        ));                                                                                        \
        CDDrawWorkerHost* pl_ = (LVL)->m_mainPlane;                                                \
        Coord tile_(pixel_.m_x >> pl_->m_tileShift.m_x, pixel_.m_y >> pl_->m_tileShift.m_y);       \
        Coord cellOrigin_(tile_.m_x << pl_->m_tileShift.m_x, tile_.m_y << pl_->m_tileShift.m_y);   \
        Coord subPixel_ = pixel_ - cellOrigin_;                                                    \
        i32 tileHandle_ = pl_->GetTileHandle(tile_.m_x, tile_.m_y);                                \
        if (tileHandle_ == UNINIT_FILL || tileHandle_ == TILE_CLEAR) {                             \
            (RESULT) = TILEKIND_PASSABLE;                                                          \
        } else {                                                                                   \
            CTileImageSet* set_ = static_cast<CTileImageSet*>(                                     \
                m_imageSets[tileHandle_ & WWD_TILE_IMAGE_SET_INDEX_MASK]                           \
            );                                                                                     \
            (RESULT) = set_->GetCollisionAt(subPixel_.m_x, subPixel_.m_y);                         \
        }                                                                                          \
    } while (0)

#include <Gruntz/ImageSets.h>
#include <Wap32/CoordUnset.h>

struct CRezItm;

struct CGameObject;
class CDDrawChildGroup;
class CDDrawSurfaceMgr;

struct LevelDims {
    void Init(i32 width, i32 height) {
        w = width;
        h = height;
    }

    i32 w;
    i32 h;
};

GZ_ENUM_CONST_BEGIN(LevelPlaneLayout)
    LEVEL_EXTENDED_PLANE_COUNT = 4
GZ_ENUM_CONST_END(LevelPlaneLayout)

class CGameLevel : public CWapObj {
public:
    i32 IsValidWwd(const char* name, WwdHeader* headerBuf);

    i32 ReadWwdHeaderName(const char* name, char* nameOut);

    Bytef* InflateMainBlock(WwdHeader* src, Bytef* dest, u32 destLen);

    virtual ~CGameLevel() OVERRIDE;
    virtual i32 IsLoaded() OVERRIDE;

    virtual void Unload() OVERRIDE;
    RVA(0x001611b0, 0x6)
    virtual LoadableClassId GetClassId() OVERRIDE {
        return CLASSID_GAMELEVEL;
    }
    virtual i32 LoadWwdWithCoords(WwdHeader* hdr, LevelCoordRect* coords);
    virtual i32 LoadSourceWithCoords(CRezItm* src, LevelCoordRect* coords);
    virtual i32 LoadFileWithCoords(const char* path, LevelCoordRect* coords);
    virtual i32 SetViewportRect(LevelCoordRect* coords);
    virtual i32 SetViewportSize(i32 w, i32 h);
    virtual i32 LoadWwd(WwdHeader* hdr);
    virtual i32 LoadFromSource(CRezItm* source);
    virtual i32 LoadFromFile(const char* path);
    virtual void ReleaseChildren();

    CGameLevel(class CDDrawSurfaceMgr* owner, i32 id, i32 flags);

    void SetSpatialDefaults() {
        m_defaultActiveGridCellSize.Init(500, 250);
        m_largeActiveGridCellSize.Init(1000, 1000);
        m_smallActiveGridCellSize.Init(250, 125);
        m_defaultActiveRegionSize.Init(1600, 1200);
        m_largeActiveRegionSize.Init(2560, 1920);
        m_smallActiveRegionSize.Init(768, 576);
    }

    void ResetSpatialDefaults();

    static i32 PointInBounds(const LevelCoordRect* r, i32 x, i32 y);

    TileCollisionKind LookupTile(i32 x, i32 y);

    i32 ActivateVisibleObjectsOnMainPlane();
    i32 DeactivateDistantObjectsOnMainPlane();
    void MainPlaneNotify();

    void UpdatePlaneViewports(LevelCoordRect* coords);

    i32 ValidateAllPlanes(char* errOut);

    i32 SetViewportSizeAndUpdatePlanes(i32 w, i32 h);

    void SyncToMainIndex(CDDrawSurfacePair* visitor);

    void SyncAfterMainIndex(CDDrawSurfacePair* visitor);

    void ResetMainPlane();

    i32 DispatchMove(CGameObject* target, i32 destX, i32 destY, i32 moveFlags);

    i32 MoveGrounded(CGameObject* target, i32 destX, i32 destY, i32 moveFlags);
    i32 MoveRising(CGameObject* target, i32 destX, i32 destY, i32 moveFlags);
    i32 MoveFalling(CGameObject* target, i32 destX, i32 destY, i32 moveFlags);
    i32 MoveClimbing(CGameObject* target, i32 destX, i32 destY, i32 moveFlags);

    CDDrawWorkerHost* FindPlaneByName(const char* name);

    i32 MoveToward(CGameObject* target, i32 destX, i32 destY, i32 moveFlags);

    TileCollisionKind ProbeColumn(CGameObject* target, i32 dx);

    i32 WalkColumnDown(CGameObject* target, i32 unused);

    i32 ProbeHeadSoft(CGameObject* target, i32 dy);

    TileCollisionKind ProbeFeetKind(CGameObject* target, i32 dx);

    i32 ProbeSpanHard(CGameObject* target, i32 x, i32 off);

    void NotifyAllPlanes();

    i32 CanSaveName(CFileMemBase* s);
    i32 CanLoadName(CFileMemBase* s);

    void VisitVisible(CDDrawSurfacePair* visitor, CDDrawChildGroup* ctx);

    i32 SerializeDispatch(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, i32 payload);

    i32 SaveName(CFileMemBase* sink);
    i32 LoadName(CFileMemBase* sink);

    i32 MoveAxisAligned(CGameObject* t, i32 x, i32 y, i32 flags);
    i32 ApplyMove(CGameObject* target, i32 destX, i32 destY, i32 moveFlags);

    i32 MoveStepXHi(CGameObject* t, i32 x, i32 y, i32* px, i32 flags);
    i32 MoveStepXLo(CGameObject* t, i32 x, i32 y, i32* px, i32 flags);
    i32 MoveStepYHi(CGameObject* t, i32 x, i32 y, i32* py, i32 flags);
    i32 MoveStepYLo(CGameObject* t, i32 x, i32 y, i32* py, i32 flags);

    i32 BroadPhase(CGameObject* t, i32 candX, i32 candY);

public:
    CDDrawWorkerHost*
    ReadObjectPlane(i32 w, i32 h, i32 tileW, i32 tileH, i32 depthX, i32 depthY, const char* name);

private:
    CDDrawWorkerHost*
    ReadPlane(const WwdPlaneHeader* planeData, const char* blockBase, RECT* bounds);

    CTileImageSet* ReadImageSet(WwdTileImageRecord* record);

    i32 StepAxisLo(CGameObject* t, i32 destX, i32 destY, i32* outX, i32 moveFlags);
    i32 StepAxisHi(CGameObject* t, i32 destX, i32 destY, i32* outX, i32 moveFlags);
    i32 ResolveCeilingCollision(CGameObject* t, i32 destX, i32 destY, i32 moveFlags);
    i32 ClampSpan(i32 lo, i32 hi, i32* outLo, i32* outHi);
    i32 HoldMove(CGameObject* t, CGameObject* carrier, i32 destX, i32 destY, i32 moveFlags);
    i32 FreeMove(CGameObject* t, i32 destX, i32 destY, i32 moveFlags);
    i32
    TryLandOnPlatform(CGameObject* object, i32 destX, i32 destY, i32* outLandingY, i32 moveFlags);
    i32 ResolveFloorCollision(CGameObject* t, i32 destX, i32 destY, i32 moveFlags);
    i32 SpanCheck(i32 x, i32 yEndExclusive, i32 yBegin, i32* outY);
    TileCollisionKind AxisProbe(i32 coord, i32 limit);

    i32 CanLandOnPlatform(
        CGameObject* object,
        CGameObject* platform,
        i32 destX,
        i32 destY,
        i32* outLandingY,
        i32 moveFlags
    );

    i32 ReadImageSets(const u32* dir, char* cursor);

    i32 RemovePlane(i32 index);

    i32 MovePlane(i32 from, i32 to);

    i32 ScanSpanTop(CGameObject* t, i32 x, i32 y, i32 unused);
    i32 SnapFloorDown(CGameObject* t, i32 x, i32 y, i32* out);
    i32 SnapCeilUp(CGameObject* t, i32 x, i32 y, i32* out);
    i32 ResolveMoveDown(CGameObject* t, i32 x, i32 y, i32 flags);
    i32 ResolveMoveUp(CGameObject* t, i32 x, i32 y, i32 flags);
    i32 StepGroundDown(CGameObject* t, i32 x, i32 y, i32* out, i32 flags);
    i32 StepGroundUp(CGameObject* t, i32 x, i32 y, i32* out, i32 flags);
    i32 ProbeStepEdge(i32 x, i32 y);
    i32 ProbeFootSoft(CGameObject* t, i32 dx);
    i32 ProbeFootBlocked(CGameObject* t, i32 dx);
    i32 ScanRowSpan(i32 x0, i32 y, i32 x1, i32 step);

    i32 ResolveRightX(CGameObject* t, i32 x, i32 y);
    i32 ResolveLeftX(CGameObject* t, i32 x, i32 y);
    i32 ResolveBottomY(CGameObject* t, i32 x, i32 y);
    i32 ResolveTopY(CGameObject* t, i32 x, i32 y);

public:
    LevelCoordRect m_viewportRect;
    CObArray m_array20;
    CObArray m_planes;
    CObArray m_imageSets;
    CDDrawWorkerHost* m_mainPlane;
    i32 m_mainIndex;
    Coord m_maxStep;
    char m_levelName[0xac - 0x6c];
    u32 m_checksum;

    LevelDims m_defaultActiveGridCellSize;
    LevelDims m_largeActiveGridCellSize;
    LevelDims m_smallActiveGridCellSize;

    LevelDims m_defaultActiveRegionSize;
    LevelDims m_largeActiveRegionSize;
    LevelDims m_smallActiveRegionSize;
    WwdHeader m_header;
};

#endif // SRC_GRUNTZ_GAMELEVEL_H

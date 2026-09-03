#ifndef GRUNTZ_CDDRAWWORKERHOST_H
#define GRUNTZ_CDDRAWWORKERHOST_H

#include <rva.h>

#include <DDrawMgr/DDrawWorker.h>
#include <Enums.h>
#include <Gruntz/CoordNode.h>
#include <Gruntz/DoubleVector.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Ints.h>
#include <Wap32/WapObj.h>
#include <Wwd/WwdPlaneFlags.h>

#include <ddraw.h>

typedef struct tagRECT LevelCoordRect;

struct CWwdSpatialMgr;

class CDDrawSurfaceMgr;
class CDDrawSurfacePair;

class CDDrawWorker;

class CFileMemBase;

struct PlaneObjectRecord;
struct WwdPlaneHeader;

class CDDrawWorkerHost : public CWapObj {
public:
    CDDrawWorkerHost(CDDrawSurfaceMgr* owner, i32 id, i32 flags);
    virtual ~CDDrawWorkerHost() OVERRIDE;

    virtual i32 IsLoaded() OVERRIDE;

    virtual void Unload() OVERRIDE;
    virtual LoadableClassId GetClassId() OVERRIDE;

    virtual i32 InitGeometry(
        i32 tileColumns,
        i32 tileRows,
        i32 tileWidthPx,
        i32 tileHeightPx,
        i32 movementXPercent,
        i32 movementYPercent,
        LevelCoordRect* viewportRect,
        char* planeName
    );

    virtual i32
    Read(const WwdPlaneHeader* planeData, const char* blockBase, LevelCoordRect* bounds);

    virtual void UnusedPlaneHook(i32);

    void SetImageSetByName(char index, const char* key);

    void SetCell(i32 tileX, i32 tileY, i32 tileHandle);
    void UpdatePlaneViewRect();
    void SetViewportRect(LevelCoordRect* coords);
    void SetTileSize(i32 tileWidthPx, i32 tileHeightPx);

    void SetTileSizeFromImage(CImage* image);
    void SetTileSizeFromImageSet(CDDrawWorker* set);
    void Draw(CDDrawSurfacePair* ctx);
    i32 Prune();
    i32 ActivateVisibleObjects();
    i32 DeactivateDistantObjects();
    i32 ActivateKeepActiveObjects();
    i32 FlushAllObjects();
    void UpdateActiveRegionSizes();
    i32 ValidateTiles(char* errOut);
    void ResolveColorKey();

    i32 SerializeDispatch(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, i32 payload);
    i32 CanSave(CFileMemBase* s);
    i32 Save(CFileMemBase* s);
    i32 Load(CFileMemBase* s);
    i32 CanLoad(CFileMemBase* s);

    i32 RebuildPlanes(const char* base, i32 count);
    i32 ReadPlaneObjects(const PlaneObjectRecord* src);

    void WorldToViewport(LONG* px, LONG* py);

    void SnapToTileCenter(struct Coord* out, i32 x, i32 y);
    i32 GetTileHandle(i32 tileX, i32 tileY);

    CDDrawWorker* ImageSetAt(u32 index) {

        return static_cast<CDDrawWorker*>(m_imageSets[static_cast<int>(index)]);
    }

    FloatVector2 m_scrollCenter;
    FloatVector2 m_scrollScale;
    i32* m_tileHandles;
    i32* m_tileRowOffsets;
    SIZE
    m_tileGridSize;
    SIZE
    m_planePixelSize;
    SIZE
    m_tilePixelSize;
    RECT m_planeViewRect;

    LevelCoordRect m_viewportRect;
    RECT m_tileRect;

    SIZE
    m_viewportSize;
    SIZE
    m_viewHalfSize;
    i32 m_zCoord;
    Coord m_scrollPixel;
    Coord m_tileShift;
    Coord m_movementPercent;
    CObArray m_imageSets;

    CWwdSpatialMgr* m_spatialMgr;
    char m_planeName[0xf4 - 0xb4];

    DDBLTFX m_fillFx;
};

#define SET_SCROLL_POSITION_SCALED_FIRST(plane, x_, y_)                                            \
    if (!HAS(static_cast<WwdPlaneFlags>((plane)->m_flags), WWD_PLANE_FLAG_MAIN)) {                 \
        plane->m_scrollCenter.Init(                                                                \
            static_cast<float>(x_) * plane->m_scrollScale.x,                                       \
            static_cast<float>(y_) * plane->m_scrollScale.y                                        \
        );                                                                                         \
    } else {                                                                                       \
        plane->m_scrollCenter.Init(static_cast<float>(x_), static_cast<float>(y_));                \
    }                                                                                              \
    plane->UpdatePlaneViewRect()

#define SET_SCROLL_POSITION_RAW_FIRST(plane, x_, y_)                                               \
    if (HAS(static_cast<WwdPlaneFlags>((plane)->m_flags), WWD_PLANE_FLAG_MAIN)) {                  \
        plane->m_scrollCenter.Init(static_cast<float>(x_), static_cast<float>(y_));                \
    } else {                                                                                       \
        plane->m_scrollCenter.Init(                                                                \
            static_cast<float>(x_) * plane->m_scrollScale.x,                                       \
            static_cast<float>(y_) * plane->m_scrollScale.y                                        \
        );                                                                                         \
    }                                                                                              \
    plane->UpdatePlaneViewRect()

#define SET_SCROLL_POSITION_PRODUCT_CAST(plane, x_, y_)                                            \
    if (!HAS(static_cast<WwdPlaneFlags>((plane)->m_flags), WWD_PLANE_FLAG_MAIN)) {                 \
        plane->m_scrollCenter.Init(                                                                \
            static_cast<float>(x_ * plane->m_scrollScale.x),                                       \
            static_cast<float>(y_ * plane->m_scrollScale.y)                                        \
        );                                                                                         \
    } else {                                                                                       \
        plane->m_scrollCenter.Init(static_cast<float>(x_), static_cast<float>(y_));                \
    }                                                                                              \
    plane->UpdatePlaneViewRect()

#define SET_SCROLL_POSITION_ZERO(plane)                                                            \
    if (!HAS(static_cast<WwdPlaneFlags>((plane)->m_flags), WWD_PLANE_FLAG_MAIN)) {                 \
        plane->m_scrollCenter.Init(0.0f * plane->m_scrollScale.x, 0.0f * plane->m_scrollScale.y);  \
    } else {                                                                                       \
        plane->m_scrollCenter.Init(0.0f, 0.0f);                                                    \
    }                                                                                              \
    plane->UpdatePlaneViewRect()

#define SET_WORKER_HOST_CELL(plane, x, y, id)                                                      \
    (plane)->m_tileHandles[(plane)->m_tileRowOffsets[y] + x] = id

#endif // GRUNTZ_CDDRAWWORKERHOST_H

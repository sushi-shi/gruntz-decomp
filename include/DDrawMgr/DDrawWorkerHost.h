#ifndef GRUNTZ_CDDRAWWORKERHOST_H
#define GRUNTZ_CDDRAWWORKERHOST_H

#include <rva.h>

#include <DDrawMgr/DDrawWorker.h>
#include <Enums.h>
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

    float m_scrollCenterX;
    float m_scrollCenterY;
    float m_scrollScaleX;
    float m_scrollScaleY;
    i32* m_tileHandles;
    i32* m_tileRowOffsets;
    i32 m_tileColumns;
    i32 m_tileRows;
    i32 m_planePixelWidth;
    i32 m_planePixelHeight;
    i32 m_tileWidthPx;
    i32 m_tileHeightPx;
    RECT m_planeViewRect;

    LevelCoordRect m_viewportRect;
    RECT m_tileRect;

    i32 m_viewportWidth;
    i32 m_viewportHeight;
    i32 m_viewHalfWidth;
    i32 m_viewHalfHeight;
    i32 m_zCoord;
    i32 m_scrollPixelX;
    i32 m_scrollPixelY;
    i32 m_shiftX;
    i32 m_shiftY;
    i32 m_movementXPercent;
    i32 m_movementYPercent;
    CObArray m_imageSets;

    CWwdSpatialMgr* m_spatialMgr;
    char m_planeName[0xf4 - 0xb4];

    DDBLTFX m_fillFx;
};

#define SET_SCROLL_POSITION_SCALED_FIRST(plane, x, y)                                              \
    if (!HAS(static_cast<WwdPlaneFlags>((plane)->m_flags), WWD_PLANE_FLAG_MAIN)) {                 \
        plane->m_scrollCenterX = static_cast<float>(x) * plane->m_scrollScaleX;                    \
        plane->m_scrollCenterY = static_cast<float>(y) * plane->m_scrollScaleY;                    \
    } else {                                                                                       \
        plane->m_scrollCenterX = static_cast<float>(x);                                            \
        plane->m_scrollCenterY = static_cast<float>(y);                                            \
    }                                                                                              \
    plane->UpdatePlaneViewRect()

#define SET_SCROLL_POSITION_RAW_FIRST(plane, x, y)                                                 \
    if (HAS(static_cast<WwdPlaneFlags>((plane)->m_flags), WWD_PLANE_FLAG_MAIN)) {                  \
        plane->m_scrollCenterX = static_cast<float>(x);                                            \
        plane->m_scrollCenterY = static_cast<float>(y);                                            \
    } else {                                                                                       \
        plane->m_scrollCenterX = static_cast<float>(x) * plane->m_scrollScaleX;                    \
        plane->m_scrollCenterY = static_cast<float>(y) * plane->m_scrollScaleY;                    \
    }                                                                                              \
    plane->UpdatePlaneViewRect()

#define SET_SCROLL_POSITION_PRODUCT_CAST(plane, x, y)                                              \
    if (!HAS(static_cast<WwdPlaneFlags>((plane)->m_flags), WWD_PLANE_FLAG_MAIN)) {                 \
        plane->m_scrollCenterX = static_cast<float>(x * plane->m_scrollScaleX);                    \
        plane->m_scrollCenterY = static_cast<float>(y * plane->m_scrollScaleY);                    \
    } else {                                                                                       \
        plane->m_scrollCenterX = static_cast<float>(x);                                            \
        plane->m_scrollCenterY = static_cast<float>(y);                                            \
    }                                                                                              \
    plane->UpdatePlaneViewRect()

#define SET_SCROLL_POSITION_ZERO(plane)                                                            \
    if (!HAS(static_cast<WwdPlaneFlags>((plane)->m_flags), WWD_PLANE_FLAG_MAIN)) {                 \
        plane->m_scrollCenterX = 0.0f * plane->m_scrollScaleX;                                     \
        plane->m_scrollCenterY = 0.0f * plane->m_scrollScaleY;                                     \
    } else {                                                                                       \
        plane->m_scrollCenterX = 0.0f;                                                             \
        plane->m_scrollCenterY = 0.0f;                                                             \
    }                                                                                              \
    plane->UpdatePlaneViewRect()

#define SET_WORKER_HOST_CELL(plane, x, y, id)                                                      \
    (plane)->m_tileHandles[(plane)->m_tileRowOffsets[y] + x] = id

#endif // GRUNTZ_CDDRAWWORKERHOST_H

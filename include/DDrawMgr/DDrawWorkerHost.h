#ifndef GRUNTZ_CDDRAWWORKERHOST_H
#define GRUNTZ_CDDRAWWORKERHOST_H

#include <rva.h>

#include <DDrawMgr/DDrawWorker.h>
#include <Enums.h>
#include <Gruntz/Loadable.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Ints.h>

#include <ddraw.h>

typedef struct tagRECT LevelCoordRect;
SIZE_UNKNOWN();

struct CWwdSpatialMgr;

class CDDrawSurfaceMgr;
class CDDrawSurfacePair;

class CDDrawWorker;

class CFileMemBase;

struct PlaneObjectRecord;
struct WwdPlaneHeader;

class CDDrawWorkerHost : public CLoadable {
public:
    CDDrawWorkerHost(CDDrawSurfaceMgr* mapData, i32 field04, i32 flags);
    virtual ~CDDrawWorkerHost() OVERRIDE;

    void* operator new(size_t n) {
        return ::operator new(n);
    }

    virtual i32 IsLoaded() OVERRIDE;

    virtual void Unload() OVERRIDE;
    virtual LoadableClassId GetClassId() OVERRIDE;

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

    virtual i32
    Read(const WwdPlaneHeader* planeData, const char* blockBase, LevelCoordRect* bounds);

    virtual void UnusedPlaneHook(i32);

    void RegisterNamed(char index, const char* key);

    void SetCell(i32 x, i32 y, i32 id);
    void RecomputePlaneCoords();
    void Build(LevelCoordRect* coords);
    void SetTileSize(i32 tileW, i32 tileH);

    void SetTileSizeFromImageSet(CDDrawWorker* set);
    void Draw(CDDrawSurfacePair* ctx);
    i32 Prune();
    i32 ActivateVisibleObjects();
    i32 DeactivateDistantObjects();
    i32 GetSize();
    void InitScrollRects();
    i32 ValidateTiles(char* errOut);
    void ResolveColorKey();

    i32 SerializeDispatch(CFileMemBase* s, SerialMode kind, LogicTypeId typeId, i32 pObj);
    i32 Save(CFileMemBase* s);
    i32 Load(CFileMemBase* s);

    i32 RebuildPlanes(const char* base, i32 count);
    i32 ReadPlaneObjects(const PlaneObjectRecord* src);

    void WrapCoord(LONG* px, LONG* py);

    void SnapToTileCenter(struct Coord* out, i32 x, i32 y);
    i32 GetTileHandle(i32 row, i32 col);

    CDDrawWorker* FrameSetAt(u32 index) {

        return static_cast<CDDrawWorker*>(m_frameSets[static_cast<int>(index)]);
    }

    float m_scaledX;
    float m_scaledY;
    float m_scaleX;
    float m_scaleY;
    i32* m_tileGrid;
    i32* m_colOffsets;
    i32 m_gridW;
    i32 m_gridH;
    i32 m_wrapW;
    i32 m_wrapH;
    i32 m_tilePxW;
    i32 m_tilePxH;
    RECT m_viewRect;

    LevelCoordRect m_bounds50;
    RECT m_fillRect;

    i32 m_viewW;
    i32 m_viewH;
    i32 m_anchorX;
    i32 m_anchorY;
    i32 m_zBound;
    i32 m_snappedX;
    i32 m_snappedY;
    i32 m_shiftX;
    i32 m_shiftY;
    i32 m_movementXPercent;
    i32 m_movementYPercent;
    CObArray m_frameSets;

    CWwdSpatialMgr* m_scroll;
    char m_name[0xf4 - 0xb4];

    DDBLTFX m_bltFx;
};
SIZE(0x158);

SIZE_UNKNOWN();

#endif // GRUNTZ_CDDRAWWORKERHOST_H

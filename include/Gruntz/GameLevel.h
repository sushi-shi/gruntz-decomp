#ifndef SRC_GRUNTZ_GAMELEVEL_H
#define SRC_GRUNTZ_GAMELEVEL_H

#include <rva.h>

#include <Mfc.h>

#include <Enums.h>
#include <Gruntz/Loadable.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/TileCollisionKind.h>
#include <Wap32/Object.h>
#include <Wap32/WapObj.h>
#include <Wwd/WwdFile.h>

class CFileMemBase;
class CDDrawSurfacePair;

static const i32 TILE_CLEAR = -1;

#define PROBE_TILE(LVL, X, Y, RESULT)                                                              \
    do {                                                                                           \
        i32 py_ = (Y);                                                                             \
        i32 px_ = (X);                                                                             \
        if (px_ < 0) {                                                                             \
            px_ = 0;                                                                               \
        } else {                                                                                   \
            if (px_ >= (LVL)->m_mainPlane->m_wrapW) {                                              \
                px_ = (LVL)->m_mainPlane->m_wrapW - 1;                                             \
            }                                                                                      \
        }                                                                                          \
        if (py_ < 0) {                                                                             \
            py_ = 0;                                                                               \
        } else {                                                                                   \
            if (py_ >= (LVL)->m_mainPlane->m_wrapH) {                                              \
                py_ = (LVL)->m_mainPlane->m_wrapH - 1;                                             \
            }                                                                                      \
        }                                                                                          \
        CDDrawWorkerHost* pl_ = (LVL)->m_mainPlane;                                                \
        i32 qx_ = px_ >> pl_->m_shiftX;                                                            \
        i32 qy_ = py_ >> pl_->m_shiftY;                                                            \
        i32 col_ = qx_;                                                                            \
        i32 subX_ = px_ - (qx_ << pl_->m_shiftX);                                                  \
        i32 idx_ = pl_->m_colOffsets[qy_] + col_;                                                  \
        i32 subY_ = py_ - (qy_ << pl_->m_shiftY);                                                  \
        i32 tile_ = pl_->m_tileGrid[idx_];                                                         \
        if (tile_ == UNINIT_FILL || tile_ == TILE_CLEAR) {                                         \
            (RESULT) = TILEKIND_PASSABLE;                                                          \
        } else {                                                                                   \
            CTileImageSet* set_ = static_cast<CTileImageSet*>(m_imageSets[tile_ & 0xffff]);        \
            (RESULT) = set_->GetCollisionAt(subX_, subY_);                                         \
        }                                                                                          \
    } while (0)

#include <Gruntz/ImageSets.h>
#include <Wap32/CoordUnset.h>

struct CParseSource;

struct CGameObject;
class CDDrawChildGroup;
class CDDrawSurfaceMgr;

struct LevelDims {
    i32 w;
    i32 h;
};
SIZE(0x8);

GZ_ENUM_CONST_BEGIN(LevelPlaneLayout)
// ToggleObjectLayer selects the penultimate plane only in this layout;
// layouts with fewer planes select their final plane.
    LEVEL_EXTENDED_PLANE_COUNT = 4
GZ_ENUM_CONST_END(LevelPlaneLayout)

class CGameLevel : public CLoadable {
public:
    i32 IsValidWwd(const char* name, void* headerBuf);

    i32 ReadWwdHeaderName(const char* name, void* nameOut);

    virtual ~CGameLevel() OVERRIDE;
    virtual i32 IsLoaded() OVERRIDE;

    virtual void Unload() OVERRIDE;
    RVA(0x001611b0, 0x6)
    virtual LoadableClassId GetClassId() OVERRIDE {
        return CLASSID_GAMELEVEL;
    }
    virtual i32 LoadWwdWithCoords(WwdHeader* hdr, LevelCoordRect* coords);
    virtual i32 LoadSourceWithCoords(CParseSource* src, LevelCoordRect* coords);
    virtual i32 LoadFileWithCoords(const char* path, LevelCoordRect* coords);
    virtual i32 SetCoords(LevelCoordRect* coords);
    virtual i32 SetCoordExtents(i32 w, i32 h);
    virtual i32 LoadWwd(WwdHeader* hdr);
    virtual i32 LoadFromSource(CParseSource* arg);
    virtual i32 LoadFromFile(const char* path);
    virtual void ReleaseChildren();

    CGameLevel(class CDDrawSurfaceMgr* owner, i32 id, i32 flags);

    // The scroll/zoom parameter block's defaults. Retail inlines this at all
    // five uses and still emits the standalone __thiscall COMDAT at 0x15d170,
    // which has no caller at all - see the forcer at the end of GameLevel.cpp.
    void ResetParamBlock() {
        m_pairA[0] = 500;
        m_pairA[1] = 250;
        m_pairB[0] = 1000;
        m_pairB[1] = 1000;
        m_pairC[0] = 250;
        m_pairC[1] = 125;
        m_rectA.w = 1600;
        m_rectA.h = 1200;
        m_rectB.w = 2560;
        m_rectB.h = 1920;
        m_rectC.w = 768;
        m_rectC.h = 576;
    }

    static i32 PointInBounds(const LevelCoordRect* r, i32 x, i32 y);

    TileCollisionKind LookupTile(i32 x, i32 y);

    i32 ActivateVisibleObjectsOnMainPlane();
    i32 DeactivateDistantObjectsOnMainPlane();
    void MainPlaneNotify();

    void BuildAllPlanes(LevelCoordRect* coords);

    i32 ValidateAllPlanes(char* errOut);

    i32 SetExtentsAndBuildAll(i32 w, i32 h);

    void SyncToMainIndex(CDDrawSurfacePair* visitor);

    void SyncAfterMainIndex(CDDrawSurfacePair* visitor);

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

    void VisitVisible(CDDrawSurfacePair* visitor, CDDrawChildGroup* ctx);

    i32 EditDispatch(CFileMemBase* sink, SerialMode mode, LogicTypeId typeId, i32 pObj);

    i32 SaveName(CFileMemBase* sink);
    i32 LoadName(CFileMemBase* sink);

    i32 MoveAxisAligned(CGameObject* t, i32 x, i32 y, i32 flags);
    i32 ApplyMove(CGameObject* obj, i32 a, i32 b, i32 c);

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
    ReadPlane(const WwdPlaneHeader* planeData, const char* blockBase, void* unused);

    CTileImageSet* ReadImageSet(void* record);

    i32 StepAxisLo(CGameObject* t, i32 destX, i32 destY, i32* outX, i32 moveFlags);
    i32 StepAxisHi(CGameObject* t, i32 destX, i32 destY, i32* outX, i32 moveFlags);
    i32 ResolveCeilingCollision(CGameObject* t, i32 destX, i32 destY, i32 moveFlags);
    i32 ClampSpan(i32 lo, i32 hi, i32* outLo, i32* outHi);
    i32 HoldMove(CGameObject* t, CGameObject* carrier, i32 destX, i32 destY, i32 moveFlags);
    i32 FreeMove(CGameObject* t, i32 destX, i32 destY, i32 moveFlags);
    i32 StepAxisAlt(CGameObject* t, i32 destX, i32 destY, i32* outY, i32 moveFlags);
    i32 ResolveFloorCollision(CGameObject* t, i32 destX, i32 destY, i32 moveFlags);
    i32 SpanCheck(i32 a, i32 b, i32 c, i32* out);
    TileCollisionKind AxisProbe(i32 coord, i32 limit);

    i32 AltStepValidate(
        CGameObject* t,
        CGameObject* payload,
        i32 destX,
        i32 destY,
        i32* outY,
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
    LevelCoordRect m_planeCtx;
    CObArray m_array20;
    CObArray m_planes;
    CObArray m_imageSets;
    CDDrawWorkerHost* m_mainPlane;
    i32 m_mainIndex;
    i32 m_maxStepX;
    i32 m_maxStepY;
    char m_levelName[0xac - 0x6c];
    u32 m_checksum;

    i32 m_pairA[2];
    i32 m_pairB[2];
    i32 m_pairC[2];

    LevelDims m_rectA;
    LevelDims m_rectB;
    LevelDims m_rectC;
    WwdHeader m_header;
};
SIZE(0x6d4);

int WapUncompress(
    unsigned char* dest,
    unsigned long* pDestLen,
    unsigned char* src,
    unsigned long srcLen
);

#endif // SRC_GRUNTZ_GAMELEVEL_H

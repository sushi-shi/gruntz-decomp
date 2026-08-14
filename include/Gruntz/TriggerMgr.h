#ifndef SRC_GRUNTZ_TRIGGERMGR_H
#define SRC_GRUNTZ_TRIGGERMGR_H

#include <rva.h>

#include <Mfc.h>

#include <Gruntz/CombatCueKind.h>
#include <Gruntz/CoordNode.h>
#include <Gruntz/CurPlayer.h>
#include <Gruntz/FinishLevelReason.h>
#include <Gruntz/FreeNodePool.h>
#include <Gruntz/GruntDeathType.h>
#include <Gruntz/GruntEntranceMode.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/TargetSelectionKind.h>
#include <Gruntz/WarpStoneFragment.h>
#include <Wwd/WwdAniDrawValue.h>

void operator delete(void*);

// Grid extents, used only as array dimensions and in index arithmetic.
GZ_ENUM_CONST_BEGIN(TmGridDim)
    TM_GRID_COLS = 15,
    TM_GRID_ROWS = 4,
    // Not a row: the "every player" selector three of the row-ranged walks
    // accept in place of one. Each opens with the same two lines - if the
    // argument is this, sweep rows 0..3; otherwise sweep just that row - so it
    // sits one past the last row rather than inside the range.
    TM_GRID_ROW_ALL = 5
GZ_ENUM_CONST_END(TmGridDim)

class CGrunt;
class CWarlord;
struct CGameObject;

class CDDrawSurfaceMgr;
class DirectSoundMgr;
struct CTmOverlay;
class CWwdGameObjectA;
class CActionOptionsMenuBar;

class CGruntPuddle;

struct CueTimer {
    i64 m_base;
    i64 m_window;
};

class CTriggerMgr {
public:
    i32 Load(CFileMemBase* ar);

    i32 SetLevel(CDDrawSurfaceMgr* lvl);

    i32 ScrollToActiveRecord();

    void OverlayTick();

    i32 OverlayRelease();

    i32 ByteTableHas(WarpStoneFragment fragment);

    void ResetAll();

    i32 RecordListHas(i32 x, i32 y);

    i32 ClearRow(i32 row);

    i32 SelectionListFind(i32 key, i32 y);

    void StopPendingFx();

    void ClearSelections();

    void ClearRecords();

    i32 DispatchCellForObject(CGrunt* obj, i32 startRow, GruntDeathType kind, i32 arg);

    i32 CellDispatch(i32 row, i32 col, GruntDeathType kind, i32 arg);

    void NotifyCell(i32 row, i32 col, i32 z);

    CGrunt* CellHitTest(i32 px, i32 py, i32* outRow, i32* outCol, i32 startRow);

    CGrunt* ScreenToCell(i32 sx, i32 sy, i32* outRow, i32* outCol, i32 startRow);

    void Cleanup();

    i32 NearestCellDist(i32 skipRow, i32 px, i32 py);

    void DestroyAllAnims();

    i32 ClearGridRange(i32 startRow);

    i32 ClearRowAndRefresh(i32 startRow);

    CGrunt* FindNearestInRow(CGrunt* g);

    i32 RemoveCellRecord(i32 x, i32 y, i32 fromSelection);

    i32 SpawnPuddle(i32 x, i32 y, i32 f124, i32 f114, i32 color, i32 f118);

    i32 PlacePuddle(CGameObject* sprite, i32 color);

    i32 PlaceObject(
        i32 row,
        i32 x,
        i32 y,
        i32 z,
        GruntEntranceMode mode,
        i32 kindDefault,
        i32 typeKind,
        i32 vehicleKind,
        i32 aiType,
        i32 aiRadius,
        i32 placeArg9,
        i32 placeArg10,
        i32 spanWord
    );

    i32 ResetCell(i32 col, i32 row, i32 force, i32 keep);

    i32 LoadCameraSprite();

    i32 ApplySwitch(CGrunt* g, i32 sx, i32 sy);

    i32 ApplyTriggerA(i32 col, i32 row, i32 worldX, i32 worldY);
    i32 ApplyTriggerB(i32 col, i32 row, i32 worldX, i32 worldY);

    i32 ClearCell(i32 col, i32 row, i32 worldX, i32 worldY, i32 arrivalPhase);

    union HitSpanArg {
        RECT* m_span;
        i32 m_outCol;
    };
    void HitTestApply(i32 x, i32 y, HitSpanArg span);

    CGrunt* HitTestCell(i32 x, i32 y, i32* outRow, i32* outCol, i32 exact);

    CGrunt* FindGruntAt(i32 px, i32 py, RECT* span, i32* outCol, i32* outRow, RECT* src);

    void ReportRecordsA(i32 tag, i32 gx, i32 gy);
    void ReportRecordsB(i32 tag, i32 gx, i32 gy, i32 flag);

    i32 PlaceObjectFull(i32 x, i32 y);

    void GridAction6(i32 hi, i32 lo);
    void GridAction7(i32 hi, i32 lo);

    i32 ResetGroup(
        i32 x,
        i32 y,
        i32 worldX,
        i32 worldY,
        i32 unused5,
        TargetSelectionKind selector,
        i32 spawnCursor
    );

    i32 DestroyGroup(i32 screenX, i32 screenY, i32 worldX, i32 worldY);

    void ReinitGroup(i32 col, i32 row);

    i32 Serialize(CFileMemBase* ar, SerialMode kind, LogicTypeId unusedC, i32 unusedD);

    i32 ScanGroup(CFileMemBase* ar);

    i32 TriggerCell(i32 x, i32 y);

    i32 SpawnGrunt(i32 col, i32 row, i32 a18, i32 a1c);

    void ResetSpawnState();

    i32 CycleMoveIcons(i32 skipRow, i32 enable);

    i32 RebuildSelectionList(i32 idx);

    i32 CenterSelectionGroup(i32 slot);

    i32 ToggleRegionA();
    i32 ToggleRegionB();

    i32 EnqueueGroupCells();

    void HudRect(RECT r, i32 flag);

    i32 LoadTeleporterGooConfig(i32 clock);

    void LoadFinishLevelSprite(FinishLevelReason state);

    i32 LoadGruntResurrectTuning(i32 cx, i32 cy, i32 r);

    CGrunt* FindNearestEnemy(CGrunt* g);

    i32 CenterOnGroup(i32 doSelect);

    i32 LoadTileArrivalFx(
        i32 ownerHi,
        i32 ownerLo,
        i32 tileX,
        i32 tileY,
        PickupType reason,
        WwdAniDrawValue cue
    );

    i32 CombatCue(i32 x, i32 y, i32 radius, CombatCueKind tier, i32 flag);

    i32 BuildRockBreakParticles(i32 cx, i32 cy, i32 r, i32 flag);

    CGrunt* FindAtPixel(i32 x, i32 y);

    i32 WireTileSwitchLogic(CGrunt* g, i32 x, i32 y);

    // Zero every POD member: the class has no retail ctor symbol, so cl
    // inlines this at `new CTriggerMgr` exactly as retail does. Without it
    // m_grid/m_overlay are recycled heap and map load faults.
    CTriggerMgr() {
        // Every POD member: the class had NO constructor, so `new CTriggerMgr` left
        // all of this as recycled heap. m_grid's garbage cells crashed
        // ClearGridRange and m_overlay's garbage crashed Deactivate. The embedded
        // CPtrList/CByteArray members construct themselves.
        memset(m_grid, 0, sizeof(m_grid));
        memset(m_rowCount, 0, sizeof(m_rowCount));
        memset(m_cellFlag, 0, sizeof(m_cellFlag));
        memset(m_gruntzExitedByPlayer, 0, sizeof(m_gruntzExitedByPlayer));
        memset(m_gruntzLostByPlayer, 0, sizeof(m_gruntzLostByPlayer));
        m_world = NULL;
        m_armed = 0;
        memset(&m_recordPosition, 0, sizeof(m_recordPosition));
        m_goal = NULL;
        m_overlay = NULL;
        memset(m_reserved274, 0, sizeof(m_reserved274));
        m_groupInitialized = 0;
        m_phase = static_cast<FinishLevelState>(0);
        m_timerBase = 0;
        m_timerWindow = 0;
        m_pendingFx = NULL;
        m_countdownActive = 0;
        m_pendingFxKind = 0;
        m_gooTimerBaseLo = 0;
        m_gooTimerBaseHi = 0;
        m_gooIntervalLo = 0;
        m_gooIntervalHi = 0;
        m_resourceTimerBaseLo = 0;
        m_resourceTimerBaseHi = 0;
        m_resourceIntervalLo = 0;
        m_resourceIntervalHi = 0;
        m_selSentinel = 0;
        m_finishReasonFrame = static_cast<FinishLevelReason>(0);
        m_rollingballLoop = NULL;
        m_teleportLoop = NULL;
        m_rollingballWanted = 0;
        m_teleportWanted = 0;
        m_groupFlag = 0;
        g_curPlayer = 0;
    }
    RVA(0x00085c50, 0x83)
    ~CTriggerMgr() {
        Cleanup();
    }

    void ReportN(i32 a, i32 b, u8* bytes, i32 c, i32 d, i32 e, i32 f);
    i32 PlaceB(i32 a, i32 b, i32 c);

    i32
    LoadPowerupIconSprites(PickupType type, i32 geoB, i32 geoA, i32 m130, i32 warpIdx, i32 m120);

    i32 SpawnTileFx(i32 x, i32 y, i32 anchorIndex);

    i32 LoadExplosionSprites(i32 x, i32 y, i32 id, i32 kind);

    i32 LoadToyBoxIcon(i32 x, i32 y, i32 col, PickupType kind, i32 moveKind);

    CPtrList m_baseList;
    CGrunt* m_grid[0x3c];
    i32 m_rowCount[4];
    i32 m_cellFlag[0x3c];

    i32 m_gruntzExitedByPlayer[4];
    i32 m_gruntzLostByPlayer[4];

    CDDrawSurfaceMgr* m_world;

    i32 m_armed;
    Coord m_recordPosition;
    CWwdGameObjectA* m_goal;

    CPtrList m_recList;

    Coord* HeadRec() {
        return static_cast<Coord*>(m_recList.GetHead());
    }
    CActionOptionsMenuBar* m_overlay;
    CByteArray m_byteArr;
    char m_reserved274[0x10];
    i32 m_groupInitialized;

    FinishLevelState m_phase;
    char _pad28c[0x4];

    union {
        CueTimer m_cueTimer;
        struct {
            i64 m_timerBase;
            i64 m_timerWindow;
        };
    };

    CWarlord* m_pendingFx;
    i32 m_countdownActive;
    i32 m_pendingFxKind;
    char _pad2ac[0x4];

    // Four 64-bit timers.  LoadTeleporterGooConfig compares them with a real
    // sub/sbb/cmp/cmp pair and writes BOTH halves on every rearm - so they are
    // i64, not the low words alone.
    union {
        i64 m_gooTimerBase;
        struct {
            i32 m_gooTimerBaseLo, m_gooTimerBaseHi;
        };
    };
    union {
        i64 m_gooInterval;
        struct {
            i32 m_gooIntervalLo, m_gooIntervalHi;
        };
    };
    union {
        i64 m_resourceTimerBase;
        struct {
            i32 m_resourceTimerBaseLo, m_resourceTimerBaseHi;
        };
    };
    union {
        i64 m_resourceInterval;
        struct {
            i32 m_resourceIntervalLo, m_resourceIntervalHi;
        };
    };
    CPtrList m_selLists[10];
    i32 m_selSentinel;
    FinishLevelReason m_finishReasonFrame;

    DirectSoundMgr* m_rollingballLoop;
    DirectSoundMgr* m_teleportLoop;
    i32 m_rollingballWanted;
    i32 m_teleportWanted;
    i32 m_groupFlag;
};

extern i32 g_groupSentinel;

#endif

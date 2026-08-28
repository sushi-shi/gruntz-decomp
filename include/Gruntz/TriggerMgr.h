#ifndef SRC_GRUNTZ_TRIGGERMGR_H
#define SRC_GRUNTZ_TRIGGERMGR_H

#include <rva.h>

#include <Mfc.h>

#include <Gruntz/CoordNode.h>
#include <Gruntz/CurPlayer.h>
#include <Gruntz/FinishLevelReason.h>
#include <Gruntz/FreeNodePool.h>
#include <Gruntz/GruntAreaEffectKind.h>
#include <Gruntz/GruntDeathType.h>
#include <Gruntz/GruntEntranceMode.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/TargetSelectionKind.h>
#include <Gruntz/WarpStoneFragment.h>
#include <Ints.h>
#include <Wwd/WwdAniDrawValue.h>

GZ_ENUM_CONST_BEGIN(TmGridDim)
    TM_UNITS_PER_PLAYER = 15,
    TM_PLAYER_COUNT = 4,
    TM_ALL_PLAYERS = 5
GZ_ENUM_CONST_END(TmGridDim)

class CGrunt;
class CWarlord;
struct CGameObject;

class CDDrawSurfaceMgr;
class SoundBuffer;
struct CTmOverlay;
class CWwdSpriteObject;
class CActionOptionsMenuBar;

class CGruntPuddle;

struct CueTimer {
    i64 m_base;
    i64 m_window;

    CueTimer() : m_base(0), m_window(0) {}
};

class CTriggerMgr {
public:
    i32 Load(CFileMemBase* ar);

    i32 SetLevel(CDDrawSurfaceMgr* lvl);

    i32 ScrollToActiveRecord();

    void CloseActionOptionsMenu();

    i32 RenderActionOptionsMenu();

    i32 ByteTableHas(WarpStoneFragment fragment);

    void ResetAll();

    i32 RecordListHas(i32 playerIndex, i32 unitIndex);

    i32 StartPlayerVictorySequence(i32 playerIndex);

    i32 SelectionListFind(i32 playerIndex, i32 unitIndex);

    void StopPendingFx();

    void ClearSelections();

    void ClearRecords();

    i32 StartUnitDeathForObject(
        CGrunt* unit,
        i32 playerSelector,
        GruntDeathType deathType,
        i32 deathParam
    );

    i32 StartUnitDeath(i32 playerIndex, i32 unitIndex, GruntDeathType deathType, i32 deathParam);

    void UnregisterUnit(i32 playerIndex, i32 unitIndex, i32 exitedLevel);

    CGrunt*
    CellHitTest(i32 px, i32 py, i32* outPlayerIndex, i32* outUnitIndex, i32 startPlayerIndex);

    CGrunt*
    ScreenToCell(i32 sx, i32 sy, i32* outPlayerIndex, i32* outUnitIndex, i32 startPlayerIndex);

    void Cleanup();

    i32 NearestOtherPlayerUnitDistSq(i32 skipPlayerIndex, i32 px, i32 py);

    void DestroyAllAnims();

    i32 RemovePlayerUnitsImmediately(i32 playerSelector);

    i32 StartPlayerDefeatSequence(i32 playerSelector);

    CGrunt* FindNearestUnitForPlayer(CGrunt* g);

    i32 RemoveCellRecord(i32 playerIndex, i32 unitIndex, i32 fromSelection);

    i32
    SpawnPuddle(i32 x, i32 y, i32 playerIndex, i32 moveIcon, b32 animatePlacement, i32 gaugePoints);

    i32 PlacePuddle(CGameObject* sprite, b32 animatePlacement);

    i32 PlaceObject(
        i32 playerIndex,
        i32 x,
        i32 y,
        i32 z,
        GruntEntranceMode mode,
        i32 kindDefault,
        i32 typeKind,
        i32 vehicleKind,
        i32 aiType,
        i32 defenderRadiusMinusOne,
        i32 defenderQueuePosition,
        i32 defenderPickupType,
        RECT* span
    );

    i32 ResetCell(i32 playerIndex, i32 unitIndex, i32 force, i32 keep);

    i32 LoadCameraSprite();

    i32 ApplySwitch(CGrunt* g, i32 sx, i32 sy);

    i32 UseEquippedToolAt(i32 playerIndex, i32 unitIndex, i32 worldX, i32 worldY);
    i32 UseToyAt(i32 playerIndex, i32 unitIndex, i32 worldX, i32 worldY);

    i32 ClearCell(i32 playerIndex, i32 unitIndex, i32 worldX, i32 worldY, i32 arrivalPhase);

    union HitSpanArg {
        RECT* m_span;
        i32 m_outPlayerIndex;
    };
    void HitTestApply(i32 x, i32 y, HitSpanArg span);

    CGrunt* HitTestCell(i32 x, i32 y, i32* outPlayerIndex, i32* outUnitIndex, i32 exact);

    CGrunt*
    FindGruntAt(i32 px, i32 py, RECT* span, i32* outPlayerIndex, i32* outUnitIndex, RECT* src);

    void EnqueueSelectedMove(b32 isLocalCommand, i32 targetX, i32 targetY);
    void EnqueueSelectedToolUse(b32 isLocalCommand, i32 targetX, i32 targetY, b32 targetIsGrunt);

    i32 PlaceObjectFull(i32 x, i32 y);

    void EnqueueGuardBegin(i32 playerIndex, i32 unitIndex);
    void EnqueueGuardEnd(i32 playerIndex, i32 unitIndex);

    i32 HandleTargetSelection(
        i32 targetX,
        i32 targetY,
        i32 pointerX,
        i32 pointerY,
        i32 unused5,
        TargetSelectionKind selector,
        i32 spawnCursor
    );

    i32 OpenActionOptionsMenu(i32 selectedWorldX, i32 selectedWorldY, i32 pointerX, i32 pointerY);

    void ReinitGroup(i32 col, i32 row);

    i32 Serialize(CFileMemBase* ar, SerialMode mode, LogicTypeId unusedTypeId, i32 unusedPayload);

    i32 ScanGroup(CFileMemBase* ar);

    i32 HandleActionOptionsPointer(i32 x, i32 y);

    i32 SpawnGrunt(i32 srcPlayerIndex, i32 srcUnitIndex, i32 dstPlayerIndex, i32 moveIcon);

    void ResetSpawnState();

    i32 CycleMoveIcons(i32 skipPlayerIndex, b32 enable);

    i32 RebuildSelectionList(i32 idx);

    i32 CenterSelectionGroup(i32 slot);

    i32 ToggleToolTargeting();
    i32 ToggleToyTargeting();

    i32 EnqueueGroupCells();

    void HudRect(RECT r, b32 selectionReset);

    i32 UpdateFrame(i32 deltaMs);

    void LoadFinishLevelSprite(FinishLevelReason state);

    i32 LoadGruntResurrectTuning(i32 cx, i32 cy, i32 r);

    CGrunt* FindNearestEnemy(CGrunt* g);

    i32 CenterOnGroup(i32 doSelect);

    i32 LoadTileArrivalFx(
        i32 playerIndex,
        i32 unitIndex,
        i32 tileX,
        i32 tileY,
        PickupType reason,
        WwdAniDrawValue cue
    );

    i32
    ApplyGruntAreaEffect(i32 x, i32 y, i32 radiusTiles, GruntAreaEffectKind effect, i32 deathParam);

    i32 BuildRockBreakParticles(i32 cx, i32 cy, i32 r, i32 flag);

    CGrunt* FindAtPixel(i32 x, i32 y);

    i32 WireTileSwitchLogic(CGrunt* g, i32 x, i32 y);

    CTriggerMgr() {
        g_curPlayer = 0;
        memset(m_units, 0, sizeof(m_units));
        memset(m_unitCountByPlayer, 0, sizeof(m_unitCountByPlayer));
        memset(m_unitExited, 0, sizeof(m_unitExited));
        memset(m_gruntzExitedByPlayer, 0, sizeof(m_gruntzExitedByPlayer));
        memset(m_gruntzLostByPlayer, 0, sizeof(m_gruntzLostByPlayer));
        m_selSentinel = -1;
        m_goal = NULL;
        m_overlay = NULL;
        m_world = NULL;
        m_countdownActive = true;
        m_groupFlag = true;
        m_rollingballLoop = NULL;
        m_teleportLoop = NULL;
        m_rollingballWanted = false;
        m_teleportWanted = false;
    }
    RVA(0x00085c50, 0x83)
    ~CTriggerMgr() {
        Cleanup();
    }

    void ReportN(i32 a, i32 b, u8* bytes, i32 c, i32 d, i32 e, i32 f);

    i32 SpawnPowerupIcon(
        PickupType type,
        i32 x,
        i32 y,
        i32 faceDirection,
        i32 warpstoneVariant,
        i32 damage
    );

    i32 SpawnTileFx(i32 x, i32 y, i32 anchorIndex);

    i32 LoadExplosionSprites(i32 x, i32 y, i32 id, i32 kind);

    i32 LoadToyBoxIcon(i32 x, i32 y, i32 col, PickupType kind, i32 moveKind);

    CPtrList m_baseList;
    CGrunt* m_units[TM_PLAYER_COUNT * TM_UNITS_PER_PLAYER];
    i32 m_unitCountByPlayer[TM_PLAYER_COUNT];
    i32 m_unitExited[TM_PLAYER_COUNT * TM_UNITS_PER_PLAYER];

    i32 m_gruntzExitedByPlayer[TM_PLAYER_COUNT];
    i32 m_gruntzLostByPlayer[TM_PLAYER_COUNT];

    CDDrawSurfaceMgr* m_world;

    b32 m_armed;
    Coord m_cameraTargetIdentity;
    CWwdSpriteObject* m_goal;

    CPtrList m_recList;

    Coord* HeadRec() {
        return static_cast<Coord*>(m_recList.GetHead());
    }
    CActionOptionsMenuBar* m_overlay;
    CByteArray m_byteArr;
    char m_reserved274[0x10];
    b32 m_groupInitialized;

    FinishLevelState m_phase;
    char _pad28c[0x4];

    CueTimer m_cueTimer;

    CWarlord* m_pendingFx;
    b32 m_countdownActive;
    i32 m_pendingFxKind;
    char _pad2ac[0x4];

    CueTimer m_gooTimer;
    CueTimer m_resourceTimer;
    CPtrList m_selLists[10];
    i32 m_selSentinel;
    FinishLevelReason m_finishReasonFrame;

    SoundBuffer* m_rollingballLoop;
    SoundBuffer* m_teleportLoop;
    b32 m_rollingballWanted;
    b32 m_teleportWanted;
    b32 m_groupFlag;
};

extern i32 g_groupSentinel;

#endif

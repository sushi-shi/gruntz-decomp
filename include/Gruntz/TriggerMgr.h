#ifndef SRC_GRUNTZ_TRIGGERMGR_H
#define SRC_GRUNTZ_TRIGGERMGR_H

#include <rva.h>

#include <Gruntz/ClockInterval.h>
#include <Gruntz/CoordNode.h>
#include <Gruntz/CoordPool.h>
#include <Gruntz/CurPlayer.h>
#include <Gruntz/FinishLevelReason.h>
#include <Gruntz/GruntAreaEffectKind.h>
#include <Gruntz/GruntDeathType.h>
#include <Gruntz/GruntEntranceMode.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/PlayerSlot.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/TargetSelectionKind.h>
#include <Gruntz/TriggerGridDimensions.h>
#include <Gruntz/WarpStoneFragment.h>
#include <Ints.h>
#include <Wwd/WwdAniDrawValue.h>

class CGrunt;
class CWarlord;
struct CGameObject;

class CGameWorld;
class SoundBuffer;
struct CTmOverlay;
class CWwdSpriteObject;
class CActionOptionsMenuBar;

class CGruntPuddle;

class CTriggerMgr {
public:
    FinishLevelReason GetFinishReason() const {
        return m_finishReason;
    }

    CWarlord* GetLocalWarlord() const {
        return m_localWarlord;
    }
    void SetLocalWarlord(CWarlord* warlord) {
        m_localWarlord = warlord;
    }

    FinishLevelState GetFinishState() const {
        return m_finishState;
    }

    i32 Load(CFileMemBase* ar);

    i32 SetWorld(CGameWorld* world);

    i32 UpdateCameraTracking();

    void CloseActionOptionsMenu();

    i32 RenderActionOptionsMenu();

    i32 HasWarpStoneFragment(WarpStoneFragment fragment);
    void AddWarpStoneFragment(WarpStoneFragment fragment) {
        m_collectedWarpStoneFragments.Add(static_cast<u8>(IDX(fragment)));
    }

    void ClearSelection();

    i32 IsUnitSelected(i32 playerIndex, i32 unitIndex);

    i32 StartPlayerVictorySequence(i32 playerIndex);

    i32 GetUnitSelectionGroupMarker(i32 playerIndex, i32 unitIndex);

    b32 IsPlayerControlEnabled() const {
        return m_playerControlEnabled;
    }

    b32 IsTargeting() const {
        return m_targetingCursorId != 0;
    }
    void CancelTargeting();

    void ClearSelectionGroups();

    void ClearSelectedUnitIds();

    i32 StartUnitDeathForObject(
        CGrunt* unit,
        i32 playerSelector,
        GruntDeathType deathType,
        i32 deathParam
    );

    i32 StartUnitDeath(i32 playerIndex, i32 unitIndex, GruntDeathType deathType, i32 deathParam);

    void UnregisterUnit(i32 playerIndex, i32 unitIndex, i32 exitedLevel);

    CGrunt* PickGruntAtWorldPoint(
        i32 worldX,
        i32 worldY,
        i32* outPlayerIndex,
        i32* outUnitIndex,
        i32 playerSelector
    );

    CGrunt* PickGruntAtScreenPoint(
        i32 screenX,
        i32 screenY,
        i32* outPlayerIndex,
        i32* outUnitIndex,
        i32 playerSelector
    );

    void Cleanup();

    i32 NearestOtherPlayerUnitDistSq(i32 skipPlayerIndex, i32 px, i32 py);

    void DestroyAllAnims();

    i32 RemovePlayerUnitsImmediately(i32 playerSelector);

    i32 StartPlayerDefeatSequence(i32 playerSelector);

    CGrunt* FindNearestUnitForPlayer(CGrunt* g);

    i32 RemoveUnitFromSelection(i32 playerIndex, i32 unitIndex, i32 removeFromGroups);

    i32 SpawnPuddle(
        i32 x,
        i32 y,
        i32 playerIndex,
        i32 colorIndex,
        b32 animatePlacement,
        i32 gaugePoints
    );

    i32 PlacePuddle(CGameObject* sprite, b32 animatePlacement);

    POSITION GetPuddleHeadPosition() const {
        return m_puddles.GetHeadPosition();
    }
    CGruntPuddle* GetNextPuddle(POSITION& position) {
        return static_cast<CGruntPuddle*>(m_puddles.GetNext(position));
    }
    CGruntPuddle* GetPuddleAt(POSITION position) {
        return static_cast<CGruntPuddle*>(m_puddles.GetAt(position));
    }
    void RemovePuddleAt(POSITION position) {
        m_puddles.RemoveAt(position);
    }

    i32 SpawnGrunt(
        i32 playerIndex,
        i32 x,
        i32 y,
        i32 z,
        GruntEntranceMode mode,
        i32 defaultColorIndex,
        i32 typeKind,
        i32 carriedToyType,
        i32 aiType,
        i32 defenderRadiusMinusOne,
        i32 defenderQueuePosition,
        i32 defenderPickupType,
        RECT* span
    );

    i32 SelectUnit(i32 playerIndex, i32 unitIndex, i32 extendSelection, i32 keepSelected);

    i32 LoadCameraSprite();
    void SetCameraTarget(i32 playerIndex, i32 unitIndex) {
        Coord target = {playerIndex, unitIndex};
        m_cameraTargetIdentity = target;
        m_cameraTrackingActive = true;
        LoadCameraSprite();
    }
    void ClearCameraSprite();
    void StopCameraTracking();

    i32 ApplySwitch(CGrunt* g, i32 sx, i32 sy);

    i32 UseEquippedToolAt(i32 playerIndex, i32 unitIndex, i32 worldX, i32 worldY);
    i32 UseToyAt(i32 playerIndex, i32 unitIndex, i32 worldX, i32 worldY);

    i32 MoveUnitTo(i32 playerIndex, i32 unitIndex, i32 worldX, i32 worldY, i32 arrivalAction);

    union HitSpanArg {
        RECT* m_span;
        i32 m_outPlayerIndex;
    };
    void CheckWarpStoneExit(i32 x, i32 y, HitSpanArg span);

    CGrunt* FindGruntAtPoint(
        i32 worldX,
        i32 worldY,
        i32* outPlayerIndex,
        i32* outUnitIndex,
        i32 requireExactPosition
    );

    CGrunt* FindGruntInArea(
        i32 worldX,
        i32 worldY,
        RECT* tileExtents,
        i32* outPlayerIndex,
        i32* outUnitIndex,
        RECT* collisionBounds
    );

    void EnqueueSelectedMove(b32 isLocalCommand, i32 targetX, i32 targetY);
    void EnqueueSelectedToolUse(b32 isLocalCommand, i32 targetX, i32 targetY, b32 targetIsGrunt);

    i32 UpdateTargetingCursor(i32 x, i32 y);

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

    void CollectLevelWarpStone(i32 worldX, i32 worldY);

    i32 Serialize(CFileMemBase* ar, SerialMode mode, LogicTypeId unusedTypeId, i32 unusedPayload);

    i32 Save(CFileMemBase* ar);

    i32 HandleActionOptionsPointer(i32 x, i32 y);

    i32 ConvertGrunt(i32 srcPlayerIndex, i32 srcUnitIndex, i32 dstPlayerIndex, i32 colorIndex);

    void LoseLevelWarpStone();

    i32 SetRandomGruntColors(i32 skipPlayerIndex, b32 enable);

    i32 SaveSelectionGroup(i32 idx);

    i32 RecallSelectionGroup(i32 slot);

    i32 ToggleToolTargeting();
    i32 ToggleToyTargeting();

    i32 EnqueueSelectedStop();

    void SelectUnitsInRect(RECT selectionRect, b32 preserveSelection);

    i32 UpdateFrame(i32 deltaMs);

    void BeginLevelFinish(FinishLevelReason reason);
    void StartFinishDelay(u32 durationMs) {
        m_finishDelayTiming.Start(durationMs);
    }

    i32 ResurrectGruntsInArea(i32 centerX, i32 centerY, i32 radiusTiles);

    CGrunt* FindNearestEnemy(CGrunt* g);

    i32 CenterOnGroup(i32 doSelect);

    i32 HandleToolAnimationCue(
        i32 playerIndex,
        i32 unitIndex,
        i32 tileX,
        i32 tileY,
        PickupType toolType,
        AnimationEventCode cue
    );

    i32
    ApplyGruntAreaEffect(i32 x, i32 y, i32 radiusTiles, GruntAreaEffectKind effect, i32 deathParam);

    i32 ApplyExplosion(i32 centerX, i32 centerY, i32 radiusTiles, i32 killerPlayerIndex);

    CGrunt* FindAtPixel(i32 x, i32 y);

    i32 WireTileSwitchLogic(CGrunt* g, i32 x, i32 y);

    CTriggerMgr() {
        g_curPlayer = 0;
        memset(m_units, 0, sizeof(m_units));
        memset(m_unitCountByPlayer, 0, sizeof(m_unitCountByPlayer));
        memset(m_unitExited, 0, sizeof(m_unitExited));
        memset(m_gruntzExitedByPlayer, 0, sizeof(m_gruntzExitedByPlayer));
        memset(m_gruntzLostByPlayer, 0, sizeof(m_gruntzLostByPlayer));
        m_lastRecalledGroup = -1;
        m_cameraSprite = NULL;
        m_actionOptionsMenu = NULL;
        m_world = NULL;
        m_countdownActive = true;
        m_playerControlEnabled = true;
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

    i32 SpawnPickup(
        PickupType pickupType,
        i32 x,
        i32 y,
        i32 faceDirection,
        i32 warpstoneVariant,
        i32 damage
    );

    i32 DropBattlezWarpStone(i32 x, i32 y, i32 anchorIndex);

    i32 SpawnExplosion(i32 x, i32 y, i32 killerPlayerIndex, i32 animationVariant);

    i32 SpawnToyBox(i32 x, i32 y, i32 playerIndex, PickupType toyType, i32 scrollSpell);

    CPtrList m_puddles;
    CGrunt* m_units[PLAYER_SLOT_COUNT * TM_UNITS_PER_PLAYER];
    i32 m_unitCountByPlayer[PLAYER_SLOT_COUNT];
    i32 m_unitExited[PLAYER_SLOT_COUNT * TM_UNITS_PER_PLAYER];

    i32 m_gruntzExitedByPlayer[PLAYER_SLOT_COUNT];
    i32 m_gruntzLostByPlayer[PLAYER_SLOT_COUNT];

    CGameWorld* m_world;

    b32 m_cameraTrackingActive;
    Coord m_cameraTargetIdentity;
    CWwdSpriteObject* m_cameraSprite;

    CPtrList m_selectedUnitIds;

    CGrunt** PlayerUnits(i32 playerIndex) {
        return &m_units[playerIndex * TM_UNITS_PER_PLAYER];
    }
    CGrunt* UnitAt(i32 playerIndex, i32 unitIndex) {
        return PlayerUnits(playerIndex)[unitIndex];
    }
    Coord* FirstSelectedUnitId() {
        return static_cast<Coord*>(m_selectedUnitIds.GetHead());
    }
    CGrunt* SoleSelectedGrunt() {
        if (m_selectedUnitIds.GetCount() != 1) {
            return NULL;
        }
        const Coord& identity = *FirstSelectedUnitId();
        return UnitAt(identity.m_x, identity.m_y);
    }
    CActionOptionsMenuBar* m_actionOptionsMenu;
    CByteArray m_collectedWarpStoneFragments;
    // @identity-TODO: Save and Load transfer this complete span; no trigger
    // operation accesses its components to prove a scalar array or aggregate type.
    char m_reserved274[0x10];
    b32 m_levelWarpStoneCollected;

    FinishLevelState m_finishState;

    ClockInterval m_finishDelayTiming;

    CWarlord* m_localWarlord;
    b32 m_countdownActive;
    i32 m_targetingCursorId;

    ClockInterval m_gooTimer;
    ClockInterval m_resourceTimer;
    CPtrList m_selectionGroups[10];
    i32 m_lastRecalledGroup;
    FinishLevelReason m_finishReason;

    SoundBuffer* m_rollingballLoop;
    SoundBuffer* m_teleportLoop;
    b32 m_rollingballWanted;
    b32 m_teleportWanted;
    b32 m_playerControlEnabled;
};

extern i32 g_defaultGruntColorIndex;

#endif

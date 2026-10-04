#ifndef GRUNTZ_GAMESTATS_H
#define GRUNTZ_GAMESTATS_H

#include <rva.h>

#include <Enums.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/PlayerSlot.h>
#include <Gruntz/QuestLevelStats.h>
#include <Gruntz/SerialArchive.h>
#include <Ints.h>

class CGameStats {
public:
    CGameStats();

    i32 ResetWithLevelRecords(QuestLevelStats* levelRecords);
    ~CGameStats();
    void Reset();
    i32 GetLevelNumber() const {
        return m_levelNumber;
    }
    void SetLevelNumber(i32 levelNumber);
    b32 IsCurrentAreaComplete() const {
        return m_currentAreaComplete;
    }
    b32 IsCustomLevel() const {
        return m_isCustomLevel;
    }
    void SetCustomLevel(b32 customLevel) {
        m_isCustomLevel = customLevel;
    }
    void AddAvailableSecret() {
        ++m_secretsAvailable;
    }
    void RecordSecretFound() {
        ++m_secretsFound;
    }
    void RecordFlagCapture(i32 capturingPlayerIndex, i32 flagOwnerPlayerIndex);
    void ClearFlagCaptures();

    const i32* GetToolPickupCounts(i32 playerIndex) const {
        return m_toolPickupsByPlayer[playerIndex];
    }

    const i32* GetToyPickupCounts(i32 playerIndex) const {
        return m_toyPickupsByPlayer[playerIndex];
    }

    const i32* GetTimedPowerupPickupCounts(i32 playerIndex) const {
        return m_timedPowerupPickupsByPlayer[playerIndex];
    }

    const i32* GetCursePickupCounts(i32 playerIndex) const {
        return m_cursePickupsByPlayer[playerIndex];
    }

    PickupType GetMostCollectedTool(i32 playerIndex) const {
        i32 best = -1;
        i32 bestIndex = 0;
        for (i32 i = 0; i < 22; i++) {
            if (m_toolPickupsByPlayer[playerIndex][i] > best) {
                best = m_toolPickupsByPlayer[playerIndex][i];
                bestIndex = i;
            }
        }
        return static_cast<PickupType>(bestIndex + IDX(PICKUP_EQUIPPABLE_FIRST));
    }

    PickupType GetMostCollectedToy(i32 playerIndex) const {
        i32 best = -1;
        i32 bestIndex = 0;
        for (i32 i = 0; i < 10; i++) {
            if (m_toyPickupsByPlayer[playerIndex][i] > best) {
                best = m_toyPickupsByPlayer[playerIndex][i];
                bestIndex = i;
            }
        }
        return static_cast<PickupType>(bestIndex + IDX(PICKUP_TOYZ_FIRST));
    }

    PickupType GetMostCollectedTimedPowerup(i32 playerIndex) const {
        i32 best = -1;
        i32 bestIndex = 0;
        for (i32 i = 0; i < 7; i++) {
            if (m_timedPowerupPickupsByPlayer[playerIndex][i] > best) {
                best = m_timedPowerupPickupsByPlayer[playerIndex][i];
                bestIndex = i;
            }
        }
        return static_cast<PickupType>(bestIndex + IDX(PICKUP_TIMEDPOWERUP_FIRST));
    }

    PickupType GetMostCollectedCurse(i32 playerIndex) const {
        i32 best = -1;
        i32 bestIndex = 0;
        for (i32 i = 0; i < 4; i++) {
            if (m_cursePickupsByPlayer[playerIndex][i] > best) {
                best = m_cursePickupsByPlayer[playerIndex][i];
                bestIndex = i;
            }
        }
        return static_cast<PickupType>(bestIndex + IDX(PICKUP_CURSEZ_FIRST));
    }

    i32 CountAllFlagCaptures(i32 validatedPlayerIndex);
    i32 GetFlagCapture(i32 capturingPlayerIndex, i32 flagOwnerPlayerIndex);
    void RecordKill(i32 killerPlayerIndex, i32 victimPlayerIndex);
    void ClearKills();
    i32 CountKillsForPlayer(i32 playerIndex);
    i32 IsCurrentLevelPerfect(i32 unused);
    i32 IsCampaignPerfect();
    float CurrentAreaCoinRatio();
    i32 CurrentAreaHasAllWarpLetters();
    i32 SumToyzCollectedForCurrentArea();
    i32 SumToyzAvailableForCurrentArea();
    i32 SumToolzCollectedForCurrentArea();
    i32 SumToolzAvailableForCurrentArea();
    i32 SumPowerupzCollectedForCurrentArea();
    i32 SumPowerupzAvailableForCurrentArea();
    i32 SumSecretsFoundForCurrentArea();
    i32 SumSecretsAvailableForCurrentArea();
    i32 SumCoinsCollectedForCurrentArea();
    i32 SumCoinsAvailableForCurrentArea();
    i32 SumGruntzLostForCurrentArea();
    i32 SumGruntzExitedForCurrentArea();
    i32 SumElapsedTimeForCurrentArea();
    i32 CurrentAreaHasWarpLetter(i32 letterIndex);
    void UpdateLevelRecord(i32 levelNumber, b32 writeAvailableCounts);
    i32 Serialize(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, i32 payload);

    QuestLevelStats* m_levelRecords;
    i32 m_levelNumber;
    b32 m_isCustomLevel;
    b32 m_currentAreaComplete;

    i32 m_elapsedTimeMs;
    i32 m_toyzCollected;
    i32 m_toolzCollected;
    i32 m_gruntzExited;
    i32 m_gruntzLost;
    i32 m_powerupzCollected;
    i32 m_secretsFound;
    i32 m_coinsCollected;
    i32 m_toyzAvailable;
    i32 m_toolzAvailable;
    i32 m_powerupzAvailable;
    i32 m_secretsAvailable;
    i32 m_coinsAvailable;
    b32 m_warpLetterFound;
    i32 m_gruntzSpawnedByPlayer[PLAYER_SLOT_COUNT];
    i32 m_killsByPlayer[PLAYER_SLOT_COUNT][PLAYER_SLOT_COUNT];
    i32 m_flagCapturesByPlayer[PLAYER_SLOT_COUNT][PLAYER_SLOT_COUNT];

    i32 m_toolPickupsByPlayer[PLAYER_SLOT_COUNT][22];
    i32 m_toyPickupsByPlayer[PLAYER_SLOT_COUNT][10];
    i32 m_timedPowerupPickupsByPlayer[PLAYER_SLOT_COUNT][7];
    i32 m_cursePickupsByPlayer[PLAYER_SLOT_COUNT][4];
};

inline CGameStats::CGameStats() {
    Reset();
}

extern const float g_zeroF;
#endif // GRUNTZ_GAMESTATS_H

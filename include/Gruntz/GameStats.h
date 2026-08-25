#ifndef GRUNTZ_GAMESTATS_H
#define GRUNTZ_GAMESTATS_H

#include <rva.h>

#include <Enums.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/QuestLevelStats.h>
#include <Gruntz/SerialArchive.h>
#include <Ints.h>

GZ_ENUM_CONST_BEGIN(BattlezPlayerCount)
    BZ_PLAYER_COUNT = 4
GZ_ENUM_CONST_END(BattlezPlayerCount)

class CGameStats {
public:
    CGameStats();

    i32 ResetWithLevelRecords(QuestLevelStats* levelRecords);
    ~CGameStats();
    void Reset();
    void SetLevelNumber(i32 levelNumber);
    void RecordFlagCapture(i32 capturingPlayerIndex, i32 flagOwnerPlayerIndex);
    void ClearFlagCaptures();
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
    void UpdateLevelRecord(i32 levelNumber, i32 writeAvailableCounts);
    i32 Serialize(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, i32 payload);

    QuestLevelStats* m_levelRecords;
    i32 m_levelNumber;
    i32 m_isCustomLevel;
    i32 m_currentAreaComplete;

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
    i32 m_warpLetterFound;
    i32 m_gruntzByPlayer[BZ_PLAYER_COUNT];
    i32 m_killsByPlayer[BZ_PLAYER_COUNT][BZ_PLAYER_COUNT];
    i32 m_flagCapturesByPlayer[BZ_PLAYER_COUNT][BZ_PLAYER_COUNT];

    i32 m_weaponPickupsByPlayer[88];
    i32 m_toyPickupsByPlayer[40];
    i32 m_powerupPickupsByPlayer[28];
    i32 m_miscPickupsByPlayer[16];
};

inline CGameStats::CGameStats() {
    Reset();
}

extern const float g_zeroF;
#endif // GRUNTZ_GAMESTATS_H

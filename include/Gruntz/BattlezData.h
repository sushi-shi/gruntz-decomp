#ifndef GRUNTZ_BATTLEZDATA_H
#define GRUNTZ_BATTLEZDATA_H

#include <Ints.h>
#include <rva.h>

#include <Gruntz/SerialArchive.h>

struct BattlezRecord {
    i32 m_populated;
    i32 m_04;
    i32 m_elapsedTimeMs;
    i32 m_toyzCollected;
    i32 m_toolzCollected;
    i32 m_gruntzExited;
    i32 m_gruntzLost;
    i32 m_powerupzCollected;
    i32 m_secretsFound;
    i32 m_coinsCollected;
    i32 m_scoreValue;
    i32 m_toyzAvailable;
    i32 m_toolzAvailable;
    i32 m_powerupzAvailable;
    i32 m_secretsAvailable;
    i32 m_coinsAvailable;
};
SIZE_UNKNOWN();

class CBattlezData {
public:
    i32 InitWithRecords(void* records);
    ~CBattlezData();
    void Init();
    void SetCount(i32 count);
    void MarkFlag(i32 y, i32 x);
    void ClearFlags();
    i32 SumFlags(i32 y);
    i32 GetFlag(i32 x, i32 y);
    void BumpWin(i32 y, i32 x);
    void ClearWins();
    i32 SumWinRow(i32 y);
    i32 InBounds(i32 unused);
    i32 AllRecordsInBounds();
    float GroupRatio();
    i32 GroupAllScored();
    i32 SumToyzCollectedForGroup();
    i32 SumToyzAvailableForGroup();
    i32 SumToolzCollectedForGroup();
    i32 SumToolzAvailableForGroup();
    i32 SumPowerupzCollectedForGroup();
    i32 SumPowerupzAvailableForGroup();
    i32 SumSecretsFoundForGroup();
    i32 SumSecretsAvailableForGroup();
    i32 SumCoinsCollectedForGroup();
    i32 SumCoinsAvailableForGroup();
    i32 SumGruntzLostForGroup();
    i32 SumGruntzExitedForGroup();
    i32 SumElapsedTimeForGroup();
    i32 GetRecordValue(i32 b);
    void FillRecord(i32 index, i32 phase);
    i32 Serialize(CFileMemBase* s, i32 op, i32 typeId, i32 pObj);

    BattlezRecord* m_records;
    i32 m_count;
    i32 m_08;
    i32 m_allDone;

    i32 m_elapsedTimeMs;
    i32 m_toyzCount;
    i32 m_toolzCount;
    i32 m_gruntzExited;
    i32 m_gruntzLost;
    i32 m_powerupCount;
    i32 m_secretsFound;
    i32 m_coinsCollected;
    i32 m_toyzAvailable;
    i32 m_toolzAvailable;
    i32 m_powerupzAvailable;
    i32 m_secretsAvailable;
    i32 m_coinsAvailable;
    i32 m_scoreValue;
    i32 m_counts[4];
    i32 m_wins[4][4];
    i32 m_flags[4][4];

    i32 m_weaponPickupz[88];
    i32 m_toyPickupz[40];
    i32 m_powerupPickupz[28];
    i32 m_miscPickupz[16];
};
SIZE_UNKNOWN();

extern float g_zeroF;
#endif // GRUNTZ_BATTLEZDATA_H

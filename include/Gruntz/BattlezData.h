#ifndef GRUNTZ_BATTLEZDATA_H
#define GRUNTZ_BATTLEZDATA_H

#include <rva.h>

#include <Enums.h>
#include <Gruntz/BattlezRecord.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Ints.h>

// The four Battlez players. It is the dimension of every per-player table on
// CBattlezData - m_counts, and the m_wins / m_flags matrices - and the stride
// the accessors index those matrices with.
//
// RETAIL OFF-BY-ONE, deliberately preserved. MarkFlag, GetFlag and SumFlags all
// bound their arguments with `<= BZ_PLAYER_COUNT` where the array needs `<`, so
// an index of 4 is accepted and written. The generated code confirms it and our
// reconstruction is byte-identical to retail: `cmp edx, 0x4; jg` for each axis,
// then `lea eax, [eax + edx*4]` - stride 4 - and a store at
// [ecx + eax*4 + 0x98]. With both indices allowed to reach 4 the linear index
// runs to 20 in a 16-entry table, so the shipped game overruns m_flags by up to
// five dwords. The guards keep the `<=` spelling on purpose: writing `<` would
// move bytes AND hide the defect.
GZ_ENUM_CONST_BEGIN(BattlezPlayerCount)
    BZ_PLAYER_COUNT = 4
GZ_ENUM_CONST_END(BattlezPlayerCount)

class CBattlezData {
public:
    // Inline: `new CBattlezData` in CGruntzMgr::Run expands to the allocation's
    // null guard around a bare `call ?Init@CBattlezData@@QAEXXZ` (0x83450 @ 0xeb1).
    CBattlezData();

    i32 InitWithRecords(BattlezRecord* records);
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
    i32 Serialize(CFileMemBase* s, SerialMode op, LogicTypeId typeId, i32 pObj);

    BattlezRecord* m_records;
    i32 m_count;
    i32 m_isCustomLevel;
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
    i32 m_counts[BZ_PLAYER_COUNT];
    i32 m_wins[BZ_PLAYER_COUNT][BZ_PLAYER_COUNT];
    i32 m_flags[BZ_PLAYER_COUNT][BZ_PLAYER_COUNT];

    i32 m_weaponPickupz[88];
    i32 m_toyPickupz[40];
    i32 m_powerupPickupz[28];
    i32 m_miscPickupz[16];
};

inline CBattlezData::CBattlezData() {
    Init();
}

extern const float g_zeroF;
#endif // GRUNTZ_BATTLEZDATA_H

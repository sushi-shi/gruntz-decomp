#ifndef GRUNTZ_BATTLEZDATA_H
#define GRUNTZ_BATTLEZDATA_H

#include <Ints.h>
#include <rva.h>

#include <Gruntz/SerialArchive.h>

struct BattlezRecord {
    i32 m_populated;
    i32 m_04;
    i32 m_08, m_0c, m_10, m_14, m_18, m_1c, m_20, m_24;
    i32 m_scoreValue;
    i32 m_2c, m_30, m_34, m_38, m_3c;
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
    i32 SumGroupField0c();
    i32 SumGroupField2c();
    i32 SumGroupField10();
    i32 SumGroupField30();
    i32 SumGroupField1c();
    i32 SumGroupField34();
    i32 SumGroupField20();
    i32 SumGroupField38();
    i32 SumGroupField24();
    i32 SumGroupField3c();
    i32 SumGroupField18();
    i32 SumGroupField14();
    i32 SumGroupField08();
    i32 GetRecordValue(i32 b);
    void FillRecord(i32 index, i32 phase);
    i32 Serialize(CFileMemBase* s, i32 op, i32 typeId, i32 pObj);

    BattlezRecord* m_records;
    i32 m_count;
    i32 m_08;
    i32 m_allDone;

    i32 m_score;
    i32 m_toyzCount;
    i32 m_weaponCount;
    i32 m_1c, m_20;
    i32 m_powerupCount;
    i32 m_28, m_2c, m_30, m_34, m_38, m_3c, m_40, m_scoreValue;
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

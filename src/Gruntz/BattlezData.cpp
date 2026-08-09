#include <rva.h>

#include <Gruntz/BattlezData.h>

#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Io/FileMem.h>

#include <stddef.h>

DATA(0x001eab40)
const float g_zeroF = 0.0f;

RVA(0x000fc9c0, 0x17)
i32 CBattlezData::InitWithRecords(void* records) {
    Init();
    m_records = static_cast<BattlezRecord*>(records);
    return 1;
}

RVA(0x000fc9f0, 0x5)
CBattlezData::~CBattlezData() {
    Init();
}

// @early-stop
RVA(0x000fca10, 0x8a)
void CBattlezData::Init() {
    m_count = 0;
    m_isCustomLevel = 0;
    m_allDone = 0;
    m_elapsedTimeMs = 0;
    m_toyzCount = 0;
    m_toolzCount = 0;
    m_gruntzExited = 0;
    m_gruntzLost = 0;
    m_powerupCount = 0;
    m_secretsFound = 0;
    m_coinsCollected = 0;
    m_scoreValue = 0;
    m_toolzAvailable = 0;
    m_toyzAvailable = 0;
    m_powerupzAvailable = 0;
    m_secretsAvailable = 0;
    m_coinsAvailable = 0;
    ClearWins();
    ClearFlags();
    m_counts[0] = 0;
    m_counts[1] = 0;
    m_counts[2] = 0;
    m_counts[3] = 0;
    i32 i;
    for (i = 0; i < 88; i++) {
        m_weaponPickupz[i] = 0;
    }
    for (i = 0; i < 40; i++) {
        m_toyPickupz[i] = 0;
    }
    for (i = 0; i < 28; i++) {
        m_powerupPickupz[i] = 0;
    }
    for (i = 0; i < 16; i++) {
        m_miscPickupz[i] = 0;
    }
}

RVA(0x000fcad0, 0x53)
void CBattlezData::SetCount(i32 count) {
    m_count = count;
    if (count > 0x24) {
        m_allDone = 0;
        return;
    }
    i32 base = (count - 1) / 4 * 4;
    i32 flag = 1;
    for (i32 i = base; i < base + 4; i++) {
        if (m_records[i].m_populated == 0) {
            flag = 0;
        }
    }
    m_allDone = flag;
}

RVA(0x000fcb50, 0x2b)
void CBattlezData::MarkFlag(i32 y, i32 x) {
    if (y >= 0 && y <= BZ_PLAYER_COUNT && x >= 0 && x <= BZ_PLAYER_COUNT) {
        m_flags[y][x] = 1;
    }
}

RVA(0x000fcb90, 0x12)
void CBattlezData::ClearFlags() {
    for (i32 i = 0; i < BZ_PLAYER_COUNT * BZ_PLAYER_COUNT; i++) {
        (&m_flags[0][0])[i] = 0;
    }
}

RVA(0x000fcbc0, 0x3a)
i32 CBattlezData::SumFlags(i32 y) {
    if (y < 0 || y > BZ_PLAYER_COUNT) {
        return 0;
    }
    i32 sum = 0;
    i32* p = &m_flags[0][0];
    for (i32 r = 0; r < BZ_PLAYER_COUNT; r++) {
        for (i32 c = 0; c < BZ_PLAYER_COUNT; c++) {
            sum += *p++;
        }
    }
    return sum;
}

RVA(0x000fcc10, 0x2f)
i32 CBattlezData::GetFlag(i32 x, i32 y) {
    if (x >= 0 && x <= BZ_PLAYER_COUNT && y >= 0 && y <= BZ_PLAYER_COUNT) {
        return m_flags[x][y];
    }
    return 0;
}

RVA(0x000fcc50, 0x2a)
void CBattlezData::BumpWin(i32 y, i32 x) {
    if (y >= 0 && y <= BZ_PLAYER_COUNT && x >= 0 && x <= BZ_PLAYER_COUNT && y != x) {
        m_wins[y][x]++;
    }
}

RVA(0x000fcc90, 0xf)
void CBattlezData::ClearWins() {
    for (i32 i = 0; i < BZ_PLAYER_COUNT * BZ_PLAYER_COUNT; i++) {
        (&m_wins[0][0])[i] = 0;
    }
}

RVA(0x000fccb0, 0x21)
i32 CBattlezData::SumWinRow(i32 y) {
    i32 sum = 0;
    i32* p = m_wins[y];
    for (i32 c = 0; c < BZ_PLAYER_COUNT; c++) {
        sum += *p++;
    }
    return sum;
}

RVA(0x000fccf0, 0x57)
i32 CBattlezData::AllRecordsInBounds() {
    i32 i = 0;
    BattlezRecord* r = m_records;
    for (; i < 0x20; i++, r++) {
        if (r->m_populated == 0) {
            return 0;
        }
        if (r->m_scoreValue == 0) {
            return 0;
        }
        if (r->m_toolzCollected < r->m_toolzAvailable) {
            return 0;
        }
        if (r->m_toyzCollected < r->m_toyzAvailable) {
            return 0;
        }
        if (r->m_powerupzCollected < r->m_powerupzAvailable) {
            return 0;
        }
        if (r->m_secretsFound < r->m_secretsAvailable) {
            return 0;
        }
        if (r->m_coinsCollected < r->m_coinsAvailable) {
            return 0;
        }
    }
    return 1;
}

RVA(0x000fcd70, 0x61)
i32 CBattlezData::InBounds(i32 unused) {
    if (m_scoreValue == 0) {
        return 0;
    }
    if (m_toyzAvailable > m_toyzCount) {
        return 0;
    }
    if (m_toolzAvailable > m_toolzCount) {
        return 0;
    }
    if (m_powerupzAvailable > m_powerupCount) {
        return 0;
    }
    if (m_secretsAvailable > m_secretsFound) {
        return 0;
    }
    return m_coinsAvailable <= m_coinsCollected;
}

RVA(0x000fce00, 0x56)
float CBattlezData::GroupRatio() {
    float den = g_zeroF;
    float num = g_zeroF;
    i32 g = (m_count - 1) / 4 * 4;
    for (i32 i = 0; i < 4; i++) {
        den += m_records[g + i].m_coinsAvailable;
        num += m_records[g + i].m_coinsCollected;
    }
    if (g_zeroF == den) {
        return g_zeroF;
    }
    return num / den;
}

RVA(0x000fce80, 0x32)
i32 CBattlezData::GroupAllScored() {
    i32 g = (m_count - 1) / 4 * 4;
    for (i32 i = 0; i < 4; i++) {
        if (m_records[g + i].m_scoreValue == 0) {
            return 0;
        }
    }
    return 1;
}

RVA(0x000fced0, 0x31)
i32 CBattlezData::GetRecordValue(i32 b) {
    i32 idx = b + (m_count - 1) / 4 * 4;
    if (idx == m_count - 1) {
        return m_scoreValue;
    }
    return m_records[idx].m_scoreValue;
}

RVA(0x000fcf20, 0x37)
i32 CBattlezData::SumToyzCollectedForGroup() {
    i32 sum = 0;
    i32 g = (m_count - 1) / 4 * 4;
    for (i32 i = g; i < g + 4; i++) {
        sum += m_records[i].m_toyzCollected;
    }
    return sum;
}

RVA(0x000fcf70, 0x37)
i32 CBattlezData::SumToyzAvailableForGroup() {
    i32 sum = 0;
    i32 g = (m_count - 1) / 4 * 4;
    for (i32 i = g; i < g + 4; i++) {
        sum += m_records[i].m_toyzAvailable;
    }
    return sum;
}

RVA(0x000fcfc0, 0x37)
i32 CBattlezData::SumToolzCollectedForGroup() {
    i32 sum = 0;
    i32 g = (m_count - 1) / 4 * 4;
    for (i32 i = g; i < g + 4; i++) {
        sum += m_records[i].m_toolzCollected;
    }
    return sum;
}

RVA(0x000fd010, 0x37)
i32 CBattlezData::SumToolzAvailableForGroup() {
    i32 sum = 0;
    i32 g = (m_count - 1) / 4 * 4;
    for (i32 i = g; i < g + 4; i++) {
        sum += m_records[i].m_toolzAvailable;
    }
    return sum;
}

RVA(0x000fd060, 0x37)
i32 CBattlezData::SumPowerupzCollectedForGroup() {
    i32 sum = 0;
    i32 g = (m_count - 1) / 4 * 4;
    for (i32 i = g; i < g + 4; i++) {
        sum += m_records[i].m_powerupzCollected;
    }
    return sum;
}

RVA(0x000fd0b0, 0x37)
i32 CBattlezData::SumPowerupzAvailableForGroup() {
    i32 sum = 0;
    i32 g = (m_count - 1) / 4 * 4;
    for (i32 i = g; i < g + 4; i++) {
        sum += m_records[i].m_powerupzAvailable;
    }
    return sum;
}

RVA(0x000fd100, 0x37)
i32 CBattlezData::SumSecretsFoundForGroup() {
    i32 sum = 0;
    i32 g = (m_count - 1) / 4 * 4;
    for (i32 i = g; i < g + 4; i++) {
        sum += m_records[i].m_secretsFound;
    }
    return sum;
}

RVA(0x000fd150, 0x37)
i32 CBattlezData::SumSecretsAvailableForGroup() {
    i32 sum = 0;
    i32 g = (m_count - 1) / 4 * 4;
    for (i32 i = g; i < g + 4; i++) {
        sum += m_records[i].m_secretsAvailable;
    }
    return sum;
}

RVA(0x000fd1a0, 0x37)
i32 CBattlezData::SumCoinsCollectedForGroup() {
    i32 sum = 0;
    i32 g = (m_count - 1) / 4 * 4;
    for (i32 i = g; i < g + 4; i++) {
        sum += m_records[i].m_coinsCollected;
    }
    return sum;
}

RVA(0x000fd1f0, 0x37)
i32 CBattlezData::SumCoinsAvailableForGroup() {
    i32 sum = 0;
    i32 g = (m_count - 1) / 4 * 4;
    for (i32 i = g; i < g + 4; i++) {
        sum += m_records[i].m_coinsAvailable;
    }
    return sum;
}

RVA(0x000fd240, 0x37)
i32 CBattlezData::SumGruntzLostForGroup() {
    i32 sum = 0;
    i32 g = (m_count - 1) / 4 * 4;
    for (i32 i = g; i < g + 4; i++) {
        sum += m_records[i].m_gruntzLost;
    }
    return sum;
}

RVA(0x000fd290, 0x37)
i32 CBattlezData::SumGruntzExitedForGroup() {
    i32 sum = 0;
    i32 g = (m_count - 1) / 4 * 4;
    for (i32 i = g; i < g + 4; i++) {
        sum += m_records[i].m_gruntzExited;
    }
    return sum;
}

RVA(0x000fd2e0, 0x37)
i32 CBattlezData::SumElapsedTimeForGroup() {
    i32 sum = 0;
    i32 g = (m_count - 1) / 4 * 4;
    for (i32 i = g; i < g + 4; i++) {
        sum += m_records[i].m_elapsedTimeMs;
    }
    return sum;
}

RVA(0x000fd330, 0x84)
void CBattlezData::FillRecord(i32 index, i32 phase) {
    BattlezRecord* rec = &m_records[index - 1];
    if (phase == 0) {
        rec->m_populated = 1;
        rec->m_elapsedTimeMs = m_elapsedTimeMs;
        rec->m_toyzCollected = m_toyzCount;
        rec->m_toolzCollected = m_toolzCount;
        rec->m_gruntzExited = m_gruntzExited;
        rec->m_gruntzLost = m_gruntzLost;
        rec->m_powerupzCollected = m_powerupCount;
        rec->m_secretsFound = m_secretsFound;
        rec->m_coinsCollected = m_coinsCollected;
        rec->m_scoreValue = m_scoreValue;
        rec->m_isEasyMode = g_gameReg->m_isEasyMode;
    } else {
        rec->m_toyzAvailable = m_toyzAvailable;
        rec->m_toolzAvailable = m_toolzAvailable;
        rec->m_powerupzAvailable = m_powerupzAvailable;
        rec->m_secretsAvailable = m_secretsAvailable;
        rec->m_coinsAvailable = m_coinsAvailable;
    }
}

// @early-stop
// retail allocates one extra dword (`push ecx`) and spills `this` into it so it can
// reuse ebp as the last loop counter; cl keeps `this` in ebp and counts in ebx, so the
// whole frame sits 4 bytes higher.
RVA(0x000fd3f0, 0x425)
i32 CBattlezData::Serialize(CFileMemBase* s, SerialMode op, LogicTypeId typeId, i32 pObj) {
    i32* p;
    i32 i;
    i32 r;
    i32 c;
    if (s == NULL) {
        return 0;
    }
    if (op != SERIAL_SAVE) {
        if (op == SERIAL_LOAD) {
            s->Read(&m_count, sizeof(m_count));
            s->Read(&m_isCustomLevel, sizeof(m_isCustomLevel));
            s->Read(&m_allDone, sizeof(m_allDone));
            s->Read(&m_elapsedTimeMs, sizeof(m_elapsedTimeMs));
            s->Read(&m_toyzCount, sizeof(m_toyzCount));
            s->Read(&m_toolzCount, sizeof(m_toolzCount));
            s->Read(&m_gruntzExited, sizeof(m_gruntzExited));
            s->Read(&m_gruntzLost, sizeof(m_gruntzLost));
            s->Read(&m_powerupCount, sizeof(m_powerupCount));
            s->Read(&m_secretsFound, sizeof(m_secretsFound));
            s->Read(&m_coinsCollected, sizeof(m_coinsCollected));
            s->Read(&m_toyzAvailable, sizeof(m_toyzAvailable));
            s->Read(&m_toolzAvailable, sizeof(m_toolzAvailable));
            s->Read(&m_powerupzAvailable, sizeof(m_powerupzAvailable));
            s->Read(&m_secretsAvailable, sizeof(m_secretsAvailable));
            s->Read(&m_coinsAvailable, sizeof(m_coinsAvailable));
            s->Read(&m_scoreValue, sizeof(m_scoreValue));
            for (p = m_counts, i = 0; i < 4; i++, p++) {
                s->Read(p, sizeof(*p));
            }
            p = &m_wins[0][0];
            for (r = 0; r < 4; r++) {
                for (c = 0; c < 4; c++, p++) {
                    s->Read(p, sizeof(*p));
                }
            }
            p = &m_flags[0][0];
            for (r = 0; r < 4; r++) {
                for (c = 0; c < 4; c++, p++) {
                    s->Read(p, sizeof(*p));
                }
            }
            p = m_weaponPickupz;
            for (r = 0; r < 4; r++) {
                for (c = 0; c < 22; c++, p++) {
                    s->Read(p, sizeof(*p));
                }
            }
            p = m_toyPickupz;
            for (r = 0; r < 4; r++) {
                for (c = 0; c < 10; c++, p++) {
                    s->Read(p, sizeof(*p));
                }
            }
            p = m_powerupPickupz;
            for (r = 0; r < 4; r++) {
                for (c = 0; c < 7; c++, p++) {
                    s->Read(p, sizeof(*p));
                }
            }
            p = m_miscPickupz;
            for (r = 0; r < 4; r++) {
                for (c = 0; c < 4; c++, p++) {
                    s->Read(p, sizeof(*p));
                }
            }
        }
    } else {
        s->Write(&m_count, sizeof(m_count));
        s->Write(&m_isCustomLevel, sizeof(m_isCustomLevel));
        s->Write(&m_allDone, sizeof(m_allDone));
        s->Write(&m_elapsedTimeMs, sizeof(m_elapsedTimeMs));
        s->Write(&m_toyzCount, sizeof(m_toyzCount));
        s->Write(&m_toolzCount, sizeof(m_toolzCount));
        s->Write(&m_gruntzExited, sizeof(m_gruntzExited));
        s->Write(&m_gruntzLost, sizeof(m_gruntzLost));
        s->Write(&m_powerupCount, sizeof(m_powerupCount));
        s->Write(&m_secretsFound, sizeof(m_secretsFound));
        s->Write(&m_coinsCollected, sizeof(m_coinsCollected));
        s->Write(&m_toyzAvailable, sizeof(m_toyzAvailable));
        s->Write(&m_toolzAvailable, sizeof(m_toolzAvailable));
        s->Write(&m_powerupzAvailable, sizeof(m_powerupzAvailable));
        s->Write(&m_secretsAvailable, sizeof(m_secretsAvailable));
        s->Write(&m_coinsAvailable, sizeof(m_coinsAvailable));
        s->Write(&m_scoreValue, sizeof(m_scoreValue));
        for (p = m_counts, i = 0; i < 4; i++, p++) {
            s->Write(p, sizeof(*p));
        }
        p = &m_wins[0][0];
        for (r = 0; r < 4; r++) {
            for (c = 0; c < 4; c++, p++) {
                s->Write(p, sizeof(*p));
            }
        }
        p = &m_flags[0][0];
        for (r = 0; r < 4; r++) {
            for (c = 0; c < 4; c++, p++) {
                s->Write(p, sizeof(*p));
            }
        }
        p = m_weaponPickupz;
        for (r = 0; r < 4; r++) {
            for (c = 0; c < 22; c++, p++) {
                s->Write(p, sizeof(*p));
            }
        }
        p = m_toyPickupz;
        for (r = 0; r < 4; r++) {
            for (c = 0; c < 10; c++, p++) {
                s->Write(p, sizeof(*p));
            }
        }
        p = m_powerupPickupz;
        for (r = 0; r < 4; r++) {
            for (c = 0; c < 7; c++, p++) {
                s->Write(p, sizeof(*p));
            }
        }
        p = m_miscPickupz;
        for (r = 0; r < 4; r++) {
            for (c = 0; c < 4; c++, p++) {
                s->Write(p, sizeof(*p));
            }
        }
    }
    return 1;
}

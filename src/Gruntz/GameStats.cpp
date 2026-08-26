#include <rva.h>

#include <Gruntz/GameStats.h>

#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Io/FileMem.h>

#include <stddef.h>

DATA(0x001eab40)
const float g_zeroF = 0.0f;

RVA(0x000fcaf0, 0x17)
i32 CGameStats::ResetWithLevelRecords(QuestLevelStats* levelRecords) {
    Reset();
    m_levelRecords = levelRecords;
    return 1;
}

RVA(0x000fcb20, 0x5)
CGameStats::~CGameStats() {
    Reset();
}

RVA(0x000fcb40, 0x8a)
void CGameStats::Reset() {
    m_levelNumber = 0;
    m_isCustomLevel = false;
    m_currentAreaComplete = false;
    m_elapsedTimeMs = 0;
    m_toyzCollected = 0;
    m_toolzCollected = 0;
    m_gruntzExited = 0;
    m_gruntzLost = 0;
    m_powerupzCollected = 0;
    m_secretsFound = 0;
    m_coinsCollected = 0;
    m_warpLetterFound = false;
    m_toolzAvailable = 0;
    m_toyzAvailable = 0;
    m_powerupzAvailable = 0;
    m_secretsAvailable = 0;
    m_coinsAvailable = 0;
    ClearKills();
    ClearFlagCaptures();
    i32 i;
    for (i = 0; i < BZ_PLAYER_COUNT; i++) {
        m_gruntzByPlayer[i] = 0;
    }
    for (i = 0; i < 88; i++) {
        m_weaponPickupsByPlayer[i] = 0;
    }
    for (i = 0; i < 40; i++) {
        m_toyPickupsByPlayer[i] = 0;
    }
    for (i = 0; i < 28; i++) {
        m_powerupPickupsByPlayer[i] = 0;
    }
    for (i = 0; i < 16; i++) {
        m_miscPickupsByPlayer[i] = 0;
    }
}

RVA(0x000fcc00, 0x53)
void CGameStats::SetLevelNumber(i32 levelNumber) {
    m_levelNumber = levelNumber;
    if (levelNumber > 0x24) {
        m_currentAreaComplete = false;
        return;
    }
    i32 areaFirstIndex = (levelNumber - 1) / 4 * 4;
    b32 areaComplete = true;
    for (i32 i = areaFirstIndex; i < areaFirstIndex + 4; i++) {
        if (m_levelRecords[i].m_completed == false) {
            areaComplete = false;
        }
    }
    m_currentAreaComplete = areaComplete;
}

RVA(0x000fcc80, 0x2b)
void CGameStats::RecordFlagCapture(i32 capturingPlayerIndex, i32 flagOwnerPlayerIndex) {
    if (capturingPlayerIndex >= 0 && capturingPlayerIndex <= BZ_PLAYER_COUNT
        && flagOwnerPlayerIndex >= 0 && flagOwnerPlayerIndex <= BZ_PLAYER_COUNT) {
        m_flagCapturesByPlayer[capturingPlayerIndex][flagOwnerPlayerIndex] = 1;
    }
}

RVA(0x000fccc0, 0x12)
void CGameStats::ClearFlagCaptures() {
    for (i32 i = 0; i < BZ_PLAYER_COUNT * BZ_PLAYER_COUNT; i++) {
        (&m_flagCapturesByPlayer[0][0])[i] = 0;
    }
}

RVA(0x000fccf0, 0x3a)
i32 CGameStats::CountAllFlagCaptures(i32 validatedPlayerIndex) {
    if (validatedPlayerIndex < 0 || validatedPlayerIndex > BZ_PLAYER_COUNT) {
        return 0;
    }
    i32 sum = 0;
    i32* capture = &m_flagCapturesByPlayer[0][0];
    for (i32 playerIndex = 0; playerIndex < BZ_PLAYER_COUNT; playerIndex++) {
        for (i32 flagOwnerIndex = 0; flagOwnerIndex < BZ_PLAYER_COUNT; flagOwnerIndex++) {
            sum += *capture++;
        }
    }
    return sum;
}

RVA(0x000fcd40, 0x2f)
i32 CGameStats::GetFlagCapture(i32 capturingPlayerIndex, i32 flagOwnerPlayerIndex) {
    if (capturingPlayerIndex >= 0 && capturingPlayerIndex <= BZ_PLAYER_COUNT
        && flagOwnerPlayerIndex >= 0 && flagOwnerPlayerIndex <= BZ_PLAYER_COUNT) {
        return m_flagCapturesByPlayer[capturingPlayerIndex][flagOwnerPlayerIndex];
    }
    return 0;
}

RVA(0x000fcd80, 0x2a)
void CGameStats::RecordKill(i32 killerPlayerIndex, i32 victimPlayerIndex) {
    if (killerPlayerIndex >= 0 && killerPlayerIndex <= BZ_PLAYER_COUNT && victimPlayerIndex >= 0
        && victimPlayerIndex <= BZ_PLAYER_COUNT && killerPlayerIndex != victimPlayerIndex) {
        m_killsByPlayer[killerPlayerIndex][victimPlayerIndex]++;
    }
}

RVA(0x000fcdc0, 0xf)
void CGameStats::ClearKills() {
    for (i32 i = 0; i < BZ_PLAYER_COUNT * BZ_PLAYER_COUNT; i++) {
        (&m_killsByPlayer[0][0])[i] = 0;
    }
}

RVA(0x000fcde0, 0x21)
i32 CGameStats::CountKillsForPlayer(i32 playerIndex) {
    i32 sum = 0;
    i32* kills = m_killsByPlayer[playerIndex];
    for (i32 opponentIndex = 0; opponentIndex < BZ_PLAYER_COUNT; opponentIndex++) {
        sum += *kills++;
    }
    return sum;
}

RVA(0x000fce20, 0x57)
i32 CGameStats::IsCampaignPerfect() {
    i32 levelIndex = 0;
    QuestLevelStats* levelStats = m_levelRecords;
    for (; levelIndex < 0x20; levelIndex++, levelStats++) {
        if (levelStats->m_completed == false) {
            return 0;
        }
        if (levelStats->m_warpLetterFound == false) {
            return 0;
        }
        if (levelStats->m_toolzCollected < levelStats->m_toolzAvailable) {
            return 0;
        }
        if (levelStats->m_toyzCollected < levelStats->m_toyzAvailable) {
            return 0;
        }
        if (levelStats->m_powerupzCollected < levelStats->m_powerupzAvailable) {
            return 0;
        }
        if (levelStats->m_secretsFound < levelStats->m_secretsAvailable) {
            return 0;
        }
        if (levelStats->m_coinsCollected < levelStats->m_coinsAvailable) {
            return 0;
        }
    }
    return 1;
}

RVA(0x000fcea0, 0x61)
i32 CGameStats::IsCurrentLevelPerfect(i32 unused) {
    if (m_warpLetterFound == false) {
        return 0;
    }
    if (m_toyzAvailable > m_toyzCollected) {
        return 0;
    }
    if (m_toolzAvailable > m_toolzCollected) {
        return 0;
    }
    if (m_powerupzAvailable > m_powerupzCollected) {
        return 0;
    }
    if (m_secretsAvailable > m_secretsFound) {
        return 0;
    }
    return m_coinsAvailable <= m_coinsCollected;
}

RVA(0x000fcf30, 0x56)
float CGameStats::CurrentAreaCoinRatio() {
    float availableCoins = g_zeroF;
    float collectedCoins = g_zeroF;
    i32 areaFirstIndex = (m_levelNumber - 1) / 4 * 4;
    for (i32 i = 0; i < 4; i++) {
        availableCoins += m_levelRecords[areaFirstIndex + i].m_coinsAvailable;
        collectedCoins += m_levelRecords[areaFirstIndex + i].m_coinsCollected;
    }
    if (g_zeroF == availableCoins) {
        return g_zeroF;
    }
    return collectedCoins / availableCoins;
}

RVA(0x000fcfb0, 0x32)
i32 CGameStats::CurrentAreaHasAllWarpLetters() {
    i32 areaFirstIndex = (m_levelNumber - 1) / 4 * 4;
    for (i32 i = 0; i < 4; i++) {
        if (m_levelRecords[areaFirstIndex + i].m_warpLetterFound == false) {
            return 0;
        }
    }
    return 1;
}

RVA(0x000fd000, 0x31)
i32 CGameStats::CurrentAreaHasWarpLetter(i32 letterIndex) {
    i32 levelIndex = letterIndex + (m_levelNumber - 1) / 4 * 4;
    if (levelIndex == m_levelNumber - 1) {
        return m_warpLetterFound;
    }
    QuestLevelStats* levelStats = &m_levelRecords[levelIndex];
    return levelStats->m_warpLetterFound;
}

RVA(0x000fd050, 0x37)
i32 CGameStats::SumToyzCollectedForCurrentArea() {
    i32 sum = 0;
    i32 areaFirstIndex = (m_levelNumber - 1) / 4 * 4;
    for (i32 i = areaFirstIndex; i < areaFirstIndex + 4; i++) {
        sum += m_levelRecords[i].m_toyzCollected;
    }
    return sum;
}

RVA(0x000fd0a0, 0x37)
i32 CGameStats::SumToyzAvailableForCurrentArea() {
    i32 sum = 0;
    i32 areaFirstIndex = (m_levelNumber - 1) / 4 * 4;
    for (i32 i = areaFirstIndex; i < areaFirstIndex + 4; i++) {
        sum += m_levelRecords[i].m_toyzAvailable;
    }
    return sum;
}

RVA(0x000fd0f0, 0x37)
i32 CGameStats::SumToolzCollectedForCurrentArea() {
    i32 sum = 0;
    i32 areaFirstIndex = (m_levelNumber - 1) / 4 * 4;
    for (i32 i = areaFirstIndex; i < areaFirstIndex + 4; i++) {
        sum += m_levelRecords[i].m_toolzCollected;
    }
    return sum;
}

RVA(0x000fd140, 0x37)
i32 CGameStats::SumToolzAvailableForCurrentArea() {
    i32 sum = 0;
    i32 areaFirstIndex = (m_levelNumber - 1) / 4 * 4;
    for (i32 i = areaFirstIndex; i < areaFirstIndex + 4; i++) {
        sum += m_levelRecords[i].m_toolzAvailable;
    }
    return sum;
}

RVA(0x000fd190, 0x37)
i32 CGameStats::SumPowerupzCollectedForCurrentArea() {
    i32 sum = 0;
    i32 areaFirstIndex = (m_levelNumber - 1) / 4 * 4;
    for (i32 i = areaFirstIndex; i < areaFirstIndex + 4; i++) {
        sum += m_levelRecords[i].m_powerupzCollected;
    }
    return sum;
}

RVA(0x000fd1e0, 0x37)
i32 CGameStats::SumPowerupzAvailableForCurrentArea() {
    i32 sum = 0;
    i32 areaFirstIndex = (m_levelNumber - 1) / 4 * 4;
    for (i32 i = areaFirstIndex; i < areaFirstIndex + 4; i++) {
        sum += m_levelRecords[i].m_powerupzAvailable;
    }
    return sum;
}

RVA(0x000fd230, 0x37)
i32 CGameStats::SumSecretsFoundForCurrentArea() {
    i32 sum = 0;
    i32 areaFirstIndex = (m_levelNumber - 1) / 4 * 4;
    for (i32 i = areaFirstIndex; i < areaFirstIndex + 4; i++) {
        sum += m_levelRecords[i].m_secretsFound;
    }
    return sum;
}

RVA(0x000fd280, 0x37)
i32 CGameStats::SumSecretsAvailableForCurrentArea() {
    i32 sum = 0;
    i32 areaFirstIndex = (m_levelNumber - 1) / 4 * 4;
    for (i32 i = areaFirstIndex; i < areaFirstIndex + 4; i++) {
        sum += m_levelRecords[i].m_secretsAvailable;
    }
    return sum;
}

RVA(0x000fd2d0, 0x37)
i32 CGameStats::SumCoinsCollectedForCurrentArea() {
    i32 sum = 0;
    i32 areaFirstIndex = (m_levelNumber - 1) / 4 * 4;
    for (i32 i = areaFirstIndex; i < areaFirstIndex + 4; i++) {
        sum += m_levelRecords[i].m_coinsCollected;
    }
    return sum;
}

RVA(0x000fd320, 0x37)
i32 CGameStats::SumCoinsAvailableForCurrentArea() {
    i32 sum = 0;
    i32 areaFirstIndex = (m_levelNumber - 1) / 4 * 4;
    for (i32 i = areaFirstIndex; i < areaFirstIndex + 4; i++) {
        sum += m_levelRecords[i].m_coinsAvailable;
    }
    return sum;
}

RVA(0x000fd370, 0x37)
i32 CGameStats::SumGruntzLostForCurrentArea() {
    i32 sum = 0;
    i32 areaFirstIndex = (m_levelNumber - 1) / 4 * 4;
    for (i32 i = areaFirstIndex; i < areaFirstIndex + 4; i++) {
        sum += m_levelRecords[i].m_gruntzLost;
    }
    return sum;
}

RVA(0x000fd3c0, 0x37)
i32 CGameStats::SumGruntzExitedForCurrentArea() {
    i32 sum = 0;
    i32 areaFirstIndex = (m_levelNumber - 1) / 4 * 4;
    for (i32 i = areaFirstIndex; i < areaFirstIndex + 4; i++) {
        sum += m_levelRecords[i].m_gruntzExited;
    }
    return sum;
}

RVA(0x000fd410, 0x37)
i32 CGameStats::SumElapsedTimeForCurrentArea() {
    i32 sum = 0;
    i32 areaFirstIndex = (m_levelNumber - 1) / 4 * 4;
    for (i32 i = areaFirstIndex; i < areaFirstIndex + 4; i++) {
        sum += m_levelRecords[i].m_elapsedTimeMs;
    }
    return sum;
}

RVA(0x000fd460, 0x84)
void CGameStats::UpdateLevelRecord(i32 levelNumber, b32 writeAvailableCounts) {
    QuestLevelStats* levelStats = &m_levelRecords[levelNumber - 1];
    if (writeAvailableCounts == false) {
        levelStats->m_completed = true;
        levelStats->m_elapsedTimeMs = m_elapsedTimeMs;
        levelStats->m_toyzCollected = m_toyzCollected;
        levelStats->m_toolzCollected = m_toolzCollected;
        levelStats->m_gruntzExited = m_gruntzExited;
        levelStats->m_gruntzLost = m_gruntzLost;
        levelStats->m_powerupzCollected = m_powerupzCollected;
        levelStats->m_secretsFound = m_secretsFound;
        levelStats->m_coinsCollected = m_coinsCollected;
        levelStats->m_warpLetterFound = m_warpLetterFound;
        levelStats->m_isEasyMode = g_gameReg->m_isEasyMode;
    } else {
        levelStats->m_toyzAvailable = m_toyzAvailable;
        levelStats->m_toolzAvailable = m_toolzAvailable;
        levelStats->m_powerupzAvailable = m_powerupzAvailable;
        levelStats->m_secretsAvailable = m_secretsAvailable;
        levelStats->m_coinsAvailable = m_coinsAvailable;
    }
}

// @early-stop
RVA(0x000fd520, 0x425)
i32 CGameStats::Serialize(CFileMemBase* s, SerialMode mode, LogicTypeId typeId, i32 payload) {
    i32* p;
    i32 i;
    i32 r;
    i32 c;
    if (s == NULL) {
        return 0;
    }
    if (mode != SERIAL_SAVE) {
        if (mode == SERIAL_LOAD) {
            s->Read(&m_levelNumber, sizeof(m_levelNumber));
            s->Read(&m_isCustomLevel, sizeof(m_isCustomLevel));
            s->Read(&m_currentAreaComplete, sizeof(m_currentAreaComplete));
            s->Read(&m_elapsedTimeMs, sizeof(m_elapsedTimeMs));
            s->Read(&m_toyzCollected, sizeof(m_toyzCollected));
            s->Read(&m_toolzCollected, sizeof(m_toolzCollected));
            s->Read(&m_gruntzExited, sizeof(m_gruntzExited));
            s->Read(&m_gruntzLost, sizeof(m_gruntzLost));
            s->Read(&m_powerupzCollected, sizeof(m_powerupzCollected));
            s->Read(&m_secretsFound, sizeof(m_secretsFound));
            s->Read(&m_coinsCollected, sizeof(m_coinsCollected));
            s->Read(&m_toyzAvailable, sizeof(m_toyzAvailable));
            s->Read(&m_toolzAvailable, sizeof(m_toolzAvailable));
            s->Read(&m_powerupzAvailable, sizeof(m_powerupzAvailable));
            s->Read(&m_secretsAvailable, sizeof(m_secretsAvailable));
            s->Read(&m_coinsAvailable, sizeof(m_coinsAvailable));
            s->Read(&m_warpLetterFound, sizeof(m_warpLetterFound));
            for (p = m_gruntzByPlayer, i = 0; i < 4; i++, p++) {
                s->Read(p, sizeof(*p));
            }
            p = &m_killsByPlayer[0][0];
            for (r = 0; r < 4; r++) {
                for (c = 0; c < 4; c++, p++) {
                    s->Read(p, sizeof(*p));
                }
            }
            p = &m_flagCapturesByPlayer[0][0];
            for (r = 0; r < 4; r++) {
                for (c = 0; c < 4; c++, p++) {
                    s->Read(p, sizeof(*p));
                }
            }
            p = m_weaponPickupsByPlayer;
            for (r = 0; r < 4; r++) {
                for (c = 0; c < 22; c++, p++) {
                    s->Read(p, sizeof(*p));
                }
            }
            p = m_toyPickupsByPlayer;
            for (r = 0; r < 4; r++) {
                for (c = 0; c < 10; c++, p++) {
                    s->Read(p, sizeof(*p));
                }
            }
            p = m_powerupPickupsByPlayer;
            for (r = 0; r < 4; r++) {
                for (c = 0; c < 7; c++, p++) {
                    s->Read(p, sizeof(*p));
                }
            }
            p = m_miscPickupsByPlayer;
            for (r = 0; r < 4; r++) {
                for (c = 0; c < 4; c++, p++) {
                    s->Read(p, sizeof(*p));
                }
            }
        }
    } else {
        s->Write(&m_levelNumber, sizeof(m_levelNumber));
        s->Write(&m_isCustomLevel, sizeof(m_isCustomLevel));
        s->Write(&m_currentAreaComplete, sizeof(m_currentAreaComplete));
        s->Write(&m_elapsedTimeMs, sizeof(m_elapsedTimeMs));
        s->Write(&m_toyzCollected, sizeof(m_toyzCollected));
        s->Write(&m_toolzCollected, sizeof(m_toolzCollected));
        s->Write(&m_gruntzExited, sizeof(m_gruntzExited));
        s->Write(&m_gruntzLost, sizeof(m_gruntzLost));
        s->Write(&m_powerupzCollected, sizeof(m_powerupzCollected));
        s->Write(&m_secretsFound, sizeof(m_secretsFound));
        s->Write(&m_coinsCollected, sizeof(m_coinsCollected));
        s->Write(&m_toyzAvailable, sizeof(m_toyzAvailable));
        s->Write(&m_toolzAvailable, sizeof(m_toolzAvailable));
        s->Write(&m_powerupzAvailable, sizeof(m_powerupzAvailable));
        s->Write(&m_secretsAvailable, sizeof(m_secretsAvailable));
        s->Write(&m_coinsAvailable, sizeof(m_coinsAvailable));
        s->Write(&m_warpLetterFound, sizeof(m_warpLetterFound));
        for (p = m_gruntzByPlayer, i = 0; i < 4; i++, p++) {
            s->Write(p, sizeof(*p));
        }
        p = &m_killsByPlayer[0][0];
        for (r = 0; r < 4; r++) {
            for (c = 0; c < 4; c++, p++) {
                s->Write(p, sizeof(*p));
            }
        }
        p = &m_flagCapturesByPlayer[0][0];
        for (r = 0; r < 4; r++) {
            for (c = 0; c < 4; c++, p++) {
                s->Write(p, sizeof(*p));
            }
        }
        p = m_weaponPickupsByPlayer;
        for (r = 0; r < 4; r++) {
            for (c = 0; c < 22; c++, p++) {
                s->Write(p, sizeof(*p));
            }
        }
        p = m_toyPickupsByPlayer;
        for (r = 0; r < 4; r++) {
            for (c = 0; c < 10; c++, p++) {
                s->Write(p, sizeof(*p));
            }
        }
        p = m_powerupPickupsByPlayer;
        for (r = 0; r < 4; r++) {
            for (c = 0; c < 7; c++, p++) {
                s->Write(p, sizeof(*p));
            }
        }
        p = m_miscPickupsByPlayer;
        for (r = 0; r < 4; r++) {
            for (c = 0; c < 4; c++, p++) {
                s->Write(p, sizeof(*p));
            }
        }
    }
    return 1;
}

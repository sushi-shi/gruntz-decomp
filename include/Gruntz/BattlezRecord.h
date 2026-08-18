#ifndef GRUNTZ_BATTLEZRECORD_H
#define GRUNTZ_BATTLEZRECORD_H

#include <Ints.h>

struct BattlezRecord {
    i32 m_populated;
    i32 m_isEasyMode;
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

#endif // GRUNTZ_BATTLEZRECORD_H

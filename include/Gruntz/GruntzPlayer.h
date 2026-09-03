#ifndef SRC_GRUNTZ_GRUNTZPLAYER_H
#define SRC_GRUNTZ_GRUNTZPLAYER_H

#include <rva.h>

#include <Mfc.h>

#include <Gruntz/BattlezDifficulty.h>
#include <Gruntz/BattlezMapConfig.h>
#include <Gruntz/ColorTint.h>
#include <Gruntz/CoordNode.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>

struct PlayerLatency {
    PlayerLatency() {
        m_avg = 0;
        m_count = 0;
    }
    RVA(0x000832e0, 0x1)
    ~PlayerLatency() {}

    void Clear() {
        m_avg = 0;
        m_count = 0;
    }

    i32 m_avg;
    i32 m_count;
};

class GruntzPlayer {
public:
    GruntzPlayer();
    RVA(0x00083260, 0x57)
    ~GruntzPlayer() {
        Clear();
    }

    i32 SeedForSlot(i32 index);
    void Clear();
    i32 Reset();

    i32 TrySetColor(ColorTint color);
    i32 ClearRoundState();
    RVA(0x0001f450, 0x20)
    CString GetName() {
        return m_name;
    }
    i32 Serialize(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, i32 payload);
    i32 Deactivate();
    CString GetDefaultName(i32);

    i32 m_playerIndex;
    CString m_name;
    ColorTint m_color;

    i32 m_warlordObjectId;
    BattlezDifficulty m_difficulty;
    b32 m_humanControlled;

    i32 m_networkPlayerId;
    b32 m_ready;
    b32 m_active;
    b32 m_clearedRound;
    b32 m_joined;
    b32 m_doneFlag;

    b32 m_optionsPresenceCounted;
    char m_pad034[0x38 - 0x34];

    CBattlezMapConfig m_battlezConfig;
    Coord m_focus;
    i32 m_maxGruntz;

    PlayerLatency m_latency;
    char m_pad234[0x238 - 0x234];
};

#endif

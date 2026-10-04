#ifndef SRC_GRUNTZ_GRUNTZPLAYER_H
#define SRC_GRUNTZ_GRUNTZPLAYER_H

#include <rva.h>

#include <Gruntz/BattlezDifficulty.h>
#include <Gruntz/BattlezMapConfig.h>
#include <Gruntz/ColorTint.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>

struct PlayerLatency {
    PlayerLatency() {
        m_averageRoundTripMs = 0;
        m_sampleCount = 0;
    }
    RVA(0x000832e0, 0x1)
    ~PlayerLatency() {}

    void Clear() {
        m_averageRoundTripMs = 0;
        m_sampleCount = 0;
    }

    i32 m_averageRoundTripMs;
    i32 m_sampleCount;
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

    b32 IsHumanControlled() const {
        return m_humanControlled;
    }

    void SetHumanControlled(b32 controlled) {
        m_humanControlled = controlled;
    }

    i32 GetPlayerIndex() const {
        return m_playerIndex;
    }

    i32 GetNetworkPlayerId() const {
        return m_networkPlayerId;
    }

    i32 GetWarlordObjectId() const {
        return m_warlordObjectId;
    }

    void SetWarlordObjectId(i32 objectId) {
        m_warlordObjectId = objectId;
    }

    i32 GetFocusX() const {
        return m_focusX;
    }

    i32 GetFocusY() const {
        return m_focusY;
    }

    ColorTint GetColor() const {
        return m_color;
    }

    b32 IsActive() const {
        return m_active;
    }

    b32 HasJoinedRound() const {
        return m_joined;
    }

    b32 HasDropped() const {
        return m_dropped;
    }

    b32 IsEliminated() const {
        return m_eliminated;
    }

    void SetEliminated(b32 eliminated) {
        m_eliminated = eliminated;
    }

    BattlezDifficulty GetDifficulty() const {
        return m_difficulty;
    }

    i32 GetMaxGruntz() const {
        return m_maxGruntz;
    }

    CBattlezAiController* GetBattlezAiController() {
        return &m_battlezAiController;
    }

    i32 m_playerIndex;
    CString m_name;
    ColorTint m_color;

    i32 m_warlordObjectId;
    BattlezDifficulty m_difficulty;
    b32 m_humanControlled;

    i32 m_networkPlayerId;
    b32 m_ready;
    b32 m_active;
    b32 m_eliminated;
    b32 m_joined;
    b32 m_dropped;

    b32 m_optionsPresenceCounted;

    CBattlezAiController m_battlezAiController;
    i32 m_focusX;
    i32 m_focusY;
    i32 m_maxGruntz;

    PlayerLatency m_latency;
};

#define CLEAR_GRUNTZ_PLAYER                                                                        \
    m_playerIndex = -1;                                                                            \
    m_networkPlayerId = -2;                                                                        \
    m_active = false;                                                                              \
    m_humanControlled = true;                                                                      \
    m_name = "";                                                                                   \
    m_color = TINT_ORANGE;                                                                         \
    m_difficulty = BZDIFF_EASY;                                                                    \
    m_focusX = 0;                                                                                  \
    m_focusY = 0;                                                                                  \
    m_maxGruntz = 0xf;                                                                             \
    m_dropped = false;                                                                             \
    m_optionsPresenceCounted = false;                                                              \
    m_latency.Clear()

#endif

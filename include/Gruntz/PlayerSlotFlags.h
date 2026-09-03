#ifndef GRUNTZ_GRUNTZ_PLAYERSLOTFLAGS_H
#define GRUNTZ_GRUNTZ_PLAYERSLOTFLAGS_H

#include <Gruntz/PlayerSlot.h>
#include <Ints.h>

struct PlayerSlotFlags {
    PlayerSlotFlags() {}

    i32& operator[](i32 index) {
        return m_values[index];
    }

    const i32& operator[](i32 index) const {
        return m_values[index];
    }

    void Init(i32 player0, i32 player1, i32 player2, i32 player3) {
        m_values[0] = player0;
        m_values[1] = player1;
        m_values[2] = player2;
        m_values[3] = player3;
    }

    void Clear() {
        Init(0, 0, 0, 0);
    }

    void EnableAll() {
        Init(1, 1, 1, 1);
    }

    void Enable(PlayerSlot playerSlot) {
        if (playerSlot == PLAYER_SLOT_ALL) {
            EnableAll();
        } else {
            m_values[IDX(playerSlot)] = 1;
        }
    }

    void EnableIfValid(i32 playerSlot) {
        switch (static_cast<PlayerSlot>(playerSlot)) {
            case PLAYER_SLOT_0:
            case PLAYER_SLOT_1:
            case PLAYER_SLOT_2:
            case PLAYER_SLOT_3:
                m_values[playerSlot] = 1;
                break;
            case PLAYER_SLOT_ALL:
                EnableAll();
                break;
        }
    }

    i32 m_values[PLAYER_SLOT_COUNT];
};

#endif // GRUNTZ_GRUNTZ_PLAYERSLOTFLAGS_H

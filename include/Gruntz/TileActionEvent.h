#ifndef GRUNTZ_TILEACTIONEVENT_H
#define GRUNTZ_TILEACTIONEVENT_H

#include <rva.h>

#include <Enums.h>
#include <Gruntz/BrickTileId.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/PlayerSlot.h>
#include <Gruntz/SerialArchive.h>

class CTileTriggerContainer;
class CGrunt;

class CBrickStack {
public:
    CBrickStack();

    ~CBrickStack() {
        m_initialized = false;
    }

    i32 Build(
        CTileTriggerContainer* owner,
        BrickTileId code,
        i32 tileX,
        i32 tileY,
        i32 cellKey,
        const RECT& revealedToPlayer
    ) {
        if (m_initialized != false) {
            return 0;
        }
        m_brickTile = code;
        m_tileX = tileX;
        m_tileY = tileY;
        m_cellKey = cellKey;
        m_owner = owner;
        m_initialized = true;
        m_revealedToPlayer[0] = revealedToPlayer.left;
        m_revealedToPlayer[1] = revealedToPlayer.top;
        m_revealedToPlayer[2] = revealedToPlayer.right;
        m_revealedToPlayer[3] = revealedToPlayer.bottom;
        SetBrickTile(code);
        return 1;
    }

    i32 IsRevealedToPlayer(i32 playerIndex) const {
        return m_revealedToPlayer[playerIndex];
    }

    BrickTileId GetBrickTile() const {
        return m_brickTile;
    }

    i32 IsClaimCandidate(i32 playerIndex) const {
        BrickTileId id = GetBrickTile();
        i32 occ = IsRevealedToPlayer(playerIndex);
        i32 special = 0;
        if (occ == 0) {
            special = 1;
        }
        if (occ != 0) {
            if (id == BRICKTILE_RED_1 || id == BRICKTILE_RED_2_TOP || id == BRICKTILE_RED_3_TOP
                || id == BRICKTILE_BLACK_1 || id == BRICKTILE_BLACK_2_TOP
                || id == BRICKTILE_BLACK_3_TOP || id == BRICKTILE_BLUE_1
                || id == BRICKTILE_BLUE_2_TOP || id == BRICKTILE_BLUE_3_TOP
                || id == BRICKTILE_BROWN_1 || id == BRICKTILE_BROWN_2 || id == BRICKTILE_BROWN_3) {
                special = 1;
            }
        }
        return special;
    }

    i32 SetBrickTile(BrickTileId code);

    i32 BreakTopBrick(CGrunt* grunt);

    i32 AddTopBrick(PickupType toolId, PlayerSlot playerSlot);

    i32 Serialize(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, i32 payload);

    i32 DeserializeFields(CFileMemBase* ar);

    i32 SerializeFields(CFileMemBase* ar);

    BrickTileId m_brickTile;
    i32 m_tileX;
    i32 m_tileY;
    i32 m_cellKey;
    b32 m_initialized;

    CTileTriggerContainer* m_owner;
    i32 m_revealedToPlayer[4];
};

#endif // GRUNTZ_TILEACTIONEVENT_H

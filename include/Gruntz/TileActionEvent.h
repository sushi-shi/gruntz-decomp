#ifndef GRUNTZ_TILEACTIONEVENT_H
#define GRUNTZ_TILEACTIONEVENT_H

#include <rva.h>

#include <Enums.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/PlayerSlot.h>
#include <Gruntz/SerialArchive.h>

GZ_ENUM_FORWARD(BrickTileId);

class CTileTriggerContainer;
class CGrunt;

class CTileActionEvent {
public:
    CTileActionEvent();

    ~CTileActionEvent() {
        m_live = false;
    }

    i32 Build(
        CTileTriggerContainer* owner,
        BrickTileId code,
        i32 tileX,
        i32 tileY,
        i32 cellKey,
        const RECT& revealedToPlayer
    ) {
        if (m_live != false) {
            return 0;
        }
        m_actionCode = code;
        m_tileX = tileX;
        m_tileY = tileY;
        m_cellKey = cellKey;
        m_owner = owner;
        m_live = true;
        m_revealedToPlayer[0] = revealedToPlayer.left;
        m_revealedToPlayer[1] = revealedToPlayer.top;
        m_revealedToPlayer[2] = revealedToPlayer.right;
        m_revealedToPlayer[3] = revealedToPlayer.bottom;
        SetActionCode(code);
        return 1;
    }

    i32 IsRevealedToPlayer(i32 playerIndex) const {
        return m_revealedToPlayer[playerIndex];
    }

    BrickTileId GetActionCode() const {
        return m_actionCode;
    }

    i32 SetActionCode(BrickTileId code);

    i32 BreakTopBrick(CGrunt* grunt);

    i32 MorphByTool(PickupType toolId, PlayerSlot playerSlot);

    i32 Serialize(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, i32 payload);

    i32 DeserializeFields(CFileMemBase* ar);

    i32 SerializeFields(CFileMemBase* ar);

    BrickTileId m_actionCode;
    i32 m_tileX;
    i32 m_tileY;
    i32 m_cellKey;
    b32 m_live;

    CTileTriggerContainer* m_owner;
    i32 m_revealedToPlayer[4];
};

#endif // GRUNTZ_TILEACTIONEVENT_H

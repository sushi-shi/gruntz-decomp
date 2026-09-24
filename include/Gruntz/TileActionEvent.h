#ifndef GRUNTZ_TILEACTIONEVENT_H
#define GRUNTZ_TILEACTIONEVENT_H

#include <rva.h>

#include <Enums.h>
#include <Gruntz/CoordNode.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/PlayerSlot.h>
#include <Gruntz/PlayerSlotFlags.h>
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

    i32 SetActionCode(BrickTileId code);

    i32 BreakTopBrick(CGrunt* grunt);

    i32 MorphByTool(PickupType toolId, PlayerSlot playerSlot);

    i32 Serialize(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, i32 payload);

    i32 DeserializeFields(CFileMemBase* ar);

    i32 SerializeFields(CFileMemBase* ar);

    BrickTileId m_actionCode;
    Coord m_tile;
    i32 m_cellKey;
    b32 m_live;

    CTileTriggerContainer* m_owner;
    PlayerSlotFlags m_playerFlags;
};

#endif // GRUNTZ_TILEACTIONEVENT_H

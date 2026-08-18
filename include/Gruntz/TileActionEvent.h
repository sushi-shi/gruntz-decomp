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
        m_live = 0;
    }

    i32 SetActionCode(BrickTileId code);

    i32 Process(CGrunt* brick);

    i32 MorphByTool(PickupType toolId, PlayerSlot playerSlot);

    i32 Serialize(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, i32 pObj);

    i32 DeserializeFields(CFileMemBase* a);

    i32 SerializeFields(CFileMemBase* a);

    BrickTileId m_actionCode;
    i32 m_tileX;
    i32 m_tileY;
    i32 m_cellKey;
    i32 m_live;

    CTileTriggerContainer* m_owner;
    i32 m_playerFlags[4];
};

#endif // GRUNTZ_TILEACTIONEVENT_H

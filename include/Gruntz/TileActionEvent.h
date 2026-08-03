#ifndef GRUNTZ_TILEACTIONEVENT_H
#define GRUNTZ_TILEACTIONEVENT_H

#include <rva.h>

#include <Enums.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>

class CTileTriggerContainer;
class CGrunt;

// Two readings of one storage: the `playerSlot` parameter is an index into
// CTileActionEvent::m_playerFlags[4] AND carries this "every player" sentinel,
// so it stays i32 and only the sentinel is named (a constant bag, not a type).
GZ_ENUM_CONST_BEGIN(PlayerSlot)
    PLAYERSLOT_ALL = 5
GZ_ENUM_CONST_END(PlayerSlot)

class CTileActionEvent {
public:
    CTileActionEvent();

    ~CTileActionEvent() {
        m_live = 0;
    }

    i32 SetActionCode(i32 code);

    i32 Process(CGrunt* brick);

    i32 MorphByTool(i32 toolId, i32 playerSlot);

    i32 Serialize(void* ar, SerialMode mode, LogicTypeId typeId, i32 pObj);

    i32 DeserializeFields(void* ar);

    i32 SerializeFields(void* ar);

    i32 m_actionCode;
    i32 m_tileX;
    i32 m_tileY;
    i32 m_cellKey;
    i32 m_live;

    CTileTriggerContainer* m_owner;
    i32 m_playerFlags[4];
};
SIZE(0x28);

#endif // GRUNTZ_TILEACTIONEVENT_H

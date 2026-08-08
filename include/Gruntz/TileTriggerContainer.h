#ifndef SRC_GRUNTZ_TILETRIGGERCONTAINER_H
#define SRC_GRUNTZ_TILETRIGGERCONTAINER_H

#include <rva.h>

#include <Mfc.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/TileActionEvent.h>
#include <Gruntz/TileTriggerLogic.h>
#include <Ints.h>

class CTileTriggerContainer;
class CTileTriggerLogic;
class CGiantRockLogic;
class CTileTriggerSwitchLogic;
struct CGameObject;

class CTileTriggerContainer {
public:
    CTileTriggerContainer() {
        m_built = 0;
    }

    // Retail loads `this` into ecx at both call sites (`mov ecx,edi`), so these
    // are __thiscall members that ignore the receiver, not free functions.
    i32 SerializeApplyA(
        CFileMemBase* s,
        SerialMode mode,
        LogicTypeId typeId,
        i32 pObj,
        CTileTriggerSwitchLogic* o
    );
    i32 SerializeApplyB(
        CFileMemBase* s,
        SerialMode mode,
        LogicTypeId typeId,
        i32 pObj,
        CTileTriggerLogic* o
    );

    i32 DelFromList1(CTileTriggerLogic* elem);

    CTileTriggerLogic* FindInLists12(i32 a, TrigLogicId b);
    i32 FilterList2(i32 arg);
    i32 MoveList1ToList2(void* data);

    i32 DelFromList3(CTileActionEvent* evt);

    ~CTileTriggerContainer();

    CTileTriggerLogic* AddLogic(
        TileCollisionKind tileType,
        TrigLogicId logicType,
        i32 tileX,
        i32 tileY,
        i32 cellKey,

        RECT extent,
        RECT area,
        RECT switchRect,
        RECT clip,
        RECT switchRectA,
        RECT switchRectB,
        i32 tileToken,
        i32 dutyOnSpan,
        i32 leadInSpan,
        i32 dutyOffSpan
    );

    CTileTriggerLogic* AddLogicDefaults(
        TileCollisionKind tileType,
        TrigLogicId logicType,
        i32 tileX,
        i32 tileY,
        i32 cellKey,
        i32 tileToken,
        i32 dutyOnSpan,
        i32 leadInSpan,
        i32 dutyOffSpan
    );

    void AddLogicFromRecord(TileCollisionKind tileType, TrigLogicId logicType, CGameObject* object);

    CTileActionEvent* AddToList3(
        BrickTileId actionCode,
        i32 tileX,
        i32 tileY,
        i32 cellKey,
        i32 player0,
        i32 player1,
        i32 player2,
        i32 player3
    );

    CGiantRockLogic* AddToList1(
        i32 tileX,
        i32 tileY,
        i32 cellKey,
        i32* block9,
        i32 powerupType,
        i32 textId,
        i32 dutyOffSpan
    );

    CTileActionEvent*
    AddToList3Switch(BrickTileId actionCode, i32 tileX, i32 tileY, i32 cellKey, i32 playerSlot);

    i32 GetFlag74();
    i32 RemoveByKeys(i32 k1, TrigLogicId k2);

    CTileTriggerSwitchLogic* FindChild(i32 k1, TrigLogicId k2);

    CTileActionEvent* FindActionByCellKey(i32 cellKey);

    CGiantRockLogic* ScanNeighborhood(i32 x, i32 y);

    CTileTriggerSwitchLogic* AddSwitchLogic(
        TrigLogicId tag,
        i32 col,
        i32 row,
        i32 key,

        RECT extent,
        RECT area,
        RECT switchRect,
        RECT clip,
        RECT switchRectA,
        RECT switchRectB,
        i32 isMatch,
        i32 damageParam,
        i32 checkpointType
    );

    i32 Serialize(CFileMemBase* s, SerialMode op, LogicTypeId typeId, i32 pObj);

    void* LoadElement(CFileMemBase* s, SerialMode op, LogicTypeId typeId, i32 pObj);

    i32 LoadFlag74(CFileMemBase* s);
    i32 TransferFlag74(CFileMemBase* s);

    void RemoveAll();

    i32 SetCell(i32 tileX, i32 tileY, i32 playerSlot);

    void DtorBase();

    CPtrList m_base;
    CPtrList m_list1;
    CPtrList m_list2;
    CPtrList m_list3;
    CTileTriggerLogic* m_latchedLeaf;
    i32 m_built;
};
SIZE_UNKNOWN();

#endif // SRC_GRUNTZ_TILETRIGGERCONTAINER_H

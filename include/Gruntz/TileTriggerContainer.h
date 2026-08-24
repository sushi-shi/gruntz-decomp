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
        m_initialized = 0;
    }

    // Retail loads `this` into ecx at both call sites (`mov ecx,edi`), so these
    // are __thiscall members that ignore the receiver, not free functions.
    i32 SerializeSwitchLogic(
        CFileMemBase* archive,
        SerialMode mode,
        LogicTypeId typeId,
        i32 payload,
        CTileTriggerSwitchLogic* logic
    );
    i32 SerializeTriggerLogic(
        CFileMemBase* archive,
        SerialMode mode,
        LogicTypeId typeId,
        i32 payload,
        CTileTriggerLogic* logic
    );

    i32 RemoveIdleLogic(CTileTriggerLogic* logic);

    CTileTriggerLogic* FindLogic(i32 cellKey, TrigLogicId logicType);
    i32 UpdateTimedLogics(i32 unusedFrameDelta);
    i32 ActivateTimedLogic(CTileTriggerLogic* logic);

    i32 RemoveActionEvent(CTileActionEvent* evt);

    // Inline, like the ctor above: retail's out-of-line copy is a COMDAT emitted by
    // play.obj - 0xc8640 is interleaved between CPlay::LoadGameAssetNamespaces
    // (0xc7ec0+0x5f5) and CPlay::ReleaseResources (0xc8700), which are also its only
    // two direct callers.  The 0x70 body is mostly compiler-generated: the four
    // CPtrList member dtors plus the /GX unwind states around them.
    RVA(0x000c8640, 0x70)
    ~CTileTriggerContainer() {
        Shutdown();
    }

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

    // A brick's four per-player flags reach the event in the level record's
    // extent rect, which the caller passes by value: retail's call site builds
    // one 16-byte block (`lea edi,[obj+0x134]` / `sub esp,0x10` / four dword
    // copies) rather than pushing four fields.
    CTileActionEvent*
    AddActionEvent(BrickTileId actionCode, i32 tileX, i32 tileY, i32 cellKey, RECT playerFlags);

    CGiantRockLogic* AddGiantRockLogic(
        i32 tileX,
        i32 tileY,
        i32 cellKey,
        i32* block9,
        i32 powerupType,
        i32 textId,
        i32 dutyOffSpan
    );

    CTileActionEvent*
    AddSwitchActionEvent(BrickTileId actionCode, i32 tileX, i32 tileY, i32 cellKey, i32 playerSlot);

    i32 Initialize();
    i32 RemoveSwitchLogic(i32 cellKey, TrigLogicId logicType);

    CTileTriggerSwitchLogic* FindSwitchLogic(i32 cellKey, TrigLogicId logicType);

    CTileActionEvent* FindActionByCellKey(i32 cellKey);

    CGiantRockLogic* ScanNeighborhood(i32 tileX, i32 tileY);

    CTileTriggerSwitchLogic* AddSwitchLogic(
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
        i32 isMatch,
        i32 damageParam,
        i32 checkpointType
    );

    i32 Serialize(CFileMemBase* archive, SerialMode mode, LogicTypeId typeId, i32 payload);

    // Heterogeneous factory: switch arms return CTileTriggerSwitchLogic-family
    // objects for m_switchLogics; trigger arms return the incompatible CTileTriggerLogic
    // family for m_idleLogics/m_timedLogics. Their vtable slot zero signatures differ, so
    // there is no common polymorphic base to substitute for this void* seam.
    void* LoadLogic(CFileMemBase* archive, SerialMode mode, LogicTypeId typeId, i32 payload);

    i32 LoadInitialized(CFileMemBase* archive);
    i32 SaveInitialized(CFileMemBase* archive);

    void RemoveAll();

    i32 SetCell(i32 tileX, i32 tileY, i32 playerSlot);

    void Shutdown();

    CPtrList m_switchLogics;
    CPtrList m_idleLogics;
    CPtrList m_timedLogics;
    CPtrList m_actionEvents;
    CTileTriggerLogic* m_latchedLeaf;
    i32 m_initialized;
};

#endif // SRC_GRUNTZ_TILETRIGGERCONTAINER_H

#ifndef GRUNTZ_GRUNTZCMDMGR_H
#define GRUNTZ_GRUNTZCMDMGR_H

#include <rva.h>

#include <Mfc.h>

#include <Gruntz/GruntzCommand.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Ints.h>

class CState;

class CGruntzMgr;

class CGruntzCmdMgr {
public:
    CGruntzCmdMgr();

    void RemoveScheduledCommand(i32 playerIndex, i32 scheduleSlot);

    i32 SetManager(CGruntzMgr* manager);

    void Shutdown();

    i32 ExecuteScheduledCommands(i32 scheduleSlot);

    void RecycleQueuedCommands();

    void ClearCommands();

    void EnqueueSingle(
        b32 isLocalCommand,
        char playerIndex,
        char unitIndex,
        char commandKind,
        i16 targetXOrPlayerIndex,
        i16 targetYOrUnitIndex,
        char pickupType,
        char scheduleSlot
    );

    void EnqueueMulti(
        b32 isLocalCommand,
        char playerIndex,
        u8 unitCount,
        u8* unitIndices,
        char commandKind,
        i16 targetXOrPlayerIndex,
        i16 targetYOrUnitIndex,
        char scheduleSlot
    );

    void EnqueueCommand(b32 isLocalCommand, CGruntzCommand* command);

    i32 Serialize(CFileMemBase* stream, SerialMode mode, LogicTypeId typeId, i32 payload);

    i32 CanSaveCommands(CFileMemBase* stream);

    i32 CanLoadCommands(CFileMemBase* stream);

    void EnqueuePlaceGruntAtScreenPoint(
        b32 isLocalCommand,
        i32 playerIndex,
        i32 screenX,
        i32 screenY,
        i32 scheduleSlot
    );

    RVA(0x00085af0, 0x56)
    ~CGruntzCmdMgr() {
        Shutdown();
    }

    CPtrList m_queuedCommands;
    CPtrList m_pendingLocalCommands;
    CGruntzMgr* m_manager;
};

inline CGruntzCmdMgr::CGruntzCmdMgr() {
    m_manager = NULL;
    m_pendingLocalCommands.RemoveAll();
}

#endif // GRUNTZ_GRUNTZCMDMGR_H

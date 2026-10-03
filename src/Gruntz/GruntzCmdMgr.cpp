#include <StdAfx.h>

#include <Ints.h>

#include <Gruntz/GruntzCmdMgr.h>

#include <Gruntz/GameLevel.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzCommand.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/Play.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/State.h>
#include <Gruntz/WwdGameReg.h>
#include <Io/FileMem.h>
#include <Rez/RezSync.h>
#include <Utils/PackedReadWrite.h>
#include <Wap32/TileGeometry.h>

#include <string.h>

const u16 g_unitIndexBitTable[16] = {
    1,
    2,
    4,
    8,
    0x10,
    0x20,
    0x40,
    0x80,
    0x100,
    0x200,
    0x400,
    0x800,
    0x1000,
    0x2000,
    0x4000,
    0x8000
};

i32 CGruntzCmdMgr::SetManager(CGruntzMgr* manager) {
    m_manager = manager;
    return 1;
}

void CGruntzCmdMgr::Shutdown() {
    m_manager = NULL;
    ClearCommands();
}

i32 CGruntzCmdMgr::ExecuteScheduledCommands(i32 scheduleSlot) {
    b32 isMultiplayer = (m_manager->m_curState->Update() == GAMESTATE_MULTI);
    CState* state = m_manager->m_curState;
    CGruntzCommand* commandsByPlayer[4];
    commandsByPlayer[0] = NULL;
    commandsByPlayer[1] = NULL;
    commandsByPlayer[2] = NULL;
    commandsByPlayer[3] = NULL;
    i32 i;
    for (i = 0; i < m_queuedCommands.GetCount(); i++) {
        POSITION pos = m_queuedCommands.FindIndex(i);
        CGruntzCommand* command = static_cast<CGruntzCommand*>(m_queuedCommands.GetAt(pos));
        GruntzCommandSubmitFlags flags = command->m_submitFlags;
        if (!(flags & COMMAND_SUBMIT_IMMEDIATE)) {
            if (!(flags & COMMAND_SUBMIT_SCHEDULED)) {
                continue;
            }
            if (static_cast<u8>(command->m_scheduleSlot) != static_cast<u32>(scheduleSlot)) {
                continue;
            }
        }
        if (isMultiplayer) {
            commandsByPlayer[command->m_playerIndex] = command;
        } else {
            command->Execute(state);
            command->Recycle();
        }
        m_queuedCommands.RemoveAt(pos);
        i--;
    }
    if (isMultiplayer) {
        for (i = 0; i < 4; i++) {
            CGruntzCommand* command = commandsByPlayer[i % 4];
            if (command) {
                command->Execute(state);
                command->Recycle();
            }
        }
    }
    return 1;
}

void CGruntzCmdMgr::RemoveScheduledCommand(i32 playerIndex, i32 scheduleSlot) {
    for (i32 i = 0; i < m_queuedCommands.GetCount(); i++) {
        POSITION pos = m_queuedCommands.FindIndex(i);
        CGruntzCommand* command = static_cast<CGruntzCommand*>(m_queuedCommands.GetAt(pos));
        if (command->m_scheduleSlot == static_cast<u8>(scheduleSlot)
            && command->m_playerIndex == static_cast<u8>(playerIndex)) {
            m_queuedCommands.RemoveAt(pos);
            command->Recycle();
            return;
        }
    }
}

void CGruntzCmdMgr::RecycleQueuedCommands() {
    while (m_queuedCommands.GetCount()) {
        CGruntzCommand* command = static_cast<CGruntzCommand*>(m_queuedCommands.RemoveTail());
        if (command) {
            command->Recycle();
        }
    }
}

void CGruntzCmdMgr::ClearCommands() {
    RecycleQueuedCommands();
    m_pendingLocalCommands.RemoveAll();
    CGruntzSingleCommand::ReleasePool();
    CGruntzMultiCommand::ReleasePool();
}

void CGruntzCmdMgr::EnqueueSingle(
    b32 isLocalCommand,
    char playerIndex,
    char unitIndex,
    char commandKind,
    i16 targetXOrPlayerIndex,
    i16 targetYOrUnitIndex,
    char pickupType,
    char scheduleSlot
) {
    CGruntzSingleCommand* command = CGruntzSingleCommand::Allocate();
    command->InitializeSingle(
        playerIndex,
        commandKind,
        scheduleSlot,
        targetXOrPlayerIndex,
        targetYOrUnitIndex,
        unitIndex,
        pickupType
    );
    EnqueueCommand(isLocalCommand, command);
}

void CGruntzCmdMgr::EnqueueMulti(
    b32 isLocalCommand,
    char playerIndex,
    u8 unitCount,
    u8* unitIndices,
    char commandKind,
    i16 targetXOrPlayerIndex,
    i16 targetYOrUnitIndex,
    char scheduleSlot
) {
    CGruntzMultiCommand* command = CGruntzMultiCommand::Allocate();
    command->InitializeMulti(
        playerIndex,
        commandKind,
        scheduleSlot,
        targetXOrPlayerIndex,
        targetYOrUnitIndex,
        unitCount,
        unitIndices
    );
    EnqueueCommand(isLocalCommand, command);
}

void CGruntzCmdMgr::EnqueueCommand(b32 isLocalCommand, CGruntzCommand* command) {
    if (!command) {
        return;
    }
    if (isLocalCommand) {
        if (m_manager->m_curState->Update() == GAMESTATE_PLAY) {
            command->m_submitFlags = COMMAND_SUBMIT_IMMEDIATE;
        } else if (m_manager->m_curState->Update() == GAMESTATE_MULTI) {
            command->m_submitFlags = COMMAND_SUBMIT_PENDING_SLOT;
        }
        m_pendingLocalCommands.AddTail(command);
    }
    m_queuedCommands.AddTail(command);
}

void CGruntzCmdMgr::EnqueuePlaceGruntAtScreenPoint(
    b32 isLocalCommand,
    i32 playerIndex,
    i32 screenX,
    i32 screenY,
    i32 scheduleSlot
) {
    CGameLevel* level = m_manager->m_world->m_level;
    const RECT* view = &level->m_mainPlane->m_planeViewRect;
    i32 targetX =
        ((view->left - level->m_viewportRect.left + static_cast<u16>(screenX)) & ~TILE_MASK_PX)
        + TILE_HALF_PX;
    i32 targetY =
        ((view->top - level->m_viewportRect.top + static_cast<u16>(screenY)) & ~TILE_MASK_PX)
        + TILE_HALF_PX;
    EnqueueSingle(
        isLocalCommand,
        static_cast<char>(playerIndex),
        0,
        0,
        static_cast<i16>(targetX),
        static_cast<i16>(targetY),
        0,
        static_cast<char>(scheduleSlot)
    );
}

i32 CGruntzCommand::InitializeCommon(
    char playerIndex,
    char commandKind,
    char scheduleSlot,
    i16 targetXOrPlayerIndex,
    i16 targetYOrUnitIndex
) {
    m_playerIndex = playerIndex;
    m_commandKind = commandKind;
    m_scheduleSlot = scheduleSlot;
    m_targetXOrPlayerIndex = targetXOrPlayerIndex;
    m_targetYOrUnitIndex = targetYOrUnitIndex;
    return 1;
}

i32 CGruntzCommand::InitializeSingle(
    char playerIndex,
    char commandKind,
    char scheduleSlot,
    i16 targetXOrPlayerIndex,
    i16 targetYOrUnitIndex,
    char unitIndex,
    char pickupType
) {
    if (!CGruntzCommand::InitializeCommon(
            playerIndex,
            commandKind,
            scheduleSlot,
            targetXOrPlayerIndex,
            targetYOrUnitIndex
        )) {
        return 0;
    }
    m_unitIndex = unitIndex;
    m_pickupType = pickupType;
    return 1;
}

i32 CGruntzCommand::InitializeMulti(
    char playerIndex,
    char commandKind,
    char scheduleSlot,
    i16 targetXOrPlayerIndex,
    i16 targetYOrUnitIndex,
    u8 unitCount,
    u8* unitIndices
) {
    if (!unitIndices) {
        return 0;
    }
    if (unitCount > 0x10) {
        return 0;
    }
    if (!CGruntzCommand::InitializeCommon(
            playerIndex,
            commandKind,
            scheduleSlot,
            targetXOrPlayerIndex,
            targetYOrUnitIndex
        )) {
        return 0;
    }

    m_unitMask = 0;
    for (i32 i = 0; i < unitCount; i++) {
        m_unitMask |= g_unitIndexBitTable[unitIndices[i]];
    }
    return 1;
}

i32 CGruntzSingleCommand::DecodePacket(char* data, i32) {
    char* start = data;
    ++data;
    m_playerIndex = *data++;
    m_commandKind = static_cast<PlayerCommandKind>(*data++);
    m_scheduleSlot = *data++;
    m_targetXOrPlayerIndex = PeekI16(data);
    data += 2;
    m_targetYOrUnitIndex = PeekI16(data);
    data += 2;
    m_unitIndex = *data++;

    if (static_cast<u8>(IDX(m_commandKind)) >= 8) {
        m_pickupType = static_cast<PickupType>(*data++);
    }
    return data - start;
}

i32 CGruntzMultiCommand::DecodePacket(char* data, i32) {
    char* start = data;
    ++data;
    m_playerIndex = *data++;
    m_commandKind = static_cast<PlayerCommandKind>(*data++);
    m_scheduleSlot = *data++;
    m_targetXOrPlayerIndex = PeekI16(data);
    data += 2;
    m_targetYOrUnitIndex = PeekI16(data);
    data += 2;
    m_unitMask = static_cast<u16>(PeekI16(data));
    data += 2;
    return data - start;
}

i32 CGruntzSingleCommand::EncodePacket(char* buffer, i32) {
    char* start = buffer;
    *buffer = static_cast<char>(GetRecordKind());
    *++buffer = m_playerIndex;
    *++buffer = static_cast<char>(IDX(m_commandKind));
    *++buffer = m_scheduleSlot;
    char* w = buffer + 1;
    PokeI16(w, static_cast<i16>(m_targetXOrPlayerIndex));
    w += 2;
    PokeI16(w, static_cast<i16>(m_targetYOrUnitIndex));
    w += 2;
    *w = m_unitIndex;
    w++;
    if (static_cast<u8>(IDX(m_commandKind)) >= 8) {
        *w = static_cast<char>(IDX(m_pickupType));
        w++;
    }
    return w - start;
}

i32 CGruntzMultiCommand::EncodePacket(char* buffer, i32) {
    char* start = buffer;
    *buffer = static_cast<char>(GetRecordKind());
    *++buffer = m_playerIndex;
    *++buffer = static_cast<char>(IDX(m_commandKind));
    *++buffer = m_scheduleSlot;
    char* w = buffer + 1;
    PokeI16(w, static_cast<i16>(m_targetXOrPlayerIndex));
    w += 2;
    PokeI16(w, static_cast<i16>(m_targetYOrUnitIndex));
    w += 2;
    PokeI16(w, static_cast<i16>(m_unitMask));
    w += 2;
    return w - start;
}

i32 CGruntzSingleCommand::Execute(CState* state) {
    CPlay* p = static_cast<CPlay*>(state);
    if (!p) {
        return 0;
    }

    return p->ExecuteCommand(
        m_playerIndex,
        m_unitIndex,
        static_cast<PlayerCommandKind>(m_commandKind),
        m_targetXOrPlayerIndex,
        m_targetYOrUnitIndex,
        static_cast<char>(IDX(m_pickupType)),
        m_scheduleSlot
    );
}

i32 CGruntzMultiCommand::Execute(CState* state) {
    CPlay* p = static_cast<CPlay*>(state);
    if (!p) {
        return 0;
    }
    b32 ok = true;
    for (i32 i = 0; i < 16; i++) {
        if (g_unitIndexBitTable[i] & m_unitMask) {
            if (!p->ExecuteCommand(
                    m_playerIndex,
                    static_cast<char>(i),
                    static_cast<PlayerCommandKind>(m_commandKind),
                    m_targetXOrPlayerIndex,
                    m_targetYOrUnitIndex,
                    0,
                    m_scheduleSlot
                )) {
                ok = false;
            }
        }
    }
    return ok;
}

CGruntzSingleCommand* CGruntzSingleCommand::Allocate() {
    CPtrList& freeList = CPtrListPool<CGruntzSingleCommand>::s_freeList;
    if (freeList.GetCount()) {
        return static_cast<CGruntzSingleCommand*>(freeList.RemoveTail());
    }
    return new CGruntzSingleCommand;
}

i32 CGruntzSingleCommand::UnusedCommandQuery() {
    return 1;
}

char CGruntzSingleCommand::GetRecordKind() {
    return static_cast<char>(IDX(COMMAND_RECORD_SINGLE));
}

void CGruntzSingleCommand::Recycle() {
    CPtrListPool<CGruntzSingleCommand>::s_freeList.AddHead(this);
}

i32 CGruntzCommand::UnusedCommandQuery() {
    return 1;
}

CGruntzMultiCommand* CGruntzMultiCommand::Allocate() {
    CPtrList& freeList = CPtrListPool<CGruntzMultiCommand>::s_freeList;
    if (freeList.GetCount()) {
        return static_cast<CGruntzMultiCommand*>(freeList.RemoveTail());
    }
    return new CGruntzMultiCommand;
}

i32 CGruntzMultiCommand::UnusedCommandQuery() {
    return 1;
}

char CGruntzMultiCommand::GetRecordKind() {
    return static_cast<char>(IDX(COMMAND_RECORD_MULTI));
}

void CGruntzMultiCommand::Recycle() {
    CPtrListPool<CGruntzMultiCommand>::s_freeList.AddHead(this);
}

void CGruntzSingleCommand::ReleasePool() {
    CPtrList& freeList = CPtrListPool<CGruntzSingleCommand>::s_freeList;
    while (freeList.GetCount()) {
        CGruntzCommand* node = static_cast<CGruntzCommand*>(freeList.RemoveTail());
        if (node) {
            delete node;
        }
    }
}

void CGruntzMultiCommand::ReleasePool() {
    CPtrList& freeList = CPtrListPool<CGruntzMultiCommand>::s_freeList;
    while (freeList.GetCount()) {
        CGruntzCommand* node = static_cast<CGruntzCommand*>(freeList.RemoveTail());
        if (node) {
            delete node;
        }
    }
}

i32 CGruntzSingleCommand::Serialize(CFileMemBase* s, SerialMode mode, LogicTypeId, i32) {
    if (!s) {
        return 0;
    }
    switch (mode) {
        case SERIAL_SAVE:
            if (!Save(s)) {
                return 0;
            }
            break;
        case SERIAL_LOAD:
            if (!Load(s)) {
                return 0;
            }
            break;
    }
    return 1;
}

i32 CGruntzSingleCommand::Save(CFileMemBase* s) {
    if (!s) {
        return 0;
    }
    if (!g_gameReg->World()) {
        return 0;
    }
    s->Write(&m_playerIndex, sizeof(m_playerIndex));
    s->Write(&m_commandKind, sizeof(m_commandKind));
    s->Write(&m_scheduleSlot, sizeof(m_scheduleSlot));
    s->Write(&m_targetXOrPlayerIndex, sizeof(m_targetXOrPlayerIndex));
    s->Write(&m_targetYOrUnitIndex, sizeof(m_targetYOrUnitIndex));
    s->Write(&m_submitFlags, sizeof(m_submitFlags));
    s->Write(&m_unitIndex, sizeof(m_unitIndex));
    s->Write(&m_pickupType, sizeof(m_pickupType));
    return 1;
}

i32 CGruntzSingleCommand::Load(CFileMemBase* s) {
    if (!s) {
        return 0;
    }
    if (!g_gameReg->World()) {
        return 0;
    }
    s->Read(&m_playerIndex, sizeof(m_playerIndex));
    s->Read(&m_commandKind, sizeof(m_commandKind));
    s->Read(&m_scheduleSlot, sizeof(m_scheduleSlot));
    s->Read(&m_targetXOrPlayerIndex, sizeof(m_targetXOrPlayerIndex));
    s->Read(&m_targetYOrUnitIndex, sizeof(m_targetYOrUnitIndex));
    s->Read(&m_submitFlags, sizeof(m_submitFlags));
    s->Read(&m_unitIndex, sizeof(m_unitIndex));
    s->Read(&m_pickupType, sizeof(m_pickupType));
    return 1;
}

i32 CGruntzMultiCommand::Serialize(CFileMemBase* s, SerialMode mode, LogicTypeId, i32) {
    if (!s) {
        return 0;
    }
    switch (mode) {
        case SERIAL_SAVE:
            if (!Save(s)) {
                return 0;
            }
            break;
        case SERIAL_LOAD:
            if (!Load(s)) {
                return 0;
            }
            break;
    }
    return 1;
}

i32 CGruntzMultiCommand::Save(CFileMemBase* s) {
    if (!s) {
        return 0;
    }
    if (!g_gameReg->World()) {
        return 0;
    }
    s->Write(&m_playerIndex, sizeof(m_playerIndex));
    s->Write(&m_commandKind, sizeof(m_commandKind));
    s->Write(&m_scheduleSlot, sizeof(m_scheduleSlot));
    s->Write(&m_targetXOrPlayerIndex, sizeof(m_targetXOrPlayerIndex));
    s->Write(&m_targetYOrUnitIndex, sizeof(m_targetYOrUnitIndex));
    s->Write(&m_submitFlags, sizeof(m_submitFlags));
    s->Write(&m_unitMask, sizeof(m_unitMask));
    return 1;
}

i32 CGruntzMultiCommand::Load(CFileMemBase* s) {
    if (!s) {
        return 0;
    }
    if (!g_gameReg->World()) {
        return 0;
    }
    s->Read(&m_playerIndex, sizeof(m_playerIndex));
    s->Read(&m_commandKind, sizeof(m_commandKind));
    s->Read(&m_scheduleSlot, sizeof(m_scheduleSlot));
    s->Read(&m_targetXOrPlayerIndex, sizeof(m_targetXOrPlayerIndex));
    s->Read(&m_targetYOrUnitIndex, sizeof(m_targetYOrUnitIndex));
    s->Read(&m_submitFlags, sizeof(m_submitFlags));
    s->Read(&m_unitMask, sizeof(m_unitMask));
    return 1;
}

i32 CGruntzCmdMgr::Serialize(
    CFileMemBase* stream,
    SerialMode mode,
    LogicTypeId typeId,
    i32 payload
) {
    if (!stream) {
        return 0;
    }
    u32 cursorOrCount;
    if (mode != SERIAL_SAVE) {
        if (mode != SERIAL_LOAD) {
            return 1;
        }

        if (!CanLoadCommands(stream)) {
            return 0;
        }
        ClearCommands();
        i32 count;
        stream->Read(&count, sizeof(count));
        cursorOrCount = 0;
        while (cursorOrCount < static_cast<u32>(count)) {
            i32 tagWord;
            stream->Read(&tagWord, sizeof(tagWord));
            GruntzCommandRecordKind tag = static_cast<GruntzCommandRecordKind>(tagWord);
            CGruntzCommand* cmd;
            if (tag == COMMAND_RECORD_SINGLE) {
                cmd = CGruntzSingleCommand::Allocate();
                if (!cmd->Serialize(stream, SERIAL_LOAD, typeId, payload)) {
                    return 0;
                }
            } else if (tag == COMMAND_RECORD_MULTI) {
                cmd = CGruntzMultiCommand::Allocate();
                if (!cmd->Serialize(stream, SERIAL_LOAD, typeId, payload)) {
                    return 0;
                }
            } else {
                return 0;
            }
            m_queuedCommands.AddTail(cmd);
            cursorOrCount++;
        }
        return 1;
    }

    if (!CanSaveCommands(stream)) {
        return 0;
    }
    cursorOrCount = m_queuedCommands.GetCount();
    stream->Write(&cursorOrCount, sizeof(cursorOrCount));

    POSITION pos = m_queuedCommands.GetHeadPosition();
    while (pos != NULL) {
        CGruntzCommand* cmd = static_cast<CGruntzCommand*>(m_queuedCommands.GetNext(pos));
        i32 tagWord = cmd->GetRecordKind() & 0xff;
        stream->Write(&tagWord, sizeof(tagWord));
        if (!cmd->Serialize(stream, SERIAL_SAVE, typeId, payload)) {
            return 0;
        }
    }
    return 1;
}

i32 CGruntzCmdMgr::CanSaveCommands(CFileMemBase* stream) {
    if (!stream) {
        return 0;
    }
    return g_gameReg->World() != NULL;
}

i32 CGruntzCmdMgr::CanLoadCommands(CFileMemBase* stream) {
    if (stream == NULL) {
        return 0;
    }
    return g_gameReg->World() != NULL;
}

#include <rva.h>

#include <Gruntz/GruntzCmdMgr.h>

#include <Mfc.h>

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
#include <Wap32/TileGeometry.h>

#include <string.h>

RVA_DYNINIT(0x000235e0, 0x5, s_gruntDirNorth)
RVA_DYNINIT(0x00023600, 0x1a, s_gruntDirNorth)
RVA_DYNINIT(0x00023630, 0x5, s_gruntDirNorthEast)
RVA_DYNINIT(0x00023650, 0x1a, s_gruntDirNorthEast)
RVA_DYNINIT(0x00023680, 0x5, s_gruntDirEast)
RVA_DYNINIT(0x000236a0, 0x1f, s_gruntDirEast)
RVA_DYNINIT(0x000236d0, 0x5, s_gruntDirSouthEast)
RVA_DYNINIT(0x000236f0, 0x1a, s_gruntDirSouthEast)
RVA_DYNINIT(0x00023720, 0x5, s_gruntDirSouth)
RVA_DYNINIT(0x00023740, 0x1f, s_gruntDirSouth)
RVA_DYNINIT(0x00023770, 0x5, s_gruntDirSouthWest)
RVA_DYNINIT(0x00023790, 0x1f, s_gruntDirSouthWest)
RVA_DYNINIT(0x000237c0, 0x5, s_gruntDirWest)
RVA_DYNINIT(0x000237e0, 0x1f, s_gruntDirWest)
RVA_DYNINIT(0x00023810, 0x5, s_gruntDirNorthWest)
RVA_DYNINIT(0x00023830, 0x17, s_gruntDirNorthWest)
RVA_DYNINIT(0x00023860, 0x5, s_gruntDirCenter)
RVA_DYNINIT(0x00023880, 0x1a, s_gruntDirCenter)

DATA(0x001e9608)
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

static inline i16 PeekI16(const char* p) {
    i16 value;
    memcpy(&value, p, sizeof(value));
    return value;
}
static inline void PokeI16(char* p, i16 v) {
    memcpy(p, &v, sizeof(v));
}

RVA(0x000239d0, 0xf)
i32 CGruntzCmdMgr::SetManager(CGruntzMgr* manager) {
    m_manager = manager;
    return 1;
}

RVA(0x000239f0, 0xc)
void CGruntzCmdMgr::Shutdown() {
    m_manager = NULL;
    ClearCommands();
}

RVA(0x00023a10, 0xe7)
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

RVA(0x00023b40, 0x53)
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

RVA(0x00023bc0, 0x25)
void CGruntzCmdMgr::RecycleQueuedCommands() {
    while (m_queuedCommands.GetCount()) {
        CGruntzCommand* command = static_cast<CGruntzCommand*>(m_queuedCommands.RemoveTail());
        if (command) {
            command->Recycle();
        }
    }
}

RVA(0x00023c00, 0x1c)
void CGruntzCmdMgr::ClearCommands() {
    RecycleQueuedCommands();
    m_pendingLocalCommands.RemoveAll();
    CGruntzSingleCommand::ReleasePool();
    CGruntzMultiCommand::ReleasePool();
}

RVA(0x00023c30, 0x47)
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

RVA(0x00023ca0, 0x47)
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

RVA(0x00023d10, 0x5a)
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

RVA(0x00023d90, 0x64)
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

RVA(0x00023e20, 0x2f)
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

RVA(0x00023e60, 0x42)
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

RVA(0x00023ed0, 0x83)
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

RVA(0x00023f90, 0x48)
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

RVA(0x00024000, 0x3e)
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

RVA(0x00024050, 0x57)
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

RVA(0x000240d0, 0x4d)
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

RVA(0x00024140, 0x35)
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

RVA(0x00024190, 0x6c)
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

RVA(0x00024220, 0x2b)
CGruntzSingleCommand* CGruntzSingleCommand::Allocate() {
    CPtrList& freeList = CPtrListPool<CGruntzSingleCommand>::s_freeList;
    if (freeList.GetCount()) {
        return static_cast<CGruntzSingleCommand*>(freeList.RemoveTail());
    }
    return new CGruntzSingleCommand;
}

RVA(0x00024260, 0x6)
i32 CGruntzSingleCommand::UnusedCommandQuery() {
    return 1;
}

RVA(0x00024280, 0x3)
char CGruntzSingleCommand::GetRecordKind() {
    return static_cast<char>(IDX(COMMAND_RECORD_SINGLE));
}

RVA(0x000242a0, 0xc)
void CGruntzSingleCommand::Recycle() {
    CPtrListPool<CGruntzSingleCommand>::s_freeList.AddHead(this);
}

RVA_COMPGEN(0x000242c0, 0x1e, ??_GCGruntzSingleCommand@@UAEPAXI@Z)
RVA_COMPGEN(0x000242f0, 0x7, ??1CGruntzSingleCommand@@UAE@XZ)

RVA(0x00024310, 0x6)
i32 CGruntzCommand::UnusedCommandQuery() {
    return 1;
}

RVA_COMPGEN(0x00024330, 0x20, ??_GCGruntzCommand@@UAEPAXI@Z)

RVA(0x00024360, 0x2b)
CGruntzMultiCommand* CGruntzMultiCommand::Allocate() {
    CPtrList& freeList = CPtrListPool<CGruntzMultiCommand>::s_freeList;
    if (freeList.GetCount()) {
        return static_cast<CGruntzMultiCommand*>(freeList.RemoveTail());
    }
    return new CGruntzMultiCommand;
}

RVA(0x000243a0, 0x6)
i32 CGruntzMultiCommand::UnusedCommandQuery() {
    return 1;
}

RVA(0x000243c0, 0x3)
char CGruntzMultiCommand::GetRecordKind() {
    return static_cast<char>(IDX(COMMAND_RECORD_MULTI));
}

RVA(0x000243e0, 0xc)
void CGruntzMultiCommand::Recycle() {
    CPtrListPool<CGruntzMultiCommand>::s_freeList.AddHead(this);
}

RVA_COMPGEN(0x00024400, 0x1e, ??_GCGruntzMultiCommand@@UAEPAXI@Z)
RVA_COMPGEN(0x00024430, 0x7, ??1CGruntzMultiCommand@@UAE@XZ)

RVA(0x00024450, 0x29)
void CGruntzSingleCommand::ReleasePool() {
    CPtrList& freeList = CPtrListPool<CGruntzSingleCommand>::s_freeList;
    while (freeList.GetCount()) {
        CGruntzCommand* node = static_cast<CGruntzCommand*>(freeList.RemoveTail());
        if (node) {
            delete node;
        }
    }
}

RVA(0x00024490, 0x29)
void CGruntzMultiCommand::ReleasePool() {
    CPtrList& freeList = CPtrListPool<CGruntzMultiCommand>::s_freeList;
    while (freeList.GetCount()) {
        CGruntzCommand* node = static_cast<CGruntzCommand*>(freeList.RemoveTail());
        if (node) {
            delete node;
        }
    }
}

RVA(0x000244d0, 0x3b)
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

RVA(0x00024520, 0x98)
i32 CGruntzSingleCommand::Save(CFileMemBase* s) {
    if (!s) {
        return 0;
    }
    if (!g_gameReg->m_world) {
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

RVA(0x000245f0, 0x98)
i32 CGruntzSingleCommand::Load(CFileMemBase* s) {
    if (!s) {
        return 0;
    }
    if (!g_gameReg->m_world) {
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

RVA(0x000246c0, 0x3b)
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

RVA(0x00024710, 0x8b)
i32 CGruntzMultiCommand::Save(CFileMemBase* s) {
    if (!s) {
        return 0;
    }
    if (!g_gameReg->m_world) {
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

RVA(0x000247d0, 0x8b)
i32 CGruntzMultiCommand::Load(CFileMemBase* s) {
    if (!s) {
        return 0;
    }
    if (!g_gameReg->m_world) {
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

RVA(0x00024890, 0x18d)
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

RVA(0x00024a90, 0x20)
i32 CGruntzCmdMgr::CanSaveCommands(CFileMemBase* stream) {
    if (!stream) {
        return 0;
    }
    return g_gameReg->m_world != NULL;
}

RVA(0x00024ac0, 0x20)
i32 CGruntzCmdMgr::CanLoadCommands(CFileMemBase* stream) {
    if (stream == NULL) {
        return 0;
    }
    return g_gameReg->m_world != NULL;
}

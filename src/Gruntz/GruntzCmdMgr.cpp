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

DATA(0x001e9608)
const u16 g_cmdBitTable[16] = {
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

static inline i16 PeekI16(const void* p) {

    return *static_cast<const i16*>(p);
}
static inline void PokeI16(void* p, i16 v) {
    *static_cast<i16*>(p) = v;
}

RVA(0x000239d0, 0xf)
i32 CGruntzCmdMgr::SetMgr(CGruntzMgr* mgr) {
    m_manager = mgr;
    return 1;
}

RVA(0x000239f0, 0xc)
void CGruntzCmdMgr::ClearAndReset() {
    m_manager = NULL;
    Clear();
}

RVA(0x00023a10, 0xe7)
i32 CGruntzCmdMgr::ScanTargets(i32 param) {
    i32 isPlay = (m_manager->m_curState->Update() == GAMESTATE_MULTI);
    CState* sp = m_manager->m_curState;
    CGruntzCommand* table[4];
    table[0] = NULL;
    table[1] = NULL;
    table[2] = NULL;
    table[3] = NULL;
    i32 i;
    for (i = 0; i < m_base.GetCount(); i++) {
        POSITION pos = m_base.FindIndex(i);
        CGruntzCommand* obj = static_cast<CGruntzCommand*>(m_base.GetAt(pos));
        i32 flags = obj->m_submitted;
        if (!(flags & 2)) {
            if (!(flags & 1)) {
                continue;
            }
            if (static_cast<u8>(obj->m_targetType) != static_cast<u32>(param)) {
                continue;
            }
        }
        if (isPlay) {
            table[obj->m_targetIndex] = obj;
        } else {
            obj->Select(sp);
            obj->Deselect();
        }
        m_base.RemoveAt(pos);
        i--;
    }
    if (isPlay) {
        for (i = 0; i < 4; i++) {
            CGruntzCommand* obj = table[i % 4];
            if (obj) {
                obj->Select(sp);
                obj->Deselect();
            }
        }
    }
    return 1;
}

RVA(0x00023b40, 0x53)
void CGruntzCmdMgr::RemoveMatchingTarget(i32 indexByte, i32 typeByte) {
    for (i32 i = 0; i < m_base.GetCount(); i++) {
        POSITION pos = m_base.FindIndex(i);
        CGruntzCommand* obj = static_cast<CGruntzCommand*>(m_base.GetAt(pos));
        if (obj->m_targetType == static_cast<u8>(typeByte)
            && obj->m_targetIndex == static_cast<u8>(indexByte)) {
            m_base.RemoveAt(pos);
            obj->Deselect();
            return;
        }
    }
}

RVA(0x00023bc0, 0x25)
void CGruntzCmdMgr::DrainBase() {
    while (m_base.GetCount()) {
        CGruntzCommand* obj = static_cast<CGruntzCommand*>(m_base.RemoveTail());
        if (obj) {
            obj->Deselect();
        }
    }
}

RVA(0x00023c00, 0x1c)
void CGruntzCmdMgr::Clear() {
    DrainBase();
    m_pendingCommands.RemoveAll();
    CGruntzSingleCommand::FreeAll();
    CGruntzMultiCommand::FreeAll();
}

RVA(0x00023c30, 0x47)
void CGruntzCmdMgr::EnqueueSingle(
    i32 enqueueFlag,
    char targetIndex,
    char gruntIndex,
    char cmdKind,
    i16 posX,
    i16 posY,
    char extraByte,
    char targetType
) {
    CGruntzSingleCommand* cmd = CGruntzSingleCommand::Allocate();
    cmd->SetParamsEx(targetIndex, cmdKind, targetType, posX, posY, gruntIndex, extraByte);
    EnqueueCommand(enqueueFlag, cmd);
}

RVA(0x00023ca0, 0x47)
void CGruntzCmdMgr::EnqueueMulti(
    i32 enqueueFlag,
    char targetIndex,
    u8 count,
    u8* gruntList,
    char cmdKind,
    i16 posX,
    i16 posY,
    char targetType
) {
    CGruntzMultiCommand* cmd = CGruntzMultiCommand::Allocate();
    cmd->SetMaskFromList(targetIndex, cmdKind, targetType, posX, posY, count, gruntList);
    EnqueueCommand(enqueueFlag, cmd);
}

RVA(0x00023d10, 0x5a)
void CGruntzCmdMgr::EnqueueCommand(i32 flag, void* cmd) {
    if (!cmd) {
        return;
    }
    if (flag) {
        if (m_manager->m_curState->Update() == GAMESTATE_PLAY) {
            (static_cast<CGruntzCommand*>(cmd))->m_submitted = 2;
        } else if (m_manager->m_curState->Update() == GAMESTATE_MULTI) {
            (static_cast<CGruntzCommand*>(cmd))->m_submitted = 4;
        }
        m_pendingCommands.AddTail(cmd);
    }
    m_base.AddTail(cmd);
}

// @early-stop
RVA(0x00023d90, 0x64)
void CGruntzCmdMgr::BlitTileMarker(i32 enqueueFlag, i32 targetIndex, i32 x, i32 y, i32 targetType) {
    CGameLevel* p = m_manager->m_world->m_level;
    const RECT* vr = &p->m_mainPlane->m_viewRect;
    i32 sx = ((vr->left - p->m_planeCtx.left + (x & 0xffff)) & ~TILE_MASK_PX) + TILE_HALF_PX;
    i32 sy = ((vr->top - p->m_planeCtx.top + (y & 0xffff)) & ~TILE_MASK_PX) + TILE_HALF_PX;
    EnqueueSingle(
        enqueueFlag,
        static_cast<char>(targetIndex),
        0,
        0,
        static_cast<i16>(sx),
        static_cast<i16>(sy),
        0,
        static_cast<char>(targetType)
    );
}

RVA(0x00023e20, 0x2f)
i32 CGruntzCommand::SetParams(char targetIndex, char cmdKind, char targetType, i16 posX, i16 posY) {
    m_targetIndex = targetIndex;
    m_commandKind = cmdKind;
    m_targetType = targetType;
    m_posX = posX;
    m_posY = posY;
    return 1;
}

RVA(0x00023e60, 0x42)
i32 CGruntzCommand::SetParamsEx(
    char targetIndex,
    char cmdKind,
    char targetType,
    i16 posX,
    i16 posY,
    char gruntIndex,
    char extraByte
) {
    if (!CGruntzCommand::SetParams(targetIndex, cmdKind, targetType, posX, posY)) {
        return 0;
    }
    m_gruntIndex = gruntIndex;
    m_extraByte = extraByte;
    return 1;
}

RVA(0x00023ed0, 0x83)
i32 CGruntzCommand::SetMaskFromList(
    char targetIndex,
    char cmdKind,
    char targetType,
    i16 posX,
    i16 posY,
    u8 count,
    u8* gruntList
) {
    if (!gruntList) {
        return 0;
    }
    if (count > 0x10) {
        return 0;
    }
    if (!CGruntzCommand::SetParams(targetIndex, cmdKind, targetType, posX, posY)) {
        return 0;
    }

    m_gruntMask = 0;
    for (i32 i = 0; i < count; i++) {
        m_gruntMask |= g_cmdBitTable[gruntList[i]];
    }
    return 1;
}

// @early-stop
RVA(0x00023f90, 0x48)
i32 CGruntzSingleCommand::Parse(void* data, i32) {
    char* buf = static_cast<char*>(data) + 1;
    char* start = buf - 1;
    m_targetIndex = *buf++;
    m_commandKind = static_cast<PlayerCommandKind>(*buf++);
    m_targetType = *buf++;
    m_posX = PeekI16(buf);
    buf += 2;
    m_posY = PeekI16(buf);
    buf += 2;
    m_gruntIndex = *buf++;

    if (static_cast<u8>(IDX(m_commandKind)) >= 8) {
        m_extraByte = *buf++;
    }
    return buf - start;
}

// @early-stop
RVA(0x00024000, 0x3e)
i32 CGruntzMultiCommand::Parse(void* data, i32) {
    char* buf = static_cast<char*>(data) + 1;
    char* start = buf - 1;
    m_targetIndex = *buf++;
    m_commandKind = static_cast<PlayerCommandKind>(*buf++);
    m_targetType = *buf++;
    m_posX = PeekI16(buf);
    buf += 2;
    m_posY = PeekI16(buf);
    buf += 2;
    m_gruntMask = static_cast<u16>(PeekI16(buf));
    buf += 2;
    return buf - start;
}

RVA(0x00024050, 0x57)
i32 CGruntzSingleCommand::Pack(char* buf, i32) {
    char* start = buf;
    *buf = static_cast<char>(GetTag());
    *++buf = m_targetIndex;
    *++buf = static_cast<char>(IDX(m_commandKind));
    *++buf = m_targetType;
    char* w = buf + 1;
    PokeI16(w, static_cast<i16>(m_posX));
    w += 2;
    PokeI16(w, static_cast<i16>(m_posY));
    w += 2;
    *w = m_gruntIndex;
    w++;
    if (static_cast<u8>(IDX(m_commandKind)) >= 8) {
        *w = m_extraByte;
        w++;
    }
    return w - start;
}

RVA(0x000240d0, 0x4d)
i32 CGruntzMultiCommand::Pack(char* buf, i32) {
    char* start = buf;
    *buf = static_cast<char>(GetTag());
    *++buf = m_targetIndex;
    *++buf = static_cast<char>(IDX(m_commandKind));
    *++buf = m_targetType;
    char* w = buf + 1;
    PokeI16(w, static_cast<i16>(m_posX));
    w += 2;
    PokeI16(w, static_cast<i16>(m_posY));
    w += 2;
    PokeI16(w, static_cast<i16>(m_gruntMask));
    w += 2;
    return w - start;
}

RVA(0x00024140, 0x35)
i32 CGruntzSingleCommand::Select(CState* state) {
    CPlay* p = static_cast<CPlay*>(state);
    if (!p) {
        return 0;
    }

    return p->ExecCommand(
        m_targetIndex,
        m_gruntIndex,
        static_cast<PlayerCommandKind>(m_commandKind),
        m_posX,
        m_posY,
        m_extraByte,
        m_targetType
    );
}

RVA(0x00024190, 0x6c)
i32 CGruntzMultiCommand::Select(CState* state) {
    CPlay* p = static_cast<CPlay*>(state);
    if (!p) {
        return 0;
    }
    i32 ok = 1;
    for (i32 i = 0; i < 16; i++) {
        if (g_cmdBitTable[i] & m_gruntMask) {
            if (!p->ExecCommand(
                    m_targetIndex,
                    static_cast<char>(i),
                    static_cast<PlayerCommandKind>(m_commandKind),
                    m_posX,
                    m_posY,
                    0,
                    m_targetType
                )) {
                ok = 0;
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
char CGruntzSingleCommand::GetTag() {
    return static_cast<char>(IDX(COMMAND_RECORD_SINGLE));
}

RVA(0x000242a0, 0xc)
void CGruntzSingleCommand::Deselect() {
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
char CGruntzMultiCommand::GetTag() {
    return static_cast<char>(IDX(COMMAND_RECORD_MULTI));
}

RVA(0x000243e0, 0xc)
void CGruntzMultiCommand::Deselect() {
    CPtrListPool<CGruntzMultiCommand>::s_freeList.AddHead(this);
}

RVA_COMPGEN(0x00024400, 0x1e, ??_GCGruntzMultiCommand@@UAEPAXI@Z)
RVA_COMPGEN(0x00024430, 0x7, ??1CGruntzMultiCommand@@UAE@XZ)

RVA(0x00024450, 0x29)
void CGruntzSingleCommand::FreeAll() {
    CPtrList& freeList = CPtrListPool<CGruntzSingleCommand>::s_freeList;
    while (freeList.GetCount()) {
        CGruntzCommand* node = static_cast<CGruntzCommand*>(freeList.RemoveTail());
        if (node) {
            delete node;
        }
    }
}

RVA(0x00024490, 0x29)
void CGruntzMultiCommand::FreeAll() {
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
    s->Write(&m_targetIndex, sizeof(m_targetIndex));
    s->Write(&m_commandKind, sizeof(m_commandKind));
    s->Write(&m_targetType, sizeof(m_targetType));
    s->Write(&m_posX, sizeof(m_posX));
    s->Write(&m_posY, sizeof(m_posY));
    s->Write(&m_submitted, sizeof(m_submitted));
    s->Write(&m_gruntIndex, sizeof(m_gruntIndex));
    s->Write(&m_extraByte, sizeof(m_extraByte));
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
    s->Read(&m_targetIndex, sizeof(m_targetIndex));
    s->Read(&m_commandKind, sizeof(m_commandKind));
    s->Read(&m_targetType, sizeof(m_targetType));
    s->Read(&m_posX, sizeof(m_posX));
    s->Read(&m_posY, sizeof(m_posY));
    s->Read(&m_submitted, sizeof(m_submitted));
    s->Read(&m_gruntIndex, sizeof(m_gruntIndex));
    s->Read(&m_extraByte, sizeof(m_extraByte));
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
    s->Write(&m_targetIndex, sizeof(m_targetIndex));
    s->Write(&m_commandKind, sizeof(m_commandKind));
    s->Write(&m_targetType, sizeof(m_targetType));
    s->Write(&m_posX, sizeof(m_posX));
    s->Write(&m_posY, sizeof(m_posY));
    s->Write(&m_submitted, sizeof(m_submitted));
    s->Write(&m_gruntMask, sizeof(m_gruntMask));
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
    s->Read(&m_targetIndex, sizeof(m_targetIndex));
    s->Read(&m_commandKind, sizeof(m_commandKind));
    s->Read(&m_targetType, sizeof(m_targetType));
    s->Read(&m_posX, sizeof(m_posX));
    s->Read(&m_posY, sizeof(m_posY));
    s->Read(&m_submitted, sizeof(m_submitted));
    s->Read(&m_gruntMask, sizeof(m_gruntMask));
    return 1;
}

RVA(0x00024890, 0x18d)
i32 CGruntzCmdMgr::Serialize(CFileMemBase* stream, SerialMode mode, LogicTypeId typeId, i32 pObj) {
    if (!stream) {
        return 0;
    }
    u32 cursorOrCount;
    if (mode != SERIAL_SAVE) {
        if (mode != SERIAL_LOAD) {
            return 1;
        }

        if (!IsActive2(stream)) {
            return 0;
        }
        Clear();
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
                if (!cmd->Serialize(stream, SERIAL_LOAD, typeId, pObj)) {
                    return 0;
                }
            } else if (tag == COMMAND_RECORD_MULTI) {
                cmd = CGruntzMultiCommand::Allocate();
                if (!cmd->Serialize(stream, SERIAL_LOAD, typeId, pObj)) {
                    return 0;
                }
            } else {
                return 0;
            }
            m_base.AddTail(cmd);
            cursorOrCount++;
        }
        return 1;
    }

    if (!IsActive(stream)) {
        return 0;
    }
    cursorOrCount = m_base.GetCount();
    stream->Write(&cursorOrCount, sizeof(cursorOrCount));

    POSITION pos = m_base.GetHeadPosition();
    while (pos != NULL) {
        CGruntzCommand* cmd = static_cast<CGruntzCommand*>(m_base.GetNext(pos));
        i32 tagWord = cmd->GetTag() & 0xff;
        stream->Write(&tagWord, sizeof(tagWord));
        if (!cmd->Serialize(stream, SERIAL_SAVE, typeId, pObj)) {
            return 0;
        }
    }
    return 1;
}

RVA(0x00024a90, 0x20)
i32 CGruntzCmdMgr::IsActive(CFileMemBase* enable) {
    if (!enable) {
        return 0;
    }
    return g_gameReg->m_world != NULL;
}

RVA(0x00024ac0, 0x20)
i32 CGruntzCmdMgr::IsActive2(void* enable) {
    if (enable == NULL) {
        return 0;
    }
    return g_gameReg->m_world != NULL;
}

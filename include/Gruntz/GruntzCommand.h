#ifndef SRC_GRUNTZ_GRUNTZCOMMAND_H
#define SRC_GRUNTZ_GRUNTZCOMMAND_H

#include <rva.h>

#include <Mfc.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/PlayerCommandKind.h>
#include <Gruntz/SerialArchive.h>
#include <Ints.h>
#include <Utils/PtrListPool.h>

GZ_ENUM_BEGIN(GruntzCommandRecordKind)
    COMMAND_RECORD_INVALID = 0,
    COMMAND_RECORD_SINGLE = 1,
    COMMAND_RECORD_MULTI = 2
GZ_ENUM_END(GruntzCommandRecordKind)

GZ_ENUM_FLAGS_BEGIN(GruntzCommandSubmitFlags, i32)
    COMMAND_SUBMIT_SCHEDULED = 0x1,
    COMMAND_SUBMIT_IMMEDIATE = 0x2,
    COMMAND_SUBMIT_PENDING_SLOT = 0x4
GZ_ENUM_FLAGS_END(GruntzCommandSubmitFlags, i32)
GZ_ENUM_FLAGS_OPS(GruntzCommandSubmitFlags)

class CPlay;

class CFileMemBase;

class CState;

class CGruntzCommand {
public:
    u8 m_playerIndex;
    GZ_ENUM_STORAGE(PlayerCommandKind, char) m_commandKind;
    u8 m_scheduleSlot;
    char m_pad07;

    i16 m_targetXOrPlayerIndex;
    i16 m_targetYOrUnitIndex;

    GruntzCommandSubmitFlags m_submitFlags;

    union {
        struct {
            char m_unitIndex;
            char m_pickupType;
        };
        u16 m_unitMask;
    };
    i16 m_pad12;

    virtual ~CGruntzCommand() {}

    virtual i32 Serialize(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, i32 payload) = 0;

    virtual i32 Save(CFileMemBase* s) = 0;
    virtual i32 Load(CFileMemBase* s) = 0;

    virtual i32 InitializeCommon(
        char playerIndex,
        char commandKind,
        char scheduleSlot,
        i16 targetXOrPlayerIndex,
        i16 targetYOrUnitIndex
    );
    virtual i32 UnusedCommandQuery();

    virtual char GetRecordKind() = 0;

    virtual i32 DecodePacket(char* data, i32 length) = 0;

    virtual i32 EncodePacket(char* buffer, i32 capacity) = 0;

    virtual i32 Execute(CState* state) = 0;

    virtual void Recycle() = 0;

    i32 InitializeSingle(
        char playerIndex,
        char commandKind,
        char scheduleSlot,
        i16 targetXOrPlayerIndex,
        i16 targetYOrUnitIndex,
        char unitIndex,
        char pickupType
    );
    i32 InitializeMulti(
        char playerIndex,
        char commandKind,
        char scheduleSlot,
        i16 targetXOrPlayerIndex,
        i16 targetYOrUnitIndex,
        u8 unitCount,
        u8* unitIndices
    );
};

extern const u16 g_unitIndexBitTable[16];

class CGruntzSingleCommand : public CGruntzCommand {
public:
    virtual i32 Serialize(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, i32 payload)
        OVERRIDE;
    virtual i32 Save(CFileMemBase* s) OVERRIDE;
    virtual i32 Load(CFileMemBase* s) OVERRIDE;
    virtual i32 UnusedCommandQuery() OVERRIDE;
    virtual char GetRecordKind() OVERRIDE;
    virtual i32 DecodePacket(char*, i32) OVERRIDE;

    virtual i32 EncodePacket(char* buffer, i32 capacity) OVERRIDE;
    virtual i32 Execute(CState* state) OVERRIDE;
    virtual void Recycle() OVERRIDE;
    CGruntzSingleCommand() {}
    static CGruntzSingleCommand* Allocate();
    static void ReleasePool();
};

class CGruntzMultiCommand : public CGruntzCommand {
public:
    virtual i32 Serialize(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, i32 payload)
        OVERRIDE;
    virtual i32 Save(CFileMemBase* s) OVERRIDE;
    virtual i32 Load(CFileMemBase* s) OVERRIDE;
    virtual i32 UnusedCommandQuery() OVERRIDE;
    virtual char GetRecordKind() OVERRIDE;
    virtual i32 DecodePacket(char*, i32) OVERRIDE;

    virtual i32 EncodePacket(char* buffer, i32 capacity) OVERRIDE;
    virtual i32 Execute(CState* state) OVERRIDE;
    virtual void Recycle() OVERRIDE;
    CGruntzMultiCommand() {}
    static CGruntzMultiCommand* Allocate();
    static void ReleasePool();
};

#endif // SRC_GRUNTZ_GRUNTZCOMMAND_H

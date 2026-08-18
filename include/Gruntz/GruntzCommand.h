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

class CPlay;

class CFileMemBase;

class CState;

class CGruntzCommand {
public:
    u8 m_targetIndex;
    GZ_ENUM_STORAGE(PlayerCommandKind, char) m_commandKind;
    u8 m_targetType;
    char m_pad07;
    i16 m_posX;
    i16 m_posY;
    i32 m_submitted;

    union {
        struct {
            char m_gruntIndex;
            char m_extraByte;
        };
        u16 m_gruntMask;
    };
    i16 m_pad12;

    virtual ~CGruntzCommand() {}

    virtual i32 Serialize(CFileMemBase* s, SerialMode mode, LogicTypeId typeId, i32 pObj) = 0;

    virtual i32 Save(CFileMemBase* s) = 0;
    virtual i32 Load(CFileMemBase* s) = 0;

    virtual i32 SetParams(char targetIndex, char cmdKind, char targetType, i16 posX, i16 posY);
    virtual i32 UnusedCommandQuery();

    virtual char GetTag() = 0;

    virtual i32 Parse(char* data, i32 len) = 0;

    virtual i32 Pack(char* buf, i32 unused) = 0;

    virtual i32 Select(CState* state) = 0;

    virtual void Deselect() = 0;

    i32 SetParamsEx(
        char targetIndex,
        char cmdKind,
        char targetType,
        i16 posX,
        i16 posY,
        char gruntIndex,
        char extraByte
    );
    i32 SetMaskFromList(
        char targetIndex,
        char cmdKind,
        char targetType,
        i16 posX,
        i16 posY,
        u8 count,
        u8* gruntList
    );
};

extern const u16 g_cmdBitTable[16];

class CGruntzSingleCommand : public CGruntzCommand {
public:
    virtual i32 Serialize(CFileMemBase* s, SerialMode mode, LogicTypeId typeId, i32 pObj) OVERRIDE;
    virtual i32 Save(CFileMemBase* s) OVERRIDE;
    virtual i32 Load(CFileMemBase* s) OVERRIDE;
    virtual i32 UnusedCommandQuery() OVERRIDE;
    virtual char GetTag() OVERRIDE;
    virtual i32 Parse(char*, i32) OVERRIDE;

    virtual i32 Pack(char* buf, i32 unused) OVERRIDE;
    virtual i32 Select(CState* state) OVERRIDE;
    virtual void Deselect() OVERRIDE;
    CGruntzSingleCommand() {}
    static CGruntzSingleCommand* Allocate();
    static void FreeAll();
};

class CGruntzMultiCommand : public CGruntzCommand {
public:
    virtual i32 Serialize(CFileMemBase* s, SerialMode mode, LogicTypeId typeId, i32 pObj) OVERRIDE;
    virtual i32 Save(CFileMemBase* s) OVERRIDE;
    virtual i32 Load(CFileMemBase* s) OVERRIDE;
    virtual i32 UnusedCommandQuery() OVERRIDE;
    virtual char GetTag() OVERRIDE;
    virtual i32 Parse(char*, i32) OVERRIDE;

    virtual i32 Pack(char* buf, i32 unused) OVERRIDE;
    virtual i32 Select(CState* state) OVERRIDE;
    virtual void Deselect() OVERRIDE;
    CGruntzMultiCommand() {}
    static CGruntzMultiCommand* Allocate();
    static void FreeAll();
};

#endif // SRC_GRUNTZ_GRUNTZCOMMAND_H

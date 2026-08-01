#ifndef SRC_GRUNTZ_GRUNTZCOMMAND_H
#define SRC_GRUNTZ_GRUNTZCOMMAND_H

#include <Mfc.h>
#include <rva.h>
#include <Ints.h>
#include <Utils/PtrListPool.h>

typedef u32 gz_size_t;
void* operator new(gz_size_t);

class CPlay;

class CFileMemBase;

class CState;

class CGruntzCommand {
public:
    u8 m_targetIndex;
    char m_5;
    u8 m_targetType;
    char m_7;
    i16 m_8;
    i16 m_a;
    i32 m_submitted;

    union {
        struct {
            char m_10;
            char m_11;
        };
        u16 m_flagWord;
    };
    i16 m_12;

    virtual ~CGruntzCommand() {}

    virtual i32 Serialize(CFileMemBase* s, i32 mode, i32 typeId, i32 pObj);

    virtual i32 Save(CFileMemBase* s);
    virtual i32 Load(CFileMemBase* s);

    virtual i32 SetParams(char targetIndex, char cmdKind, char targetType, i16 posX, i16 posY);
    virtual i32 UnusedCommandQuery();

    virtual char GetTag();

    virtual i32 Parse(void* data, i32 len);

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
        i32 count,
        u8* gruntList
    );
};
SIZE(0x14);

extern const u16 g_cmdBitTable[16];

class CGruntzSingleCommand : public CGruntzCommand {
public:
    virtual i32 Serialize(CFileMemBase* s, i32 mode, i32 typeId, i32 pObj) OVERRIDE;
    virtual i32 Save(CFileMemBase* s) OVERRIDE;
    virtual i32 Load(CFileMemBase* s) OVERRIDE;
    virtual i32 UnusedCommandQuery() OVERRIDE;
    virtual char GetTag() OVERRIDE;
    virtual i32 Parse(void*, i32) OVERRIDE;

    virtual i32 Pack(char* buf, i32 unused) OVERRIDE;
    virtual i32 Select(CState* state) OVERRIDE;
    virtual void Deselect() OVERRIDE;
    CGruntzSingleCommand() {}
    static CGruntzSingleCommand* Allocate();
    static void FreeAll();
};
SIZE(0x14);

class CGruntzMultiCommand : public CGruntzCommand {
public:
    virtual i32 Serialize(CFileMemBase* s, i32 mode, i32 typeId, i32 pObj) OVERRIDE;
    virtual i32 Save(CFileMemBase* s) OVERRIDE;
    virtual i32 Load(CFileMemBase* s) OVERRIDE;
    virtual i32 UnusedCommandQuery() OVERRIDE;
    virtual char GetTag() OVERRIDE;
    virtual i32 Parse(void*, i32) OVERRIDE;

    virtual i32 Pack(char* buf, i32 unused) OVERRIDE;
    virtual i32 Select(CState* state) OVERRIDE;
    virtual void Deselect() OVERRIDE;
    CGruntzMultiCommand() {}
    static CGruntzMultiCommand* Allocate();
    static void FreeAll();
};
SIZE(0x14);

#endif // SRC_GRUNTZ_GRUNTZCOMMAND_H

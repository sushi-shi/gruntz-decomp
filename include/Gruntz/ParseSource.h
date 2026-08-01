#ifndef GRUNTZ_CPARSESOURCE_H
#define GRUNTZ_CPARSESOURCE_H

#include <Ints.h>
#include <rva.h>
#include <Bute/Hash.h>

typedef enum ParseEntryTag {
    PARSETAG_VAW = 0x574156,
    PARSETAG_INA = 0x414e49,
} ParseEntryTag;

class CSymTab;
class CSymRec;

class CRezItmBase;

struct CParseSlotHashNode : public CHashElement {

    CParseSlotHashNode() {
        m_parseSource = 0;
    }

    virtual u32 Hash() OVERRIDE;
};
SIZE(0x18);

struct CParseSource {

    i32 GetEntryTag();
    char* BeginParse();
    i32 EndParse();

    char* CurrentScopeName();

    char* CurrentScopePath(char* dst, i32 size);

    CParseSource();

    void Build(
        CSymTab* owner,
        const char* name,
        void* f4,
        CSymRec* rec,
        void* str2,
        i32 f3,
        i32 f1,
        i32 f2,
        i32 f6,
        void* arr,
        CRezItmBase* stream
    );
    void Teardown();
    i32 SetPos(i32 pos);
    i32 ReadAt(void* dst, i32 pos, u32 len);
    i32 Read(void* dst, u32 len, i32 seekPos);

    char* m_name;
    CSymRec* m_entry;
    i32 m_typeTag;

    union {
        u32 m_length;
        const char* m_keyHandle;
    };
    CSymTab* m_owner;

    i32 m_base;
    i32 m_cursor;

    CParseSlotHashNode m_node1c;
    CRezItmBase* m_reader;
    char* m_buffer;
};
SIZE_UNKNOWN();

#endif // GRUNTZ_CPARSESOURCE_H

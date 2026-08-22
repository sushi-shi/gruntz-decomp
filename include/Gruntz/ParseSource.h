#ifndef GRUNTZ_CPARSESOURCE_H
#define GRUNTZ_CPARSESOURCE_H

#include <rva.h>

#include <Bute/Hash.h>
#include <Enums.h>
#include <Ints.h>
#include <Rez/RezTypeTag.h>

#include <stddef.h>

class CSymTab;
class CSymRec;

class CRezItmBase;

struct CParseSlotHashNode : public CHashElement {

    CParseSlotHashNode() {
        m_parseSource = NULL;
    }

    virtual u32 Hash() OVERRIDE;
};

struct CParseSource {

    GZ_ENUM_RETURN(RezTypeTag, u32) GetEntryTag();
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
    i32 ReadAll(void* dst);
    i32 Read(void* dst, u32 len, i32 seekPos);
    char ReadChar();
    i32 IsResident();
    i32 AtEnd();

    char* m_name;
    CSymRec* m_entry;
    i32 m_typeTag;

    u32 m_length;
    CSymTab* m_owner;

    i32 m_base;
    i32 m_cursor;

    CParseSlotHashNode m_node1c;
    CRezItmBase* m_reader;
    char* m_buffer;
};

#define BEGIN_FILE_IMAGE_PARSE(source, format, bytes)                                              \
    FileImageFormat format;                                                                        \
    switch (static_cast<u32>(source->GetEntryTag())) {                                             \
        case IMGTAG_PMB:                                                                           \
            format = FMT_BMP;                                                                      \
            break;                                                                                 \
        case IMGTAG_XCP:                                                                           \
            format = FMT_PCX;                                                                      \
            break;                                                                                 \
        case IMGTAG_DIR:                                                                           \
            format = FMT_RID;                                                                      \
            break;                                                                                 \
        case IMGTAG_DIP:                                                                           \
            format = FMT_PID;                                                                      \
            break;                                                                                 \
        default:                                                                                   \
            return 0;                                                                              \
    }                                                                                              \
    char* bytes = source->BeginParse();                                                            \
    if (bytes == NULL) {                                                                           \
        return 0;                                                                                  \
    }

#endif // GRUNTZ_CPARSESOURCE_H

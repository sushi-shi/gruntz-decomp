#ifndef REZ_REZARCHIVEENTRY_H
#define REZ_REZARCHIVEENTRY_H

#include <rva.h>

#include <Bute/Hash.h>
#include <Enums.h>
#include <Ints.h>
#include <Rez/RezTypeTag.h>

#include <stddef.h>

class CRezArchiveDir;
class CRezArchiveType;

class CRezItmBase;

struct CRezArchiveEntryHashNode : public CHashElement {

    CRezArchiveEntryHashNode() {
        m_archiveEntry = NULL;
    }

    void SetArchiveEntry(CRezArchiveEntry* entry) {
        m_archiveEntry = entry;
    }

    CRezArchiveEntry* GetArchiveEntry() {
        return m_archiveEntry;
    }

    CRezArchiveEntryHashNode* Next() {
        return static_cast<CRezArchiveEntryHashNode*>(CHashElement::Next());
    }

    CRezArchiveEntryHashNode* Prev() {
        return static_cast<CRezArchiveEntryHashNode*>(CHashElement::Prev());
    }

    CRezArchiveEntryHashNode* NextInBucket() {
        return static_cast<CRezArchiveEntryHashNode*>(CHashElement::NextInBucket());
    }

    CRezArchiveEntryHashNode* PrevInBucket() {
        return static_cast<CRezArchiveEntryHashNode*>(CHashElement::PrevInBucket());
    }

    virtual u32 Hash() OVERRIDE;

    CRezArchiveEntry* m_archiveEntry;
};

inline void CRezEntryNameHash::Insert(CRezArchiveEntryHashNode* entry) {
    CHashBase::Insert(entry);
}

inline void CRezEntryNameHash::Delete(CRezArchiveEntryHashNode* entry) {
    CHashBase::Remove(entry);
}

inline CRezArchiveEntryHashNode* CRezEntryNameHash::First() {
    return static_cast<CRezArchiveEntryHashNode*>(CHashBase::First());
}

inline CRezArchiveEntryHashNode* CRezEntryNameHash::Last() {
    return static_cast<CRezArchiveEntryHashNode*>(CHashBase::Last());
}

inline CRezArchiveEntryHashNode* CRezEntryNameHash::Lookup(u32 bucketIndex) {
    return static_cast<CRezArchiveEntryHashNode*>(CHashBase::Lookup(bucketIndex));
}

struct CRezArchiveEntry {

    GZ_ENUM_RETURN(RezTypeTag, u32) GetTypeTag();
    u32 GetSize() {
        return m_size;
    }
    char* LoadData();
    i32 ReleaseData();

    char* GetDirectoryName();

    char* GetDirectoryPath(char* destination, i32 size);

    CRezArchiveEntry();

    void Initialize(
        CRezArchiveDir* directory,
        const char* name,
        void* resourceId,
        CRezArchiveType* type,
        void* comment,
        i32 size,
        i32 dataOffset,
        i32 time,
        i32 keyCount,
        void* keys,
        CRezItmBase* storage
    );
    void Reset();
    i32 SetPos(i32 position);
    i32 ReadAt(void* destination, i32 position, u32 byteCount);
    i32 ReadAll(void* destination);
    i32 Read(void* destination, u32 byteCount, i32 seekPosition);
    char ReadChar();
    i32 IsDataLoaded();
    i32 AtEnd();

    char* m_name;
    CRezArchiveType* m_type;
    i32 m_time;

    u32 m_size;
    CRezArchiveDir* m_directory;

    i32 m_dataOffset;
    i32 m_cursor;

    CRezArchiveEntryHashNode m_nameNode;
    CRezItmBase* m_storage;
    char* m_loadedData;
};

#define BEGIN_FILE_IMAGE_PARSE(source, format, bytes)                                              \
    FileImageFormat format;                                                                        \
    switch (static_cast<u32>(source->GetTypeTag())) {                                              \
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
    char* bytes = source->LoadData();                                                              \
    if (bytes == NULL) {                                                                           \
        return 0;                                                                                  \
    }

#endif // REZ_REZARCHIVEENTRY_H

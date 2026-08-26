#include <rva.h>

#include <Rez/RezArchive.h>

#include <Mfc.h>

#include <AddrWord.h>
#include <Dsndmgr/SoundBankLoad.h>
#include <Enums.h>
#include <Gruntz/CustomWorldInfoDlg.h>
#include <Pix16.h>
#include <Rez/DebugPrintf.h>
#include <Rez/RezArchiveDir.h>
#include <Rez/RezArchiveEntry.h>
#include <Rez/RezFile.h>
#include <Rez/RezMgr.h>
#include <Rez/RezTypeTag.h>

#include <io.h>
#include <new>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

static __inline i32 IsPathComponentCharacter(const char* delimiters, char character) {
    if (delimiters) {
        return strchr(delimiters, character) == NULL;
    }
    if (character >= ' ' && character <= '.') {
        return 1;
    }
    if (character >= '0' && character <= '9') {
        return 1;
    }
    if (character >= 'A' && character <= 'Z') {
        return 1;
    }
    if (character >= 'a' && character <= 'z') {
        return 1;
    }
    return 0;
}

static inline CRezEntryPoolBlock* FirstEntryPoolBlock(IntrusiveList& list) {
    return static_cast<CRezEntryPoolBlock*>(list.m_head);
}

static inline CRezItmBase* FirstStorage(CObjList& list) {
    return list.m_head;
}

static inline void SetArchiveType(CRezArchiveTypeHashNode& node, CRezArchiveType* type) {
    node.m_archiveType = type;
}

static inline void SetArchiveDirectory(CRezArchiveDirHashNode& node, CRezArchiveDir* directory) {
    node.m_archiveDirectory = directory;
}

static const i32 REZ_SCAN_PATH_MAX = 0x308;

// Byte-forced view of packed serialized storage.
static inline i32 ReadPackedI32(const char* bytes) {
    i32 value;
    memcpy(&value, bytes, sizeof(value));
    return value;
}

RVA(0x00139900, 0x1a)
CRezArchiveEntry::CRezArchiveEntry() {

    m_storage = NULL;
    m_directory = NULL;
    m_name = NULL;
    m_nameNode.m_archiveEntry = this;
}

// @early-stop
RVA(0x00139920, 0x8d)
void CRezArchiveEntry::Initialize(
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
) {
    static_cast<void>(resourceId);
    static_cast<void>(comment);
    static_cast<void>(keyCount);
    static_cast<void>(keys);
    m_storage = storage;
    m_directory = directory;
    if (name == NULL) {
        m_name = const_cast<char*>(name);
    } else {
        m_name = new char[strlen(name) + 1];
        if (m_name) {
            strcpy(m_name, name);
        }
    }
    m_type = type;
    m_size = size;
    m_dataOffset = dataOffset;
    m_time = time;
    m_loadedData = NULL;
    m_cursor = 0;
    m_nameNode.m_archiveEntry = this;
}

RVA(0x001399b0, 0x57)
void CRezArchiveEntry::Reset() {
    if (m_name) {
        delete[] m_name;
    }
    if (m_directory != NULL) {
        if (m_directory->m_preloadedData == NULL) {
            if (m_loadedData) {
                delete[] m_loadedData;
            }
        }
    } else {
        if (m_loadedData) {
            delete[] m_loadedData;
        }
    }
    m_name = NULL;
    m_type = NULL;
    m_time = 0;
    m_size = 0;
    m_loadedData = NULL;
    m_directory = NULL;
    m_dataOffset = 0;
    m_cursor = 0;
    m_nameNode.m_archiveEntry = NULL;
}

RVA(0x00139a10, 0x6)
GZ_ENUM_RETURN(RezTypeTag, u32) CRezArchiveEntry::GetTypeTag() {
    return static_cast<RezTypeTag>(m_type->m_typeTag);
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00139a20, 0x140)
char* CRezArchiveEntry::GetDirectoryPath(char* destination, i32 size) {
    if (m_directory->m_parent == NULL) {
        strcpy(destination, DATA_COMPGEN(0x0020df98, "\\"));
    } else {
        char* scratch = new char[size];
        strcpy(destination, "");
        CRezArchiveDir* directory = m_directory;
        while (directory != NULL) {
            strcpy(scratch, destination);
            if (directory->m_parent != NULL) {
                strcpy(destination, "\\");
            } else {
                destination[0] = 0;
            }
            strcat(destination, directory->m_name);
            strcat(destination, scratch);
            directory = directory->m_parent;
        }
        delete[] scratch;
    }
    return destination;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00139b60, 0x6)
char* CRezArchiveEntry::GetDirectoryName() {
    return m_directory->m_name;
}

RVA(0x00139b70, 0x6b)
char* CRezArchiveEntry::LoadData() {
    if (m_directory->m_preloadedData != NULL) {
        return m_directory->m_preloadedData + (m_dataOffset - m_directory->m_minDataOffset);
    }
    if (m_loadedData != NULL) {
        return m_loadedData;
    }
    if (m_size == 0) {
        return NULL;
    }
    m_loadedData = new char[m_size];
    if (m_loadedData == NULL) {
        return NULL;
    }
    if (m_storage->Read(m_dataOffset, 0, m_size, m_loadedData) != static_cast<i32>(m_size)) {
        delete[] m_loadedData;
        m_loadedData = NULL;
    }
    return m_loadedData;
}

RVA(0x00139be0, 0x21)
i32 CRezArchiveEntry::ReleaseData() {
    if (m_loadedData != NULL) {
        delete[] m_loadedData;
        m_loadedData = NULL;
    }
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00139c10, 0x1b)
i32 CRezArchiveEntry::IsDataLoaded() {
    if (m_directory->m_preloadedData != NULL) {
        return 1;
    }
    return m_loadedData != NULL;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00139c30, 0x13)
i32 CRezArchiveEntry::ReadAll(void* destination) {
    return ReadAt(destination, 0, m_size);
}

RVA(0x00139c50, 0x95)
i32 CRezArchiveEntry::ReadAt(void* destination, i32 position, u32 byteCount) {
    CRezArchiveDir* directory = m_directory;
    if (directory->m_preloadedData != NULL) {
        memcpy(
            destination,
            directory->m_preloadedData + (m_dataOffset - directory->m_minDataOffset + position),
            byteCount
        );
        return 1;
    }
    if (m_loadedData != NULL) {
        memcpy(destination, (m_loadedData + position), byteCount);
        return 1;
    }
    return m_storage->Read(m_dataOffset, position, byteCount, destination)
           == static_cast<i32>(byteCount);
}

RVA(0x00139cf0, 0xf)
i32 CRezArchiveEntry::SetPos(i32 position) {
    m_cursor = position;
    return 1;
}

// @early-stop
RVA(0x00139d00, 0xcc)
i32 CRezArchiveEntry::Read(void* destination, u32 byteCount, i32 seekPosition) {
    if (seekPosition != -1) {
        SetPos(seekPosition);
    }

    u32 position = static_cast<u32>(m_cursor);
    u32 bytesToRead = byteCount;
    if (bytesToRead + position > m_size) {
        bytesToRead = m_size - position;
    }

    if (bytesToRead > 0) {
        CRezArchiveDir* directory = m_directory;
        if (directory->m_preloadedData) {

            const char* source =
                m_dataOffset - directory->m_minDataOffset + directory->m_preloadedData;
            source += position;
            memcpy(destination, source, bytesToRead);
            m_cursor += bytesToRead;
            return bytesToRead;
        }
        if (m_loadedData) {
            const char* source = (m_loadedData + position);
            memcpy(destination, source, bytesToRead);
            m_cursor += bytesToRead;
            return bytesToRead;
        }
        if (m_storage->Read(m_dataOffset, position, bytesToRead, destination)
            == static_cast<i32>(bytesToRead)) {
            m_cursor += bytesToRead;
            return bytesToRead;
        }
    }
    return 0;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00139dd0, 0xc)
i32 CRezArchiveEntry::AtEnd() {
    return static_cast<u32>(m_cursor) >= m_size;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00139de0, 0x15)
char CRezArchiveEntry::ReadChar() {
    char value;
    Read(&value, 1, -1);
    return value;
}

// @early-stop
RVA(0x00139e00, 0x71)
CRezArchiveType::CRezArchiveType(
    i32 typeTag,
    CRezArchiveDir* directory,
    i32 resourceIdBucketCount,
    i32 resourceNameBucketCount
)
    : m_idIndex(resourceIdBucketCount), m_nameIndex(resourceNameBucketCount) {
    m_typeNode.m_archiveType = this;
    m_directory = directory;
    m_typeTag = typeTag;
}

RVA(0x00139e90, 0x6c)
CRezArchiveType::CRezArchiveType(
    i32 typeTag,
    CRezArchiveDir* directory,
    i32 resourceNameBucketCount
)
    : m_idIndex(), m_nameIndex(resourceNameBucketCount) {
    m_typeTag = typeTag;
    m_typeNode.m_archiveType = this;
    m_directory = directory;
}

RVA(0x00139f00, 0xd7)
CRezArchiveType::~CRezArchiveType() {
    if (m_directory->m_archive->m_useIdIndex != false) {
        CHashElement* node = m_idIndex.First();
        while (node) {
            CHashElement* current = node;
            node = current->Next();
            m_idIndex.Remove(current);
        }
    }
    CHashElement* node = m_nameIndex.First();
    while (node) {
        CHashElement* current = node;
        node = current->Next();
        m_nameIndex.Remove(current);
        current->m_archiveEntry->Reset();
        m_directory->m_archive->ReleaseEntry(current->m_archiveEntry);
    }
    m_typeTag = 0;
    SetArchiveType(m_typeNode, NULL);
}

RVA(0x00139ff0, 0xd4)
CRezArchiveDir::CRezArchiveDir(
    CRezArchive* archive,
    CRezArchiveDir* parent,
    const char* name,
    i32 bodyOffset,
    i32 bodySize,
    i32 time,
    i32 subdirectoryBucketCount,
    i32 typeBucketCount
)
    : m_subdirectories(subdirectoryBucketCount), m_types(typeBucketCount) {
    m_name = new char[strlen(name) + 1];
    if (m_name) {
        strcpy(m_name, name);
    }
    m_time = time;
    m_bodySize = bodySize;
    m_bodyOffset = bodyOffset;
    m_archive = archive;
    m_totalDataSize = 0;
    m_minDataOffset = 0;
    m_preloadedData = NULL;
    m_parent = parent;
    SetArchiveDirectory(m_nameNode, this);
}

RVA(0x0013a0f0, 0x11e)
CRezArchiveDir::~CRezArchiveDir() {

    CHashElement* node;
    for (node = m_types.First(); node != NULL;) {
        CHashElement* current = node;
        node = current->Next();
        m_types.Remove(current);
        CRezArchiveType* type = current->m_archiveType;
        delete type;
    }
    for (node = m_subdirectories.First(); node != NULL;) {
        CHashElement* current = node;
        node = current->Next();
        m_subdirectories.Remove(current);
        CRezArchiveDir* subdirectory = current->m_archiveDirectory;
        delete subdirectory;
    }
    if (m_name) {
        delete[] m_name;
    }
    if (m_preloadedData) {
        delete[] m_preloadedData;
    }
    m_name = NULL;
    m_time = 0;
    m_bodySize = 0;
    m_bodyOffset = 0;
    m_totalDataSize = 0;
    m_minDataOffset = 0;
    m_preloadedData = NULL;
    m_archive = NULL;
    m_parent = NULL;
    m_nameNode.m_archiveDirectory = NULL;
}

RVA(0x0013a210, 0x37)
CRezArchiveEntry* CRezArchiveDir::FindEntry(const char* name, RezTypeTag typeTag) {
    CRezArchiveType* type = m_types.FindTypeByTag(IDX(typeTag));
    if (!type) {
        return NULL;
    }
    return type->m_nameIndex.FindByName(name, m_archive->m_caseSensitive == false);
}

RVA(0x0013a250, 0xa2)
CRezArchiveEntry* CRezArchiveDir::FindEntryByFilename(const char* filename) {
    char directoryPath[260];
    char resourceName[260];
    char extension[260];
    char drive[4];
    char typeName[8];
    _splitpath(filename, drive, directoryPath, resourceName, extension);
    RezTypeTag typeTag;
    if (strlen(extension) != 0) {
        strcpy(typeName, extension + 1);
        _strupr(typeName);
        typeTag = m_archive->PackTag(typeName);
    } else {
        typeTag = REZ_TAG_NONE;
    }
    return FindEntry(resourceName, typeTag);
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0013a300, 0x99)
i32 CRezArchiveDir::PreloadData(b32 recursive) {
    if (m_preloadedData != NULL) {
        return 1;
    }

    CRezArchive* archive = m_archive;
    if (archive->m_isDataContiguous == false || archive->m_storages.m_storageCount > 1) {
        RezAssertFail("CRezDir::Load Failed! (File is not sorted!)\n");
        return 0;
    }

    if (m_totalDataSize > 0) {
        m_preloadedData = new char[m_totalDataSize];
        if (m_preloadedData != NULL) {
            m_archive->m_primaryStorage->Read(m_minDataOffset, 0, m_totalDataSize, m_preloadedData);
        }
    }

    if (recursive != false) {
        for (CHashElement* node = m_subdirectories.First(); node != NULL; node = node->Next()) {

            node->m_archiveDirectory->PreloadData(true);
        }
    }
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0013a3a0, 0x94)
i32 CRezArchiveDir::ReleaseEntryData(b32 recursive) {
    if (m_preloadedData != NULL) {
        delete[] m_preloadedData;
        m_preloadedData = NULL;
    } else {
        CRezArchiveType* type = FirstType();
        while (type) {
            CRezArchiveEntry* entry = FirstEntry(type);
            while (entry) {
                entry->ReleaseData();
                entry = NextEntry(entry);
            }
            type = NextType(type);
        }
    }
    if (recursive) {
        CHashElement* node = m_subdirectories.First();
        while (node) {
            node->m_archiveDirectory->ReleaseEntryData(true);
            node = node->Next();
        }
    }
    return 1;
}

RVA(0x0013a440, 0x29)
CRezArchiveDir* CRezArchiveDir::FindSubdirectory(const char* name) {
    if (!name) {
        return NULL;
    }
    return m_subdirectories.FindByName(name, m_archive->m_caseSensitive == false);
}

RVA(0x0013a470, 0x11)
CRezArchiveDir* CRezArchiveDir::FirstSubdirectory() {
    CHashElement* node = m_subdirectories.First();
    if (!node) {
        return NULL;
    }
    return node->m_archiveDirectory;
}

RVA(0x0013a490, 0x19)
CRezArchiveDir* CRezArchiveDir::NextSubdirectory(CRezArchiveDir* directory) {
    CHashElement* node = directory->m_nameNode.Next();
    if (!node) {
        return NULL;
    }
    return node->m_archiveDirectory;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0013a4b0, 0x10)
CRezArchiveType* CRezArchiveDir::FindType(u32 typeTag) {
    return m_types.FindTypeByTag(typeTag);
}

RVA(0x0013a4c0, 0x11)
CRezArchiveType* CRezArchiveDir::FirstType() {
    CHashElement* node = m_types.First();
    if (!node) {
        return NULL;
    }
    return node->m_archiveType;
}

RVA(0x0013a4e0, 0x19)
CRezArchiveType* CRezArchiveDir::NextType(CRezArchiveType* type) {
    CHashElement* node = type->m_typeNode.Next();
    if (!node) {
        return NULL;
    }
    return node->m_archiveType;
}

RVA(0x0013a500, 0x19)
CRezArchiveEntry* CRezArchiveDir::FirstEntry(CRezArchiveType* type) {
    CHashElement* node = type->m_nameIndex.First();
    if (!node) {
        return NULL;
    }
    return node->m_archiveEntry;
}

RVA(0x0013a520, 0x19)
CRezArchiveEntry* CRezArchiveDir::NextEntry(CRezArchiveEntry* entry) {
    CHashElement* node = entry->m_nameNode.Next();
    if (!node) {
        return NULL;
    }
    return node->m_archiveEntry;
}

RVA(0x0013a540, 0xce)
CRezArchiveDir* CRezArchiveDir::CreateSubdirectory(const char* name) {

    if (m_subdirectories.FindByName(name, m_archive->m_caseSensitive == false) != NULL) {
        return NULL;
    }
    CRezArchiveDir* child = new CRezArchiveDir(
        m_archive,
        this,
        name,
        0,
        0,
        m_archive->MakeTimestamp(),
        m_archive->m_subdirectoryBucketCount,
        m_archive->m_typeBucketCount
    );
    if (!child) {
        return NULL;
    }
    m_subdirectories.Insert(&child->m_nameNode);

    u32 nameLength = strlen(name);
    if (m_archive->m_largestDirectoryNameSize <= nameLength) {
        m_archive->m_largestDirectoryNameSize = nameLength + 1;
    }
    return child;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0013a610, 0xa9)
CRezArchiveEntry*
CRezArchiveDir::CreateNamedEntry(void* resourceId, const char* name, i32 typeTag) {
    CRezArchiveType* type = FindOrCreateType(typeTag);
    if (type->m_nameIndex.FindByName(name, m_archive->m_caseSensitive == false) != NULL) {
        return NULL;
    }
    CRezArchiveEntry* entry = m_archive->AcquireEntry();
    entry->Initialize(
        this,
        name,
        resourceId,
        type,
        NULL,
        0,
        0,
        m_archive->MakeTimestamp(),
        0,
        NULL,
        m_archive->m_primaryStorage
    );
    if (entry == NULL) {
        return NULL;
    }
    type->m_nameIndex.Insert(&entry->m_nameNode);
    u32 nameLength = strlen(name);
    if (m_archive->m_largestResourceNameSize <= nameLength) {
        m_archive->m_largestResourceNameSize = nameLength + 1;
    }
    return entry;
}

RVA(0x0013a6c0, 0x75)
CRezArchiveEntry* CRezArchiveDir::CreateEntry(
    u32 resourceId,
    const char* name,
    CRezArchiveType* type,
    CRezItmBase* storage
) {
    CRezArchiveEntry* entry = m_archive->AcquireEntry();
    if (entry == NULL) {
        return entry;
    }
    entry->Initialize(
        this,
        name,

        // API-forced pointer-key boundary.
        reinterpret_cast<void*>(resourceId),
        type,
        NULL,
        0,
        0,
        m_archive->MakeTimestamp(),
        0,
        NULL,
        storage
    );
    type->m_nameIndex.Insert(&entry->m_nameNode);
    u32 nameLength = strlen(name);
    if (m_archive->m_largestResourceNameSize <= nameLength) {
        m_archive->m_largestResourceNameSize = nameLength + 1;
    }
    return entry;
}

RVA(0x0013a740, 0x47)
i32 CRezArchiveDir::RemoveEntry(CRezArchiveType* type, CRezArchiveEntry* entry) {
    m_totalDataSize -= entry->m_size;
    type->m_nameIndex.Remove(&entry->m_nameNode);
    entry->Reset();
    m_archive->ReleaseEntry(entry);
    m_archive->m_isDataContiguous = false;
    return 1;
}

RVA(0x0013a790, 0xb2)
i32 CRezArchiveDir::ReadDirectoryTree(
    CRezItmBase* storage,
    i32 bodyOffset,
    i32 bodySize,
    b32 replaceExisting
) {
    b32 success = true;
    if (static_cast<u32>(bodySize) <= 0) {
        return success;
    }
    CHashElement* node = m_subdirectories.First();
    while (node) {
        node->m_archiveDirectory->m_bodyOffset = 0;
        node = node->Next();
    }
    if (ReadDirectoryBody(storage, bodyOffset, bodySize, replaceExisting) != 0) {
        node = m_subdirectories.First();
        while (node) {
            CRezArchiveDir* subdirectory = node->m_archiveDirectory;
            if (subdirectory->m_bodyOffset != 0) {
                if (subdirectory->ReadDirectoryTree(
                        storage,
                        subdirectory->m_bodyOffset,
                        subdirectory->m_bodySize,
                        replaceExisting
                    )
                    == 0) {
                    success = false;
                }
            }
            node = node->Next();
        }
    } else {
        success = false;
    }
    return success;
}

GZ_ENUM_BEGIN(RezDirectoryRecordKind)
    REZ_DIRECTORY_RECORD_RESOURCE = 0,
    REZ_DIRECTORY_RECORD_SUBDIRECTORY = 1
GZ_ENUM_END(RezDirectoryRecordKind)

RVA(0x0013a850, 0x2f7)
i32 CRezArchiveDir::ReadDirectoryBody(
    CRezItmBase* storage,
    i32 bodyOffset,
    i32 bodySize,
    b32 replaceExisting
) {
    m_totalDataSize = 0;
    m_minDataOffset = -1;
    i32 maximumDataOffset = 0;
    char* body = new char[static_cast<u32>(bodySize)];
    if (!body) {
        return 0;
    }
    if (storage->Read(bodyOffset, 0, bodySize, body) != bodySize) {
        delete[] body;
        return 0;
    }
    char* cursor = body;
    char* end = body + bodySize;
    while (cursor < end) {
        RezDirectoryRecordKind recordKind =
            static_cast<RezDirectoryRecordKind>(ReadPackedI32(cursor));
        if (recordKind == REZ_DIRECTORY_RECORD_SUBDIRECTORY) {
            cursor += sizeof(i32);
            i32 childBodyOffset = ReadPackedI32(cursor);
            cursor += sizeof(i32);
            i32 childBodySize = ReadPackedI32(cursor);
            cursor += sizeof(i32);
            i32 childTime = ReadPackedI32(cursor);
            cursor += sizeof(i32);
            char* name = cursor;
            cursor += strlen(name) + 1;
            CRezDirectoryNameHash* subdirectories = &m_subdirectories;
            CRezArchiveDir* existing =
                subdirectories->FindByName(name, m_archive->m_caseSensitive == false);
            if (existing == NULL) {
                CRezArchiveDir* directory = new CRezArchiveDir(
                    m_archive,
                    this,
                    name,
                    childBodyOffset,
                    childBodySize,
                    childTime,
                    m_archive->m_subdirectoryBucketCount,
                    m_archive->m_typeBucketCount
                );
                subdirectories->Insert(&directory->m_nameNode);
            } else {
                existing->m_bodyOffset = childBodyOffset;
                existing->m_bodySize = childBodySize;
                existing->m_time = childTime;
            }
        } else {

            cursor += sizeof(i32);
            i32 dataOffset = ReadPackedI32(cursor);
            cursor += sizeof(i32);
            i32 size = ReadPackedI32(cursor);
            cursor += sizeof(i32);
            i32 time = ReadPackedI32(cursor);
            cursor += sizeof(i32);
            i32 resourceId = ReadPackedI32(cursor);
            cursor += sizeof(i32);
            i32 typeTag = ReadPackedI32(cursor);
            cursor += sizeof(i32);
            i32 keyCount = ReadPackedI32(cursor);
            cursor += sizeof(i32);
            char* name = cursor;
            cursor += strlen(name) + 1;
            CRezArchiveType* type = FindOrCreateType(typeTag);
            i32 skipEntry = 0;
            CRezArchiveEntry* found = type->m_nameIndex.FindByName(name, 1);
            if (found) {
                if (replaceExisting != false) {
                    RemoveEntry(type, found);
                } else {
                    skipEntry = 1;
                }
            }
            char* comment = cursor;
            cursor += strlen(cursor) + 1;
            if (*comment == 0) {
                comment = NULL;
            }
            i32* keys;
            if (static_cast<u32>(keyCount) > 0) {
                keys = new i32[keyCount];
                for (u32 keyIndex = 0; keyIndex < static_cast<u32>(keyCount); keyIndex++) {
                    keys[keyIndex] = ReadPackedI32(cursor);
                    cursor += sizeof(i32);
                }
            } else {
                keys = NULL;
            }
            if (!skipEntry) {
                CRezArchiveEntry* entry = m_archive->AcquireEntry();
                AddrWord<char> resourceIdValue;
                resourceIdValue.m_word = resourceId;
                entry->Initialize(
                    this,
                    name,
                    resourceIdValue.m_addr,
                    type,
                    comment,
                    size,
                    dataOffset,
                    time,
                    keyCount,
                    keys,
                    storage
                );
                type->m_nameIndex.Insert(&entry->m_nameNode);
                m_totalDataSize = m_totalDataSize + entry->m_size;
                if (static_cast<u32>(entry->m_dataOffset) < static_cast<u32>(m_minDataOffset)) {
                    m_minDataOffset = entry->m_dataOffset;
                }
                if (static_cast<u32>(entry->m_dataOffset) > static_cast<u32>(maximumDataOffset)) {
                    maximumDataOffset = entry->m_dataOffset;
                }
            }
            if (keys) {
                delete[] keys;
            }
        }
    }
    delete[] body;
    return 1;
}

RVA(0x0013ab50, 0xc2)
CRezArchiveType* CRezArchiveDir::FindOrCreateType(i32 typeTag) {

    CRezArchiveType* type = m_types.FindTypeByTag(static_cast<u32>(typeTag));
    if (!type) {
        if (m_archive->m_useIdIndex != false) {
            type = new CRezArchiveType(
                typeTag,
                this,
                m_archive->m_resourceIdBucketCount,
                m_archive->m_resourceNameBucketCount
            );
        } else {
            type = new CRezArchiveType(typeTag, this, m_archive->m_resourceNameBucketCount);
        }
        if (type == NULL) {
            return NULL;
        }
        m_types.Insert(&type->m_typeNode);
    }
    return type;
}

GZ_ENUM_BEGIN(RezArchiveVersion)
    REZ_ARCHIVE_VERSION_NONE = 0,
    REZ_ARCHIVE_VERSION_1 = 1
GZ_ENUM_END(RezArchiveVersion)

GZ_ENUM_CONST_BEGIN(RezArchiveDefaults)
    REZ_ARCHIVE_FIRST_GENERATED_RESOURCE_ID = 2000000000,
    REZ_ARCHIVE_DEFAULT_MAX_OPEN_FILES = 3,
    REZ_ARCHIVE_DEFAULT_RESOURCE_NAME_BUCKET_COUNT = 19,
    REZ_ARCHIVE_DEFAULT_RESOURCE_ID_BUCKET_COUNT = 19,
    REZ_ARCHIVE_DEFAULT_SUBDIRECTORY_BUCKET_COUNT = 5,
    REZ_ARCHIVE_DEFAULT_TYPE_BUCKET_COUNT = 9,
    REZ_ARCHIVE_DEFAULT_ENTRIES_PER_POOL_BLOCK = 100
GZ_ENUM_CONST_END(RezArchiveDefaults)

RVA(0x0013ac20, 0xe2)
CRezArchive::CRezArchive() : m_freeEntries(1) {
    m_isOpen = false;
    m_primaryStorage = NULL;
    m_storages.m_storageCount = 0;
    m_rootDirectoryOffset = 0;
    m_nextWritePos = 0;
    m_rootDirectory = NULL;
    m_archiveTime = 0;
    m_isNewArchive = false;
    m_version = REZ_ARCHIVE_VERSION_NONE;
    m_largestKeyArrayLength = 0;
    m_largestDirectoryNameSize = 0;
    m_largestResourceNameSize = 0;
    m_largestCommentSize = 0;
    m_archivePath = NULL;
    m_pathDelimiters = NULL;
    m_caseSensitive = false;
    m_useIdIndex = false;
    m_resourceNameBucketCount = REZ_ARCHIVE_DEFAULT_RESOURCE_NAME_BUCKET_COUNT;
    m_resourceIdBucketCount = REZ_ARCHIVE_DEFAULT_RESOURCE_ID_BUCKET_COUNT;
    m_reserved24 = 1;
    m_nextGeneratedResourceId = REZ_ARCHIVE_FIRST_GENERATED_RESOURCE_ID;
    m_readOnly = true;
    m_isDataContiguous = true;
    m_maxOpenFiles = REZ_ARCHIVE_DEFAULT_MAX_OPEN_FILES;
    m_subdirectoryBucketCount = REZ_ARCHIVE_DEFAULT_SUBDIRECTORY_BUCKET_COUNT;
    m_typeBucketCount = REZ_ARCHIVE_DEFAULT_TYPE_BUCKET_COUNT;
    m_entriesPerPoolBlock = REZ_ARCHIVE_DEFAULT_ENTRIES_PER_POOL_BLOCK;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0013ad20, 0xac)
CRezArchive::CRezArchive(char* path, b32 readOnly, b32 createNew) : m_freeEntries(1) {
    {
        CRezArchive defaults;
    }
    Open(path, readOnly, createNew);
}

RVA(0x0013ade0, 0x13f)
CRezArchive::~CRezArchive() {

    if (m_isOpen) {
        Close(0);
    }
    CRezItmBase* storage;
    for (storage = m_storages.m_head; storage != NULL; storage = m_storages.m_head) {
        m_storages.Remove(storage);
        m_storages.m_storageCount--;
        delete storage;
    }
    CRezArchiveDir* rootDirectory = m_rootDirectory;
    if (rootDirectory) {
        delete rootDirectory;
        m_rootDirectory = NULL;
    }
    if (m_archivePath) {
        delete[] m_archivePath;
        m_archivePath = NULL;
    }
    if (m_pathDelimiters) {
        delete[] m_pathDelimiters;
        m_pathDelimiters = NULL;
    }
    CRezEntryPoolBlock* block = FirstEntryPoolBlock(m_entryPoolBlocks);
    m_isOpen = false;
    m_primaryStorage = NULL;
    m_rootDirectoryOffset = 0;
    m_rootDirectorySize = 0;
    m_rootDirectoryTime = 0;
    m_nextWritePos = 0;
    m_readOnly = true;
    m_rootDirectory = NULL;
    m_archiveTime = 0;
    m_isNewArchive = false;
    m_version = REZ_ARCHIVE_VERSION_1;
    m_largestKeyArrayLength = 0;
    m_largestDirectoryNameSize = 0;
    m_largestResourceNameSize = 0;
    m_largestCommentSize = 0;
    m_isDataContiguous = true;
    m_archivePath = NULL;
    if (block) {
        do {
            delete[] block->m_entries;
            m_entryPoolBlocks.Unlink(block);
            delete block;
            block = FirstEntryPoolBlock(m_entryPoolBlocks);
        } while (block);
    }
}

// @early-stop
RVA(0x0013af20, 0x3fa)
i32 CRezArchive::Open(char* path, b32 readOnly, b32 createNew) {
    m_readOnly = readOnly;
    if (readOnly == false) {
        return 0;
    }
    if (m_archivePath) {
        delete[] m_archivePath;
    }
    char* archivePath = new char[strlen(path) + 1];
    m_archivePath = archivePath;
    strcpy(archivePath, path);
    if (IsDirectoryPath(path) != 0) {

        if (m_readOnly == false) {
            return 0;
        }
        CRezItmBase* storage = new CRezDir(this, m_maxOpenFiles);
        if (storage == NULL) {
            delete[] m_archivePath;
            m_archivePath = NULL;
            return 0;
        }
        m_primaryStorage = storage;
        m_storages.AddHead(storage);
        m_storages.m_storageCount++;
        if (storage->Open(path, readOnly, createNew) == 0) {
            return 0;
        }
        m_isOpen = true;
        CRezArchiveDir* rootDirectory = new CRezArchiveDir(
            this,
            NULL,
            "",
            0,
            0,
            this->MakeTimestamp(),
            m_subdirectoryBucketCount,
            m_typeBucketCount
        );
        m_rootDirectory = rootDirectory;
        ImportDirectoryTree(storage, rootDirectory, m_archivePath, false);
        return 1;
    }

    CRezItmBase* storage = new CRezItm(this);
    if (storage == NULL) {
        delete[] m_archivePath;
        m_archivePath = NULL;
        return 0;
    }
    m_primaryStorage = storage;
    m_storages.AddHead(storage);
    m_storages.m_storageCount++;
    if (storage->Open(path, readOnly, createNew) == 0) {
        return 0;
    }
    m_isOpen = true;
    if (createNew != false) {
        m_nextWritePos = sizeof(RezArchiveHeader);
        m_isNewArchive = true;
        CRezArchiveDir* rootDirectory = new CRezArchiveDir(
            this,
            NULL,
            "",
            0,
            0,
            this->MakeTimestamp(),
            m_subdirectoryBucketCount,
            m_typeBucketCount
        );
        m_rootDirectory = rootDirectory;
        return 1;
    }

    RezArchiveHeader header;
    storage->Read(0, 0, sizeof(header), &header);
    m_version = header.m_version;
    m_rootDirectoryOffset = header.m_rootDirectoryOffset;
    m_rootDirectorySize = header.m_rootDirectorySize;
    m_rootDirectoryTime = header.m_rootDirectoryTime;
    m_nextWritePos = header.m_nextWritePos;
    m_archiveTime = header.m_archiveTime;
    m_largestKeyArrayLength = header.m_largestKeyArrayLength;
    m_largestDirectoryNameSize = header.m_largestDirectoryNameSize;
    m_largestResourceNameSize = header.m_largestResourceNameSize;
    m_largestCommentSize = header.m_largestCommentSize;
    m_isDataContiguous = header.m_isDataContiguous;
    if (header.m_initialCarriageReturn != REZ_ARCHIVE_MAGIC_CR) {
        return 0;
    }
    if (header.m_firstBannerLineFeed != REZ_ARCHIVE_MAGIC_LF) {
        return 0;
    }
    if (header.m_dosEndMarker != REZ_ARCHIVE_MAGIC_EOF) {
        return 0;
    }
    if (header.m_version != REZ_ARCHIVE_VERSION_1) {
        return 0;
    }
    CRezArchiveDir* rootDirectory = new CRezArchiveDir(
        this,
        NULL,
        "",
        m_rootDirectoryOffset,
        m_rootDirectorySize,
        m_rootDirectoryTime,
        m_subdirectoryBucketCount,
        m_typeBucketCount
    );
    m_rootDirectory = rootDirectory;
    rootDirectory->ReadDirectoryTree(storage, m_rootDirectoryOffset, m_rootDirectorySize, false);
    return 1;
}

// @early-stop
RVA(0x0013b320, 0x238)
i32 CRezArchive::MergeArchive(char* path, b32 replaceExisting) {
    if (m_readOnly == false) {
        return 0;
    }
    m_isDataContiguous = false;
    if (m_archivePath) {
        delete[] m_archivePath;
    }
    char* archivePath = new char[strlen(path) + 1];
    m_archivePath = archivePath;
    strcpy(archivePath, path);

    if (IsDirectoryPath(path)) {
        CRezItmBase* storage = new CRezDir(this, m_maxOpenFiles);
        if (storage == NULL) {
            delete[] m_archivePath;
            m_archivePath = NULL;
            return 0;
        }
        m_storages.AddHead(storage);
        m_storages.m_storageCount++;
        if (storage->Open(path, true, false) == 0) {
            return 0;
        }
        m_isOpen = true;
        ImportDirectoryTree(storage, m_rootDirectory, m_archivePath, replaceExisting);
        return 1;
    }

    CRezItmBase* storage = new CRezItm(this);
    if (storage == NULL) {
        delete[] m_archivePath;
        m_archivePath = NULL;
        return 0;
    }
    m_storages.AddHead(storage);
    m_storages.m_storageCount++;
    if (storage->Open(path, true, false) == 0) {
        return 0;
    }

    RezArchiveHeader header;
    storage->Read(0, 0, sizeof(header), &header);
    u32 mergedMaximum;
    mergedMaximum = header.m_largestKeyArrayLength;
    if (mergedMaximum > m_largestKeyArrayLength) {
        m_largestKeyArrayLength = mergedMaximum;
    }
    mergedMaximum = header.m_largestDirectoryNameSize;
    if (mergedMaximum > m_largestDirectoryNameSize) {
        m_largestDirectoryNameSize = mergedMaximum;
    }
    mergedMaximum = header.m_largestResourceNameSize;
    if (mergedMaximum > m_largestResourceNameSize) {
        m_largestResourceNameSize = mergedMaximum;
    }
    mergedMaximum = header.m_largestCommentSize;
    if (mergedMaximum > m_largestCommentSize) {
        m_largestCommentSize = mergedMaximum;
    }
    m_rootDirectory->ReadDirectoryTree(
        storage,
        header.m_rootDirectoryOffset,
        header.m_rootDirectorySize,
        replaceExisting
    );
    return 1;
}

RVA(0x0013b560, 0x545)
i32 CRezArchive::ImportDirectoryTree(
    CRezItmBase* storage,
    CRezArchiveDir* directory,
    char* path,
    b32 replaceExisting
) {
    char pattern[REZ_SCAN_PATH_MAX];
    strcpy(pattern, path);
    if (pattern[strlen(pattern) - 1] != '\\') {
        strcat(pattern, "\\");
    }
    char full[REZ_SCAN_PATH_MAX];
    strcpy(full, pattern);
    strcat(full, g_wildcard);
    _finddata_t fileData;
    i32 searchHandle = _findfirst(full, &fileData);
    if (searchHandle < 0) {
        return 1;
    }
    do {
        if (strcmp(fileData.name, g_singleDot) == 0 || strcmp(fileData.name, g_dotDot) == 0) {
            continue;
        }
        if ((fileData.attrib & _A_SUBDIR) == _A_SUBDIR) {

            char subdirectoryName[REZ_SCAN_PATH_MAX];
            strcpy(subdirectoryName, fileData.name);
            if (m_caseSensitive == false) {
                _strupr(subdirectoryName);
            }
            char childPath[REZ_SCAN_PATH_MAX];
            strcpy(childPath, pattern);
            strcat(childPath, subdirectoryName);
            strcat(childPath, "\\");
            CRezArchiveDir* child = directory->FindSubdirectory(subdirectoryName);
            if (child == NULL) {
                child = directory->CreateSubdirectory(subdirectoryName);
                if (child == NULL) {
                    continue;
                }
            }
            ImportDirectoryTree(storage, child, childPath, replaceExisting);
            continue;
        }

        char filePath[REZ_SCAN_PATH_MAX];
        strcpy(filePath, pattern);
        strcat(filePath, fileData.name);
        char drive[_MAX_DRIVE];
        char directoryPath[_MAX_PATH];
        char splitName[_MAX_PATH];
        char resourceName[REZ_SCAN_PATH_MAX];
        char extension[_MAX_PATH];
        _splitpath(filePath, drive, directoryPath, splitName, extension);
        strcpy(resourceName, splitName);
        _strupr(resourceName);
        i32 nameLength = static_cast<i32>(strlen(resourceName));
        i32 leadingDigitCount = 0;
        while (leadingDigitCount < nameLength && resourceName[leadingDigitCount] >= '0'
               && resourceName[leadingDigitCount] <= '9') {
            leadingDigitCount++;
        }
        i32 resourceId = (leadingDigitCount < nameLength)
                             ? static_cast<i32>(m_nextGeneratedResourceId++)
                             : atol(resourceName);
        RezTypeTag typeTag;
        char extensionName[8];
        char unpackedTag[8];
        if (strlen(extension) != 0) {
            strcpy(extensionName, extension + 1);
            _strupr(extensionName);
            typeTag = PackTag(extensionName);
        } else {
            typeTag = REZ_TAG_NONE;
        }
        UnpackTag(typeTag, unpackedTag);
        CRezArchiveType* type = directory->FindOrCreateType(IDX(typeTag));
        CRezArchiveEntry* existing = directory->FindEntry(resourceName, typeTag);
        CRezArchiveEntry* entry;
        if (existing == NULL) {
            entry = directory->CreateEntry(static_cast<u32>(resourceId), resourceName, type, NULL);
        } else if (replaceExisting != false) {
            directory->RemoveEntry(type, existing);
            entry = directory->CreateEntry(static_cast<u32>(resourceId), resourceName, type, NULL);
        } else {
            entry = NULL;
        }
        if (entry != NULL) {
            entry->m_time = static_cast<i32>(fileData.time_write);
            entry->m_size = static_cast<u32>(fileData.size);
            entry->m_storage = new CRezFile(this, filePath, static_cast<CRezDir*>(storage));
        }
    } while (_findnext(searchHandle, &fileData) == 0);
    _findclose(searchHandle);
    return 1;
}

RVA(0x0013bab0, 0xa8)
i32 CRezArchive::Close(i32 unusedFinal) {
    static_cast<void>(unusedFinal);
    i32 result = m_primaryStorage->Close();
    m_storages.Remove(m_primaryStorage);
    m_storages.m_storageCount--;
    delete m_primaryStorage;
    m_primaryStorage = NULL;
    CRezItmBase* storage;
    for (storage = FirstStorage(m_storages); storage != NULL; storage = FirstStorage(m_storages)) {
        storage->Close();
        m_storages.Remove(storage);
        m_storages.m_storageCount--;
        delete storage;
    }
    if (m_rootDirectory) {
        delete m_rootDirectory;
        m_rootDirectory = NULL;
    }
    if (m_archivePath) {
        delete[] m_archivePath;
        m_archivePath = NULL;
    }
    m_isOpen = false;
    return result;
}

RVA(0x0013bb60, 0x4)
CRezArchiveDir* CRezArchive::GetRootDirectory() {
    return m_rootDirectory;
}

RVA(0x0013bb70, 0x58)
RezTypeTag CRezArchive::PackTag(const char* typeName) {
    if (!typeName) {
        return REZ_TAG_NONE;
    }
    DwordBytes packedTag;
    packedTag.m_value = 0;
    u8* bytes = packedTag.m_bytes;
    i32 length = static_cast<i32>(strlen(typeName));
    if (length > 0) {
        bytes[length - 1] = typeName[0];
    }
    if (length > 1) {
        bytes[length - 2] = typeName[1];
    }
    if (length > 2) {
        bytes[length - 3] = typeName[2];
    }
    if (length > 3) {
        bytes[length - 4] = typeName[3];
    }
    return static_cast<RezTypeTag>(packedTag.m_value);
}

RVA(0x0013bbd0, 0x72)
void CRezArchive::UnpackTag(RezTypeTag tag, char* destination) {
    if (!destination) {
        return;
    }
    RecordBytes<RezTypeTag> tagBytes;
    tagBytes.m_rec = &tag;
    const u8* bytes = tagBytes.m_bytes;
    i32 length = 0;
    if (bytes[3]) {
        length = 4;
    } else if (bytes[2]) {
        length = 3;
    } else if (bytes[1]) {
        length = 2;
    } else if (bytes[0]) {
        length = 1;
    }
    if (length > 0) {
        destination[0] = bytes[length - 1];
    }
    if (length > 1) {
        destination[1] = bytes[length - 2];
    }
    if (length > 2) {
        destination[2] = bytes[length - 3];
    }
    if (length > 3) {
        destination[3] = bytes[length - 4];
    }
    destination[length] = 0;
}

RVA(0x0013bc50, 0x5)
i32 CRezArchive::UnusedArchiveQuery(i32 unused) {
    static_cast<void>(unused);
    return 0;
}

RVA(0x0013bc60, 0x3)
void CRezArchive::UnusedArchiveAction(i32 unused) {
    static_cast<void>(unused);
}

RVA(0x0013bc70, 0x3)
i32 CRezArchive::RetryStorageOperation() {
    return 0;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0013bc80, 0x27)
i32 CRezArchive::CheckStorages() {
    b32 allStoragesValid = true;
    for (CRezItmBase* storage = m_storages.m_head; storage != NULL; storage = storage->m_next) {
        if (storage->Check() == 0) {
            allStoragesValid = false;
        }
    }
    return allStoragesValid;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0013bcb0, 0x1f)
void CRezArchive::SetBucketCounts(
    i32 resourceNameBuckets,
    i32 resourceIdBuckets,
    i32 subdirectoryBuckets,
    i32 typeBuckets
) {
    m_resourceNameBucketCount = resourceNameBuckets;
    m_resourceIdBucketCount = resourceIdBuckets;
    m_subdirectoryBucketCount = subdirectoryBuckets;
    m_typeBucketCount = typeBuckets;
}

RVA(0x0013bcd0, 0x10)
i32 CRezArchive::MakeTimestamp() {
    time_t timestamp;
    return static_cast<i32>(time(&timestamp));
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0013bce0, 0x57)
void CRezArchive::SetPathDelimiters(char* delimiters) {
    if (m_pathDelimiters != NULL) {
        delete[] m_pathDelimiters;
    }
    m_pathDelimiters = new char[strlen(delimiters) + 1];
    strcpy(m_pathDelimiters, delimiters);
}

RVA(0x0013bd40, 0x1d7)
CRezArchiveDir* CRezArchiveDir::FindDirectoryByPath(const char* path) {
    char component[0x40];
    if (static_cast<i32>(strlen(path)) > 1) {
        if (!IsPathComponentCharacter(m_archive->m_pathDelimiters, *path)) {
            ++path;
        }
    }
    const char* cursor = path;
    i32 componentLength = 0;
    while (IsPathComponentCharacter(m_archive->m_pathDelimiters, *cursor)) {
        component[componentLength] = *cursor;
        ++componentLength;
        ++cursor;
    }
    component[componentLength] = 0;
    CRezArchiveDir* subdirectory = FindSubdirectory(component);
    if (!subdirectory) {
        return subdirectory;
    }
    char separator = path[componentLength];
    if (separator == 0) {
        return subdirectory;
    }
    while (!IsPathComponentCharacter(m_archive->m_pathDelimiters, separator)) {
        separator = path[componentLength + 1];
        ++componentLength;
        if (separator == 0) {
            return subdirectory;
        }
    }
    return subdirectory->FindDirectoryByPath(path + componentLength);
}

RVA(0x0013bf20, 0x1ac)
CRezArchiveEntry* CRezArchiveDir::FindEntryByPath(const char* qualifiedPath) {
    char directoryPath[0x100];
    char resourceName[0x20];
    i32 pathLength = static_cast<i32>(strlen(qualifiedPath));
    if (pathLength > 1) {
        if (!IsPathComponentCharacter(m_archive->m_pathDelimiters, *qualifiedPath)) {
            ++qualifiedPath;
            --pathLength;
        }
    }
    i32 separatorIndex = pathLength - 1;
    while (IsPathComponentCharacter(m_archive->m_pathDelimiters, qualifiedPath[separatorIndex])) {
        --separatorIndex;
        if (separatorIndex < 0) {
            break;
        }
    }
    if (separatorIndex == pathLength) {
        return NULL;
    }
    const char* nameStart = qualifiedPath + separatorIndex + 1;
    strcpy(resourceName, nameStart);
    if (separatorIndex <= 1) {
        return FindEntryByFilename(resourceName);
    }
    strncpy(directoryPath, qualifiedPath, static_cast<u32>(separatorIndex));
    directoryPath[separatorIndex] = 0;
    CRezArchiveDir* directory = FindDirectoryByPath(directoryPath);
    if (!directory) {
        return NULL;
    }
    return directory->FindEntryByFilename(resourceName);
}

RVA(0x0013c0d0, 0x1bc)
CRezArchiveEntry* CRezArchiveDir::FindEntryByPath(const char* qualifiedPath, RezTypeTag typeTag) {
    char directoryPath[0x100];
    char resourceName[0x20];
    i32 pathLength = static_cast<i32>(strlen(qualifiedPath));
    if (pathLength > 1) {
        if (!IsPathComponentCharacter(m_archive->m_pathDelimiters, *qualifiedPath)) {
            ++qualifiedPath;
            --pathLength;
        }
    }
    i32 separatorIndex = pathLength - 1;
    while (IsPathComponentCharacter(m_archive->m_pathDelimiters, qualifiedPath[separatorIndex])) {
        --separatorIndex;
        if (separatorIndex < 0) {
            break;
        }
    }
    if (separatorIndex == pathLength) {
        return NULL;
    }
    const char* nameStart = qualifiedPath + separatorIndex + 1;
    strcpy(resourceName, nameStart);
    if (separatorIndex <= 1) {
        return FindEntry(resourceName, typeTag);
    }
    strncpy(directoryPath, qualifiedPath, static_cast<u32>(separatorIndex));
    directoryPath[separatorIndex] = 0;
    CRezArchiveDir* directory = FindDirectoryByPath(directoryPath);
    if (!directory) {
        return NULL;
    }
    return directory->FindEntry(resourceName, typeTag);
}

RVA(0x0013c290, 0x19)
CRezArchiveEntry* CRezArchive::FindEntryByPath(const char* path, RezTypeTag typeTag) {
    return GetRootDirectory()->FindEntryByPath(path, typeTag);
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0013c2b0, 0x14)
CRezArchiveEntry* CRezArchive::FindEntryByPath(const char* path) {
    return GetRootDirectory()->FindEntryByPath(path);
}

RVA(0x0013c2d0, 0x14)
CRezArchiveDir* CRezArchive::FindDirectoryByPath(const char* path) {
    return GetRootDirectory()->FindDirectoryByPath(path);
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0013c2f0, 0x28)
i32 CRezArchive::Reload() {
    if (m_isOpen == false) {
        return 0;
    }
    Close(0);
    return Open(m_archivePath, true, false);
}

RVA(0x0013c320, 0x3c)
i32 CRezArchive::IsDirectoryPath(char* path) {
    struct _stat fileInfo;
    if (_stat(path, &fileInfo) != 0) {
        return 0;
    }
    return (fileInfo.st_mode & _S_IFDIR) == _S_IFDIR;
}

// @early-stop
RVA(0x0013c360, 0x14b)
CRezArchiveEntry* CRezArchive::AcquireEntry() {
    CRezArchiveEntry* entry = NULL;
    CHashElement* node = m_freeEntries.First();
    if (node != NULL) {
        entry = node->m_archiveEntry;
    }
    if (entry == NULL) {
        CRezEntryPoolBlock* block = new CRezEntryPoolBlock;
        if (block == NULL) {
            return NULL;
        }
        CRezArchiveEntry* entries = new CRezArchiveEntry[m_entriesPerPoolBlock];
        block->m_entries = entries;
        if (entries == NULL) {
            delete block;
            return NULL;
        }
        for (i32 index = 0; static_cast<u32>(index) < static_cast<u32>(m_entriesPerPoolBlock);
             index++) {
            block->m_entries[index].m_nameNode.m_archiveEntry = &block->m_entries[index];
            m_freeEntries.Insert(&block->m_entries[index].m_nameNode);
        }
        m_entryPoolBlocks.InsertHead(block);
        node = m_freeEntries.First();
        entry = node->m_archiveEntry;
    }
    if (entry) {
        m_freeEntries.Remove(&entry->m_nameNode);
    }
    return entry;
}

RVA(0x0013c4b0, 0x1a)
void CRezArchive::ReleaseEntry(CRezArchiveEntry* entry) {
    if (entry) {
        m_freeEntries.Insert(&entry->m_nameNode);
    }
}

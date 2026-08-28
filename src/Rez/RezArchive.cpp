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

static inline CRezEntryPoolBlock* FirstEntryPoolBlock(CLTBaseList& list) {
    return static_cast<CRezEntryPoolBlock*>(list.GetFirst());
}

static inline CRezItmBase* FirstStorage(CObjList& list) {
    return list.m_head;
}

static const i32 REZ_SCAN_PATH_MAX = 0x308;

// Byte-forced view of packed serialized storage.
static inline i32 ReadPackedI32(const char* bytes) {
    i32 value;
    memcpy(&value, bytes, sizeof(value));
    return value;
}

RVA(0x001396f0, 0x1a)
CRezArchiveEntry::CRezArchiveEntry() {

    m_storage = NULL;
    m_directory = NULL;
    m_name = NULL;
    m_heName.SetRezItm(this);
}

RVA(0x00139710, 0x8d)
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
    m_heName.SetRezItm(this);
}

RVA(0x001397a0, 0x57)
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
    m_heName.SetRezItm(NULL);
}

RVA(0x00139800, 0x6)
GZ_ENUM_RETURN(RezTypeTag, u32) CRezArchiveEntry::GetTypeTag() {
    return static_cast<RezTypeTag>(m_type->m_typeTag);
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00139810, 0x140)
char* CRezArchiveEntry::GetDirectoryPath(char* destination, i32 size) {
    if (m_directory->m_parent == NULL) {
        strcpy(destination, DATA_COMPGEN(0x0020cff0, "\\"));
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
RVA(0x00139950, 0x6)
char* CRezArchiveEntry::GetDirectoryName() {
    return m_directory->m_name;
}

RVA(0x00139960, 0x6b)
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

RVA(0x001399d0, 0x21)
i32 CRezArchiveEntry::ReleaseData() {
    if (m_loadedData != NULL) {
        delete[] m_loadedData;
        m_loadedData = NULL;
    }
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00139a00, 0x1b)
i32 CRezArchiveEntry::IsDataLoaded() {
    if (m_directory->m_preloadedData != NULL) {
        return 1;
    }
    return m_loadedData != NULL;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00139a20, 0x13)
i32 CRezArchiveEntry::ReadAll(void* destination) {
    return ReadAt(destination, 0, m_size);
}

RVA(0x00139a40, 0x95)
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

RVA(0x00139ae0, 0xf)
i32 CRezArchiveEntry::SetPos(i32 position) {
    m_cursor = position;
    return 1;
}

RVA(0x00139af0, 0xcc)
i32 CRezArchiveEntry::Read(void* destination, u32 byteCount, i32 seekPosition) {
    if (seekPosition != -1) {
        SetPos(seekPosition);
    }

    if (byteCount + m_cursor > m_size) {
        byteCount = m_size - m_cursor;
    }
    if (byteCount <= 0) {
        return 0;
    }
    if (m_directory->m_preloadedData != NULL) {
        memcpy(
            destination,
            m_directory->m_preloadedData + m_dataOffset + m_cursor - m_directory->m_minDataOffset,
            byteCount
        );
        m_cursor += byteCount;
        return byteCount;
    }
    if (m_loadedData != NULL) {
        memcpy(destination, m_loadedData + m_cursor, byteCount);
        m_cursor += byteCount;
        return byteCount;
    }
    if (m_storage->Read(m_dataOffset, m_cursor, byteCount, destination)
        == static_cast<i32>(byteCount)) {
        m_cursor += byteCount;
        return byteCount;
    }
    return 0;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00139bc0, 0xc)
i32 CRezArchiveEntry::AtEnd() {
    return static_cast<u32>(m_cursor) >= m_size;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00139bd0, 0x15)
char CRezArchiveEntry::ReadChar() {
    char value;
    Read(&value, 1, -1);
    return value;
}

RVA(0x00139bf0, 0x71)
CRezArchiveType::CRezArchiveType(
    i32 typeTag,
    CRezArchiveDir* directory,
    i32 resourceIdBucketCount,
    i32 resourceNameBucketCount
)
    : m_haID(resourceIdBucketCount), m_haName(resourceNameBucketCount) {
    m_typeTag = typeTag;
    m_heType.SetRezTyp(this);
    m_directory = directory;
}

RVA_COMPGEN(0x00139c70, 0x5, ??1CRezItmHashTableByID@@QAE@XZ)

RVA(0x00139c80, 0x6c)
CRezArchiveType::CRezArchiveType(
    i32 typeTag,
    CRezArchiveDir* directory,
    i32 resourceNameBucketCount
)
    : m_haID(), m_haName(resourceNameBucketCount) {
    m_typeTag = typeTag;
    m_heType.SetRezTyp(this);
    m_directory = directory;
}

RVA(0x00139cf0, 0xd7)
CRezArchiveType::~CRezArchiveType() {
    if (m_directory->m_archive->m_useIdIndex != false) {
        CRezItmHashByID* node = m_haID.GetFirst();
        CRezItmHashByID* current;
        while (node) {
            current = node;
            node = current->Next();
            m_haID.Delete(current);
        }
    }
    {
        CRezItmHashByName* node = m_haName.GetFirst();
        CRezItmHashByName* current;
        while (node) {
            current = node;
            node = current->Next();
            m_haName.Delete(current);
            current->GetRezItm()->Reset();
            m_directory->m_archive->ReleaseEntry(current->GetRezItm());
        }
    }
    m_typeTag = 0;
    m_heType.SetRezTyp(NULL);
}

RVA_COMPGEN(0x00139dd0, 0x5, ??1CRezItmHashTableByName@@QAE@XZ)

RVA(0x00139de0, 0xd4)
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
    : m_haDir(subdirectoryBucketCount), m_haTypes(typeBucketCount) {
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
    m_heDir.SetRezDir(this);
}

RVA_COMPGEN(0x00139ec0, 0x5, ??1CRezDirHashTable@@QAE@XZ)
RVA_COMPGEN(0x00139ed0, 0x5, ??1CRezTypeHashTable@@QAE@XZ)

RVA(0x00139ee0, 0x11e)
CRezArchiveDir::~CRezArchiveDir() {

    {
        CRezTypeHash* node = m_haTypes.GetFirst();
        CRezTypeHash* current;
        while (node != NULL) {
            current = node;
            node = current->Next();
            m_haTypes.Delete(current);
            delete current->GetRezTyp();
        }
    }
    {
        CRezDirHash* node = m_haDir.GetFirst();
        CRezDirHash* current;
        while (node != NULL) {
            current = node;
            node = current->Next();
            m_haDir.Delete(current);
            delete current->GetRezDir();
        }
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
    m_heDir.SetRezDir(NULL);
}

RVA(0x0013a000, 0x37)
CRezArchiveEntry* CRezArchiveDir::FindEntry(const char* name, RezTypeTag typeTag) {
    CRezArchiveType* type = m_haTypes.Find(IDX(typeTag));
    if (!type) {
        return NULL;
    }
    return type->m_haName.Find(name, m_archive->m_caseSensitive == false);
}

RVA(0x0013a040, 0xa2)
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
RVA(0x0013a0f0, 0x99)
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
        for (CRezDirHash* node = m_haDir.GetFirst(); node != NULL; node = node->Next()) {

            node->GetRezDir()->PreloadData(true);
        }
    }
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0013a190, 0x94)
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
        CRezDirHash* node = m_haDir.GetFirst();
        while (node) {
            node->GetRezDir()->ReleaseEntryData(true);
            node = node->Next();
        }
    }
    return 1;
}

RVA(0x0013a230, 0x29)
CRezArchiveDir* CRezArchiveDir::FindSubdirectory(const char* name) {
    if (!name) {
        return NULL;
    }
    return m_haDir.Find(name, m_archive->m_caseSensitive == false);
}

RVA(0x0013a260, 0x11)
CRezArchiveDir* CRezArchiveDir::FirstSubdirectory() {
    CRezDirHash* node = m_haDir.GetFirst();
    if (!node) {
        return NULL;
    }
    return node->GetRezDir();
}

RVA(0x0013a280, 0x19)
CRezArchiveDir* CRezArchiveDir::NextSubdirectory(CRezArchiveDir* directory) {
    CRezDirHash* node = directory->m_heDir.Next();
    if (!node) {
        return NULL;
    }
    return node->GetRezDir();
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0013a2a0, 0x10)
CRezArchiveType* CRezArchiveDir::FindType(u32 typeTag) {
    return m_haTypes.Find(typeTag);
}

RVA(0x0013a2b0, 0x11)
CRezArchiveType* CRezArchiveDir::FirstType() {
    CRezTypeHash* node = m_haTypes.GetFirst();
    if (!node) {
        return NULL;
    }
    return node->GetRezTyp();
}

RVA(0x0013a2d0, 0x19)
CRezArchiveType* CRezArchiveDir::NextType(CRezArchiveType* type) {
    CRezTypeHash* node = type->m_heType.Next();
    if (!node) {
        return NULL;
    }
    return node->GetRezTyp();
}

RVA(0x0013a2f0, 0x19)
CRezArchiveEntry* CRezArchiveDir::FirstEntry(CRezArchiveType* type) {
    CRezItmHashByName* node = type->m_haName.GetFirst();
    if (!node) {
        return NULL;
    }
    return node->GetRezItm();
}

RVA(0x0013a310, 0x19)
CRezArchiveEntry* CRezArchiveDir::NextEntry(CRezArchiveEntry* entry) {
    CRezItmHashByName* node = entry->m_heName.Next();
    if (!node) {
        return NULL;
    }
    return node->GetRezItm();
}

RVA(0x0013a330, 0xce)
CRezArchiveDir* CRezArchiveDir::CreateSubdirectory(const char* name) {

    if (m_haDir.Find(name, m_archive->m_caseSensitive == false) != NULL) {
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
    m_haDir.Insert(&child->m_heDir);

    u32 nameLength = strlen(name);
    if (m_archive->m_largestDirectoryNameSize <= nameLength) {
        m_archive->m_largestDirectoryNameSize = nameLength + 1;
    }
    return child;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0013a400, 0xa9)
CRezArchiveEntry*
CRezArchiveDir::CreateNamedEntry(void* resourceId, const char* name, i32 typeTag) {
    CRezArchiveType* type = FindOrCreateType(typeTag);
    if (type->m_haName.Find(name, m_archive->m_caseSensitive == false) != NULL) {
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
    type->m_haName.Insert(&entry->m_heName);
    u32 nameLength = strlen(name);
    if (m_archive->m_largestResourceNameSize <= nameLength) {
        m_archive->m_largestResourceNameSize = nameLength + 1;
    }
    return entry;
}

RVA(0x0013a4b0, 0x75)
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
    type->m_haName.Insert(&entry->m_heName);
    u32 nameLength = strlen(name);
    if (m_archive->m_largestResourceNameSize <= nameLength) {
        m_archive->m_largestResourceNameSize = nameLength + 1;
    }
    return entry;
}

RVA(0x0013a530, 0x47)
i32 CRezArchiveDir::RemoveEntry(CRezArchiveType* type, CRezArchiveEntry* entry) {
    m_totalDataSize -= entry->m_size;
    type->m_haName.Delete(&entry->m_heName);
    entry->Reset();
    m_archive->ReleaseEntry(entry);
    m_archive->m_isDataContiguous = false;
    return 1;
}

RVA(0x0013a580, 0xb2)
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
    CRezDirHash* node = m_haDir.GetFirst();
    while (node) {
        node->GetRezDir()->m_bodyOffset = 0;
        node = node->Next();
    }
    if (ReadDirectoryBody(storage, bodyOffset, bodySize, replaceExisting) != 0) {
        node = m_haDir.GetFirst();
        while (node) {
            CRezArchiveDir* subdirectory = node->GetRezDir();
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

RVA(0x0013a640, 0x2f7)
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
            CRezDirHashTable* subdirectories = &m_haDir;
            CRezArchiveDir* existing =
                subdirectories->Find(name, m_archive->m_caseSensitive == false);
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
                subdirectories->Insert(&directory->m_heDir);
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
            CRezArchiveEntry* found = type->m_haName.Find(name, 1);
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
                type->m_haName.Insert(&entry->m_heName);
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

RVA(0x0013a940, 0xc2)
CRezArchiveType* CRezArchiveDir::FindOrCreateType(i32 typeTag) {

    CRezArchiveType* type = m_haTypes.Find(static_cast<u32>(typeTag));
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
        m_haTypes.Insert(&type->m_heType);
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

RVA(0x0013aa10, 0xdc)
CRezArchive::CRezArchive() : m_hashRezItmFreeList(1) {
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
RVA(0x0013ab00, 0xac)
CRezArchive::CRezArchive(char* path, b32 readOnly, b32 createNew) : m_hashRezItmFreeList(1) {
    {
        CRezArchive defaults;
    }
    Open(path, readOnly, createNew);
}

RVA(0x0013abc0, 0x13f)
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
            m_entryPoolBlocks.Delete(block);
            delete block;
            block = FirstEntryPoolBlock(m_entryPoolBlocks);
        } while (block);
    }
}

RVA(0x0013ad00, 0x3b8)
i32 CRezArchive::Open(char* path, b32 readOnly, b32 createNew) {
    m_readOnly = readOnly;
    if (readOnly == false) {
        return 0;
    }
    if (m_archivePath) {
        delete[] m_archivePath;
    }
    m_archivePath = new char[strlen(path) + 1];
    strcpy(m_archivePath, path);
    if (IsDirectoryPath(path) != 0) {

        if (m_readOnly == false) {
            return 0;
        }
        CRezDir* storage = new CRezDir(this, m_maxOpenFiles);
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
        m_rootDirectory = new CRezArchiveDir(
            this,
            NULL,
            "",
            0,
            0,
            this->MakeTimestamp(),
            m_subdirectoryBucketCount,
            m_typeBucketCount
        );
        ImportDirectoryTree(storage, m_rootDirectory, m_archivePath, false);
        return 1;
    }

    CRezItm* storage = new CRezItm(this);
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
        m_rootDirectory = new CRezArchiveDir(
            this,
            NULL,
            "",
            0,
            0,
            this->MakeTimestamp(),
            m_subdirectoryBucketCount,
            m_typeBucketCount
        );
        return 1;
    }

    RezArchiveHeader header;
    storage->Read(0, 0, sizeof(header), &header);
    m_nextWritePos = header.m_nextWritePos;
    m_rootDirectoryOffset = header.m_rootDirectoryOffset;
    m_rootDirectorySize = header.m_rootDirectorySize;
    m_rootDirectoryTime = header.m_rootDirectoryTime;
    m_archiveTime = header.m_archiveTime;
    m_version = header.m_version;
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
    m_rootDirectory = new CRezArchiveDir(
        this,
        NULL,
        "",
        m_rootDirectoryOffset,
        m_rootDirectorySize,
        m_rootDirectoryTime,
        m_subdirectoryBucketCount,
        m_typeBucketCount
    );
    m_rootDirectory->ReadDirectoryTree(storage, m_rootDirectoryOffset, m_rootDirectorySize, false);
    return 1;
}

RVA(0x0013b0c0, 0x238)
i32 CRezArchive::MergeArchive(char* path, b32 replaceExisting) {
    b32 readOnly = true;
    b32 createNew = false;
    if (m_readOnly == false) {
        return 0;
    }
    m_isDataContiguous = false;
    if (m_archivePath) {
        delete[] m_archivePath;
    }
    m_archivePath = new char[strlen(path) + 1];
    strcpy(m_archivePath, path);

    if (IsDirectoryPath(path)) {
        CRezDir* storage = new CRezDir(this, m_maxOpenFiles);
        if (storage == NULL) {
            delete[] m_archivePath;
            m_archivePath = NULL;
            return 0;
        }
        m_storages.AddHead(storage);
        m_storages.m_storageCount++;
        if (storage->Open(path, readOnly, createNew) == 0) {
            return 0;
        }
        m_isOpen = true;
        ImportDirectoryTree(storage, m_rootDirectory, m_archivePath, replaceExisting);
        return 1;
    }

    CRezItm* storage = new CRezItm(this);
    if (storage == NULL) {
        delete[] m_archivePath;
        m_archivePath = NULL;
        return 0;
    }
    m_storages.AddHead(storage);
    m_storages.m_storageCount++;
    if (storage->Open(path, readOnly, createNew) == 0) {
        return 0;
    }

    RezArchiveHeader header;
    storage->Read(0, 0, sizeof(header), &header);
    if (header.m_largestKeyArrayLength > m_largestKeyArrayLength) {
        m_largestKeyArrayLength = header.m_largestKeyArrayLength;
    }
    if (header.m_largestDirectoryNameSize > m_largestDirectoryNameSize) {
        m_largestDirectoryNameSize = header.m_largestDirectoryNameSize;
    }
    if (header.m_largestResourceNameSize > m_largestResourceNameSize) {
        m_largestResourceNameSize = header.m_largestResourceNameSize;
    }
    if (header.m_largestCommentSize > m_largestCommentSize) {
        m_largestCommentSize = header.m_largestCommentSize;
    }
    m_rootDirectory->ReadDirectoryTree(
        storage,
        header.m_rootDirectoryOffset,
        header.m_rootDirectorySize,
        replaceExisting
    );
    return 1;
}

RVA(0x0013b300, 0x545)
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

RVA(0x0013b850, 0xa8)
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

RVA(0x0013b900, 0x4)
CRezArchiveDir* CRezArchive::GetRootDirectory() {
    return m_rootDirectory;
}

RVA(0x0013b910, 0x58)
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

RVA(0x0013b970, 0x72)
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

RVA(0x0013b9f0, 0x5)
i32 CRezArchive::UnusedArchiveQuery(i32 unused) {
    static_cast<void>(unused);
    return 0;
}

RVA(0x0013ba00, 0x3)
void CRezArchive::UnusedArchiveAction(i32 unused) {
    static_cast<void>(unused);
}

RVA(0x0013ba10, 0x3)
i32 CRezArchive::RetryStorageOperation() {
    return 0;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0013ba20, 0x27)
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
RVA(0x0013ba50, 0x1f)
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

RVA(0x0013ba70, 0x10)
i32 CRezArchive::MakeTimestamp() {
    time_t timestamp;
    return static_cast<i32>(time(&timestamp));
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0013ba80, 0x57)
void CRezArchive::SetPathDelimiters(char* delimiters) {
    if (m_pathDelimiters != NULL) {
        delete[] m_pathDelimiters;
    }
    m_pathDelimiters = new char[strlen(delimiters) + 1];
    strcpy(m_pathDelimiters, delimiters);
}

RVA(0x0013bae0, 0x1b9)
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

RVA(0x0013bca0, 0x19c)
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

RVA(0x0013be40, 0x1ac)
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

RVA(0x0013bff0, 0x19)
CRezArchiveEntry* CRezArchive::FindEntryByPath(const char* path, RezTypeTag typeTag) {
    return GetRootDirectory()->FindEntryByPath(path, typeTag);
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0013c010, 0x14)
CRezArchiveEntry* CRezArchive::FindEntryByPath(const char* path) {
    return GetRootDirectory()->FindEntryByPath(path);
}

RVA(0x0013c030, 0x14)
CRezArchiveDir* CRezArchive::FindDirectoryByPath(const char* path) {
    return GetRootDirectory()->FindDirectoryByPath(path);
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0013c050, 0x28)
i32 CRezArchive::Reload() {
    if (m_isOpen == false) {
        return 0;
    }
    Close(0);
    return Open(m_archivePath, true, false);
}

RVA(0x0013c080, 0x3c)
i32 CRezArchive::IsDirectoryPath(char* path) {
    struct _stat fileInfo;
    if (_stat(path, &fileInfo) != 0) {
        return 0;
    }
    return (fileInfo.st_mode & _S_IFDIR) == _S_IFDIR;
}

RVA(0x0013c0c0, 0x14b)
CRezArchiveEntry* CRezArchive::AcquireEntry() {
    CRezArchiveEntry* entry = NULL;
    CRezItmHashByName* node;
    node = m_hashRezItmFreeList.GetFirst();
    if (node != NULL) {
        entry = node->GetRezItm();
    }
    if (entry == NULL) {
        CRezEntryPoolBlock* block;
        block = new CRezEntryPoolBlock;
        if (block == NULL) {
            return NULL;
        }
        block->m_entries = new CRezArchiveEntry[m_entriesPerPoolBlock];
        if (block->m_entries == NULL) {
            delete block;
            return NULL;
        }
        for (u32 index = 0; index < static_cast<u32>(m_entriesPerPoolBlock); index++) {
            block->m_entries[index].m_heName.SetRezItm(&block->m_entries[index]);
            m_hashRezItmFreeList.Insert(&block->m_entries[index].m_heName);
        }
        m_entryPoolBlocks.InsertFirst(block);
        entry = m_hashRezItmFreeList.GetFirst()->GetRezItm();
    }
    if (entry) {
        m_hashRezItmFreeList.Delete(&entry->m_heName);
    }
    return entry;
}

RVA(0x0013c210, 0x1a)
void CRezArchive::ReleaseEntry(CRezArchiveEntry* entry) {
    if (entry) {
        m_hashRezItmFreeList.Insert(&entry->m_heName);
    }
}

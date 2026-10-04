#include <StdAfx.h>

#include <Ints.h>

#include <Rez/RezArchive.h>

#include <Enums.h>
#include <Pix16.h>
#include <Rez/DebugPrintf.h>
#include <Rez/RezArchiveDir.h>
#include <Rez/RezArchiveEntry.h>
#include <Rez/RezFile.h>
#include <Rez/RezMgr.h>
#include <Rez/RezTypeTag.h>
#include <SafeDelete.h>
#include <Utils/PackedReadWrite.h>

#include <io.h>
#include <new>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

inline i32 CRezDir::IsGoodChar(char character) {
    if (m_pRezMgr->m_sDirSeparators) {
        return strchr(m_pRezMgr->m_sDirSeparators, character) == NULL;
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

static const i32 s_rezScanPathMax = 0x308;

CRezItm::CRezItm() {

    m_pRezFile = NULL;
    m_pParentDir = NULL;
    m_sName = NULL;
    m_heName.SetRezItm(this);
}

void CRezItm::InitRezItm(
    CRezDir* directory,
    const char* name,
    REZID resourceId,
    CRezTyp* type,
    REZDESC comment,
    REZSIZE size,
    u32 dataOffset,
    REZTIME time,
    u32 keyCount,
    REZKEYVAL* keys,
    CBaseRezFile* storage
) {
    static_cast<void>(resourceId);
    static_cast<void>(comment);
    static_cast<void>(keyCount);
    static_cast<void>(keys);
    m_pRezFile = storage;
    m_pParentDir = directory;
    directory->m_loaded = false;
    if (name == NULL) {
        m_sName = const_cast<char*>(name);
    } else {
        m_sName = new char[strlen(name) + 1];
        if (m_sName) {
            strcpy(m_sName, name);
        }
    }
    m_pType = type;
    m_nSize = size;
    m_nFilePos = dataOffset;
    m_nTime = time;
    m_pData = NULL;
    m_nCurPos = 0;
    m_heName.SetRezItm(this);
}

void CRezItm::TermRezItm() {
    if (m_pParentDir) m_pParentDir->m_loaded = false;
    if (m_sName) {
        delete[] m_sName;
    }
    delete[] m_pData;
    m_sName = NULL;
    m_pType = NULL;
    m_nTime = 0;
    m_nSize = 0;
    m_pData = NULL;
    m_pParentDir = NULL;
    m_nFilePos = 0;
    m_nCurPos = 0;
    m_heName.SetRezItm(this);
}

GZ_ENUM_RETURN(RezTypeTag, u32) CRezItm::GetType() {
    return static_cast<RezTypeTag>(m_pType->m_nType);
}

char* CRezItm::GetPath(char* destination, i32 size) {
    if (m_pParentDir->m_pParentDir == NULL) {
        strcpy(destination, "\\");
    } else {
        char* scratch = new char[size];
        strcpy(destination, "");
        CRezDir* directory = m_pParentDir;
        while (directory != NULL) {
            strcpy(scratch, destination);
            if (directory->m_pParentDir != NULL) {
                strcpy(destination, "\\");
            } else {
                destination[0] = 0;
            }
            strcat(destination, directory->m_sDirName);
            strcat(destination, scratch);
            directory = directory->m_pParentDir;
        }
        delete[] scratch;
    }
    return destination;
}

char* CRezItm::GetDir() {
    return m_pParentDir->m_sDirName;
}

u8* CRezItm::Load() {
    if (m_pData != NULL) {
        return m_pData;
    }
    if (m_nSize == 0) {
        return NULL;
    }
    m_pData = new u8[m_nSize];
    if (m_pData == NULL) {
        return NULL;
    }
    if (m_pRezFile->Read(m_nFilePos, 0, m_nSize, m_pData) != static_cast<i32>(m_nSize)) {
        delete[] m_pData;
        m_pData = NULL;
    }
    return m_pData;
}

i32 CRezItm::UnLoad() {
    if (m_pParentDir) m_pParentDir->m_loaded = false;
    SAFE_DELETE_ARRAY(m_pData);
    return 1;
}

i32 CRezItm::IsLoaded() { return m_pData != NULL; }

i32 CRezItm::Get(u8* destination) { return Get(destination, 0, m_nSize); }

i32 CRezItm::Get(u8* destination, u32 position, u32 byteCount) {
    if ((!destination && byteCount) || !io::containsRange(m_nSize, position, byteCount)) return 0;
    if (!byteCount) return 1;
    if (m_pData) {
        memcpy(destination, m_pData + position, byteCount);
        return 1;
    }
    return m_pRezFile->Read(m_nFilePos, position, byteCount, destination) == byteCount;
}

i32 CRezItm::Seek(u32 position) {
    if (position > m_nSize) return 0;
    m_nCurPos = position;
    return 1;
}

u32 CRezItm::Read(u8* destination, u32 byteCount, u32 seekPosition) {
    if (seekPosition != 0xffffffffU && !Seek(seekPosition)) return 0;
    if (m_nCurPos > m_nSize) return 0;
    if (byteCount > m_nSize - m_nCurPos) byteCount = m_nSize - m_nCurPos;
    if (!byteCount || !Get(destination, m_nCurPos, byteCount)) return 0;
    m_nCurPos += byteCount;
    return byteCount;
}

i32 CRezItm::EndOfRes() {
    return static_cast<u32>(m_nCurPos) >= m_nSize;
}

char CRezItm::GetChar() {
    char value = 0;
    Read(&value, 1, -1);
    return value;
}

CRezTyp::CRezTyp(
    REZTYPE typeTag,
    CRezDir* directory,
    u32 resourceIdBucketCount,
    u32 resourceNameBucketCount
)
    : m_haID(resourceIdBucketCount), m_haName(resourceNameBucketCount) {
    m_nType = typeTag;
    m_heType.SetRezTyp(this);
    m_pParentDir = directory;
}

CRezTyp::CRezTyp(REZTYPE typeTag, CRezDir* directory, u32 resourceNameBucketCount)
    : m_haID(), m_haName(resourceNameBucketCount) {
    m_nType = typeTag;
    m_heType.SetRezTyp(this);
    m_pParentDir = directory;
}

CRezTyp::~CRezTyp() {
    if (m_pParentDir->m_pRezMgr->m_bItemByIDUsed != false) {
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
            current->GetRezItm()->TermRezItm();
            m_pParentDir->m_pRezMgr->DeAllocateRezItm(current->GetRezItm());
        }
    }
    m_nType = REZ_TAG_NONE;
    m_heType.SetRezTyp(NULL);
}

CRezDir::CRezDir(
    CRezMgr* archive,
    CRezDir* parent,
    const char* name,
    u32 bodyOffset,
    u32 bodySize,
    REZTIME time,
    u32 subdirectoryBucketCount,
    u32 typeBucketCount
)
    : m_haDir(subdirectoryBucketCount), m_haTypes(typeBucketCount) {
    m_sDirName = new char[strlen(name) + 1];
    if (m_sDirName) {
        strcpy(m_sDirName, name);
    }
    m_nLastTimeModified = time;
    m_nDirSize = bodySize;
    m_nDirPos = bodyOffset;
    m_pRezMgr = archive;
    m_loaded = false;
    m_pParentDir = parent;
    m_heDir.SetRezDir(this);
}

CRezDir::~CRezDir() {

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
    if (m_sDirName) {
        delete[] m_sDirName;
    }
    m_sDirName = NULL;
    m_nLastTimeModified = 0;
    m_nDirSize = 0;
    m_nDirPos = 0;
    m_loaded = false;
    m_pRezMgr = NULL;
    m_pParentDir = NULL;
    m_heDir.SetRezDir(NULL);
}

CRezItm* CRezDir::GetRez(const char* name, RezTypeTag typeTag) {
    CRezTyp* type = m_haTypes.Find(IDX(typeTag));
    if (!type) {
        return NULL;
    }
    return type->m_haName.Find(name, m_pRezMgr->m_bLowerCaseUsed == false);
}

CRezItm* CRezDir::GetRezFromDosName(const char* filename) {
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
        typeTag = m_pRezMgr->StrToType(typeName);
    } else {
        typeTag = REZ_TAG_NONE;
    }
    return GetRez(resourceName, typeTag);
}

i32 CRezDir::Load(b32 recursive) {
    bool loaded = true;
    for (CRezTyp* type = GetFirstType(); type; type = GetNextType(type)) {
        for (CRezItm* item = GetFirstItem(type); item; item = GetNextItem(item)) {
            if (item->GetSize() && !item->Load()) loaded = false;
        }
    }
    if (recursive) {
        for (CRezDirHash* node = m_haDir.GetFirst(); node; node = node->Next()) {
            if (!node->GetRezDir()->Load(true)) loaded = false;
        }
    }
    m_loaded = loaded;
    return loaded;
}

i32 CRezDir::UnLoad(b32 recursive) {
    for (CRezTyp* type = GetFirstType(); type; type = GetNextType(type)) {
        for (CRezItm* item = GetFirstItem(type); item; item = GetNextItem(item)) item->UnLoad();
    }
    if (recursive) {
        for (CRezDirHash* node = m_haDir.GetFirst(); node; node = node->Next()) node->GetRezDir()->UnLoad(true);
    }
    m_loaded = false;
    return 1;
}

CRezDir* CRezDir::GetDir(const char* name) {
    if (!name) {
        return NULL;
    }
    return m_haDir.Find(name, m_pRezMgr->m_bLowerCaseUsed == false);
}

CRezDir* CRezDir::GetFirstSubDir() {
    CRezDirHash* node = m_haDir.GetFirst();
    if (!node) {
        return NULL;
    }
    return node->GetRezDir();
}

CRezDir* CRezDir::GetNextSubDir(CRezDir* directory) {
    CRezDirHash* node = directory->m_heDir.Next();
    if (!node) {
        return NULL;
    }
    return node->GetRezDir();
}

CRezTyp* CRezDir::GetRezTyp(REZTYPE typeTag) {
    return m_haTypes.Find(typeTag);
}

CRezTyp* CRezDir::GetFirstType() {
    CRezTypeHash* node = m_haTypes.GetFirst();
    if (!node) {
        return NULL;
    }
    return node->GetRezTyp();
}

CRezTyp* CRezDir::GetNextType(CRezTyp* type) {
    CRezTypeHash* node = type->m_heType.Next();
    if (!node) {
        return NULL;
    }
    return node->GetRezTyp();
}

CRezItm* CRezDir::GetFirstItem(CRezTyp* type) {
    CRezItmHashByName* node = type->m_haName.GetFirst();
    if (!node) {
        return NULL;
    }
    return node->GetRezItm();
}

CRezItm* CRezDir::GetNextItem(CRezItm* entry) {
    CRezItmHashByName* node = entry->m_heName.Next();
    if (!node) {
        return NULL;
    }
    return node->GetRezItm();
}

CRezDir* CRezDir::CreateDir(const char* name) {

    if (m_haDir.Find(name, m_pRezMgr->m_bLowerCaseUsed == false) != NULL) {
        return NULL;
    }
    CRezDir* child = new CRezDir(
        m_pRezMgr,
        this,
        name,
        0,
        0,
        m_pRezMgr->GetCurTime(),
        m_pRezMgr->m_nDirNumHashBins,
        m_pRezMgr->m_nTypNumHashBins
    );
    if (!child) {
        return NULL;
    }
    m_haDir.Insert(&child->m_heDir);

    u32 nameLength = strlen(name);
    if (m_pRezMgr->m_nLargestDirNameSize <= nameLength) {
        m_pRezMgr->m_nLargestDirNameSize = nameLength + 1;
    }
    return child;
}

CRezItm* CRezDir::CreateRez(REZID resourceId, const char* name, REZTYPE typeTag) {
    CRezTyp* type = GetOrMakeTyp(typeTag);
    if (type->m_haName.Find(name, m_pRezMgr->m_bLowerCaseUsed == false) != NULL) {
        return NULL;
    }
    CRezItm* entry = m_pRezMgr->AllocateRezItm();
    entry->InitRezItm(
        this,
        name,
        resourceId,
        type,
        NULL,
        0,
        0,
        m_pRezMgr->GetCurTime(),
        0,
        NULL,
        m_pRezMgr->m_pPrimaryRezFile
    );
    if (entry == NULL) {
        return NULL;
    }
    type->m_haName.Insert(&entry->m_heName);
    u32 nameLength = strlen(name);
    if (m_pRezMgr->m_nLargestRezNameSize <= nameLength) {
        m_pRezMgr->m_nLargestRezNameSize = nameLength + 1;
    }
    return entry;
}

CRezItm* CRezDir::CreateRezInternal(
    REZID resourceId,
    const char* name,
    CRezTyp* type,
    CBaseRezFile* storage
) {
    CRezItm* entry = m_pRezMgr->AllocateRezItm();
    if (entry == NULL) {
        return entry;
    }
    entry->InitRezItm(
        this,
        name,

        resourceId,
        type,
        NULL,
        0,
        0,
        m_pRezMgr->GetCurTime(),
        0,
        NULL,
        storage
    );
    type->m_haName.Insert(&entry->m_heName);
    u32 nameLength = strlen(name);
    if (m_pRezMgr->m_nLargestRezNameSize <= nameLength) {
        m_pRezMgr->m_nLargestRezNameSize = nameLength + 1;
    }
    return entry;
}

i32 CRezDir::RemoveRezInternal(CRezTyp* type, CRezItm* entry) {
    type->m_haName.Delete(&entry->m_heName);
    entry->TermRezItm();
    m_pRezMgr->DeAllocateRezItm(entry);
    m_pRezMgr->m_bIsSorted = false;
    return 1;
}

void CRezMgr::ImportArchive(CBaseRezFile* storage, const rez::Archive& archive, b32 replaceExisting) {
    std::vector<CRezDir*> directories(archive.directories.size());
    directories[0] = m_pRootDir;
    for (size_t index = 0; index < archive.directories.size(); ++index) {
        const rez::Directory& record = archive.directories[index];
        CRezDir* directory;
        if (index == 0) directory = m_pRootDir;
        else {
            CRezDir* parent = directories[record.parent];
            directory = parent->m_haDir.Find(record.name.c_str(), m_bLowerCaseUsed == false);
            if (!directory) {
                directory = new CRezDir(this, parent, record.name.c_str(), record.offset,
                    record.size, record.time, m_nDirNumHashBins, m_nTypNumHashBins);
                parent->m_haDir.Insert(&directory->m_heDir);
            } else {
                directory->m_nDirPos = record.offset;
                directory->m_nDirSize = record.size;
                directory->m_nLastTimeModified = record.time;
            }
        }
        directories[index] = directory;
        directory->m_loaded = false;
        for (size_t i = 0; i < record.resources.size(); ++i) {
            const rez::Resource& item = record.resources[i];
            CRezTyp* type = directory->GetOrMakeTyp(static_cast<REZTYPE>(item.type));
            CRezItm* found = type->m_haName.Find(item.name.c_str(), 1);
            if (found) {
                if (!replaceExisting) continue;
                directory->RemoveRezInternal(type, found);
            }
            CRezItm* entry = AllocateRezItm();
            entry->InitRezItm(directory, item.name.c_str(), item.id, type, NULL,
                item.size, item.offset, item.time, 0, NULL, storage);
            type->m_haName.Insert(&entry->m_heName);
        }
    }
}

CRezTyp* CRezDir::GetOrMakeTyp(REZTYPE typeTag) {

    CRezTyp* type = m_haTypes.Find(static_cast<u32>(typeTag));
    if (!type) {
        if (m_pRezMgr->m_bItemByIDUsed != false) {
            type = new CRezTyp(
                typeTag,
                this,
                m_pRezMgr->m_nByIDNumHashBins,
                m_pRezMgr->m_nByNameNumHashBins
            );
        } else {
            type = new CRezTyp(typeTag, this, m_pRezMgr->m_nByNameNumHashBins);
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

CRezMgr::CRezMgr() : m_hashRezItmFreeList(1) { Initialize(); }

void CRezMgr::Initialize() {
    m_bFileOpened = false;
    m_pPrimaryRezFile = NULL;
    m_nNumRezFiles = 0;
    m_nRootDirPos = 0;
    m_nRootDirSize = 0;
    m_nRootDirTime = 0;
    m_nNextWritePos = 0;
    m_pRootDir = NULL;
    m_nLastTimeModified = 0;
    m_bMustReWriteDirs = false;
    m_nFileFormatVersion = REZ_ARCHIVE_VERSION_NONE;
    m_nLargestKeyAry = 0;
    m_nLargestDirNameSize = 0;
    m_nLargestRezNameSize = 0;
    m_nLargestCommentSize = 0;
    m_sFileName = NULL;
    m_sDirSeparators = NULL;
    m_bLowerCaseUsed = false;
    m_bItemByIDUsed = false;
    m_nByNameNumHashBins = REZ_ARCHIVE_DEFAULT_RESOURCE_NAME_BUCKET_COUNT;
    m_nByIDNumHashBins = REZ_ARCHIVE_DEFAULT_RESOURCE_ID_BUCKET_COUNT;
    m_bRenumberIDCollisions = 1;
    m_nNextIDNumToUse = REZ_ARCHIVE_FIRST_GENERATED_RESOURCE_ID;
    m_bReadOnly = true;
    m_bIsSorted = true;
    m_nMaxOpenFilesInEmulatedDir = REZ_ARCHIVE_DEFAULT_MAX_OPEN_FILES;
    m_nDirNumHashBins = REZ_ARCHIVE_DEFAULT_SUBDIRECTORY_BUCKET_COUNT;
    m_nTypNumHashBins = REZ_ARCHIVE_DEFAULT_TYPE_BUCKET_COUNT;
    m_nRezItmChunkSize = REZ_ARCHIVE_DEFAULT_ENTRIES_PER_POOL_BLOCK;
}

CRezMgr::CRezMgr(const char* path, b32 readOnly, b32 createNew) : m_hashRezItmFreeList(1) {
    Initialize();
    Open(path, readOnly, createNew);
}

CRezMgr::~CRezMgr() {

    if (m_bFileOpened) {
        Close(0);
    }
    CBaseRezFile* storage;
    for (storage = m_lstRezFiles.GetFirst(); storage != NULL; storage = m_lstRezFiles.GetFirst()) {
        m_lstRezFiles.Delete(storage);
        m_nNumRezFiles--;
        delete storage;
    }
    CRezDir* rootDirectory = m_pRootDir;
    if (rootDirectory) {
        delete rootDirectory;
        m_pRootDir = NULL;
    }
    SAFE_DELETE_ARRAY(m_sFileName);
    SAFE_DELETE_ARRAY(m_sDirSeparators);
    CRezItmChunk* block = m_lstRezItmChunks.GetFirst();
    m_bFileOpened = false;
    m_pPrimaryRezFile = NULL;
    m_nRootDirPos = 0;
    m_nRootDirSize = 0;
    m_nRootDirTime = 0;
    m_nNextWritePos = 0;
    m_bReadOnly = true;
    m_pRootDir = NULL;
    m_nLastTimeModified = 0;
    m_bMustReWriteDirs = false;
    m_nFileFormatVersion = REZ_ARCHIVE_VERSION_1;
    m_nLargestKeyAry = 0;
    m_nLargestDirNameSize = 0;
    m_nLargestRezNameSize = 0;
    m_nLargestCommentSize = 0;
    m_bIsSorted = true;
    m_sFileName = NULL;
    if (block) {
        do {
            delete[] block->m_pRezItmAry;
            m_lstRezItmChunks.Delete(block);
            delete block;
            block = m_lstRezItmChunks.GetFirst();
        } while (block);
    }
}

i32 CRezMgr::Open(const char* path, b32 readOnly, b32 createNew) {
    if (!path || !*path || !readOnly || createNew) return 0;
    const std::string filename(path);
    if (IsDirectory(filename.c_str())) {
        if (m_bFileOpened) Close();
        CRezFileDirectoryEmulation* storage = new CRezFileDirectoryEmulation(this, m_nMaxOpenFilesInEmulatedDir);
        if (!storage->Open(filename.c_str(), true, false)) { delete storage; return 0; }
        m_pPrimaryRezFile = storage;
        m_lstRezFiles.Insert(storage); ++m_nNumRezFiles;
        m_bFileOpened = true; m_bReadOnly = true;
        m_sFileName = new char[filename.size() + 1]; strcpy(m_sFileName, filename.c_str());
        m_pRootDir = new CRezDir(this, NULL, "", 0, 0, GetCurTime(), m_nDirNumHashBins, m_nTypNumHashBins);
        if (!ReadEmulationDirectory(storage, m_pRootDir, m_sFileName, false)) { Close(); return 0; }
        return 1;
    }
    CRezFile* storage = new CRezFile(this);
    rez::Archive archive;
    if (!storage->Open(filename.c_str(), true, false)
        || rez::decode(storage->DataSource(), archive) != rez::Decoded) { delete storage; return 0; }
    // Malformed archives cannot mutate an existing archive or its resources.
    if (m_bFileOpened) Close();
    m_pPrimaryRezFile = storage;
    m_lstRezFiles.Insert(storage); ++m_nNumRezFiles;
    m_bFileOpened = true; m_bReadOnly = true;
    m_sFileName = new char[filename.size() + 1]; strcpy(m_sFileName, filename.c_str());
    const rez::Directory& root = archive.directories[0];
    m_nRootDirPos = root.offset; m_nRootDirSize = root.size; m_nRootDirTime = root.time;
    m_nNextWritePos = archive.nextWrite; m_nLastTimeModified = archive.time;
    m_nFileFormatVersion = REZ_ARCHIVE_VERSION_1; m_bIsSorted = archive.sorted;
    m_nLargestKeyAry = archive.largestKeys; m_nLargestDirNameSize = archive.largestDirectory;
    m_nLargestRezNameSize = archive.largestName; m_nLargestCommentSize = archive.largestComment;
    m_pRootDir = new CRezDir(this, NULL, "", root.offset, root.size, root.time,
        m_nDirNumHashBins, m_nTypNumHashBins);
    ImportArchive(storage, archive, false);
    return 1;
}

i32 CRezMgr::OpenAdditional(const char* path, b32 replaceExisting) {
    if (!path || !*path || !m_bReadOnly || !m_bFileOpened || !m_pRootDir) return 0;
    if (IsDirectory(path)) {
        CRezFileDirectoryEmulation* storage = new CRezFileDirectoryEmulation(this, m_nMaxOpenFilesInEmulatedDir);
        if (!storage->Open(path, true, false)) { delete storage; return 0; }
        m_lstRezFiles.Insert(storage); ++m_nNumRezFiles;
        m_bIsSorted = false;
        std::vector<char> directory(strlen(path) + 1);
        strcpy(&directory[0], path);
        return ReadEmulationDirectory(storage, m_pRootDir, &directory[0], replaceExisting);
    }
    CRezFile* storage = new CRezFile(this);
    rez::Archive archive;
    if (!storage->Open(path, true, false)
        || rez::decode(storage->DataSource(), archive) != rez::Decoded) { delete storage; return 0; }
    m_lstRezFiles.Insert(storage); ++m_nNumRezFiles;
    m_bIsSorted = false;
    ImportArchive(storage, archive, replaceExisting);
    if (archive.largestKeys > m_nLargestKeyAry) m_nLargestKeyAry = archive.largestKeys;
    if (archive.largestDirectory > m_nLargestDirNameSize) m_nLargestDirNameSize = archive.largestDirectory;
    if (archive.largestName > m_nLargestRezNameSize) m_nLargestRezNameSize = archive.largestName;
    if (archive.largestComment > m_nLargestCommentSize) m_nLargestCommentSize = archive.largestComment;
    return 1;
}

i32 CRezMgr::ReadEmulationDirectory(
    CRezFileDirectoryEmulation* storage,
    CRezDir* directory,
    char* path,
    b32 replaceExisting
) {
    char pattern[s_rezScanPathMax];
    strcpy(pattern, path);
    if (pattern[strlen(pattern) - 1] != '\\') {
        strcat(pattern, "\\");
    }
    char full[s_rezScanPathMax];
    strcpy(full, pattern);
    strcat(full, g_wildcard);
    _finddata_t fileData;
    i32 searchHandle = _findfirst(full, &fileData);
    if (searchHandle < 0) {
        return 1;
    }
    do {
        if (strcmp(fileData.name, ".") == 0 || strcmp(fileData.name, "..") == 0) {
            continue;
        }
        if ((fileData.attrib & _A_SUBDIR) == _A_SUBDIR) {

            char subdirectoryName[s_rezScanPathMax];
            strcpy(subdirectoryName, fileData.name);
            if (m_bLowerCaseUsed == false) {
                _strupr(subdirectoryName);
            }
            char childPath[s_rezScanPathMax];
            strcpy(childPath, pattern);
            strcat(childPath, subdirectoryName);
            strcat(childPath, "\\");
            CRezDir* child = directory->GetDir(subdirectoryName);
            if (child == NULL) {
                child = directory->CreateDir(subdirectoryName);
                if (child == NULL) {
                    continue;
                }
            }
            ReadEmulationDirectory(storage, child, childPath, replaceExisting);
            continue;
        }

        char filePath[s_rezScanPathMax];
        strcpy(filePath, pattern);
        strcat(filePath, fileData.name);
        char drive[_MAX_DRIVE];
        char directoryPath[_MAX_PATH];
        char splitName[_MAX_PATH];
        char resourceName[s_rezScanPathMax];
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
        i32 resourceId = (leadingDigitCount < nameLength) ? static_cast<i32>(m_nNextIDNumToUse++)
                                                          : atol(resourceName);
        RezTypeTag typeTag;
        char extensionName[8];
        char unpackedTag[8];
        if (strlen(extension) != 0) {
            strcpy(extensionName, extension + 1);
            _strupr(extensionName);
            typeTag = StrToType(extensionName);
        } else {
            typeTag = REZ_TAG_NONE;
        }
        TypeToStr(typeTag, unpackedTag);
        CRezTyp* type = directory->GetOrMakeTyp(IDX(typeTag));
        CRezItm* existing = directory->GetRez(resourceName, typeTag);
        CRezItm* entry;
        if (existing == NULL) {
            entry = directory
                        ->CreateRezInternal(static_cast<u32>(resourceId), resourceName, type, NULL);
        } else if (replaceExisting != false) {
            directory->RemoveRezInternal(type, existing);
            entry = directory
                        ->CreateRezInternal(static_cast<u32>(resourceId), resourceName, type, NULL);
        } else {
            entry = NULL;
        }
        if (entry != NULL) {
            entry->m_nTime = static_cast<i32>(fileData.time_write);
            entry->m_nSize = static_cast<u32>(fileData.size);
            entry->m_pRezFile = new CRezFileSingleFile(this, filePath, storage);
        }
    } while (_findnext(searchHandle, &fileData) == 0);
    _findclose(searchHandle);
    return 1;
}

i32 CRezMgr::Close(b32 unusedFinal) {
    static_cast<void>(unusedFinal);
    SAFE_DELETE(m_pRootDir);
    bool success = true;
    for (CBaseRezFile* storage = m_lstRezFiles.GetFirst(); storage; storage = m_lstRezFiles.GetFirst()) {
        if (!storage->Close()) success = false;
        m_lstRezFiles.Delete(storage);
        delete storage;
    }
    m_nNumRezFiles = 0; m_pPrimaryRezFile = NULL;
    SAFE_DELETE_ARRAY(m_sFileName);
    m_bFileOpened = false;
    return success;
}

CRezDir* CRezMgr::GetRootDir() {
    return m_pRootDir;
}

RezTypeTag CRezMgr::StrToType(const char* typeName) {
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

void CRezMgr::TypeToStr(RezTypeTag tag, char* destination) {
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

void* CRezMgr::Alloc(u32 numBytes) {
    static_cast<void>(numBytes);
    return NULL;
}

void CRezMgr::Free(void* ptr) {
    static_cast<void>(ptr);
}

i32 CRezMgr::DiskError() {
    return 0;
}

i32 CRezMgr::VerifyFileOpen() {
    b32 allStoragesValid = true;
    for (CBaseRezFile* storage = m_lstRezFiles.GetFirst(); storage != NULL;
         storage = storage->Next()) {
        if (storage->VerifyFileOpen() == 0) {
            allStoragesValid = false;
        }
    }
    return allStoragesValid;
}

void CRezMgr::SetHashTableBins(
    u32 resourceNameBuckets,
    u32 resourceIdBuckets,
    u32 subdirectoryBuckets,
    u32 typeBuckets
) {
    m_nByNameNumHashBins = resourceNameBuckets;
    m_nByIDNumHashBins = resourceIdBuckets;
    m_nDirNumHashBins = subdirectoryBuckets;
    m_nTypNumHashBins = typeBuckets;
}

REZTIME CRezMgr::GetCurTime() {
    time_t timestamp;
    return static_cast<REZTIME>(time(&timestamp));
}

void CRezMgr::SetDirSeparators(const char* delimiters) {
    if (m_sDirSeparators != NULL) {
        delete[] m_sDirSeparators;
    }
    m_sDirSeparators = new char[strlen(delimiters) + 1];
    strcpy(m_sDirSeparators, delimiters);
}

CRezDir* CRezDir::GetDirFromPath(const char* path) {
    char component[0x40];
    if (static_cast<i32>(strlen(path)) > 1) {
        if (!IsGoodChar(*path)) {
            ++path;
        }
    }
    const char* cursor = path;
    i32 componentLength = 0;
    while (IsGoodChar(*cursor)) {
        component[componentLength] = *cursor;
        ++componentLength;
        ++cursor;
    }
    component[componentLength] = 0;
    CRezDir* subdirectory = GetDir(component);
    if (!subdirectory) {
        return subdirectory;
    }
    char separator = path[componentLength];
    if (separator == 0) {
        return subdirectory;
    }
    while (!IsGoodChar(separator)) {
        separator = path[componentLength + 1];
        ++componentLength;
        if (separator == 0) {
            return subdirectory;
        }
    }
    return subdirectory->GetDirFromPath(path + componentLength);
}

CRezItm* CRezDir::GetRezFromDosPath(const char* qualifiedPath) {
    char directoryPath[0x100];
    char resourceName[0x20];
    i32 pathLength = static_cast<i32>(strlen(qualifiedPath));
    if (pathLength > 1) {
        if (!IsGoodChar(*qualifiedPath)) {
            ++qualifiedPath;
            --pathLength;
        }
    }
    i32 separatorIndex = pathLength - 1;
    while (IsGoodChar(qualifiedPath[separatorIndex])) {
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
        return GetRezFromDosName(resourceName);
    }
    strncpy(directoryPath, qualifiedPath, static_cast<u32>(separatorIndex));
    directoryPath[separatorIndex] = 0;
    CRezDir* directory = GetDirFromPath(directoryPath);
    if (!directory) {
        return NULL;
    }
    return directory->GetRezFromDosName(resourceName);
}

CRezItm* CRezDir::GetRezFromPath(const char* qualifiedPath, RezTypeTag typeTag) {
    char directoryPath[0x100];
    char resourceName[0x20];
    i32 pathLength = static_cast<i32>(strlen(qualifiedPath));
    if (pathLength > 1) {
        if (!IsGoodChar(*qualifiedPath)) {
            ++qualifiedPath;
            --pathLength;
        }
    }
    i32 separatorIndex = pathLength - 1;
    while (IsGoodChar(qualifiedPath[separatorIndex])) {
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
        return GetRez(resourceName, typeTag);
    }
    strncpy(directoryPath, qualifiedPath, static_cast<u32>(separatorIndex));
    directoryPath[separatorIndex] = 0;
    CRezDir* directory = GetDirFromPath(directoryPath);
    if (!directory) {
        return NULL;
    }
    return directory->GetRez(resourceName, typeTag);
}

CRezItm* CRezMgr::GetRezFromPath(const char* path, RezTypeTag typeTag) {
    return GetRootDir()->GetRezFromPath(path, typeTag);
}

CRezItm* CRezMgr::GetRezFromDosPath(const char* path) {
    return GetRootDir()->GetRezFromDosPath(path);
}

CRezDir* CRezMgr::GetDirFromPath(const char* path) {
    return GetRootDir()->GetDirFromPath(path);
}

i32 CRezMgr::Reset() {
    if (!IsOpen()) {
        return 0;
    }
    const std::string filename(m_sFileName);
    return Open(filename.c_str());
}

i32 CRezMgr::IsDirectory(const char* path) {
    struct _stat fileInfo;
    if (_stat(path, &fileInfo) != 0) {
        return 0;
    }
    return (fileInfo.st_mode & _S_IFDIR) == _S_IFDIR;
}

CRezItm* CRezMgr::AllocateRezItm() {
    CRezItm* entry = NULL;
    CRezItmHashByName* node;
    node = m_hashRezItmFreeList.GetFirst();
    if (node != NULL) {
        entry = node->GetRezItm();
    }
    if (entry == NULL) {
        CRezItmChunk* block;
        block = new CRezItmChunk;
        if (block == NULL) {
            return NULL;
        }
        block->m_pRezItmAry = new CRezItm[m_nRezItmChunkSize];
        if (block->m_pRezItmAry == NULL) {
            delete block;
            return NULL;
        }
        for (u32 index = 0; index < static_cast<u32>(m_nRezItmChunkSize); index++) {
            block->m_pRezItmAry[index].m_heName.SetRezItm(&block->m_pRezItmAry[index]);
            m_hashRezItmFreeList.Insert(&block->m_pRezItmAry[index].m_heName);
        }
        m_lstRezItmChunks.InsertFirst(block);
        entry = m_hashRezItmFreeList.GetFirst()->GetRezItm();
    }
    if (entry) {
        m_hashRezItmFreeList.Delete(&entry->m_heName);
    }
    return entry;
}

void CRezMgr::DeAllocateRezItm(CRezItm* entry) {
    if (entry) {
        m_hashRezItmFreeList.Insert(&entry->m_heName);
    }
}

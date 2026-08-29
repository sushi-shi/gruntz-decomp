#include <rva.h>

#include <Rez/RezArchive.h>

#include <Mfc.h>

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

static const i32 REZ_SCAN_PATH_MAX = 0x308;

// Byte-forced view of packed serialized storage.
static inline u32 ReadPackedDWORD(const u8* bytes) {
    u32 value;
    memcpy(&value, bytes, sizeof(value));
    return value;
}

RVA(0x001396f0, 0x1a)
CRezItm::CRezItm() {

    m_pRezFile = NULL;
    m_pParentDir = NULL;
    m_sName = NULL;
    m_heName.SetRezItm(this);
}

RVA(0x00139710, 0x8d)
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

RVA(0x001397a0, 0x57)
void CRezItm::TermRezItm() {
    if (m_sName) {
        delete[] m_sName;
    }
    if (m_pParentDir != NULL) {
        if (m_pParentDir->m_pMemBlock == NULL) {
            if (m_pData) {
                delete[] m_pData;
            }
        }
    } else {
        if (m_pData) {
            delete[] m_pData;
        }
    }
    m_sName = NULL;
    m_pType = NULL;
    m_nTime = 0;
    m_nSize = 0;
    m_pData = NULL;
    m_pParentDir = NULL;
    m_nFilePos = 0;
    m_nCurPos = 0;
    m_heName.SetRezItm(NULL);
}

RVA(0x00139800, 0x6)
GZ_ENUM_RETURN(RezTypeTag, u32) CRezItm::GetType() {
    return static_cast<RezTypeTag>(m_pType->m_nType);
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00139810, 0x140)
char* CRezItm::GetPath(char* destination, i32 size) {
    if (m_pParentDir->m_pParentDir == NULL) {
        strcpy(destination, DATA_COMPGEN(0x0020cff0, "\\"));
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

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00139950, 0x6)
char* CRezItm::GetDir() {
    return m_pParentDir->m_sDirName;
}

RVA(0x00139960, 0x6b)
u8* CRezItm::Load() {
    if (m_pParentDir->m_pMemBlock != NULL) {
        return m_pParentDir->m_pMemBlock + (m_nFilePos - m_pParentDir->m_nItemsPos);
    }
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

RVA(0x001399d0, 0x21)
i32 CRezItm::UnLoad() {
    if (m_pData != NULL) {
        delete[] m_pData;
        m_pData = NULL;
    }
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00139a00, 0x1b)
i32 CRezItm::IsLoaded() {
    if (m_pParentDir->m_pMemBlock != NULL) {
        return 1;
    }
    return m_pData != NULL;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00139a20, 0x13)
i32 CRezItm::Get(u8* destination) {
    i32 result = Get(destination, 0, m_nSize);
    return result;
}

RVA(0x00139a40, 0x95)
i32 CRezItm::Get(u8* destination, u32 position, u32 byteCount) {
    CRezDir* directory = m_pParentDir;
    if (directory->m_pMemBlock != NULL) {
        memcpy(
            destination,
            directory->m_pMemBlock + (m_nFilePos - directory->m_nItemsPos + position),
            byteCount
        );
        return 1;
    }
    if (m_pData != NULL) {
        memcpy(destination, (m_pData + position), byteCount);
        return 1;
    }
    return m_pRezFile->Read(m_nFilePos, position, byteCount, destination)
           == static_cast<i32>(byteCount);
}

RVA(0x00139ae0, 0xf)
i32 CRezItm::Seek(u32 position) {
    m_nCurPos = position;
    return 1;
}

RVA(0x00139af0, 0xcc)
u32 CRezItm::Read(u8* destination, u32 byteCount, u32 seekPosition) {
    if (seekPosition != 0xffffffffu) {
        Seek(seekPosition);
    }

    if (byteCount + m_nCurPos > m_nSize) {
        byteCount = m_nSize - m_nCurPos;
    }
    if (byteCount <= 0) {
        return 0;
    }
    if (m_pParentDir->m_pMemBlock != NULL) {
        memcpy(
            destination,
            m_pParentDir->m_pMemBlock + m_nFilePos + m_nCurPos - m_pParentDir->m_nItemsPos,
            byteCount
        );
        m_nCurPos += byteCount;
        return byteCount;
    }
    if (m_pData != NULL) {
        memcpy(destination, m_pData + m_nCurPos, byteCount);
        m_nCurPos += byteCount;
        return byteCount;
    }
    if (m_pRezFile->Read(m_nFilePos, m_nCurPos, byteCount, destination)
        == static_cast<i32>(byteCount)) {
        m_nCurPos += byteCount;
        return byteCount;
    }
    return 0;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00139bc0, 0xc)
i32 CRezItm::EndOfRes() {
    return static_cast<u32>(m_nCurPos) >= m_nSize;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00139bd0, 0x15)
char CRezItm::GetChar() {
    char value;
    Read(&value, 1, -1);
    return value;
}

RVA(0x00139bf0, 0x71)
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

RVA_COMPGEN(0x00139c70, 0x5, ??1CRezItmHashTableByID@@QAE@XZ)

RVA(0x00139c80, 0x6c)
CRezTyp::CRezTyp(REZTYPE typeTag, CRezDir* directory, u32 resourceNameBucketCount)
    : m_haID(), m_haName(resourceNameBucketCount) {
    m_nType = typeTag;
    m_heType.SetRezTyp(this);
    m_pParentDir = directory;
}

RVA(0x00139cf0, 0xd7)
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

RVA_COMPGEN(0x00139dd0, 0x5, ??1CRezItmHashTableByName@@QAE@XZ)

RVA(0x00139de0, 0xd4)
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
    m_nItemsSize = 0;
    m_nItemsPos = 0;
    m_pMemBlock = NULL;
    m_pParentDir = parent;
    m_heDir.SetRezDir(this);
}

RVA_COMPGEN(0x00139ec0, 0x5, ??1CRezDirHashTable@@QAE@XZ)
RVA_COMPGEN(0x00139ed0, 0x5, ??1CRezTypeHashTable@@QAE@XZ)

RVA(0x00139ee0, 0x11e)
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
    if (m_pMemBlock) {
        delete[] m_pMemBlock;
    }
    m_sDirName = NULL;
    m_nLastTimeModified = 0;
    m_nDirSize = 0;
    m_nDirPos = 0;
    m_nItemsSize = 0;
    m_nItemsPos = 0;
    m_pMemBlock = NULL;
    m_pRezMgr = NULL;
    m_pParentDir = NULL;
    m_heDir.SetRezDir(NULL);
}

RVA(0x0013a000, 0x37)
CRezItm* CRezDir::GetRez(const char* name, RezTypeTag typeTag) {
    CRezTyp* type = m_haTypes.Find(IDX(typeTag));
    if (!type) {
        return NULL;
    }
    return type->m_haName.Find(name, m_pRezMgr->m_bLowerCaseUsed == false);
}

RVA(0x0013a040, 0xa2)
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

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0013a0f0, 0x99)
i32 CRezDir::Load(b32 recursive) {
    if (m_pMemBlock != NULL) {
        return 1;
    }

    CRezMgr* archive = m_pRezMgr;
    if (archive->m_bIsSorted == false || archive->m_nNumRezFiles > 1) {
        dprintf("CRezDir::Load Failed! (File is not sorted!)\n");
        return 0;
    }

    if (m_nItemsSize > 0) {
        m_pMemBlock = new u8[m_nItemsSize];
        if (m_pMemBlock != NULL) {
            m_pRezMgr->m_pPrimaryRezFile->Read(m_nItemsPos, 0, m_nItemsSize, m_pMemBlock);
        }
    }

    if (recursive != false) {
        for (CRezDirHash* node = m_haDir.GetFirst(); node != NULL; node = node->Next()) {

            node->GetRezDir()->Load(true);
        }
    }
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0013a190, 0x94)
i32 CRezDir::UnLoad(b32 recursive) {
    if (m_pMemBlock != NULL) {
        delete[] m_pMemBlock;
        m_pMemBlock = NULL;
    } else {
        CRezTyp* type = GetFirstType();
        while (type) {
            CRezItm* entry = GetFirstItem(type);
            while (entry) {
                entry->UnLoad();
                entry = GetNextItem(entry);
            }
            type = GetNextType(type);
        }
    }
    if (recursive) {
        CRezDirHash* node = m_haDir.GetFirst();
        while (node) {
            node->GetRezDir()->UnLoad(true);
            node = node->Next();
        }
    }
    return 1;
}

RVA(0x0013a230, 0x29)
CRezDir* CRezDir::GetDir(const char* name) {
    if (!name) {
        return NULL;
    }
    return m_haDir.Find(name, m_pRezMgr->m_bLowerCaseUsed == false);
}

RVA(0x0013a260, 0x11)
CRezDir* CRezDir::GetFirstSubDir() {
    CRezDirHash* node = m_haDir.GetFirst();
    if (!node) {
        return NULL;
    }
    return node->GetRezDir();
}

RVA(0x0013a280, 0x19)
CRezDir* CRezDir::GetNextSubDir(CRezDir* directory) {
    CRezDirHash* node = directory->m_heDir.Next();
    if (!node) {
        return NULL;
    }
    return node->GetRezDir();
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0013a2a0, 0x10)
CRezTyp* CRezDir::GetRezTyp(REZTYPE typeTag) {
    return m_haTypes.Find(typeTag);
}

RVA(0x0013a2b0, 0x11)
CRezTyp* CRezDir::GetFirstType() {
    CRezTypeHash* node = m_haTypes.GetFirst();
    if (!node) {
        return NULL;
    }
    return node->GetRezTyp();
}

RVA(0x0013a2d0, 0x19)
CRezTyp* CRezDir::GetNextType(CRezTyp* type) {
    CRezTypeHash* node = type->m_heType.Next();
    if (!node) {
        return NULL;
    }
    return node->GetRezTyp();
}

RVA(0x0013a2f0, 0x19)
CRezItm* CRezDir::GetFirstItem(CRezTyp* type) {
    CRezItmHashByName* node = type->m_haName.GetFirst();
    if (!node) {
        return NULL;
    }
    return node->GetRezItm();
}

RVA(0x0013a310, 0x19)
CRezItm* CRezDir::GetNextItem(CRezItm* entry) {
    CRezItmHashByName* node = entry->m_heName.Next();
    if (!node) {
        return NULL;
    }
    return node->GetRezItm();
}

RVA(0x0013a330, 0xce)
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

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0013a400, 0xa9)
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

RVA(0x0013a4b0, 0x75)
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

RVA(0x0013a530, 0x47)
i32 CRezDir::RemoveRezInternal(CRezTyp* type, CRezItm* entry) {
    m_nItemsSize -= entry->m_nSize;
    type->m_haName.Delete(&entry->m_heName);
    entry->TermRezItm();
    m_pRezMgr->DeAllocateRezItm(entry);
    m_pRezMgr->m_bIsSorted = false;
    return 1;
}

RVA(0x0013a580, 0xb2)
i32 CRezDir::ReadAllDirs(CBaseRezFile* storage, u32 bodyOffset, u32 bodySize, b32 replaceExisting) {
    b32 success = true;
    if (static_cast<u32>(bodySize) <= 0) {
        return success;
    }
    CRezDirHash* node = m_haDir.GetFirst();
    while (node) {
        node->GetRezDir()->m_nDirPos = 0;
        node = node->Next();
    }
    if (ReadDirBlock(storage, bodyOffset, bodySize, replaceExisting) != 0) {
        node = m_haDir.GetFirst();
        while (node) {
            CRezDir* subdirectory = node->GetRezDir();
            if (subdirectory->m_nDirPos != 0) {
                if (subdirectory->ReadAllDirs(
                        storage,
                        subdirectory->m_nDirPos,
                        subdirectory->m_nDirSize,
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
i32 CRezDir::ReadDirBlock(
    CBaseRezFile* storage,
    u32 bodyOffset,
    u32 bodySize,
    b32 replaceExisting
) {
    m_nItemsSize = 0;
    m_nItemsPos = 0xffffffffu;
    u32 maximumDataOffset = 0;
    u8* body = new u8[bodySize];
    if (!body) {
        return 0;
    }
    if (storage->Read(bodyOffset, 0, bodySize, body) != bodySize) {
        delete[] body;
        return 0;
    }
    u8* cursor = body;
    u8* end = body + bodySize;
    while (cursor < end) {
        RezDirectoryRecordKind recordKind =
            static_cast<RezDirectoryRecordKind>(ReadPackedDWORD(cursor));
        if (recordKind == REZ_DIRECTORY_RECORD_SUBDIRECTORY) {
            cursor += sizeof(u32);
            u32 childBodyOffset = ReadPackedDWORD(cursor);
            cursor += sizeof(u32);
            u32 childBodySize = ReadPackedDWORD(cursor);
            cursor += sizeof(u32);
            REZTIME childTime = ReadPackedDWORD(cursor);
            cursor += sizeof(u32);
            char* name = static_cast<char*>(static_cast<void*>(cursor));
            cursor += strlen(name) + 1;
            CRezDirHashTable* subdirectories = &m_haDir;
            CRezDir* existing = subdirectories->Find(name, m_pRezMgr->m_bLowerCaseUsed == false);
            if (existing == NULL) {
                CRezDir* directory = new CRezDir(
                    m_pRezMgr,
                    this,
                    name,
                    childBodyOffset,
                    childBodySize,
                    childTime,
                    m_pRezMgr->m_nDirNumHashBins,
                    m_pRezMgr->m_nTypNumHashBins
                );
                subdirectories->Insert(&directory->m_heDir);
            } else {
                existing->m_nDirPos = childBodyOffset;
                existing->m_nDirSize = childBodySize;
                existing->m_nLastTimeModified = childTime;
            }
        } else {

            cursor += sizeof(u32);
            u32 dataOffset = ReadPackedDWORD(cursor);
            cursor += sizeof(u32);
            REZSIZE size = ReadPackedDWORD(cursor);
            cursor += sizeof(u32);
            REZTIME time = ReadPackedDWORD(cursor);
            cursor += sizeof(u32);
            REZID resourceId = ReadPackedDWORD(cursor);
            cursor += sizeof(u32);
            REZTYPE typeTag = static_cast<REZTYPE>(ReadPackedDWORD(cursor));
            cursor += sizeof(u32);
            u32 keyCount = ReadPackedDWORD(cursor);
            cursor += sizeof(u32);
            char* name = static_cast<char*>(static_cast<void*>(cursor));
            cursor += strlen(name) + 1;
            CRezTyp* type = GetOrMakeTyp(typeTag);
            i32 skipEntry = 0;
            CRezItm* found = type->m_haName.Find(name, 1);
            if (found) {
                if (replaceExisting != false) {
                    RemoveRezInternal(type, found);
                } else {
                    skipEntry = 1;
                }
            }
            char* comment = static_cast<char*>(static_cast<void*>(cursor));
            cursor += strlen(comment) + 1;
            if (*comment == 0) {
                comment = NULL;
            }
            REZKEYVAL* keys;
            if (keyCount > 0) {
                keys = new REZKEYVAL[keyCount];
                for (REZKEYINDEX keyIndex = 0; keyIndex < keyCount; keyIndex++) {
                    keys[keyIndex] = ReadPackedDWORD(cursor);
                    cursor += sizeof(u32);
                }
            } else {
                keys = NULL;
            }
            if (!skipEntry) {
                CRezItm* entry = m_pRezMgr->AllocateRezItm();
                entry->InitRezItm(
                    this,
                    name,
                    resourceId,
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
                m_nItemsSize = m_nItemsSize + entry->m_nSize;
                if (entry->m_nFilePos < m_nItemsPos) {
                    m_nItemsPos = entry->m_nFilePos;
                }
                if (entry->m_nFilePos > maximumDataOffset) {
                    maximumDataOffset = entry->m_nFilePos;
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

RVA(0x0013aa10, 0xdc)
CRezMgr::CRezMgr() : m_hashRezItmFreeList(1) {
    m_bFileOpened = false;
    m_pPrimaryRezFile = NULL;
    m_nNumRezFiles = 0;
    m_nRootDirPos = 0;
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

RVA_COMPGEN(0x0013aaf0, 0x7, ??1CBaseRezFileList@@QAE@XZ)

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0013ab00, 0xac)
CRezMgr::CRezMgr(const char* path, b32 readOnly, b32 createNew) : m_hashRezItmFreeList(1) {
    {
        CRezMgr defaults;
    }
    Open(path, readOnly, createNew);
}

RVA(0x0013abc0, 0x13f)
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
    if (m_sFileName) {
        delete[] m_sFileName;
        m_sFileName = NULL;
    }
    if (m_sDirSeparators) {
        delete[] m_sDirSeparators;
        m_sDirSeparators = NULL;
    }
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

RVA(0x0013ad00, 0x3b8)
i32 CRezMgr::Open(const char* path, b32 readOnly, b32 createNew) {
    m_bReadOnly = readOnly;
    if (readOnly == false) {
        return 0;
    }
    if (m_sFileName) {
        delete[] m_sFileName;
    }
    m_sFileName = new char[strlen(path) + 1];
    strcpy(m_sFileName, path);
    if (IsDirectory(path) != 0) {

        if (m_bReadOnly == false) {
            return 0;
        }
        CRezFileDirectoryEmulation* storage =
            new CRezFileDirectoryEmulation(this, m_nMaxOpenFilesInEmulatedDir);
        if (storage == NULL) {
            delete[] m_sFileName;
            m_sFileName = NULL;
            return 0;
        }
        m_pPrimaryRezFile = storage;
        m_lstRezFiles.Insert(storage);
        m_nNumRezFiles++;
        if (storage->Open(path, readOnly, createNew) == 0) {
            return 0;
        }
        m_bFileOpened = true;
        m_pRootDir = new CRezDir(
            this,
            NULL,
            "",
            0,
            0,
            this->GetCurTime(),
            m_nDirNumHashBins,
            m_nTypNumHashBins
        );
        ReadEmulationDirectory(storage, m_pRootDir, m_sFileName, false);
        return 1;
    }

    CRezFile* storage = new CRezFile(this);
    if (storage == NULL) {
        delete[] m_sFileName;
        m_sFileName = NULL;
        return 0;
    }
    m_pPrimaryRezFile = storage;
    m_lstRezFiles.Insert(storage);
    m_nNumRezFiles++;
    if (storage->Open(path, readOnly, createNew) == 0) {
        return 0;
    }
    m_bFileOpened = true;
    if (createNew != false) {
        m_nNextWritePos = sizeof(FileMainHeaderStruct);
        m_bMustReWriteDirs = true;
        m_pRootDir = new CRezDir(
            this,
            NULL,
            "",
            0,
            0,
            this->GetCurTime(),
            m_nDirNumHashBins,
            m_nTypNumHashBins
        );
        return 1;
    }

    FileMainHeaderStruct header;
    storage->Read(0, 0, sizeof(header), &header);
    m_nNextWritePos = header.NextWritePos;
    m_nRootDirPos = header.RootDirPos;
    m_nRootDirSize = header.RootDirSize;
    m_nRootDirTime = header.RootDirTime;
    m_nLastTimeModified = header.Time;
    m_nFileFormatVersion = header.FileFormatVersion;
    m_nLargestKeyAry = header.LargestKeyAry;
    m_nLargestDirNameSize = header.LargestDirNameSize;
    m_nLargestRezNameSize = header.LargestRezNameSize;
    m_nLargestCommentSize = header.LargestCommentSize;
    m_bIsSorted = header.IsSorted;
    if (header.CR1 != REZ_ARCHIVE_MAGIC_CR) {
        return 0;
    }
    if (header.LF2 != REZ_ARCHIVE_MAGIC_LF) {
        return 0;
    }
    if (header.EOF1 != REZ_ARCHIVE_MAGIC_EOF) {
        return 0;
    }
    if (header.FileFormatVersion != REZ_ARCHIVE_VERSION_1) {
        return 0;
    }
    m_pRootDir = new CRezDir(
        this,
        NULL,
        "",
        m_nRootDirPos,
        m_nRootDirSize,
        m_nRootDirTime,
        m_nDirNumHashBins,
        m_nTypNumHashBins
    );
    m_pRootDir->ReadAllDirs(storage, m_nRootDirPos, m_nRootDirSize, false);
    return 1;
}

RVA(0x0013b0c0, 0x238)
i32 CRezMgr::OpenAdditional(const char* path, b32 replaceExisting) {
    b32 readOnly = true;
    b32 createNew = false;
    if (m_bReadOnly == false) {
        return 0;
    }
    m_bIsSorted = false;
    if (m_sFileName) {
        delete[] m_sFileName;
    }
    m_sFileName = new char[strlen(path) + 1];
    strcpy(m_sFileName, path);

    if (IsDirectory(path)) {
        CRezFileDirectoryEmulation* storage =
            new CRezFileDirectoryEmulation(this, m_nMaxOpenFilesInEmulatedDir);
        if (storage == NULL) {
            delete[] m_sFileName;
            m_sFileName = NULL;
            return 0;
        }
        m_lstRezFiles.Insert(storage);
        m_nNumRezFiles++;
        if (storage->Open(path, readOnly, createNew) == 0) {
            return 0;
        }
        m_bFileOpened = true;
        ReadEmulationDirectory(storage, m_pRootDir, m_sFileName, replaceExisting);
        return 1;
    }

    CRezFile* storage = new CRezFile(this);
    if (storage == NULL) {
        delete[] m_sFileName;
        m_sFileName = NULL;
        return 0;
    }
    m_lstRezFiles.Insert(storage);
    m_nNumRezFiles++;
    if (storage->Open(path, readOnly, createNew) == 0) {
        return 0;
    }

    FileMainHeaderStruct header;
    storage->Read(0, 0, sizeof(header), &header);
    if (header.LargestKeyAry > m_nLargestKeyAry) {
        m_nLargestKeyAry = header.LargestKeyAry;
    }
    if (header.LargestDirNameSize > m_nLargestDirNameSize) {
        m_nLargestDirNameSize = header.LargestDirNameSize;
    }
    if (header.LargestRezNameSize > m_nLargestRezNameSize) {
        m_nLargestRezNameSize = header.LargestRezNameSize;
    }
    if (header.LargestCommentSize > m_nLargestCommentSize) {
        m_nLargestCommentSize = header.LargestCommentSize;
    }
    m_pRootDir->ReadAllDirs(storage, header.RootDirPos, header.RootDirSize, replaceExisting);
    return 1;
}

RVA(0x0013b300, 0x545)
i32 CRezMgr::ReadEmulationDirectory(
    CRezFileDirectoryEmulation* storage,
    CRezDir* directory,
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
            if (m_bLowerCaseUsed == false) {
                _strupr(subdirectoryName);
            }
            char childPath[REZ_SCAN_PATH_MAX];
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

RVA(0x0013b850, 0xa8)
i32 CRezMgr::Close(b32 unusedFinal) {
    static_cast<void>(unusedFinal);
    i32 result = m_pPrimaryRezFile->Close();
    m_lstRezFiles.Delete(m_pPrimaryRezFile);
    m_nNumRezFiles--;
    delete m_pPrimaryRezFile;
    m_pPrimaryRezFile = NULL;
    CBaseRezFile* storage;
    for (storage = m_lstRezFiles.GetFirst(); storage != NULL; storage = m_lstRezFiles.GetFirst()) {
        storage->Close();
        m_lstRezFiles.Delete(storage);
        m_nNumRezFiles--;
        delete storage;
    }
    if (m_pRootDir) {
        delete m_pRootDir;
        m_pRootDir = NULL;
    }
    if (m_sFileName) {
        delete[] m_sFileName;
        m_sFileName = NULL;
    }
    m_bFileOpened = false;
    return result;
}

RVA(0x0013b900, 0x4)
CRezDir* CRezMgr::GetRootDir() {
    return m_pRootDir;
}

RVA(0x0013b910, 0x58)
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

RVA(0x0013b970, 0x72)
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

RVA(0x0013b9f0, 0x5)
void* CRezMgr::Alloc(u32 numBytes) {
    static_cast<void>(numBytes);
    return NULL;
}

RVA(0x0013ba00, 0x3)
void CRezMgr::Free(void* ptr) {
    static_cast<void>(ptr);
}

RVA(0x0013ba10, 0x3)
i32 CRezMgr::DiskError() {
    return 0;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0013ba20, 0x27)
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

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0013ba50, 0x1f)
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

RVA(0x0013ba70, 0x10)
REZTIME CRezMgr::GetCurTime() {
    time_t timestamp;
    return static_cast<REZTIME>(time(&timestamp));
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0013ba80, 0x57)
void CRezMgr::SetDirSeparators(const char* delimiters) {
    if (m_sDirSeparators != NULL) {
        delete[] m_sDirSeparators;
    }
    m_sDirSeparators = new char[strlen(delimiters) + 1];
    strcpy(m_sDirSeparators, delimiters);
}

RVA(0x0013bae0, 0x1b9)
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

RVA(0x0013bca0, 0x19c)
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

RVA(0x0013be40, 0x1ac)
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

RVA(0x0013bff0, 0x19)
CRezItm* CRezMgr::GetRezFromPath(const char* path, RezTypeTag typeTag) {
    return GetRootDir()->GetRezFromPath(path, typeTag);
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0013c010, 0x14)
CRezItm* CRezMgr::GetRezFromDosPath(const char* path) {
    return GetRootDir()->GetRezFromDosPath(path);
}

RVA(0x0013c030, 0x14)
CRezDir* CRezMgr::GetDirFromPath(const char* path) {
    return GetRootDir()->GetDirFromPath(path);
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0013c050, 0x28)
i32 CRezMgr::Reset() {
    if (!IsOpen()) {
        return 0;
    }
    Close();
    return Open(m_sFileName);
}

RVA(0x0013c080, 0x3c)
i32 CRezMgr::IsDirectory(const char* path) {
    struct _stat fileInfo;
    if (_stat(path, &fileInfo) != 0) {
        return 0;
    }
    return (fileInfo.st_mode & _S_IFDIR) == _S_IFDIR;
}

RVA(0x0013c0c0, 0x14b)
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

RVA(0x0013c210, 0x1a)
void CRezMgr::DeAllocateRezItm(CRezItm* entry) {
    if (entry) {
        m_hashRezItmFreeList.Insert(&entry->m_heName);
    }
}

#ifndef REZ_REZARCHIVE_H
#define REZ_REZARCHIVE_H

#include <rva.h>

#include <Enums.h>
#include <Ints.h>
#include <Rez/RezArchiveDir.h>
#include <Rez/RezFile.h>
#include <Rez/RezHash.h>
#include <Rez/RezList.h>
#include <Rez/RezTypes.h>

struct CRezItm;

#pragma pack(push, 1)
GZ_ENUM_CONST_BEGIN(RezArchiveMagic)
    REZ_ARCHIVE_MAGIC_CR = '\r',
    REZ_ARCHIVE_MAGIC_LF = '\n',
    REZ_ARCHIVE_MAGIC_EOF = 0x1a,
    REZ_MGR_USER_TITLE_SIZE = 60
GZ_ENUM_CONST_END(RezArchiveMagic)

GZ_ENUM_FORWARD(RezArchiveVersion);

struct FileMainHeaderStruct {
    char CR1;
    char LF1;
    char FileType[REZ_MGR_USER_TITLE_SIZE];
    char CR2;
    char LF2;
    char UserTitle[REZ_MGR_USER_TITLE_SIZE];
    char CR3;
    char LF3;
    char EOF1;
    RezArchiveVersion FileFormatVersion;
    u32 RootDirPos;
    u32 RootDirSize;
    REZTIME RootDirTime;
    u32 NextWritePos;
    REZTIME Time;
    u32 LargestKeyAry;
    u32 LargestDirNameSize;
    u32 LargestRezNameSize;
    u32 LargestCommentSize;
    u8 IsSorted;
};

#pragma pack(pop)

class CRezMgr {
public:
    virtual void* Alloc(u32 numBytes);
    virtual void Free(void* ptr);
    virtual i32 DiskError();

    CRezMgr();
    CRezMgr(const char* fileName, b32 readOnly = true, b32 createNew = false);

    ~CRezMgr();

    i32 Close(b32 compact = false);

    CRezDir* GetRootDir();

    i32 Open(const char* fileName, b32 readOnly = true, b32 createNew = false);

    i32 OpenAdditional(const char* fileName, b32 overwriteItems = false);

    RezTypeTag StrToType(const char* typeName);

    void TypeToStr(RezTypeTag type, char* destination);

    i32 Reset();

    i32 IsOpen() {
        return m_bFileOpened;
    }

    REZTIME GetTime() {
        return m_nLastTimeModified;
    }

    i32 IsSorted() {
        return m_bIsSorted;
    }

    i32 VerifyFileOpen();

    void SetDirSeparators(const char* separators);

    void SetHashTableBins(
        u32 byNameNumHashBins,
        u32 byIdNumHashBins,
        u32 dirNumHashBins,
        u32 typeNumHashBins
    );

    void SetRenumberIDCollisions(b32 flag) {
        m_bRenumberIDCollisions = flag;
    }

    void SetNextIDNumber(u32 id) {
        m_nNextIDNumToUse = id;
    }

    i32 GetLowerCaseUsed() {
        return m_bLowerCaseUsed;
    }

    void SetLowerCaseUsed(b32 lowerCaseUsed) {
        m_bLowerCaseUsed = lowerCaseUsed;
    }

    i32 GetItemByIDUsed() {
        return m_bItemByIDUsed;
    }

    void SetItemByIDUsed(b32 itemByIDUsed) {
        m_bItemByIDUsed = itemByIDUsed;
    }

    void ForceIsSortedFlag(b32 flag) {
        m_bIsSorted = flag;
    }

    void SetMaxOpenFilesInEmulatedDir(i32 numFiles) {
        m_nMaxOpenFilesInEmulatedDir = numFiles;
    }

    void SetNextIDNumToUse(u32 nextIDNumToUse) {
        m_nNextIDNumToUse = nextIDNumToUse;
    }

    u32 GetNextIDNumToUse() {
        return m_nNextIDNumToUse;
    }

    CRezItm* GetRezFromDosPath(const char* path);
    CRezItm* GetRezFromPath(const char* path, RezTypeTag type);
    CRezDir* GetDirFromPath(const char* path);

private:
    friend class CRezDir;
    friend class CRezTyp;
    friend struct CRezItm;

    struct CRezItmChunk : public CBaseListItem {
        CRezItmChunk* Next() {
            return static_cast<CRezItmChunk*>(CBaseListItem::Next());
        }

        CRezItm* m_pRezItmAry;
    };

    struct CRezItmChunkList : public CLTBaseList {
        RVA(0x0013abb0, 0x1)
        ~CRezItmChunkList() {}

        CRezItmChunk* GetFirst() {
            return static_cast<CRezItmChunk*>(CLTBaseList::GetFirst());
        }
    };

    i32 ReadEmulationDirectory(
        CRezFileDirectoryEmulation* rezFileEmulation,
        CRezDir* directory,
        char* path,
        b32 overwriteItems
    );
    i32 IsDirectory(const char* fileName);
    CRezItm* AllocateRezItm();
    void DeAllocateRezItm(CRezItm* item);
    REZTIME GetCurTime();

    char* m_sDirSeparators;
    b32 m_bIsSorted;
    b32 m_bFileOpened;
    CBaseRezFileList m_lstRezFiles;
    u32 m_nNumRezFiles;
    CBaseRezFile* m_pPrimaryRezFile;
    b32 m_bRenumberIDCollisions;
    u32 m_nNextIDNumToUse;
    i32 m_nMaxOpenFilesInEmulatedDir;
    u32 m_nRootDirPos;
    u32 m_nRootDirSize;
    REZTIME m_nRootDirTime;
    u32 m_nNextWritePos;
    b32 m_bReadOnly;
    CRezDir* m_pRootDir;
    REZTIME m_nLastTimeModified;
    b32 m_bMustReWriteDirs;
    RezArchiveVersion m_nFileFormatVersion;

    u32 m_nLargestKeyAry;
    u32 m_nLargestDirNameSize;
    u32 m_nLargestRezNameSize;
    u32 m_nLargestCommentSize;
    char* m_sFileName;
    b32 m_bLowerCaseUsed;
    b32 m_bItemByIDUsed;
    u32 m_nByNameNumHashBins;
    u32 m_nByIDNumHashBins;
    u32 m_nDirNumHashBins;
    u32 m_nTypNumHashBins;
    CRezItmHashTableByName m_hashRezItmFreeList;
    CRezItmChunkList m_lstRezItmChunks;
    u32 m_nRezItmChunkSize;
};

#endif // REZ_REZARCHIVE_H

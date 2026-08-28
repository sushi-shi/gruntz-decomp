#ifndef REZ_REZARCHIVE_H
#define REZ_REZARCHIVE_H

#include <rva.h>

#include <Enums.h>
#include <Ints.h>
#include <Rez/RezArchiveDir.h>
#include <Rez/RezFile.h>
#include <Rez/RezHash.h>
#include <Rez/RezList.h>
#include <Rez/RezTypeTag.h>

struct CRezItm;

#pragma pack(push, 1)
GZ_ENUM_CONST_BEGIN(RezArchiveMagic)
    REZ_ARCHIVE_MAGIC_CR = '\r',
    REZ_ARCHIVE_MAGIC_LF = '\n',
    REZ_ARCHIVE_MAGIC_EOF = 0x1a
GZ_ENUM_CONST_END(RezArchiveMagic)

GZ_ENUM_FORWARD(RezArchiveVersion);

struct RezArchiveHeader {
    u8 m_initialCarriageReturn;
    char m_bannerBlock1[0x3f - 0x01];
    u8 m_firstBannerLineFeed;
    char m_bannerBlock2[0x7e - 0x40];
    u8 m_dosEndMarker;
    RezArchiveVersion m_nFileFormatVersion;
    i32 m_nRootDirPos;
    i32 m_nRootDirSize;
    i32 m_nRootDirTime;
    i32 m_nNextWritePos;
    i32 m_nLastTimeModified;
    u32 m_nLargestKeyAry;
    u32 m_nLargestDirNameSize;
    u32 m_nLargestRezNameSize;
    u32 m_nLargestCommentSize;
    u8 m_bIsSorted;
};

#pragma pack(pop)

class CRezMgr {
public:
    virtual void* Alloc(u32 numBytes);
    virtual void Free(void* ptr);
    virtual i32 DiskError();

    CRezMgr();
    CRezMgr(const char* fileName, b32 readOnly, b32 createNew);

    ~CRezMgr();

    i32 Close(b32 compact);

    CRezDir* GetRootDir();

    i32 Open(const char* fileName, b32 readOnly, b32 createNew);

    i32 OpenAdditional(const char* fileName, b32 overwriteItems);

    RezTypeTag StrToType(const char* typeName);

    void TypeToStr(RezTypeTag type, char* destination);

    i32 Reset();

    i32 VerifyFileOpen();

    void SetDirSeparators(const char* separators);

    void SetHashTableBins(
        u32 byNameNumHashBins,
        u32 byIdNumHashBins,
        u32 dirNumHashBins,
        u32 typeNumHashBins
    );

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
    i32 GetCurTime();

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
    i32 m_nRootDirTime;
    i32 m_nNextWritePos;
    b32 m_bReadOnly;
    CRezDir* m_pRootDir;
    i32 m_nLastTimeModified;
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

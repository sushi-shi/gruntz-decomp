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

struct CRezArchiveEntry;

struct CRezEntryPoolBlock : public CBaseListItem {
    CRezArchiveEntry* m_entries;
};

struct CRezEntryPoolBlockList : public CLTBaseList {
    RVA(0x0013abb0, 0x1)
    ~CRezEntryPoolBlockList() {}
};

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
    RezArchiveVersion m_version;
    i32 m_rootDirectoryOffset;
    i32 m_rootDirectorySize;
    i32 m_rootDirectoryTime;
    i32 m_nextWritePos;
    i32 m_archiveTime;
    u32 m_largestKeyArrayLength;
    u32 m_largestDirectoryNameSize;
    u32 m_largestResourceNameSize;
    u32 m_largestCommentSize;
    u8 m_isDataContiguous;
};

#pragma pack(pop)

class CRezArchive {
public:
    virtual i32 UnusedArchiveQuery(i32 unused);
    virtual void UnusedArchiveAction(i32 unused);

    virtual i32 RetryStorageOperation();

    CRezArchive();
    CRezArchive(char* path, b32 readOnly, b32 createNew);

    ~CRezArchive();

    i32 Close(i32 unusedFinal);

    CRezArchiveDir* GetRootDirectory();

    i32 Open(char* path, b32 readOnly, b32 createNew);

    i32 MergeArchive(char* path, b32 replaceExisting);

    i32 ImportDirectoryTree(
        CBaseRezFile* storage,
        CRezArchiveDir* directory,
        char* path,
        b32 replaceExisting
    );

    i32 IsDirectoryPath(char* path);

    RezTypeTag PackTag(const char* typeName);

    void UnpackTag(RezTypeTag tag, char* destination);

    i32 Reload();

    CRezArchiveEntry* AcquireEntry();

    i32 MakeTimestamp();

    i32 CheckStorages();

    void SetPathDelimiters(char* delimiters);

    void SetBucketCounts(
        i32 resourceNameBuckets,
        i32 resourceIdBuckets,
        i32 subdirectoryBuckets,
        i32 typeBuckets
    );

    CRezArchiveEntry* FindEntryByPath(const char* path);
    CRezArchiveEntry* FindEntryByPath(const char* path, RezTypeTag typeTag);
    CRezArchiveDir* FindDirectoryByPath(const char* path);
    void ReleaseEntry(CRezArchiveEntry* entry);

    char* m_pathDelimiters;
    b32 m_isDataContiguous;
    b32 m_isOpen;
    CBaseRezFileList m_lstRezFiles;
    u32 m_nNumRezFiles;
    CBaseRezFile* m_primaryStorage;
    i32 m_reserved24;
    i32 m_nextGeneratedResourceId;
    i32 m_maxOpenFiles;
    i32 m_rootDirectoryOffset;
    i32 m_rootDirectorySize;
    i32 m_rootDirectoryTime;
    i32 m_nextWritePos;
    b32 m_readOnly;
    CRezArchiveDir* m_rootDirectory;
    i32 m_archiveTime;
    b32 m_isNewArchive;
    RezArchiveVersion m_version;

    u32 m_largestKeyArrayLength;
    u32 m_largestDirectoryNameSize;
    u32 m_largestResourceNameSize;
    u32 m_largestCommentSize;
    char* m_archivePath;
    b32 m_caseSensitive;
    b32 m_useIdIndex;
    i32 m_resourceNameBucketCount;
    i32 m_resourceIdBucketCount;
    i32 m_subdirectoryBucketCount;
    i32 m_typeBucketCount;
    CRezItmHashTableByName m_hashRezItmFreeList;
    CRezEntryPoolBlockList m_entryPoolBlocks;
    i32 m_entriesPerPoolBlock;
};

#endif // REZ_REZARCHIVE_H

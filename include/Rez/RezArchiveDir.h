#ifndef REZ_REZARCHIVEDIR_H
#define REZ_REZARCHIVEDIR_H

#include <rva.h>

#include <Enums.h>
#include <Ints.h>
#include <Rez/RezHash.h>
#include <Rez/RezTypeTag.h>

#include <stddef.h>

class CRezArchive;

class CRezArchiveDir;

class CRezArchiveType {
public:
    i32 GetType() {
        return m_typeTag;
    }

    CRezArchiveType(
        i32 typeTag,
        CRezArchiveDir* directory,
        i32 resourceIdBucketCount,
        i32 resourceNameBucketCount
    );
    CRezArchiveType(i32 typeTag, CRezArchiveDir* directory, i32 resourceNameBucketCount);

    ~CRezArchiveType();

    i32 m_typeTag;
    CRezTypeHash m_heType;
    CRezItmHashTableByID m_haID;
    CRezItmHashTableByName m_haName;
    CRezArchiveDir* m_directory;
};

struct CRezArchiveEntry;
class CBaseRezFile;

class CRezArchiveDir {
public:
    char* GetDirName() {
        return m_name;
    }

    CRezArchiveDir(
        CRezArchive* archive,
        CRezArchiveDir* parent,
        const char* name,
        i32 bodyOffset,
        i32 bodySize,
        i32 time,
        i32 subdirectoryBucketCount,
        i32 typeBucketCount
    );

    ~CRezArchiveDir();

    i32 PreloadData(b32 recursive);

    CRezArchiveDir* CreateSubdirectory(const char* name);

    CRezArchiveType* FindOrCreateType(i32 typeTag);

    CRezArchiveEntry* CreateNamedEntry(void* resourceId, const char* name, i32 typeTag);

    i32 ReadDirectoryTree(CBaseRezFile* storage, i32 bodyOffset, i32 bodySize, b32 replaceExisting);

    i32 ReadDirectoryBody(CBaseRezFile* storage, i32 bodyOffset, i32 bodySize, b32 replaceExisting);

    CRezArchiveEntry*
    CreateEntry(u32 resourceId, const char* name, CRezArchiveType* type, CBaseRezFile* storage);

    i32 RemoveEntry(CRezArchiveType* type, CRezArchiveEntry* entry);

    CRezArchiveDir* FindSubdirectory(const char* name);

    CRezArchiveDir* FindDirectoryByPath(const char* path);

    CRezArchiveEntry* FindEntryByPath(const char* path, RezTypeTag typeTag);

    CRezArchiveEntry* FindEntry(const char* name, RezTypeTag typeTag);

    CRezArchiveEntry* FindEntryByFilename(const char* filename);

    CRezArchiveEntry* FindEntryByPath(const char* path);

    i32 ReleaseEntryData(b32 recursive);

    CRezArchiveDir* FirstSubdirectory();
    CRezArchiveDir* NextSubdirectory(CRezArchiveDir* directory);
    CRezArchiveType* FindType(u32 typeTag);
    CRezArchiveType* FirstType();
    CRezArchiveType* NextType(CRezArchiveType* type);
    CRezArchiveEntry* FirstEntry(CRezArchiveType* type);
    CRezArchiveEntry* NextEntry(CRezArchiveEntry* entry);

    char* m_name;

    i32 m_bodyOffset;
    i32 m_bodySize;
    i32 m_minDataOffset;

    u32 m_totalDataSize;

    i32 m_time;
    CRezArchive* m_archive;

    CRezArchiveDir* m_parent;

    CRezDirHash m_heDir;
    CRezDirHashTable m_haDir;
    CRezTypeHashTable m_haTypes;
    char* m_preloadedData;
};

#endif // REZ_REZARCHIVEDIR_H

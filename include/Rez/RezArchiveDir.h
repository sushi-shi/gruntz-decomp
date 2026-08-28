#ifndef REZ_REZARCHIVEDIR_H
#define REZ_REZARCHIVEDIR_H

#include <rva.h>

#include <Bute/Hash.h>
#include <Enums.h>
#include <Ints.h>
#include <Rez/RezTypeTag.h>

#include <stddef.h>

struct CRezArchiveDirHashNode : public CHashElement {

    virtual u32 Hash() OVERRIDE;

    void SetArchiveDirectory(CRezArchiveDir* directory) {
        m_archiveDirectory = directory;
    }

    CRezArchiveDir* GetArchiveDirectory() {
        return m_archiveDirectory;
    }

    CRezArchiveDirHashNode* Next() {
        return static_cast<CRezArchiveDirHashNode*>(CHashElement::Next());
    }

    CRezArchiveDirHashNode* Prev() {
        return static_cast<CRezArchiveDirHashNode*>(CHashElement::Prev());
    }

    CRezArchiveDirHashNode* NextInBucket() {
        return static_cast<CRezArchiveDirHashNode*>(CHashElement::NextInBucket());
    }

    CRezArchiveDirHashNode* PrevInBucket() {
        return static_cast<CRezArchiveDirHashNode*>(CHashElement::PrevInBucket());
    }

    CRezArchiveDirHashNode() {
        m_archiveDirectory = NULL;
    }

    CRezArchiveDir* m_archiveDirectory;
};

class CRezArchive;

class CRezArchiveDir;

struct CRezArchiveTypeHashNode : public CHashElement {

    virtual u32 Hash() OVERRIDE;

    void SetArchiveType(CRezArchiveType* type) {
        m_archiveType = type;
    }

    CRezArchiveType* GetArchiveType() {
        return m_archiveType;
    }

    CRezArchiveTypeHashNode* Next() {
        return static_cast<CRezArchiveTypeHashNode*>(CHashElement::Next());
    }

    CRezArchiveTypeHashNode* Prev() {
        return static_cast<CRezArchiveTypeHashNode*>(CHashElement::Prev());
    }

    CRezArchiveTypeHashNode* NextInBucket() {
        return static_cast<CRezArchiveTypeHashNode*>(CHashElement::NextInBucket());
    }

    CRezArchiveTypeHashNode* PrevInBucket() {
        return static_cast<CRezArchiveTypeHashNode*>(CHashElement::PrevInBucket());
    }

    CRezArchiveTypeHashNode() {
        m_archiveType = NULL;
    }

    CRezArchiveType* m_archiveType;
};

inline void CRezDirectoryNameHash::Insert(CRezArchiveDirHashNode* directory) {
    CHashBase::Insert(directory);
}

inline void CRezDirectoryNameHash::Delete(CRezArchiveDirHashNode* directory) {
    CHashBase::Remove(directory);
}

inline CRezArchiveDirHashNode* CRezDirectoryNameHash::First() {
    return static_cast<CRezArchiveDirHashNode*>(CHashBase::First());
}

inline CRezArchiveDirHashNode* CRezDirectoryNameHash::Last() {
    return static_cast<CRezArchiveDirHashNode*>(CHashBase::Last());
}

inline CRezArchiveDirHashNode* CRezDirectoryNameHash::Lookup(u32 bucketIndex) {
    return static_cast<CRezArchiveDirHashNode*>(CHashBase::Lookup(bucketIndex));
}

inline void CRezTypeTagHash::Insert(CRezArchiveTypeHashNode* type) {
    CHashBase::Insert(type);
}

inline void CRezTypeTagHash::Delete(CRezArchiveTypeHashNode* type) {
    CHashBase::Remove(type);
}

inline CRezArchiveTypeHashNode* CRezTypeTagHash::First() {
    return static_cast<CRezArchiveTypeHashNode*>(CHashBase::First());
}

inline CRezArchiveTypeHashNode* CRezTypeTagHash::Last() {
    return static_cast<CRezArchiveTypeHashNode*>(CHashBase::Last());
}

inline CRezArchiveTypeHashNode* CRezTypeTagHash::Lookup(u32 bucketIndex) {
    return static_cast<CRezArchiveTypeHashNode*>(CHashBase::Lookup(bucketIndex));
}

class CRezArchiveType {
public:
    CRezArchiveType(
        i32 typeTag,
        CRezArchiveDir* directory,
        i32 resourceIdBucketCount,
        i32 resourceNameBucketCount
    );
    CRezArchiveType(i32 typeTag, CRezArchiveDir* directory, i32 resourceNameBucketCount);

    ~CRezArchiveType();

    i32 m_typeTag;
    CRezArchiveTypeHashNode m_typeNode;
    CRezEntryIdHash m_idIndex;
    CRezEntryNameHash m_nameIndex;
    CRezArchiveDir* m_directory;
};

struct CRezArchiveEntry;
class CRezItmBase;

class CRezArchiveDir {
public:
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

    i32 ReadDirectoryTree(CRezItmBase* storage, i32 bodyOffset, i32 bodySize, b32 replaceExisting);

    i32 ReadDirectoryBody(CRezItmBase* storage, i32 bodyOffset, i32 bodySize, b32 replaceExisting);

    CRezArchiveEntry*
    CreateEntry(u32 resourceId, const char* name, CRezArchiveType* type, CRezItmBase* storage);

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

    CRezArchiveDirHashNode m_nameNode;
    CRezDirectoryNameHash m_subdirectories;
    CRezTypeTagHash m_types;
    char* m_preloadedData;
};

#endif // REZ_REZARCHIVEDIR_H

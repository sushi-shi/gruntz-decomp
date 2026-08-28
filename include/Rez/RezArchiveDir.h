#ifndef REZ_REZARCHIVEDIR_H
#define REZ_REZARCHIVEDIR_H

#include <rva.h>

#include <Enums.h>
#include <Ints.h>
#include <Rez/RezHash.h>
#include <Rez/RezTypeTag.h>

#include <stddef.h>

class CRezMgr;

class CRezDir;

class CRezTyp {
public:
    i32 GetType() {
        return m_nType;
    }

private:
    friend class CRezItm;
    friend class CRezDir;
    friend class CRezMgr;

    CRezTyp(
        i32 typeTag,
        CRezDir* directory,
        i32 resourceIdBucketCount,
        i32 resourceNameBucketCount
    );
    CRezTyp(i32 typeTag, CRezDir* directory, i32 resourceNameBucketCount);

    ~CRezTyp();

    i32 m_nType;
    CRezTypeHash m_heType;
    CRezItmHashTableByID m_haID;
    CRezItmHashTableByName m_haName;
    CRezDir* m_pParentDir;
};

struct CRezItm;
class CBaseRezFile;

class CRezDir {
public:
    char* GetDirName() {
        return m_sDirName;
    }

    i32 Load(b32 loadAllSubDirs);
    i32 UnLoad(b32 unloadAllSubDirs);
    CRezItm* GetRez(const char* name, RezTypeTag type);
    CRezItm* GetRezFromDosName(const char* filename);
    CRezItm* GetRezFromPath(const char* path, RezTypeTag type);
    CRezItm* GetRezFromDosPath(const char* path);
    CRezDir* GetDir(const char* name);
    CRezDir* GetDirFromPath(const char* path);
    CRezDir* GetFirstSubDir();
    CRezDir* GetNextSubDir(CRezDir* directory);
    CRezTyp* GetRezTyp(u32 type);
    CRezTyp* GetFirstType();
    CRezTyp* GetNextType(CRezTyp* type);
    CRezItm* GetFirstItem(CRezTyp* type);
    CRezItm* GetNextItem(CRezItm* item);
    CRezDir* CreateDir(const char* name);
    CRezItm* CreateRez(void* id, const char* name, i32 type);

private:
    friend struct CRezItm;
    friend class CRezTyp;
    friend class CRezMgr;

    CRezDir(
        CRezMgr* archive,
        CRezDir* parent,
        const char* name,
        i32 bodyOffset,
        i32 bodySize,
        i32 time,
        i32 subdirectoryBucketCount,
        i32 typeBucketCount
    );

    ~CRezDir();

    CRezTyp* GetOrMakeTyp(i32 type);

    i32 ReadAllDirs(CBaseRezFile* rezFile, i32 pos, i32 size, b32 overwriteItems);

    i32 ReadDirBlock(CBaseRezFile* rezFile, i32 pos, i32 size, b32 overwriteItems);

    CRezItm* CreateRezInternal(u32 id, const char* name, CRezTyp* type, CBaseRezFile* rezFile);

    i32 RemoveRezInternal(CRezTyp* type, CRezItm* item);

    char* m_sDirName;

    i32 m_nDirPos;
    i32 m_nDirSize;
    i32 m_nItemsPos;

    u32 m_nItemsSize;

    i32 m_nLastTimeModified;
    CRezMgr* m_pRezMgr;

    CRezDir* m_pParentDir;

    CRezDirHash m_heDir;
    CRezDirHashTable m_haDir;
    CRezTypeHashTable m_haTypes;
    char* m_pMemBlock;
};

#endif // REZ_REZARCHIVEDIR_H

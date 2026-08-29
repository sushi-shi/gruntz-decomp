#ifndef REZ_REZARCHIVEDIR_H
#define REZ_REZARCHIVEDIR_H

#include <rva.h>

#include <Enums.h>
#include <Ints.h>
#include <Rez/RezHash.h>
#include <Rez/RezTypes.h>

#include <stddef.h>

class CRezMgr;

class CRezDir;

class CRezTyp {
public:
    REZTYPE GetType() {
        return m_nType;
    }

private:
    friend class CRezItm;
    friend class CRezDir;
    friend class CRezMgr;

    CRezTyp(
        REZTYPE typeTag,
        CRezDir* directory,
        u32 resourceIdBucketCount,
        u32 resourceNameBucketCount
    );
    CRezTyp(REZTYPE typeTag, CRezDir* directory, u32 resourceNameBucketCount);

    ~CRezTyp();

    REZTYPE m_nType;
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

    CRezDir* GetParentDir() {
        return m_pParentDir;
    }

    CRezMgr* GetParentMgr() {
        return m_pRezMgr;
    }

    i32 Load(b32 loadAllSubDirs = false);
    i32 UnLoad(b32 unloadAllSubDirs = false);

    i32 IsLoaded() {
        return m_pMemBlock != NULL;
    }

    REZTIME GetTime() {
        return m_nLastTimeModified;
    }
    CRezItm* GetRez(const char* name, RezTypeTag type);
    CRezItm* GetRezFromDosName(const char* filename);
    CRezItm* GetRezFromPath(const char* path, RezTypeTag type);
    CRezItm* GetRezFromDosPath(const char* path);
    CRezDir* GetDir(const char* name);
    CRezDir* GetDirFromPath(const char* path);
    CRezDir* GetFirstSubDir();
    CRezDir* GetNextSubDir(CRezDir* directory);
    CRezTyp* GetRezTyp(REZTYPE type);
    CRezTyp* GetFirstType();
    CRezTyp* GetNextType(CRezTyp* type);
    CRezItm* GetFirstItem(CRezTyp* type);
    CRezItm* GetNextItem(CRezItm* item);
    CRezDir* CreateDir(const char* name);
    CRezItm* CreateRez(REZID id, const char* name, REZTYPE type);

private:
    friend struct CRezItm;
    friend class CRezTyp;
    friend class CRezMgr;

    CRezDir(
        CRezMgr* archive,
        CRezDir* parent,
        const char* name,
        u32 bodyOffset,
        u32 bodySize,
        REZTIME time,
        u32 subdirectoryBucketCount,
        u32 typeBucketCount
    );

    ~CRezDir();

    CRezTyp* GetOrMakeTyp(REZTYPE type);

    i32 ReadAllDirs(CBaseRezFile* rezFile, u32 pos, u32 size, b32 overwriteItems);

    i32 ReadDirBlock(CBaseRezFile* rezFile, u32 pos, u32 size, b32 overwriteItems);

    CRezItm* CreateRezInternal(REZID id, const char* name, CRezTyp* type, CBaseRezFile* rezFile);

    i32 RemoveRezInternal(CRezTyp* type, CRezItm* item);

    i32 IsGoodChar(char character);

    char* m_sDirName;

    u32 m_nDirPos;
    u32 m_nDirSize;
    u32 m_nItemsPos;

    u32 m_nItemsSize;

    REZTIME m_nLastTimeModified;
    CRezMgr* m_pRezMgr;

    CRezDir* m_pParentDir;

    CRezDirHash m_heDir;
    CRezDirHashTable m_haDir;
    CRezTypeHashTable m_haTypes;
    u8* m_pMemBlock;
};

#endif // REZ_REZARCHIVEDIR_H

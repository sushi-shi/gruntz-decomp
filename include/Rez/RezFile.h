#ifndef REZ_REZFILE_H
#define REZ_REZFILE_H

#include <rva.h>

#include <Mfc.h>

#include <Enums.h>
#include <Ints.h>
#include <Lith/VirtList.h>

class CRezArchive;
class CBaseRezFile;
class CRezFileSingleFile;

class CBaseRezFileList : public CVirtBaseList {
public:
    CBaseRezFile* GetFirst();
    CBaseRezFile* GetLast();

    virtual void VirtualFoo() OVERRIDE;
};

class CRezFileSingleFileList : public CVirtBaseList {
public:
    CRezFileSingleFile* GetFirst();
    CRezFileSingleFile* GetLast();

    virtual void VirtualFoo() OVERRIDE;
};

class CBaseRezFile : public CVirtBaseListItem {
public:
    CBaseRezFile(CRezArchive* rezMgr);
    virtual ~CBaseRezFile();

    virtual u32 Read(u32 itemPos, u32 itemOffset, u32 size, void* data) = 0;
    virtual u32 Write(u32 itemPos, u32 itemOffset, u32 size, void* data) = 0;
    virtual i32 Open(const char* fileName, b32 readOnly, b32 createNew) = 0;
    virtual i32 Close() = 0;
    virtual i32 Flush() = 0;
    virtual i32 VerifyFileOpen() = 0;

    CBaseRezFile* Next() {
        return static_cast<CBaseRezFile*>(CVirtBaseListItem::Next());
    }

    virtual void VirtualFoo() OVERRIDE;

protected:
    CRezArchive* m_pRezMgr;
};

class CRezFile : public CBaseRezFile {
public:
    CRezFile(CRezArchive* rezMgr);
    virtual ~CRezFile() OVERRIDE;

    virtual u32 Read(u32 itemPos, u32 itemOffset, u32 size, void* data) OVERRIDE;
    virtual u32 Write(u32 itemPos, u32 itemOffset, u32 size, void* data) OVERRIDE;
    virtual i32 Open(const char* fileName, b32 readOnly, b32 createNew) OVERRIDE;
    virtual i32 Close() OVERRIDE;
    virtual i32 Flush() OVERRIDE;
    virtual i32 VerifyFileOpen() OVERRIDE;

private:
    FILE* m_pFile;
    char* m_sFileName;
    b32 m_bReadOnly;
    b32 m_bCreateNew;
    u32 m_nLastSeekPos;
};

class CRezFileDirectoryEmulation : public CBaseRezFile {
public:
    CRezFileDirectoryEmulation(CRezArchive* rezMgr, i32 maxOpenFiles);
    virtual ~CRezFileDirectoryEmulation() OVERRIDE;

    virtual u32 Read(u32 itemPos, u32 itemOffset, u32 size, void* data) OVERRIDE;
    virtual u32 Write(u32 itemPos, u32 itemOffset, u32 size, void* data) OVERRIDE;
    virtual i32 Open(const char* fileName, b32 readOnly, b32 createNew) OVERRIDE;
    virtual i32 Close() OVERRIDE;
    virtual i32 Flush() OVERRIDE;
    virtual i32 VerifyFileOpen() OVERRIDE;

private:
    friend class CRezFileSingleFile;

    CRezFileSingleFileList m_lstOpenFiles;
    CRezFileSingleFileList m_lstClosedFiles;
    i32 m_nNumOpenFiles;
    i32 m_nMaxOpenFiles;
    b32 m_bReadOnly;
    b32 m_bCreateNew;
};

class CRezFileSingleFile : public CBaseRezFile {
public:
    CRezFileSingleFile(
        CRezArchive* rezMgr,
        const char* fileName,
        CRezFileDirectoryEmulation* dirEmulation
    );
    virtual ~CRezFileSingleFile() OVERRIDE;

    virtual u32 Read(u32 itemPos, u32 itemOffset, u32 size, void* data) OVERRIDE;
    virtual u32 Write(u32 itemPos, u32 itemOffset, u32 size, void* data) OVERRIDE;
    virtual i32 Open(const char* fileName, b32 readOnly, b32 createNew) OVERRIDE;
    virtual i32 Close() OVERRIDE;
    virtual i32 Flush() OVERRIDE;
    virtual i32 VerifyFileOpen() OVERRIDE;
    virtual void VirtualFoo() OVERRIDE;

private:
    friend class CRezFileDirectoryEmulation;

    i32 ReallyOpen();
    i32 ReallyClose();

    char* m_sFileName;
    FILE* m_pFile;
    CRezFileDirectoryEmulation* m_pDirEmulation;
};

inline CBaseRezFile* CBaseRezFileList::GetFirst() {
    return static_cast<CBaseRezFile*>(CVirtBaseList::GetFirst());
}

inline CBaseRezFile* CBaseRezFileList::GetLast() {
    return static_cast<CBaseRezFile*>(CVirtBaseList::GetLast());
}

inline CRezFileSingleFile* CRezFileSingleFileList::GetFirst() {
    return static_cast<CRezFileSingleFile*>(CVirtBaseList::GetFirst());
}

inline CRezFileSingleFile* CRezFileSingleFileList::GetLast() {
    return static_cast<CRezFileSingleFile*>(CVirtBaseList::GetLast());
}

extern char g_wildcard[];
extern char s_rPlusB[];
extern char s_wPlusB[];

#endif // REZ_REZFILE_H

#ifndef REZ_REZFILE_H
#define REZ_REZFILE_H

#include <Ints.h>
#include <Io/File.h>

#include <Enums.h>
#include <Ints.h>
#include <Lith/VirtList.h>

class CRezMgr;
class CBaseRezFile;
class CRezFileSingleFile;

class CBaseRezFileList : public CVirtBaseList {
public:
    CBaseRezFile* GetFirst();
    CBaseRezFile* GetLast();

    virtual void VirtualFoo()  ;
};

class CRezFileSingleFileList : public CVirtBaseList {
public:
    CRezFileSingleFile* GetFirst();
    CRezFileSingleFile* GetLast();

    virtual void VirtualFoo()  ;
};

class CBaseRezFile : public CVirtBaseListItem {
public:
    CBaseRezFile(CRezMgr* rezMgr);
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

    virtual void VirtualFoo()  ;

protected:
    CRezMgr* m_pRezMgr;
};

class CRezFile : public CBaseRezFile {
public:
    CRezFile(CRezMgr* rezMgr);
    virtual ~CRezFile()  ;

    virtual u32 Read(u32 itemPos, u32 itemOffset, u32 size, void* data)  ;
    virtual u32 Write(u32 itemPos, u32 itemOffset, u32 size, void* data)  ;
    virtual i32 Open(const char* fileName, b32 readOnly, b32 createNew)  ;
    virtual i32 Close()  ;
    virtual i32 Flush()  ;
    virtual i32 VerifyFileOpen()  ;

    io::RandomInput& DataSource() { return m_file; }
private:
    io::File m_file;
};

class CRezFileDirectoryEmulation : public CBaseRezFile {
public:
    CRezFileDirectoryEmulation(CRezMgr* rezMgr, i32 maxOpenFiles);
    virtual ~CRezFileDirectoryEmulation()  ;

    virtual u32 Read(u32 itemPos, u32 itemOffset, u32 size, void* data)  ;
    virtual u32 Write(u32 itemPos, u32 itemOffset, u32 size, void* data)  ;
    virtual i32 Open(const char* fileName, b32 readOnly, b32 createNew)  ;
    virtual i32 Close()  ;
    virtual i32 Flush()  ;
    virtual i32 VerifyFileOpen()  ;

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
        CRezMgr* rezMgr,
        const char* fileName,
        CRezFileDirectoryEmulation* dirEmulation
    );
    virtual ~CRezFileSingleFile()  ;

    virtual u32 Read(u32 itemPos, u32 itemOffset, u32 size, void* data)  ;
    virtual u32 Write(u32 itemPos, u32 itemOffset, u32 size, void* data)  ;
    virtual i32 Open(const char* fileName, b32 readOnly, b32 createNew)  ;
    virtual i32 Close()  ;
    virtual i32 Flush()  ;
    virtual i32 VerifyFileOpen()  ;
    virtual void VirtualFoo()  ;

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
extern char g_rPlusB[];
extern char g_wPlusB[];

#endif

#include <StdAfx.h>

#include <Ints.h>

#include <Rez/RezFile.h>

#include <Rez/RezArchive.h>

#include <stdio.h>
#include <string.h>

char g_rPlusB[] = "r+b";

char g_wPlusB[] = "w+b";

char g_wildcard[] = "*.*";

void CRezFileSingleFileList::VirtualFoo() {}

CBaseRezFile::CBaseRezFile(CRezMgr* rezMgr) {
    m_pRezMgr = rezMgr;
}

CBaseRezFile::~CBaseRezFile() {
    m_pRezMgr = NULL;
}

void CBaseRezFile::VirtualFoo() {}

CRezFile::CRezFile(CRezMgr* rezMgr) : CBaseRezFile(rezMgr) {}
CRezFile::~CRezFile() {}

u32 CRezFile::Read(u32 itemPos, u32 itemOffset, u32 size, void* data) {
    if (itemOffset > 0xffffffffU - itemPos) return 0;
    return m_file.readAt(itemPos + itemOffset, data, size) ? size : 0;
}
u32 CRezFile::Write(u32 itemPos, u32 itemOffset, u32 size, void* data) {
    if (itemPos > 0x7fffffffU || itemOffset > 0x7fffffffU - itemPos
        || size > 0x7fffffffU - itemPos - itemOffset) return 0;
    return m_file.seek(static_cast<long>(itemPos + itemOffset), io::Start)
        && m_file.write(data, size) ? size : 0;
}
i32 CRezFile::Open(const char* fileName, b32 readOnly, b32 createNew) {
    if (readOnly && createNew) return 0;
    return m_file.open(fileName, createNew ? io::Replace : readOnly ? io::ReadOnly : io::Update);
}
i32 CRezFile::Close() { return m_file.finish(); }
i32 CRezFile::Flush() { return m_file.flush(); }
i32 CRezFile::VerifyFileOpen() { return m_file.good(); }

CRezFileDirectoryEmulation::CRezFileDirectoryEmulation(CRezMgr* rezMgr, i32 maxOpenFiles)
    : CBaseRezFile(rezMgr) {
    m_nNumOpenFiles = 0;
    m_nMaxOpenFiles = maxOpenFiles;
    m_bReadOnly = true;
    m_bCreateNew = false;
}

CRezFileDirectoryEmulation::~CRezFileDirectoryEmulation() {

    while (m_lstOpenFiles.GetFirst() != NULL) {
        delete m_lstOpenFiles.GetFirst();
    }
    while (m_lstClosedFiles.GetFirst() != NULL) {
        delete m_lstClosedFiles.GetFirst();
    }
}

u32 CRezFileDirectoryEmulation::Read(u32 itemPos, u32 itemOffset, u32 size, void* data) {
    return 0;
}

u32 CRezFileDirectoryEmulation::Write(u32 itemPos, u32 itemOffset, u32 size, void* data) {
    return 0;
}

i32 CRezFileDirectoryEmulation::Open(const char* fileName, b32 readOnly, b32 createNew) {
    m_bReadOnly = readOnly;
    m_bCreateNew = createNew;
    return 1;
}

i32 CRezFileDirectoryEmulation::Close() {

    while (m_lstOpenFiles.GetFirst() != NULL) {
        m_lstOpenFiles.GetFirst()->ReallyClose();
    }
    return 1;
}

i32 CRezFileDirectoryEmulation::Flush() {
    return 1;
}

i32 CRezFileDirectoryEmulation::VerifyFileOpen() {
    return 1;
}

CRezFileSingleFile::CRezFileSingleFile(
    CRezMgr* rezMgr,
    const char* fileName,
    CRezFileDirectoryEmulation* dirEmulation
)
    : CBaseRezFile(rezMgr) {
    m_pDirEmulation = dirEmulation;
    m_pFile = NULL;

    m_sFileName = new char[strlen(fileName) + 1];
    strcpy(m_sFileName, fileName);

    m_pDirEmulation->m_lstClosedFiles.Insert(this);
}

CRezFileSingleFile::~CRezFileSingleFile() {
    if (m_pFile) {
        ReallyClose();
    }
    if (m_sFileName) {
        delete[] m_sFileName;
    }
    m_pDirEmulation->m_lstClosedFiles.Delete(this);
}

u32 CRezFileSingleFile::Read(u32 itemPos, u32 itemOffset, u32 size, void* data) {
    static_cast<void>(itemPos);
    if (size <= 0) {
        return 0;
    }
    if (m_pFile == NULL) {
        ReallyOpen();
    }
    while (fseek(m_pFile, itemOffset, 0) != 0) {
        if (m_pDirEmulation->m_pRezMgr->DiskError() == 0) {
            return 0;
        }
    }
    u32 got = fread(data, 1, size, m_pFile);
    while (got != size) {
        if (m_pDirEmulation->m_pRezMgr->DiskError() == 0) {
            return 0;
        }
        got = fread(data, 1, size, m_pFile);
    }
    return got;
}

u32 CRezFileSingleFile::Write(u32 itemPos, u32 itemOffset, u32 size, void* data) {
    static_cast<void>(itemPos);
    if (size <= 0) {
        return 0;
    }
    if (m_pFile == NULL) {
        ReallyOpen();
    }
    while (fseek(m_pFile, itemOffset, 0) != 0) {
        if (m_pDirEmulation->m_pRezMgr->DiskError() == 0) {
            return 0;
        }
    }
    u32 put = fwrite(data, 1, size, m_pFile);
    while (put != size) {
        if (m_pDirEmulation->m_pRezMgr->DiskError() == 0) {
            return 0;
        }
        put = fwrite(data, 1, size, m_pFile);
    }
    return put;
}

i32 CRezFileSingleFile::Open(const char* fileName, b32 readOnly, b32 createNew) {
    return 0;
}

i32 CRezFileSingleFile::Close() {
    return 0;
}

i32 CRezFileSingleFile::Flush() {
    if (m_pFile != NULL) {
        b32 ok = (fflush(m_pFile) == 0);
        while (!ok) {
            if (m_pDirEmulation->m_pRezMgr->DiskError() == 0) {
                return 0;
            }
            ok = (fflush(m_pFile) == 0);
        }
        return ok;
    }
    return 1;
}

i32 CRezFileSingleFile::VerifyFileOpen() {
    return 0;
}

i32 CRezFileSingleFile::ReallyOpen() {
    if (m_pFile != NULL) {
        return 1;
    }
    if (m_pDirEmulation->m_nNumOpenFiles > m_pDirEmulation->m_nMaxOpenFiles) {

        CRezFileSingleFile* lru = m_pDirEmulation->m_lstOpenFiles.GetLast();
        if (lru != NULL) {
            lru->ReallyClose();
        }
    }
    for (;;) {
        if (m_pDirEmulation->m_bCreateNew) {
            if (m_pDirEmulation->m_bReadOnly) {
                return 0;
            }
            m_pFile = fopen(m_sFileName, g_wPlusB);
        } else if (m_pDirEmulation->m_bReadOnly) {
            m_pFile = fopen(m_sFileName, "rb");
        } else {
            m_pFile = fopen(m_sFileName, g_rPlusB);
        }
        if (m_pFile != NULL) {
            break;
        }
        if (m_pDirEmulation->m_pRezMgr->DiskError() == 0) {
            return 0;
        }
        if (m_pFile != NULL) {
            break;
        }
    }
    m_pDirEmulation->m_lstClosedFiles.Delete(this);
    m_pDirEmulation->m_lstOpenFiles.InsertFirst(this);
    m_pDirEmulation->m_nNumOpenFiles++;
    return 1;
}

i32 CRezFileSingleFile::ReallyClose() {
    if (m_pFile == NULL) {
        return 1;
    }
    b32 ok = (fclose(m_pFile) == 0);
    while (!ok) {
        if (m_pDirEmulation->m_pRezMgr->DiskError() == 0) {
            return 0;
        }
        ok = (fclose(m_pFile) == 0);
    }
    m_pDirEmulation->m_nNumOpenFiles--;
    m_pDirEmulation->m_lstOpenFiles.Delete(this);
    m_pDirEmulation->m_lstClosedFiles.Insert(this);
    m_pFile = NULL;
    return ok;
}

void CRezFileSingleFile::VirtualFoo() {}

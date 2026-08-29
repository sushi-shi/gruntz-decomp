#include <rva.h>

#include <Rez/RezFile.h>

#include <Rez/RezArchive.h>

#include <stdio.h>
#include <string.h>

DATA(0x0021a0a4)
char s_rPlusB[] = "r+b";
DATA(0x0021a0a8)
char s_wPlusB[] = "w+b";

DATA(0x0021a0a0)
char g_wildcard[] = "*.*";

RVA(0x0013c4d0, 0x1)
void CRezFileSingleFileList::VirtualFoo() {}

RVA(0x0013c4e0, 0x12)
CBaseRezFile::CBaseRezFile(CRezMgr* rezMgr) {
    m_pRezMgr = rezMgr;
}

RVA_COMPGEN(0x0013c500, 0x1e, ??_GCBaseRezFile@@UAEPAXI@Z)

RVA(0x0013c520, 0xe)
CBaseRezFile::~CBaseRezFile() {
    m_pRezMgr = NULL;
}

RVA(0x0013c530, 0x1)
void CBaseRezFile::VirtualFoo() {}

RVA(0x0013c540, 0x28)
CRezFile::CRezFile(CRezMgr* rezMgr) : CBaseRezFile(rezMgr) {
    m_pFile = NULL;
    m_sFileName = NULL;
    m_nLastSeekPos = 0xffffffff;
}

RVA_COMPGEN(0x0013c570, 0x1e, ??_GCRezFile@@UAEPAXI@Z)

RVA(0x0013c590, 0x66)
CRezFile::~CRezFile() {
    if (m_pFile != NULL) {
        Close();
    }
    if (m_sFileName != NULL) {
        delete[] m_sFileName;
    }
}

RVA(0x0013c600, 0xbd)
u32 CRezFile::Read(u32 itemPos, u32 itemOffset, u32 size, void* data) {
    if (size <= 0) {
        return 0;
    }

    u32 seekPos = itemPos + itemOffset;

    if (m_nLastSeekPos != seekPos) {
        while (fseek(m_pFile, seekPos, 0) != 0) {
            if (m_pRezMgr->DiskError() == 0) {
                m_nLastSeekPos = 0xffffffff;
                return 0;
            }
        }
    }

    u32 got = fread(data, 1, size, m_pFile);
    while (got != size) {
        if (m_pRezMgr->DiskError() == 0) {
            m_nLastSeekPos = 0xffffffff;
            return 0;
        }
        got = fread(data, 1, size, m_pFile);
    }

    m_nLastSeekPos = got + seekPos;
    return got;
}

RVA(0x0013c6c0, 0x97)
u32 CRezFile::Write(u32 itemPos, u32 itemOffset, u32 size, void* data) {
    m_nLastSeekPos = 0xffffffff;
    if (size <= 0) {
        return 0;
    }

    while (fseek(m_pFile, itemPos + itemOffset, 0) != 0) {
        if (m_pRezMgr->DiskError() == 0) {
            return 0;
        }
    }

    u32 put = fwrite(data, 1, size, m_pFile);
    while (put != size) {
        if (m_pRezMgr->DiskError() == 0) {
            return 0;
        }
        put = fwrite(data, 1, size, m_pFile);
    }
    return put;
}

RVA(0x0013c760, 0xc1)
i32 CRezFile::Open(const char* fileName, b32 readOnly, b32 createNew) {
    for (;;) {
        if (createNew) {
            if (readOnly) {
                return 0;
            }
            m_pFile = fopen(fileName, s_wPlusB);
        } else if (readOnly) {
            m_pFile = fopen(fileName, "rb");
        } else {
            m_pFile = fopen(fileName, s_rPlusB);
        }
        if (m_pFile != NULL) {
            break;
        }
        if (m_pRezMgr->DiskError() == 0) {
            return 0;
        }
        if (m_pFile != NULL) {
            break;
        }
    }

    m_bReadOnly = readOnly;
    if (m_sFileName != NULL) {
        delete[] m_sFileName;
    }
    m_sFileName = new char[strlen(fileName) + 1];
    if (m_sFileName != NULL) {
        strcpy(m_sFileName, fileName);
    }
    m_nLastSeekPos = 0xffffffff;
    return 1;
}

RVA(0x0013c830, 0x63)
i32 CRezFile::Close() {
    b32 ok;
    i32 check;
    if (m_pFile != NULL) {
        do {
            check = fclose(m_pFile);
            if (check == 0) {
                ok = true;
            } else {
                ok = false;
                if (m_pRezMgr->DiskError() == 0) {
                    return 0;
                }
            }
        } while (!ok);

    } else {
        return 0;
    }
    m_pFile = NULL;
    if (m_sFileName != NULL) {
        delete[] m_sFileName;
    }
    m_sFileName = NULL;
    m_nLastSeekPos = 0xffffffff;
    return ok;
}

RVA(0x0013c8a0, 0x45)
i32 CRezFile::Flush() {
    m_nLastSeekPos = 0xffffffff;
    if (m_pFile) {
        b32 found;
        do {
            if (fflush(m_pFile) == 0) {
                found = true;
            } else {
                found = false;
                if (m_pRezMgr->DiskError() == 0) {
                    return 0;
                }
            }
        } while (!found);
        return found;
    }
    return 0;
}

RVA(0x0013c8f0, 0x41)
i32 CRezFile::VerifyFileOpen() {
    m_nLastSeekPos = 0xffffffff;
    if (!m_pFile) {
        return 0;
    }
    if (ftell(m_pFile) != -1) {
        return 1;
    }
    return Open(m_sFileName, m_bReadOnly, false) != 0;
}

RVA(0x0013c940, 0x46)
CRezFileDirectoryEmulation::CRezFileDirectoryEmulation(CRezMgr* rezMgr, i32 maxOpenFiles)
    : CBaseRezFile(rezMgr) {
    m_nNumOpenFiles = 0;
    m_nMaxOpenFiles = maxOpenFiles;
    m_bReadOnly = true;
    m_bCreateNew = false;
}

RVA_COMPGEN(0x0013c990, 0x1e, ??_GCRezFileDirectoryEmulation@@UAEPAXI@Z)

RVA(0x0013c9b0, 0x7f)
CRezFileDirectoryEmulation::~CRezFileDirectoryEmulation() {

    while (m_lstOpenFiles.GetFirst() != NULL) {
        delete m_lstOpenFiles.GetFirst();
    }
    while (m_lstClosedFiles.GetFirst() != NULL) {
        delete m_lstClosedFiles.GetFirst();
    }
}

RVA_COMPGEN(0x0013ca30, 0x7, ??1CRezFileSingleFileList@@QAE@XZ)

RVA(0x0013ca40, 0x5)
u32 CRezFileDirectoryEmulation::Read(u32 itemPos, u32 itemOffset, u32 size, void* data) {
    return 0;
}
RVA(0x0013ca50, 0x5)
u32 CRezFileDirectoryEmulation::Write(u32 itemPos, u32 itemOffset, u32 size, void* data) {
    return 0;
}

RVA(0x0013ca60, 0x16)
i32 CRezFileDirectoryEmulation::Open(const char* fileName, b32 readOnly, b32 createNew) {
    m_bReadOnly = readOnly;
    m_bCreateNew = createNew;
    return 1;
}

RVA(0x0013ca80, 0x1d)
i32 CRezFileDirectoryEmulation::Close() {

    while (m_lstOpenFiles.GetFirst() != NULL) {
        m_lstOpenFiles.GetFirst()->ReallyClose();
    }
    return 1;
}

RVA(0x0013caa0, 0x6)
i32 CRezFileDirectoryEmulation::Flush() {
    return 1;
}
RVA(0x0013cab0, 0x6)
i32 CRezFileDirectoryEmulation::VerifyFileOpen() {
    return 1;
}

RVA(0x0013cac0, 0x9b)
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

RVA_COMPGEN(0x0013cb60, 0x1e, ??_GCRezFileSingleFile@@UAEPAXI@Z)

RVA(0x0013cb80, 0x72)
CRezFileSingleFile::~CRezFileSingleFile() {
    if (m_pFile) {
        ReallyClose();
    }
    if (m_sFileName) {
        delete[] m_sFileName;
    }
    m_pDirEmulation->m_lstClosedFiles.Delete(this);
}

RVA(0x0013cc00, 0x9f)
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

RVA(0x0013cca0, 0x9f)
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

RVA(0x0013cd40, 0x5)
i32 CRezFileSingleFile::Open(const char* fileName, b32 readOnly, b32 createNew) {
    return 0;
}
RVA(0x0013cd50, 0x3)
i32 CRezFileSingleFile::Close() {
    return 0;
}

RVA(0x0013cd60, 0x49)
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

RVA(0x0013cdb0, 0x3)
i32 CRezFileSingleFile::VerifyFileOpen() {
    return 0;
}

RVA(0x0013cdc0, 0xad)
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
            m_pFile = fopen(m_sFileName, s_wPlusB);
        } else if (m_pDirEmulation->m_bReadOnly) {
            m_pFile = fopen(m_sFileName, "rb");
        } else {
            m_pFile = fopen(m_sFileName, s_rPlusB);
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

RVA(0x0013ce70, 0x7c)
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

RVA(0x0013cef0, 0x1)
void CRezFileSingleFile::VirtualFoo() {}

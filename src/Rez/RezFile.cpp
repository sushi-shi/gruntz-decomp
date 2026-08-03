#include <rva.h>

#include <Rez/RezFile.h>

#include <Bute/SymParser.h>
#include <Enums.h>
#include <Rez/RezList.h>
#include <Rez/RezMgr.h>

#include <stdio.h>

DATA(0x0021a0a4)
const char s_rPlusB[] = "r+b";
DATA(0x0021a0a8)
const char s_wPlusB[] = "w+b";

VTBL(CRezDir, 0x001ef7a8);
VTBL(CRezList, 0x001ef7c8);
VTBL(CRezFile, 0x001ef7d0);
DATA(0x0021a0a0)
const char g_wildcard[] = "*.*";

RVA(0x0013c4d0, 0x1)
void CRezList::UnusedListHook() {}

RVA(0x0013c4e0, 0x12)
CRezItmBase::CRezItmBase(void* parent) {

    m_parent = static_cast<CSymParser*>(parent);
}

RVA_COMPGEN(0x0013c500, 0x1e, ??_GCRezItmBase@@UAEPAXI@Z)

RVA(0x0013c520, 0xe)
CRezItmBase::~CRezItmBase() {
    m_parent = 0;
}

RVA(0x0013c530, 0x1)
void CRezItmBase::Noop() {}

RVA(0x0013c540, 0x28)
CRezItm::CRezItm(void* parent) : CRezItmBase(parent) {
    m_fp = 0;
    m_readBuf = 0;
    m_pos = -1;
}

RVA_COMPGEN(0x0013c570, 0x1e, ??_GCRezItm@@UAEPAXI@Z)

RVA(0x0013c590, 0x66)
CRezItm::~CRezItm() {
    if (m_fp != 0) {
        Close();
    }
    if (m_readBuf != 0) {
        ::operator delete(m_readBuf);
    }
}

RVA(0x0013c600, 0xbd)
i32 CRezItm::Read(i32 off, i32 base, u32 count, void* buf) {
    if (count <= 0) {
        return 0;
    }

    i32 pos = base + off;

    if (m_pos != pos) {
        while (fseek(m_fp, pos, 0) != 0) {
            if (m_parent->Retry() == 0) {
                m_pos = -1;
                return 0;
            }
        }
    }

    u32 got = fread(buf, 1, count, m_fp);
    while (got != count) {
        if (m_parent->Retry() == 0) {
            m_pos = -1;
            return 0;
        }
        got = fread(buf, 1, count, m_fp);
    }

    m_pos = got + pos;
    return got;
}

RVA(0x0013c6c0, 0x97)
i32 CRezItm::Write(i32 base, i32 off, u32 count, void* buf) {
    m_pos = -1;
    if (count <= 0) {
        return 0;
    }

    i32 pos = off + base;

    while (fseek(m_fp, pos, 0) != 0) {
        if (m_parent->Retry() == 0) {
            return 0;
        }
    }

    u32 put = fwrite(buf, 1, count, m_fp);
    while (put != count) {
        if (m_parent->Retry() == 0) {
            return 0;
        }
        put = fwrite(buf, 1, count, m_fp);
    }
    return put;
}

RVA(0x0013c760, 0xc1)
i32 CRezItm::Open(char* filename, i32 readonly, i32 write) {
    for (;;) {
        if (write) {
            if (readonly) {
                return 0;
            }
            m_fp = fopen(filename, s_wPlusB);
        } else if (readonly) {
            m_fp = fopen(filename, s_rb);
        } else {
            m_fp = fopen(filename, s_rPlusB);
        }
        if (m_fp != 0) {
            break;
        }
        if (m_parent->Retry() == 0) {
            return 0;
        }
        if (m_fp != 0) {
            break;
        }
    }

    m_readonly = readonly;
    if (m_readBuf != 0) {
        ::operator delete(m_readBuf);
    }
    m_readBuf = static_cast<char*>(::operator new(strlen(filename) + 1));
    if (m_readBuf != 0) {
        strcpy(m_readBuf, filename);
    }
    m_pos = -1;
    return 1;
}

// @early-stop
RVA(0x0013c830, 0x63)
i32 CRezItm::Close() {
    if (m_fp != 0) {
        i32 ok = 0;
        while (ok == 0) {
            if (fclose(m_fp) == 0) {
                ok = 1;
            } else if (m_parent->Retry() == 0) {
                return 0;
            }
        }

        m_fp = 0;
        if (m_readBuf != 0) {
            ::operator delete(m_readBuf);
        }
        m_readBuf = 0;
        m_pos = -1;
        return ok;
    }
    return 0;
}

RVA(0x0013c8a0, 0x45)
i32 CRezItm::Flush() {
    m_pos = -1;
    if (m_fp) {
        i32 found;
        do {
            if (fflush(m_fp) == 0) {
                found = 1;
            } else {
                found = 0;
                if (m_parent->Retry() == 0) {
                    return 0;
                }
            }
        } while (!found);
        return found;
    }
    return 0;
}

RVA(0x0013c8f0, 0x41)
i32 CRezItm::Check() {
    m_pos = -1;
    if (!m_fp) {
        return 0;
    }
    if (ftell(m_fp) != -1) {
        return 1;
    }
    return Open(m_readBuf, m_readonly, 0) != 0;
}

// @early-stop
RVA(0x0013c940, 0x46)
CRezDir::CRezDir(void* parent, i32 maxOpen) : CRezItmBase(parent) {
    m_openCount = 0;
    m_write = 0;
    m_readonly = 1;
    m_maxOpen = maxOpen;
}

RVA_COMPGEN(0x0013c990, 0x1e, ??_GCRezDir@@UAEPAXI@Z)

RVA(0x0013c9b0, 0x7f)
CRezDir::~CRezDir() {

    while (m_openList.m_head != 0) {
        delete m_openList.m_head;
    }
    while (m_closedList.m_head != 0) {
        delete m_closedList.m_head;
    }
}

RVA_COMPGEN(0x0013ca30, 0x7, ??1CRezList@@QAE@XZ)

RVA(0x0013ca40, 0x5)
i32 CRezDir::Read(i32 off, i32 base, u32 count, void* buf) {
    return 0;
}
RVA(0x0013ca50, 0x5)
i32 CRezDir::Write(i32 base, i32 off, u32 count, void* buf) {
    return 0;
}

RVA(0x0013ca60, 0x16)
i32 CRezDir::Open(char* name, i32 readonly, i32 write) {
    m_readonly = readonly;
    m_write = write;
    return 1;
}

RVA(0x0013ca80, 0x1d)
i32 CRezDir::Close() {

    while (m_openList.m_head != 0) {
        (static_cast<CRezFile*>(m_openList.m_head))->CloseFile();
    }
    return 1;
}

RVA(0x0013caa0, 0x6)
i32 CRezDir::Flush() {
    return 1;
}
RVA(0x0013cab0, 0x6)
i32 CRezDir::Check() {
    return 1;
}

RVA(0x0013cac0, 0x9b)
CRezFile::CRezFile(void* parent, char* nameSrc, CRezDir* dir) : CRezItmBase(parent) {
    m_dir = dir;
    m_handle = 0;

    char* buf = static_cast<char*>(::operator new(strlen(nameSrc) + 1));
    m_name = buf;
    strcpy(buf, nameSrc);

    m_dir->m_closedList.AddHead(this);
}

RVA_COMPGEN(0x0013cb60, 0x1e, ??_GCRezFile@@UAEPAXI@Z)

RVA(0x0013cb80, 0x72)
CRezFile::~CRezFile() {
    if (m_handle) {
        CloseFile();
    }
    if (m_name) {
        ::operator delete(m_name);
    }
    m_dir->m_closedList.Remove(this);
}

RVA(0x0013cc00, 0x9f)
i32 CRezFile::Read(i32 a, i32 pos, u32 count, void* buf) {
    static_cast<void>(a);
    if (count <= 0) {
        return 0;
    }
    if (m_handle == 0) {
        OpenFile();
    }
    while (fseek(m_handle, pos, 0) != 0) {
        if (m_dir->m_parent->Retry() == 0) {
            return 0;
        }
    }
    u32 got = fread(buf, 1, count, m_handle);
    while (got != count) {
        if (m_dir->m_parent->Retry() == 0) {
            return 0;
        }
        got = fread(buf, 1, count, m_handle);
    }
    return got;
}

RVA(0x0013cca0, 0x9f)
i32 CRezFile::Write(i32 a, i32 pos, u32 count, void* buf) {
    static_cast<void>(a);
    if (count <= 0) {
        return 0;
    }
    if (m_handle == 0) {
        OpenFile();
    }
    while (fseek(m_handle, pos, 0) != 0) {
        if (m_dir->m_parent->Retry() == 0) {
            return 0;
        }
    }
    u32 put = fwrite(buf, 1, count, m_handle);
    while (put != count) {
        if (m_dir->m_parent->Retry() == 0) {
            return 0;
        }
        put = fwrite(buf, 1, count, m_handle);
    }
    return put;
}

RVA(0x0013cd40, 0x5)
i32 CRezFile::Open(char* name, i32 readonly, i32 write) {
    return 0;
}
RVA(0x0013cd50, 0x3)
i32 CRezFile::Close() {
    return 0;
}

RVA(0x0013cd60, 0x49)
i32 CRezFile::Flush() {
    if (m_handle != 0) {
        i32 ok = (fflush(m_handle) == 0);
        while (!ok) {
            if (m_dir->m_parent->Retry() == 0) {
                return 0;
            }
            ok = (fflush(m_handle) == 0);
        }
        return ok;
    }
    return 1;
}

RVA(0x0013cdb0, 0x3)
i32 CRezFile::Check() {
    return 0;
}

RVA(0x0013cdc0, 0xad)
i32 CRezFile::OpenFile() {
    if (m_handle != 0) {
        return 1;
    }
    if (m_dir->m_openCount > m_dir->m_maxOpen) {

        CRezFile* lru = static_cast<CRezFile*>(m_dir->m_openList.m_tail);
        if (lru != 0) {
            lru->CloseFile();
        }
    }
    for (;;) {
        if (m_dir->m_write) {
            if (m_dir->m_readonly) {
                return 0;
            }
            m_handle = fopen(m_name, s_wPlusB);
        } else if (m_dir->m_readonly) {
            m_handle = fopen(m_name, s_rb);
        } else {
            m_handle = fopen(m_name, s_rPlusB);
        }
        if (m_handle != 0) {
            break;
        }
        if (m_dir->m_parent->Retry() == 0) {
            return 0;
        }
        if (m_handle != 0) {
            break;
        }
    }
    m_dir->m_closedList.Remove(this);
    m_dir->m_openList.AddHead(this);
    m_dir->m_openCount++;
    return 1;
}

RVA(0x0013ce70, 0x7c)
i32 CRezFile::CloseFile() {
    if (m_handle == 0) {
        return 1;
    }
    i32 ok = (fclose(m_handle) == 0);
    while (!ok) {
        if (m_dir->m_parent->Retry() == 0) {
            return 0;
        }
        ok = (fclose(m_handle) == 0);
    }
    m_dir->m_openCount--;
    m_dir->m_openList.Remove(this);
    m_dir->m_closedList.AddHead(this);
    m_handle = 0;
    return ok;
}

RVA(0x0013cef0, 0x1)
void CRezFile::Noop() {}

VTBL(CRezItmBase, 0x001ef768);
VTBL(CRezItm, 0x001ef788);

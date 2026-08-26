#include <rva.h>

#include <Mfc.h>

#include <DDrawMgr/ShadeTableCache.h>

RVA(0x00150460, 0xd)
CShadeTable::CShadeTable() {
    m_alloc = false;
    m_size = 0;
    m_data = NULL;
}

RVA(0x00150470, 0xb)
void CShadeTable::Reset() {
    if (m_alloc != false) {
        Free();
    }
}

RVA(0x00150480, 0x44)
i32 CShadeTable::Set(u32 size, i32 id) {
    if (m_data) {
        delete[] m_data;
    }
    m_size = size;
    m_data = new u8[size];
    if (!m_data) {
        return 0;
    }
    m_alloc = true;
    m_key = id;
    return 1;
}

RVA(0x001504d0, 0x54)
i32 CShadeTable::ReadFrom(CFile* file, i32 id) {
    file->Read(&m_size, sizeof(m_size));
    if (Set(m_size, id) == 0) {
        return 0;
    }
    file->Read(m_data, m_size);
    m_alloc = true;
    m_key = id;
    return 1;
}

RVA(0x00150530, 0xd1)
i32 CShadeTable::LoadFromFile(CString path, i32 id) {
    CFile file;
    if (!file.Open(path, CFile::modeRead, NULL)) {
        return 0;
    }
    i32 ok = ReadFrom(&file, id);
    file.Close();
    m_alloc = ok;
    m_key = id;
    return ok;
}

RVA(0x00150610, 0x87)
i32 CShadeTable::LoadFromMem(u8* buf, u32 len, i32 id) {
    CMemFile file(0x400);
    file.Attach(buf, len);
    i32 ok = ReadFrom(&file, id);
    m_alloc = ok;
    m_key = id;
    return ok;
}

RVA(0x001506a0, 0x2e)
void CShadeTable::Free() {
    if (m_alloc != false) {
        if (m_data) {
            delete[] m_data;
            m_data = NULL;
        }
        m_size = 0;
    }
    m_alloc = false;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x001506d0, 0xdc)
i32 CShadeTable::SaveToFile(CString path) {
    CFile file;
    if (!file.Open(path, CFile::modeCreate | CFile::modeWrite, NULL)) {
        return 0;
    }
    file.Write(&m_size, sizeof(m_size));
    file.Write(m_data, m_size);
    file.Close();
    return 1;
}

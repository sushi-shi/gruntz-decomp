#include <rva.h>

#include <Mfc.h>

#include <DDrawMgr/ShadeTableCache.h>

RVA(0x00150180, 0xd)
CShadeTable::CShadeTable() {
    m_alloc = 0;
    m_size = 0;
    m_data = NULL;
}

RVA(0x00150190, 0xb)
void CShadeTable::Reset() {
    if (m_alloc != 0) {
        Free();
    }
}

RVA(0x001501a0, 0x44)
i32 CShadeTable::Set(u32 size, i32 id) {
    if (m_data) {
        delete[] m_data;
    }
    m_size = size;
    m_data = new u8[size];
    if (!m_data) {
        return 0;
    }
    m_alloc = 1;
    m_key = id;
    return 1;
}

RVA(0x001501f0, 0x54)
i32 CShadeTable::ReadFrom(CFile* file, i32 id) {
    file->Read(&m_size, sizeof(m_size));
    if (Set(m_size, id) == 0) {
        return 0;
    }
    file->Read(m_data, m_size);
    m_alloc = 1;
    m_key = id;
    return 1;
}

RVA(0x00150250, 0xd1)
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

RVA(0x00150330, 0x87)
i32 CShadeTable::LoadFromMem(u8* buf, u32 len, i32 id) {
    CMemFile file(0x400);
    file.Attach(buf, len);
    i32 ok = ReadFrom(&file, id);
    m_alloc = ok;
    m_key = id;
    return ok;
}

RVA(0x001503c0, 0x2e)
void CShadeTable::Free() {
    if (m_alloc != 0) {
        if (m_data) {
            delete[] m_data;
            m_data = NULL;
        }
        m_size = 0;
    }
    m_alloc = 0;
}

RVA(0x001503f0, 0xdc)
i32 CShadeTable::SaveToFile(CString path) {
    CFile file;
    if (!file.Open(path, CFile::modeCreate | CFile::modeWrite, 0)) {
        return 0;
    }
    file.Write(&m_size, sizeof(m_size));
    file.Write(m_data, m_size);
    file.Close();
    return 1;
}

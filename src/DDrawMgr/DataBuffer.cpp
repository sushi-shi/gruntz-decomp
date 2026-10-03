#include <StdAfx.h>

#include <Ints.h>

#include <DDrawMgr/ShadeTableCache.h>
#include <SafeDelete.h>

CShadeTable::CShadeTable() {
    m_alloc = false;
    m_size = 0;
    m_data = NULL;
}

void CShadeTable::Reset() {
    if (m_alloc != false) {
        Free();
    }
}

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

i32 CShadeTable::LoadFromFile(std::string path, i32 id) {
    CFile file;
    if (!file.Open((path).c_str(), CFile::modeRead, NULL)) {
        return 0;
    }
    i32 ok = ReadFrom(&file, id);
    file.Close();
    m_alloc = ok;
    m_key = id;
    return ok;
}

i32 CShadeTable::LoadFromMem(u8* buf, u32 len, i32 id) {
    CMemFile file(0x400);
    file.Attach(buf, len);
    i32 ok = ReadFrom(&file, id);
    m_alloc = ok;
    m_key = id;
    return ok;
}

void CShadeTable::Free() {
    if (m_alloc != false) {
        SAFE_DELETE_ARRAY(m_data);
        m_size = 0;
    }
    m_alloc = false;
}

i32 CShadeTable::SaveToFile(std::string path) {
    CFile file;
    if (!file.Open((path).c_str(), CFile::modeCreate | CFile::modeWrite, NULL)) {
        return 0;
    }
    file.Write(&m_size, sizeof(m_size));
    file.Write(m_data, m_size);
    file.Close();
    return 1;
}

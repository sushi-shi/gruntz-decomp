#include <StdAfx.h>
#include <Io/File.h>

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

i32 CShadeTable::ReadFrom(io::Input& source, i32 id) {
    std::vector<unsigned char> bytes;
    if (!io::readSizedBytes(source, bytes)) return 0;
    if (!Set(static_cast<u32>(bytes.size()), id)) return 0;
    if (!bytes.empty()) memcpy(m_data, &bytes[0], bytes.size());
    return 1;
}

i32 CShadeTable::LoadFromFile(const std::string& path, i32 id) {
    io::File file;
    return file.open(path, io::ReadOnly) && ReadFrom(file, id);
}

i32 CShadeTable::LoadFromMem(u8* buf, u32 len, i32 id) {
    io::MemoryInput source(buf, len);
    return ReadFrom(source, id);
}

void CShadeTable::Free() {
    if (m_alloc != false) {
        SAFE_DELETE_ARRAY(m_data);
        m_size = 0;
    }
    m_alloc = false;
}

i32 CShadeTable::SaveToFile(const std::string& path) {
    io::File file;
    if (!file.open((path).c_str(), io::Replace)) {
        return 0;
    }
    return io::writeSizedBytes(file, m_data, m_size) && file.finish();
}

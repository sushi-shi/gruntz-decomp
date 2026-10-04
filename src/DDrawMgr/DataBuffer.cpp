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

i32 CShadeTable::ReadFrom(io::File* file, i32 id) {
    const u32 length = file->size();
    if (!file->good() || length < sizeof(m_size)
        || file->read(&m_size, sizeof(m_size)) != sizeof(m_size)
        || m_size > length - sizeof(m_size)) return 0;
    if (Set(m_size, id) == 0) {
        return 0;
    }
    if (file->read(m_data, m_size) != m_size) { Free(); return 0; }
    m_alloc = true;
    m_key = id;
    return 1;
}

i32 CShadeTable::LoadFromFile(const std::string& path, i32 id) {
    io::File file;
    if (!file.open((path).c_str(), io::ReadOnly)) {
        return 0;
    }
    i32 ok = ReadFrom(&file, id);
    file.finish();
    if (!ok) Free();
    m_key = id;
    return ok;
}

i32 CShadeTable::LoadFromMem(u8* buf, u32 len, i32 id) {
    if (!buf || len < sizeof(u32)) return 0;
    u32 size;
    memcpy(&size, buf, sizeof(size));
    if (size > len - sizeof(size) || !Set(size, id)) return 0;
    memcpy(m_data, buf + sizeof(size), size);
    return 1;
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
    file.write(&m_size, sizeof(m_size));
    file.write(m_data, m_size);
    return file.finish();
}

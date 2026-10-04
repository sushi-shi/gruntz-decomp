#ifndef GRUNTZ_PORTABLE_TEST
#include <StdAfx.h>
#endif
#include <Io/StreamArchive.h>

std::string CFileMemBase::GetName() {
    return m_name;
}

i32 CFileMemBase::WantRead() {
    return m_mode;
}

i32 CFileMemBase::WantCreate() {
    return m_mode == 0;
}

void CFileMemBase::Reset() {
    m_option = 0;
    m_mode = 0;
    (m_name).erase();
}

i32 CFileMemBase::SetName(const std::string& name, i32 mode, i32 option) {
    m_name = name;
    m_mode = mode;
    m_option = option;
    return 1;
}


CStreamArchive::CStreamArchive(io::Input& input)
    : m_input(&input), m_output(NULL), m_offset(0), m_good(false) { m_mode = 1; }
CStreamArchive::CStreamArchive(io::Output& output)
    : m_input(NULL), m_output(&output), m_offset(0), m_good(false) { m_mode = 0; }
i32 CStreamArchive::Open() {
    m_good = m_input ? m_input->good() : m_output->good();
    return m_good;
}
i32 CStreamArchive::Ready() {
    return m_good && (m_input ? m_input->good() : m_output->good());
}
i32 CStreamArchive::GetLength() {
    if (!m_input) return m_offset;
    const size_t length = m_input->size();
    if (!m_input->good() || length > 0x7fffffffU) { m_good = false; return 0; }
    return static_cast<i32>(length);
}
i32 CStreamArchive::GetOffset() { return m_offset; }
i32 CStreamArchive::Read(void* data, i32 count) {
    if (!m_good || !m_input || count < 0 || count > 0x7fffffff - m_offset) {
        m_good = false; return 0;
    }
    io::BinaryReader reader(*m_input);
    if (!reader.bytes(data, count)) { m_good = false; return 0; }
    m_offset += count;
    return 1;
}
i32 CStreamArchive::Write(const void* data, i32 count) {
    if (!m_good || !m_output || count < 0 || count > 0x7fffffff - m_offset
        || !m_output->write(data, count)) { m_good = false; return 0; }
    m_offset += count;
    return 1;
}
void CStreamArchive::Close() { m_good = false; }
void CStreamArchive::Reset() { m_good = false; m_offset = 0; CFileMemBase::Reset(); }

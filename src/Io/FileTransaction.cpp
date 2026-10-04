#ifndef GRUNTZ_PORTABLE_TEST
#include <StdAfx.h>
#endif
#include <Io/FileTransaction.h>

namespace io {
FileTransaction::FileTransaction(const std::string& destination)
    : m_good(false), m_finished(false), m_committed(false) {
    m_good = absolutePath(destination, m_destination)
        && m_file.createSibling(m_destination, m_temporary);
}
FileTransaction::~FileTransaction() {
    m_file.finish();
    if (!m_committed && !m_temporary.empty()) remove(m_temporary.c_str());
}
bool FileTransaction::write(const void* data, size_t count) {
    if (!good() || m_finished) { m_good = false; return false; }
    m_good = m_file.write(data, count);
    return m_good;
}
bool FileTransaction::finish() {
    if (!m_finished) {
        const bool closed = m_file.finish();
        m_good = m_good && closed;
        m_finished = true;
    }
    return m_good;
}
bool FileTransaction::commit() {
    if (m_committed || !finish()) return false;
    if (!replaceFile(m_temporary, m_destination)) { m_good = false; return false; }
    m_committed = true;
    return true;
}
bool FileTransaction::commitReferencing(FileTransaction& dependency) {
    if (&dependency == this || m_destination == dependency.m_temporary
        || !dependency.good() || !dependency.finish()) return false;
    if (!commit()) return false;
    return dependency.commitUnique();
}
bool FileTransaction::commitUnique() {
    if (m_committed || !finish()) return false;
    m_committed = true;
    return true;
}
}

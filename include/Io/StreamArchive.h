#ifndef GRUNTZ_STREAM_ARCHIVE_H
#define GRUNTZ_STREAM_ARCHIVE_H
#include <Io/FileMem.h>
#include <Io/Bytes.h>

// Borrows the source/sink for synchronous serialization; never closes its owner.
class CStreamArchive : public CFileMemBase {
public:
    explicit CStreamArchive(io::Input& input);
    explicit CStreamArchive(io::Output& output);
    virtual i32 GetLength();
    virtual i32 GetOffset();
    virtual i32 Open();
    virtual i32 Ready();
    virtual i32 Read(void* data, i32 count);
    virtual i32 Write(const void* data, i32 count);
    virtual void Close();
    virtual void Reset();
private:
    CStreamArchive(const CStreamArchive&);
    CStreamArchive& operator=(const CStreamArchive&);
    io::Input* m_input;
    io::Output* m_output;
    i32 m_offset;
    bool m_good;
};
#endif

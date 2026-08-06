#ifndef SRC_IO_FILEMEM_H
#define SRC_IO_FILEMEM_H

#include <rva.h>

#include <Mfc.h>

#include <Enums.h>
#include <Ints.h>
#include <Io/FileStream.h>

class CFileMemBase {
public:
    CFileMemBase();
    virtual ~CFileMemBase() {
        Reset();
    }
    virtual i32 SetName(const char* name, i32 a, i32 b);
    virtual void Close();

    virtual void Reset();
    virtual CString GetName();
    virtual i32 GetLength() = 0;
    virtual i32 GetOffset() = 0;
    virtual i32 WantRead();
    virtual i32 WantCreate();
    virtual i32 Open() = 0;
    virtual i32 Ready() = 0;
    virtual i32 Read(void* buf, i32 n) = 0;
    virtual i32 Write(const void* buf, i32 n) = 0;

    i32 m_4;
    i32 m_mode;
    CString m_name;
};
SIZE(0x10);

class CFileMem : public CFileMemBase {
public:
    virtual ~CFileMem() OVERRIDE {
        Reset();
    }
    virtual void Close() OVERRIDE;
    virtual void Reset() OVERRIDE;
    virtual i32 GetLength() OVERRIDE;
    virtual i32 GetOffset() OVERRIDE;
    virtual i32 Open() OVERRIDE;
    virtual i32 Ready() OVERRIDE;
    virtual i32 Read(void* buf, i32 n) OVERRIDE;
    virtual i32 Write(const void* buf, i32 n) OVERRIDE;

    CFile m_file;
    i32 m_length;
    i32 m_offset;
};
SIZE(0x28);

#endif // SRC_IO_FILEMEM_H

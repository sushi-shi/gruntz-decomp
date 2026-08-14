#ifndef SRC_IO_FILEMEM_H
#define SRC_IO_FILEMEM_H

#include <rva.h>

#include <Mfc.h>

#include <Enums.h>
#include <Ints.h>
#include <Io/FileStream.h>

class CFileMemBase {
public:
    RVA(0x00157850, 0x54)
    CFileMemBase() {
        m_option = 0;
        m_mode = 0;
        m_name.Empty();
    }
    virtual ~CFileMemBase() {
        Close();
    }
    virtual i32 SetName(const char* name, i32 mode, i32 option);
    RVA(0x00157910, 0x5)
    virtual void Close() {
        Reset();
    }

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

    i32 m_option;
    i32 m_mode;
    CString m_name;
};

class CFileMem : public CFileMemBase {
public:
    CFileMem() {
        Reset();
    }
    virtual ~CFileMem() OVERRIDE {
        Close();
    }
    RVA(0x00157a70, 0x5)
    virtual void Close() OVERRIDE {
        Reset();
    }
    RVA(0x00157a50, 0x16)
    virtual void Reset() OVERRIDE {
        m_length = 0;
        m_offset = 0;
        m_option = 0;
        m_mode = 0;
        m_name.Empty();
    }
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

#endif // SRC_IO_FILEMEM_H

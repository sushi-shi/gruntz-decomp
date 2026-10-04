#ifndef SRC_IO_FILEMEM_H
#define SRC_IO_FILEMEM_H
#include <Io/File.h>

#include <string>

#include <Ints.h>

#include <Enums.h>
#include <Ints.h>
#include <Io/FileStream.h>

class CFileMemBase {
public:

    CFileMemBase() {
        m_option = 0;
        m_mode = 0;
        (m_name).erase();
    }
    virtual ~CFileMemBase() {
        Close();
    }
    virtual i32 SetName(const std::string& name, i32 mode, i32 option);

    virtual void Close() {
        Reset();
    }

    virtual void Reset();
    virtual std::string GetName();
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
    std::string m_name;
};

class CFileMem : public CFileMemBase {
public:
    CFileMem() {
        Reset();
    }
    virtual ~CFileMem()   {
        Close();
    }

    virtual void Close()   {
        Reset();
    }

    virtual void Reset()   {
        m_file.finish();
        m_length = 0;
        m_offset = 0;
        m_option = 0;
        m_mode = 0;
        (m_name).erase();
    }
    virtual i32 GetLength()  ;
    virtual i32 GetOffset()  ;
    virtual i32 Open()  ;
    virtual i32 Ready()  ;
    virtual i32 Read(void* buf, i32 n)  ;
    virtual i32 Write(const void* buf, i32 n)  ;

    io::File m_file;
    i32 m_length;
    i32 m_offset;
};

#endif

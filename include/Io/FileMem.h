#ifndef SRC_IO_FILEMEM_H
#define SRC_IO_FILEMEM_H
#include <rva.h>

#include <Ints.h>
#include <Mfc.h> // CString + <windows.h>

#include <Io/FileStream.h> // CFileIO (the embedded physical file)

class CFileMemBase {
public:
    CFileMemBase();
    virtual ~CFileMemBase();                             // slot 0  (0x157960 ??_G)
    virtual i32 SetName(const char* name, i32 a, i32 b); // slot 1  0x00165e30
    virtual void Close();                        // slot 2  0x157910: tail-jmp slot 3 (Reset)
    // OUT-OF-LINE (body in DDrawSubMgr.cpp @0x157a40): retail is a real function called
    // directly - an inline body here would let cl inline it at every known-type call site
    // (e.g. CFileMem S; S.Reset()) where retail emits `call 0x157a40/0x157a50`.
    virtual void Reset();                          // slot 3  0x00157a40
    virtual CString GetName();                     // slot 4  0x157920 (return m_name copy)
    virtual void GetLength() = 0;              // slot 5  __purecall (CFileMem: return m_length)
    virtual void GetOffset() = 0;              // slot 6  __purecall (CFileMem: return m_offset)
    virtual i32 WantRead();                        // slot 7  (0x157940) read-vs-create gate
    virtual void WantCreate();                  // slot 8  0x157950: return m_mode == 0 (mode 0 = create)
    virtual i32 Open() = 0;                        // slot 9  __purecall
    virtual i32 Ready() = 0;                       // slot 10 __purecall
    virtual i32 Read(void* buf, i32 n) = 0;        // slot 11 __purecall
    virtual i32 Write(const void* buf, i32 n) = 0; // slot 12 __purecall

    // vptr implicit @ +0x00
    i32 m_4;        // +0x04
    i32 m_mode;        // +0x08
    CString m_name; // +0x0c
};
SIZE(CFileMemBase, 0x10); // vptr + m_4 + m_8 + CString (base sub-object; m_file follows at +0x10)
VTBL(CFileMemBase, 0x001efe68);

class CFileMem : public CFileMemBase {
public:
    virtual ~CFileMem() OVERRIDE;                // slot 0  0x00157980 (real ~ / ??_G 0x157a20)
    virtual void Close() OVERRIDE;       // slot 2  (0x157a70)
    virtual void Reset() OVERRIDE;               // slot 3  0x00157a50 (out-of-line; see base Reset)
    virtual void GetLength() OVERRIDE;       // slot 5
    virtual void GetOffset() OVERRIDE;       // slot 6
    virtual i32 Open() OVERRIDE;                 // slot 9  0x00165e60
    virtual i32 Ready() OVERRIDE;                // slot 10 0x00165ef0
    virtual i32 Read(void* buf, i32 n) OVERRIDE; // slot 11 0x00165f00
    virtual i32 Write(const void* buf, i32 n) OVERRIDE; // slot 12 0x00165f50

    CFileIO m_file; // +0x10
    i32 m_length;   // +0x20
    i32 m_offset;   // +0x24
};
SIZE(CFileMem, 0x28); // base 0x10 + CFileIO m_file 0x10 + m_length + m_offset
VTBL(CFileMem, 0x001efe30);

#endif // SRC_IO_FILEMEM_H

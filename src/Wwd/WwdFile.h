// WwdFile.h - Gruntz WWD level-file loader (Monolith-faithful reconstruction).
//
// On-disk WWD header layout is pinned in structure/formats/wwd.h; this header
// reproduces the load-bearing fields the loaders actually touch.
#ifndef SRC_WWD_WWDFILE_H
#define SRC_WWD_WWDFILE_H

typedef unsigned char  Bytef;
typedef unsigned long  uLong;
typedef unsigned long  uLongf;

// WWD file header (1524 = 0x5F4 bytes on disk). Only the fields the loaders
// read are named; the rest is padding to preserve offsets. See formats/wwd.h.
struct WwdHeader
{
    unsigned int wwdSignature;      // +0x000  == 1524 (header size)
    unsigned int field_4;           // +0x004
    unsigned int flags;             // +0x008  bit1 (0x2) = COMPRESS
    unsigned char pad_c[0x2e8 - 0x0c];
    unsigned int mainBlockLength;   // +0x2E8  inflated main-block size
    unsigned char pad_2ec[0x5f4 - 0x2ec];
};

// ---------------------------------------------------------------------------
// Engine binary file stream (16 bytes). The validators construct one on the
// stack, Open() a file, Read() the header, then let the destructor (Close) run.
// All four members are UNMATCHED engine functions (ctor @0x1befd7, dtor @0x1bf121,
// Open @0x1bf200, Read @0x1bf328); their call bytes are reloc-masked in objdiff.
// Layout: vtable@+0, HANDLE@+4, openFlag@+8, CString filename@+0xc.
//   - ctor sets up the vtable + CString and leaves HANDLE = -1.
//   - Open(name, mode, errSink) opens via CreateFileA; returns nonzero on
//     success, 0 on failure (the engine's reversed sense).
//   - Read(buf, len) ReadFiles len bytes into buf; returns the count read.
//   - the destructor CloseHandle()s an open handle (HANDLE != -1).
// ---------------------------------------------------------------------------
class WwdInputStream
{
public:
    WwdInputStream();
    ~WwdInputStream();
    int Open(const char* name, int mode, void* errSink);
    int Read(void* buf, int len);

private:
    void*        m_vtbl;     // +0x00
    void*        m_handle;   // +0x04  HANDLE (-1 when closed)
    int          m_open;     // +0x08  open/refcount flag
    void*        m_name;     // +0x0C  CString filename buffer
};

// zlib one-shot decompressor (matched, vendor/zlib-1.0.4/uncompr.c @0x185320).
extern "C" int uncompress(Bytef* dest, uLongf* destLen, const Bytef* source, uLong sourceLen);

// Inflate the WWD main block (planes/tiles) in place into a caller buffer.
// __stdcall free function (callee cleans 12 bytes).
extern "C" int __stdcall WwdFile_InflateMainBlock(WwdHeader* src, Bytef* dest, unsigned int destLen);

// Validate a WWD file by name: open it, read the 1524-byte header into the
// caller buffer, require read == 0x5F4 and header signature <= 0x5F4.
// __stdcall free function (callee cleans 8 bytes); returns 1 / 0.
int __stdcall WwdFile_IsValidWwd(const char* name, void* headerBuf);

// Validate a WWD file by name into a private 1524-byte buffer, then copy that
// header into the caller buffer. __stdcall (callee cleans 8 bytes); returns 1 / 0.
int __stdcall WwdFile_CheckHeader(const char* name, void* headerOut);

#endif // SRC_WWD_WWDFILE_H

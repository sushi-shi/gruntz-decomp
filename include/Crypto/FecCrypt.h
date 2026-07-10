// FecCrypt.h - CFecFile, the "FEC File" encrypted-resource archive reader/writer
// (0x17b5f0 references the "Opened FEC File %s" diagnostics) AND the CMoviePlayer decode
// store (it is CMoviePlayer's embedded m_540 sub-object). Formerly modeled under three
// names - CFecFile (FecCrypt.cpp), CPageStore17b510 (BoundaryUpper2Views.h) and
// CMovieDecodeStore (MoviePlayer.h) - all the SAME object (same +0x124 polymorphic
// stream, +0x138 CDWordArray index). Unified here as the single-source shape.
//
// Field names are placeholders; the offsets, the vtable SLOT offsets and the code bytes
// are the load-bearing facts (campaign doctrine).
#ifndef CRYPTO_FECCRYPT_H
#define CRYPTO_FECCRYPT_H

#include <Ints.h>
#include <Mfc.h> // CDWordArray, CFile, MSG/PeekMessageA (windows.h)

// The stream subobject at CFecFile+0x124 is the embedded MFC CFile (recovered identity):
//   - CMoviePlayer::~CMoviePlayer (0x38fc0) and CCredits390a0::~ (0x390a0) both run
//     ~CFile in-place on +0x124 (see src/Io/MoviePlayer.cpp);
//   - its virtual-dispatch slots Open@+0x28 / Seek@+0x30 / Read@+0x3c / Write@+0x40 /
//     Close@+0x50 / Abort@+0x54 are exactly MFC CFile's vtable (cf. the local CFile in
//     AddFile whose non-virtual Open/Seek/Read are 0x1bf200/0x1bf3ad/0x1bf328).
// It is modeled here as a DECLARED-ONLY polymorphic proxy (not a concrete `CFile m_stream`
// member) on purpose: the archive methods reach it through a CFile* handle and dispatch
// VIRTUALLY (`call [eax+0x28]`), whereas an exact-type `CFile` member devirtualizes to
// direct library calls - binary-disproven (it would regress every archive method). The
// declared-only virtuals are the load-bearing device that forces retail's vtable dispatch.
class FecStream {
public:
    virtual void Slot00();
    virtual void Slot04();
    virtual void Slot08();
    virtual void Slot0C();
    virtual void Slot10();
    virtual void Slot14();
    virtual void Slot18();
    virtual void Slot1C();
    virtual void Slot20();
    virtual void Slot24();
    virtual i32 Open(const char* name, i32 a2, i32 a3); // +0x28  CFile::Open
    virtual void Slot2C();                              // +0x2c
    virtual i32 Seek(i32 off, i32 origin);              // +0x30  CFile::Seek
    virtual void Slot34();
    virtual void Slot38();
    virtual i32 Read(void* buf, i32 size);        // +0x3c  CFile::Read
    virtual i32 Write(const void* buf, i32 size); // +0x40  CFile::Write
    virtual void Slot44();
    virtual void Slot48();
    virtual void Slot4C();
    virtual void Close(); // +0x50  CFile::Close
    virtual void Abort(); // +0x54  CFile::Abort (OnFail drops the active stream through this)
};

class CFecFile {
public:
    // Init/Close/Lookup are the CMoviePlayer-driven decode-store lifecycle (this same
    // object is CMoviePlayer's m_540 "decode store"): Init resets + arms the store,
    // Close tears it down, Lookup resolves an entry offset. OnFail (0x17b5a0) is the
    // read-fail teardown ReadArchive/Close both call. AddFile/ExtractArchive are the
    // build/unpack halves of the archive (CFile-local file + CDWordArray offset index).
    i32 Init();                          // 0x17b510  reset + arm (open-gate -> 1)
    void Close();                        // 0x17b570  teardown (OnFail + reset + gate -> 0)
    i32 Lookup(u32 idx);                 // 0x17b840  resolve entry idx (1-based) -> m_128
    i32 CreateArchive(const char* name); // 0x17b8a0
    i32 ReadArchive(const char* name);   // 0x17b5f0
    i32 OnFail();                        // 0x17b5a0
    i32 AddFile(const char* name, i32* pCancel, void* pProgress);       // 0x17b950
    i32 ExtractArchive(const char* dir, i32* pCancel, void* pProgress); // 0x17bcd0

    i32 m_00;           // +0x00  open-gate (must be nonzero)
    i32 m_04;           // +0x04  read-open flag
    i32 m_08;           // +0x08  write-open flag
    i32 m_0c;           // +0x0c  version major (12-byte header word 0)
    i32 m_10;           // +0x10  version minor
    i32 m_14;           // +0x14  entry count
    char m_18[0x10c];   // +0x18  per-entry record (index/m_1c namelen/m_1e name/m_11e/m_120)
    FecStream m_stream; // +0x124
    i32 m_128;          // +0x128  Lookup's success result
    i32 _pad12c[(0x134 - 0x12c) / 4];
    i32 m_134;           // +0x134  running entry counter (write path); cleared by Init/OnFail
    CDWordArray m_index; // +0x138  per-entry offset table (m_pData @+0x13c, m_nSize @+0x140)
    char m_14c[0x8000];  // +0x14c  32 KB streaming copy buffer
};

// The m_18 record's internal fields (a serialized on-disk 0x10c-byte record):
//   +0x00 i32 index | +0x04 u16 nameLen | +0x06 char name[0x100] |
//   +0x106 u16 scramble (rand()%0x400 + 0x2b8) | +0x108 i32 payloadLen
#define FEC_NAMELEN(p) (*(u16*)((char*)(p) + 0x1c))
#define FEC_NAME(p) ((char*)(p) + 0x1e)
#define FEC_W(p) (*(u16*)((char*)(p) + 0x11e))      // scramble word (m_11e)
#define FEC_STRIDE(p) (*(i32*)((char*)(p) + 0x120)) // payload length (m_120)

#endif // CRYPTO_FECCRYPT_H

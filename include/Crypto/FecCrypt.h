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
//   - ~CFecFile (0x390a0) runs ~CFile in-place on +0x124, and ~CMoviePlayer (0x38fc0)
//     inlines that same dtor for its m_540 store (see src/Io/MoviePlayer.cpp);
//   - the archive methods dispatch exactly MFC CFile's vtable slots (RTTI-backed retail
//     vtable @0x1ed15c): Open@+0x28 / Seek@+0x30 / Read@+0x3c / Write@+0x40 /
//     Flush@+0x50 / Close@+0x54 (the pre-2026-07-13 model misread +0x50/+0x54 as
//     Close/Abort - the real afx.h declaration order puts Flush at 20 and Close at 21);
//   - the layout fits byte-exactly: sizeof(CFile)==0x10 {vptr, m_hFile @+4,
//     m_bCloseOnDelete @+8, m_strFileName @+0xc}, so the old "+0x128 Lookup success
//     result" member IS m_stream.m_hFile - Lookup returns the raw Win32 file HANDLE.
// Modeled as a concrete `CFile` member: MSVC5 does NOT devirtualize member-object
// virtual calls (measured - OnFail/Init were byte-exact with virtual dispatch through
// the member), so `m_stream.Read(...)` lowers to the same `mov eax,[this+0x124];
// call [eax+0x3c]` retail has. (The former `FecStream` declared-only proxy - and its
// devirtualization fear - is dissolved; the fear was disproven by the same measurement.)

class CFecFile {
public:
    // Init/Close/Lookup are the CMoviePlayer-driven decode-store lifecycle (this same
    // object is CMoviePlayer's m_540 "decode store"): Init resets + arms the store,
    // Close tears it down, Lookup resolves an entry offset. OnFail (0x17b5a0) is the
    // read-fail teardown ReadArchive/Close both call. AddFile/ExtractArchive are the
    // build/unpack halves of the archive (CFile-local file + CDWordArray offset index).
    // The dtor (Close + the member teardown) is DEFINED in the movie TU
    // (src/Io/MoviePlayer.cpp) at 0x0390a0; ~CMoviePlayer (same retail TU) inlines it.
    ~CFecFile(); // 0x0390a0  { Close(); } + ~CDWordArray(+0x138) + ~CFile(+0x124)
    i32 Init();                          // 0x17b510  reset + arm (open-gate -> 1)
    void Close();                        // 0x17b570  teardown (OnFail + reset + gate -> 0)
    i32 Lookup(u32 idx);                 // 0x17b840  resolve entry idx (1-based) -> m_128
    i32 CreateArchive(const char* name); // 0x17b8a0
    i32 ReadArchive(const char* name);   // 0x17b5f0
    i32 OnFail();                        // 0x17b5a0
    i32 AddFile(const char* name, i32* pCancel, void* pProgress);       // 0x17b950
    i32 ExtractArchive(const char* dir, i32* pCancel, void* pProgress); // 0x17bcd0

    i32 m_00;         // +0x00  open-gate (must be nonzero)
    i32 m_04;         // +0x04  read-open flag
    i32 m_08;         // +0x08  write-open flag
    i32 m_0c;         // +0x0c  version major (12-byte header word 0)
    i32 m_10;         // +0x10  version minor
    i32 m_14;         // +0x14  entry count
    char m_18[0x10c]; // +0x18  per-entry record (index/m_1c namelen/m_1e name/m_11e/m_120)
    // +0x124  the embedded MFC CFile (0x10 B: vptr, m_hFile @+0x128, m_bCloseOnDelete
    // @+0x12c, m_strFileName @+0x130). Lookup's "+0x128 success result" is m_stream.m_hFile.
    CFile m_stream;
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

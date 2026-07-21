#ifndef CRYPTO_FECCRYPT_H
#define CRYPTO_FECCRYPT_H

#include <Ints.h>
#include <Mfc.h> // CDWordArray, CFile, MSG/PeekMessageA (windows.h)

struct FecEntry {
    i32 m_index;        // +0x00
    u16 m_nameLen;      // +0x04
    char m_name[0x100]; // +0x06
    u16 m_scramble;     // +0x106
    i32 m_payloadLen;   // +0x108
};

class CFecFile {
public:
    // Init/Close/Lookup are the CMoviePlayer-driven decode-store lifecycle (this same
    // object is CMoviePlayer's m_540 "decode store"): Init resets + arms the store,
    // Close tears it down, Lookup resolves an entry offset. OnFail (0x17b5a0) is the
    // read-fail teardown ReadArchive/Close both call. AddFile/ExtractArchive are the
    // build/unpack halves of the archive (CFile-local file + CDWordArray offset index).
    // The dtor (Close + the member teardown) is DEFINED in the movie TU
    // (src/Io/MoviePlayer.cpp) at 0x0390a0; ~CMoviePlayer (same retail TU) inlines it.
    ~CFecFile();         // 0x0390a0  { Close(); } + ~CDWordArray(+0x138) + ~CFile(+0x124)
    i32 Init();          // 0x17b510  reset + arm (open-gate -> 1)
    void Close();        // 0x17b570  teardown (OnFail + reset + gate -> 0)
    i32 Lookup(u32 idx); // 0x17b840  resolve entry idx (1-based) -> m_128
    i32 CreateArchive(const char* name);                                // 0x17b8a0
    i32 ReadArchive(const char* name);                                  // 0x17b5f0
    i32 OnFail();                                                       // 0x17b5a0
    i32 AddFile(const char* name, i32* pCancel, void* pProgress);       // 0x17b950
    i32 ExtractArchive(const char* dir, i32* pCancel, void* pProgress); // 0x17bcd0

    i32 m_openGate;         // +0x00  open-gate (must be nonzero)
    i32 m_readOpen;         // +0x04  read-open flag
    i32 m_writeOpen;         // +0x08  write-open flag
    i32 m_versionMajor;         // +0x0c  version major (12-byte header word 0)
    i32 m_versionMinor;         // +0x10  version minor
    i32 m_fileCount;         // +0x14  entry count
    FecEntry m_entry; // +0x18  per-entry record (0x10c B; typed above, streamed as a blob)
    // +0x124  the embedded MFC CFile (0x10 B: vptr, m_hFile @+0x128, m_bCloseOnDelete
    // @+0x12c, m_strFileName @+0x130). Lookup's "+0x128 success result" is m_stream.m_hFile.
    CFile m_stream;
    i32 m_nextIndex;           // +0x134  running entry counter (write path); cleared by Init/OnFail
    CDWordArray m_index; // +0x138  per-entry offset table (m_pData @+0x13c, m_nSize @+0x140)
    char m_copyBuf[0x8000];  // +0x14c  32 KB streaming copy buffer
};

#endif // CRYPTO_FECCRYPT_H

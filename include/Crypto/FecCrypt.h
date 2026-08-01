#ifndef CRYPTO_FECCRYPT_H
#define CRYPTO_FECCRYPT_H

#include <Ints.h>
#include <rva.h>
#include <Mfc.h>

struct FecEntry {
    i32 m_index;
    u16 m_nameLen;
    char m_name[0x100];
    u16 m_scramble;
    i32 m_payloadLen;
};
SIZE(0x10c);

class CFecFile {
public:
    CFecFile();
    ~CFecFile();
    i32 Init();
    void Close();
    i32 Lookup(u32 idx);
    i32 CreateArchive(const char* name);
    i32 ReadArchive(const char* name);
    i32 OnFail();
    i32 AddFile(const char* name, i32* pCancel, void* pProgress);
    i32 ExtractArchive(const char* dir, i32* pCancel, void* pProgress);

    void FecEncode(const char* src, char* dst);
    void FecDecode(const char* src, char* dst, u16 len);

    i32 m_openGate;
    i32 m_readOpen;
    i32 m_writeOpen;
    i32 m_versionMajor;
    i32 m_versionMinor;
    i32 m_fileCount;
    FecEntry m_entry;

    CFile m_stream;
    i32 m_nextIndex;
    CDWordArray m_index;
    char m_copyBuf[0x8000];
};
SIZE(0x814c);

#endif // CRYPTO_FECCRYPT_H

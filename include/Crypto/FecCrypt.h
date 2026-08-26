#ifndef CRYPTO_FECCRYPT_H
#define CRYPTO_FECCRYPT_H

#include <rva.h>

#include <Mfc.h>

#include <Enums.h>
#include <Ints.h>

GZ_ENUM_CONST_BEGIN(FecFormatSize)
    FEC_MAGIC_SIZE = 3,
    FEC_ENTRY_NAME_CAPACITY = 0x100,
    FEC_RANDOM_BYTE_MODULUS = 0xff,
    FEC_SCRAMBLE_BASE = 0x2b8,
    FEC_SCRAMBLE_RANGE = 0x400,
    FEC_COPY_BUFFER_SIZE = 0x8000
GZ_ENUM_CONST_END(FecFormatSize)

struct FecEntry {
    i32 m_index;
    u16 m_nameLen;
    char m_name[FEC_ENTRY_NAME_CAPACITY];
    u16 m_scramble;
    i32 m_payloadLen;
};

struct FecArchiveHeader {
    i32 m_versionMajor;
    i32 m_versionMinor;
    i32 m_fileCount;
};

GZ_ENUM_CONST_BEGIN(FecFormatOffset)
    FEC_ENTRY_TABLE_OFFSET = FEC_MAGIC_SIZE + sizeof(FecArchiveHeader),
    FEC_FILE_COUNT_OFFSET = FEC_MAGIC_SIZE + sizeof(i32) * 2,
    FEC_FIRST_PAYLOAD_ADJUSTMENT = FEC_SCRAMBLE_BASE - (FEC_ENTRY_TABLE_OFFSET + sizeof(FecEntry)),
    FEC_NEXT_PAYLOAD_ADJUSTMENT = FEC_SCRAMBLE_BASE - sizeof(FecEntry)
GZ_ENUM_CONST_END(FecFormatOffset)

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

    i32 Random();

    void FecEncode(const char* src, char* dst);
    void FecDecode(const char* src, char* dst, u16 len);

    b32 m_openGate;
    b32 m_readOpen;
    b32 m_writeOpen;
    FecArchiveHeader m_header;
    FecEntry m_entry;

    CFile m_stream;
    i32 m_nextIndex;
    CDWordArray m_index;
    char m_copyBuf[FEC_COPY_BUFFER_SIZE];
};

inline CFecFile::~CFecFile() {
    Close();
}

#endif // CRYPTO_FECCRYPT_H

#include <rva.h>

#include <Crypto/FecCrypt.h>

#include <Enums.h>
#include <Ints.h>

#include <direct.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

RVA(0x0017b510, 0x55)
i32 CFecFile::Init() {
    if (m_openGate) {
        return 0;
    }
    m_readOpen = false;
    m_writeOpen = false;
    m_index.SetSize(0, -1);
    memset(&m_header, 0, sizeof(m_header));
    memset(&m_entry, 0, sizeof(m_entry));
    m_nextIndex = 0;
    m_openGate = true;
    return 1;
}

RVA(0x0017b570, 0x24)
void CFecFile::Close() {
    if (!m_openGate) {
        return;
    }
    OnFail();
    m_index.SetSize(0, -1);
    m_openGate = false;
}

RVA(0x0017b5a0, 0x48)
i32 CFecFile::OnFail() {
    if (m_openGate && (m_readOpen || m_writeOpen)) {
        m_stream.Close();
        m_readOpen = false;
        m_writeOpen = false;
        m_nextIndex = 0;
        return 1;
    }
    return 0;
}

// @early-stop
RVA(0x0017b5f0, 0x249)
i32 CFecFile::ReadArchive(const char* name) {
    if (name == NULL) {
        return 0;
    }
    if (m_readOpen != false) {
        return 0;
    }
    if (m_openGate == false) {
        return 0;
    }
    if (m_stream.Open(name, 0, NULL) == false) {
        return 0;
    }
    m_readOpen = true;

    char magic[FEC_MAGIC_SIZE];
    if (m_stream.Read(magic, sizeof(magic)) != sizeof(magic)) {
        goto fail;
    }
    if (magic[0] != 'F' || magic[1] != 'E' || magic[2] != 'C') {
        goto fail;
    }
    if (m_stream.Read(&m_header, sizeof(m_header)) != sizeof(m_header)) {
        goto fail;
    }

    char buf[FEC_ENTRY_NAME_CAPACITY];
    sprintf(buf, "Opened FEC File %s\n", name);
    sprintf(
        buf,
        "FEC File Version: %d.%d\nNumber of Files: %d\n",
        m_header.m_versionMajor,
        m_header.m_versionMinor,
        m_header.m_fileCount
    );

    if (m_stream.Read(&m_entry, sizeof(m_entry)) != sizeof(m_entry)) {
        goto fail;
    }
    {
        if (m_stream.Seek(m_entry.m_scramble - FEC_SCRAMBLE_BASE, CFile::current)
            != m_entry.m_scramble - FEC_FIRST_PAYLOAD_ADJUSTMENT) {
            goto fail;
        }
        m_index.Add(m_entry.m_scramble - FEC_FIRST_PAYLOAD_ADJUSTMENT);

        for (u16 i = 1; i < static_cast<u32>(m_header.m_fileCount); i++) {
            i32 stride = m_entry.m_payloadLen;
            if (m_stream.Seek(stride, CFile::current)
                != static_cast<i32>(m_index[i - 1]) + stride) {
                goto fail;
            }
            memset(&m_entry, 0, sizeof(m_entry));
            if (m_stream.Read(&m_entry, sizeof(m_entry)) != sizeof(m_entry)) {
                goto fail;
            }
            u16 scr = m_entry.m_scramble;
            if (m_stream.Seek(scr - FEC_SCRAMBLE_BASE, CFile::current)
                != static_cast<i32>(m_index[i - 1]) + stride + scr - FEC_NEXT_PAYLOAD_ADJUSTMENT) {
                goto fail;
            }
            m_index.Add(
                static_cast<i32>(m_index[i - 1]) + stride + scr - FEC_NEXT_PAYLOAD_ADJUSTMENT
            );
        }
    }
    return 1;

fail:
    OnFail();
    return 0;
}

RVA(0x0017b840, 0x53)
i32 CFecFile::Lookup(u32 idx) {
    if (m_readOpen && m_openGate && idx <= static_cast<u32>(m_header.m_fileCount) && idx != 0) {
        const DWORD* slot = &m_index.GetData()[idx - 1];
        if (m_stream.Seek(static_cast<i32>(*slot), CFile::begin) == static_cast<i32>(*slot)) {
            return m_stream.m_hFile;
        }
    }
    return 0;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0017b8a0, 0xa2)
i32 CFecFile::CreateArchive(const char* name) {
    if (name != NULL && m_writeOpen == false && m_openGate != false
        && m_stream.Open(name, CFile::modeCreate | CFile::modeReadWrite, NULL) != false) {
        m_writeOpen = true;

        char magic[FEC_MAGIC_SIZE];
        magic[0] = 'F';
        magic[1] = 'E';
        magic[2] = 'C';
        m_stream.Write(magic, sizeof(magic));

        memset(&m_header, 0, sizeof(m_header));
        m_header.m_fileCount = 0;
        m_header.m_versionMajor = 1;
        m_header.m_versionMinor = 1;
        m_stream.Write(&m_header, sizeof(m_header));
        m_stream.Flush();
        return 1;
    }
    return 0;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0017b950, 0x380)
i32 CFecFile::AddFile(const char* name, i32* pCancel, void* pProgress) {
    i32 i;
    MSG msg;
    if (m_writeOpen == false || m_openGate == false) {
        return 0;
    }

    CFile file;
    if (file.Open(name, 0, NULL) == false) {
        return 0;
    }

    m_nextIndex++;

    CString base = name;
    i32 slash = base.ReverseFind('\\');
    if (slash != -1) {
        base = base.Right(base.GetLength() - slash - 1);
    }

    memset(&m_entry, 0, sizeof(m_entry));
    m_entry.m_index = m_nextIndex;
    m_entry.m_nameLen = static_cast<u16>(base.GetLength());

    char* enc = new char[base.GetLength() + 1];
    FecEncode(base, enc);
    memcpy(m_entry.m_name, enc, base.GetLength());
    delete[] enc;

    i32 length = base.GetLength();
    if (length < FEC_ENTRY_NAME_CAPACITY) {

        char* p = m_entry.m_name + length;
        i32 c = FEC_ENTRY_NAME_CAPACITY - length;
        do {
            memset(p, static_cast<u8>(Random() % FEC_RANDOM_BYTE_MODULUS), sizeof(*p));
            p++;
        } while (--c);
    }

    m_entry.m_scramble = static_cast<u16>((Random() % FEC_SCRAMBLE_RANGE + FEC_SCRAMBLE_BASE));
    m_entry.m_payloadLen = file.Seek(0, CFile::end);
    if (file.Seek(0, CFile::begin) != 0) {
        m_nextIndex--;
        return 0;
    }

    m_stream.Seek(0, CFile::end);
    m_stream.Write(&m_entry, sizeof(m_entry));

    char* pad = new char[m_entry.m_scramble - FEC_SCRAMBLE_BASE];
    for (i = 0; i < m_entry.m_scramble - FEC_SCRAMBLE_BASE; i++) {
        pad[i] = static_cast<char>((Random() % FEC_RANDOM_BYTE_MODULUS));
    }
    m_stream.Write(pad, m_entry.m_scramble - FEC_SCRAMBLE_BASE);
    delete[] pad;

    memset(m_copyBuf, 0, sizeof(m_copyBuf));
    b32 done = false;
    u32 copied = 0;
    while (done == false) {
        if (pProgress != NULL) {
            if (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessageA(&msg);
            }
        }

        if (*pCancel != 0) {
            return 0;
        }
        u32 chunk;
        if (copied + FEC_COPY_BUFFER_SIZE > static_cast<u32>(m_entry.m_payloadLen)) {
            chunk = m_entry.m_payloadLen - copied;
        } else {
            chunk = FEC_COPY_BUFFER_SIZE;
        }
        file.Read(m_copyBuf, chunk);
        m_stream.Write(m_copyBuf, chunk);
        copied += chunk;
        if (copied == static_cast<u32>(m_entry.m_payloadLen)) {
            done = true;
        }
    }

    m_stream.Seek(FEC_FILE_COUNT_OFFSET, CFile::begin);
    m_stream.Write(&m_nextIndex, sizeof(m_nextIndex));
    m_stream.Flush();
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0017bcd0, 0x28b)
i32 CFecFile::ExtractArchive(const char* dir, i32* pCancel, void* pProgress) {
    if (m_readOpen == false || m_openGate == false) {
        return 0;
    }
    if (m_header.m_versionMajor == 1 && m_header.m_versionMinor == 0) {
        return 0;
    }

    char cwd[_MAX_PATH];
    if (_getcwd(cwd, sizeof(cwd)) == NULL) {
        return 0;
    }
    if (_chdir(dir) != 0) {
        return 0;
    }

    CFile file;
    m_stream.Seek(FEC_ENTRY_TABLE_OFFSET, CFile::begin);

    for (u16 i = 0; i < static_cast<u32>(m_header.m_fileCount); i++) {
        if (m_stream.Read(&m_entry, sizeof(m_entry)) != sizeof(m_entry)) {
            _chdir(cwd);
            return 0;
        }
        char decoded[FEC_ENTRY_NAME_CAPACITY];
        FecDecode(m_entry.m_name, decoded, m_entry.m_nameLen);
        if (file.Open(decoded, CFile::modeCreate | CFile::modeReadWrite, NULL) == false) {
            _chdir(cwd);
            return 0;
        }
        if (m_stream.Seek(static_cast<i32>(m_index[i]), CFile::begin)
            != static_cast<i32>(m_index[i])) {
            _chdir(cwd);
            return 0;
        }
        b32 done = false;
        u32 copied = 0;
        while (done == false) {
            if (pProgress != NULL) {
                MSG msg;
                if (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
                    TranslateMessage(&msg);
                    DispatchMessageA(&msg);
                }
            }
            if (*pCancel != 0) {
                return 0;
            }
            u32 chunk = m_entry.m_payloadLen;
            if (copied + FEC_COPY_BUFFER_SIZE > chunk) {
                chunk -= copied;
            } else {
                chunk = FEC_COPY_BUFFER_SIZE;
            }
            m_stream.Read(m_copyBuf, chunk);
            file.Write(m_copyBuf, chunk);
            copied += chunk;
            if (copied == static_cast<u32>(m_entry.m_payloadLen)) {
                done = true;
            }
        }
        file.Flush();
        file.Close();
    }

    _chdir(cwd);
    return 1;
}

RVA(0x0017bf60, 0x5)
i32 CFecFile::Random() {
    return rand();
}

RVA(0x0017bf70, 0x65)
void CFecFile::FecEncode(const char* src, char* dst) {
    for (unsigned short i = 0; i < strlen(src); i++) {
        if (i % 2 == 0) {
            dst[i] = src[i] + 0x4f;
        } else {
            dst[i] = src[i] + 0x53;
        }
    }
}

RVA(0x0017bfe0, 0x5d)
void CFecFile::FecDecode(const char* src, char* dst, u16 len) {
    for (unsigned short i = 0; i < len; i++) {
        if (i % 2 == 0) {
            dst[i] = src[i] - 0x4f;
        } else {
            dst[i] = src[i] - 0x53;
        }
    }
    dst[len] = 0;
}

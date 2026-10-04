#include <StdAfx.h>
#include <Io/File.h>

#include <Ints.h>

#include <Crypto/FecCrypt.h>

#include <Enums.h>
#include <Ints.h>

#include <direct.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

i32 CFecFile::Init() {
    if (m_openGate) {
        return 0;
    }
    m_readOpen = false;
    m_writeOpen = false;
    m_index.clear();
    memset(&m_header, 0, sizeof(m_header));
    memset(&m_entry, 0, sizeof(m_entry));
    m_nextIndex = 0;
    m_openGate = true;
    return 1;
}

void CFecFile::Close() {
    if (!m_openGate) {
        return;
    }
    OnFail();
    m_index.clear();
    m_openGate = false;
}

i32 CFecFile::OnFail() {
    if (m_openGate && (m_readOpen || m_writeOpen)) {
        m_stream.finish();
        m_readOpen = false;
        m_writeOpen = false;
        m_nextIndex = 0;
        return 1;
    }
    return 0;
}

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
    if (m_stream.open(name, io::ReadOnly) == false) {
        return 0;
    }
    m_readOpen = true;

    char magic[FEC_MAGIC_SIZE];
    if (m_stream.read(magic, sizeof(magic)) != sizeof(magic)) {
        goto fail;
    }
    if (magic[0] != 'F' || magic[1] != 'E' || magic[2] != 'C') {
        goto fail;
    }
    if (m_stream.read(&m_header, sizeof(m_header)) != sizeof(m_header)) {
        goto fail;
    }


    if (m_stream.read(&m_entry, sizeof(m_entry)) != sizeof(m_entry)) {
        goto fail;
    }
    {
        if (!m_stream.seek(m_entry.Scramble() - FEC_SCRAMBLE_BASE, io::Current) || m_stream.position() != m_entry.Scramble() - FEC_FIRST_PAYLOAD_ADJUSTMENT) {
            goto fail;
        }
        m_index.push_back(m_entry.Scramble() - FEC_FIRST_PAYLOAD_ADJUSTMENT);

        for (u16 i = 1; i < static_cast<u32>(m_header.m_fileCount); i++) {
            i32 stride = m_entry.PayloadLength();
            if (!m_stream.seek(stride, io::Current) || m_stream.position() != static_cast<i32>(EntryOffset(i - 1)) + stride) {
                goto fail;
            }
            memset(&m_entry, 0, sizeof(m_entry));
            if (m_stream.read(&m_entry, sizeof(m_entry)) != sizeof(m_entry)) {
                goto fail;
            }
            u16 scr = m_entry.Scramble();
            if (!m_stream.seek(scr - FEC_SCRAMBLE_BASE, io::Current) || m_stream.position() != static_cast<i32>(EntryOffset(i - 1)) + stride + scr
                       - FEC_NEXT_PAYLOAD_ADJUSTMENT) {
                goto fail;
            }
            m_index.push_back(static_cast<i32>(EntryOffset(i - 1)) + stride + scr - FEC_NEXT_PAYLOAD_ADJUSTMENT);
        }
    }
    return 1;

fail:
    OnFail();
    return 0;
}

io::File* CFecFile::Lookup(u32 idx) {
    if (m_readOpen && m_openGate && idx <= static_cast<u32>(m_header.m_fileCount) && idx != 0) {
        const u32* slot = &m_index[idx - 1];
        if (m_stream.seek(static_cast<i32>(*slot), io::Start)) {
            return &m_stream;
        }
    }
    return 0;
}

i32 CFecFile::CreateArchive(const char* name) {
    if (name != NULL && m_writeOpen == false && m_openGate != false
        && m_stream.open(name, io::Replace) != false) {
        m_writeOpen = true;

        char magic[FEC_MAGIC_SIZE];
        magic[0] = 'F';
        magic[1] = 'E';
        magic[2] = 'C';
        m_stream.write(magic, sizeof(magic));

        memset(&m_header, 0, sizeof(m_header));
        m_header.m_fileCount = 0;
        m_header.m_versionMajor = 1;
        m_header.m_versionMinor = 1;
        m_stream.write(&m_header, sizeof(m_header));
        if (!m_stream.flush()) { OnFail(); return 0; }
        return 1;
    }
    return 0;
}

i32 CFecFile::AddFile(const char* name, i32* pCancel, void* pProgress) {
    i32 i;
    MSG msg;
    if (m_writeOpen == false || m_openGate == false) {
        return 0;
    }

    io::File file;
    if (file.open(name, io::ReadOnly) == false) {
        return 0;
    }

    std::string base = name;
    i32 slash = stringIndex((base).find_last_of("/\\"));
    if (slash != -1) {
        base = rightText(base, static_cast<i32>((base).size()) - slash - 1);
    }

    if (base.size() >= FEC_ENTRY_NAME_CAPACITY) return 0;
    ++m_nextIndex;
    memset(&m_entry, 0, sizeof(m_entry));
    m_entry.m_index = m_nextIndex;
    m_entry.m_nameLen = static_cast<u16>(static_cast<i32>((base).size()));

    char* enc = new char[static_cast<i32>((base).size()) + 1];
    FecEncode((base).c_str(), enc);
    memcpy(m_entry.m_name, enc, static_cast<i32>((base).size()));
    delete[] enc;

    i32 length = static_cast<i32>((base).size());
    if (length < FEC_ENTRY_NAME_CAPACITY) {

        char* p = m_entry.m_name + length;
        i32 c = FEC_ENTRY_NAME_CAPACITY - length;
        do {
            memset(p, static_cast<u8>(Random() % FEC_RANDOM_BYTE_MODULUS), sizeof(*p));
            p++;
        } while (--c);
    }

    m_entry.m_scramble = static_cast<u16>((Random() % FEC_SCRAMBLE_RANGE + FEC_SCRAMBLE_BASE));

    m_entry.m_payloadLen = file.size();
    if (!file.good() || !file.seek(0, io::Start)) {
        m_nextIndex--;
        return 0;
    }

    m_stream.seek(0, io::End);
    m_stream.write(&m_entry, sizeof(m_entry));

    char* pad = new char[m_entry.Scramble() - FEC_SCRAMBLE_BASE];
    for (i = 0; i < m_entry.Scramble() - FEC_SCRAMBLE_BASE; i++) {
        pad[i] = static_cast<char>((Random() % FEC_RANDOM_BYTE_MODULUS));
    }
    m_stream.write(pad, m_entry.Scramble() - FEC_SCRAMBLE_BASE);
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

        if (pCancel && *pCancel != 0) {
            OnFail();
            return 0;
        }
        u32 chunk;
        if (copied + FEC_COPY_BUFFER_SIZE > static_cast<u32>(m_entry.PayloadLength())) {
            chunk = m_entry.PayloadLength() - copied;
        } else {
            chunk = FEC_COPY_BUFFER_SIZE;
        }
        if (file.read(m_copyBuf, chunk) != chunk) { OnFail(); return 0; }
        if (!m_stream.write(m_copyBuf, chunk)) { OnFail(); return 0; }
        copied += chunk;
        if (copied == static_cast<u32>(m_entry.PayloadLength())) {
            done = true;
        }
    }

    m_stream.seek(FEC_FILE_COUNT_OFFSET, io::Start);
    m_stream.write(&m_nextIndex, sizeof(m_nextIndex));
    if (!m_stream.flush()) { OnFail(); return 0; }
    return 1;
}

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

    io::File file;
    m_stream.seek(FEC_ENTRY_TABLE_OFFSET, io::Start);

    for (u16 i = 0; i < static_cast<u32>(m_header.m_fileCount); i++) {
        if (m_stream.read(&m_entry, sizeof(m_entry)) != sizeof(m_entry)) {
            _chdir(cwd);
            return 0;
        }
        if (m_entry.m_nameLen >= FEC_ENTRY_NAME_CAPACITY) { _chdir(cwd); return 0; }
        char decoded[FEC_ENTRY_NAME_CAPACITY];
        FecDecode(m_entry.m_name, decoded, m_entry.m_nameLen);
        if (file.open(decoded, io::Replace) == false) {
            _chdir(cwd);
            return 0;
        }
        if (!m_stream.seek(static_cast<i32>(EntryOffset(i)), io::Start) || m_stream.position() != static_cast<i32>(EntryOffset(i))) {
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
            if (pCancel && *pCancel != 0) {
                _chdir(cwd);
                return 0;
            }
            u32 chunk = m_entry.PayloadLength();
            if (copied + FEC_COPY_BUFFER_SIZE > chunk) {
                chunk -= copied;
            } else {
                chunk = FEC_COPY_BUFFER_SIZE;
            }
            if (m_stream.read(m_copyBuf, chunk) != chunk) { _chdir(cwd); return 0; }
            file.write(m_copyBuf, chunk);
            copied += chunk;
            if (copied == static_cast<u32>(m_entry.PayloadLength())) {
                done = true;
            }
        }
        if (!file.finish()) { _chdir(cwd); return 0; }
    }

    _chdir(cwd);
    return 1;
}

i32 CFecFile::Random() {
    return rand();
}

void CFecFile::FecEncode(const char* src, char* dst) {
    for (unsigned short i = 0; i < strlen(src); i++) {
        if (i % 2 == 0) {
            dst[i] = src[i] + 0x4f;
        } else {
            dst[i] = src[i] + 0x53;
        }
    }
}

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

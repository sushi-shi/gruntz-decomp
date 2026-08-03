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
    m_readOpen = 0;
    m_writeOpen = 0;
    m_index.SetSize(0, -1);
    memset(&m_header, 0, sizeof(m_header));
    memset(&m_entry, 0, sizeof(m_entry));
    m_nextIndex = 0;
    m_openGate = 1;
    return 1;
}

RVA(0x0017b570, 0x24)
void CFecFile::Close() {
    if (!m_openGate) {
        return;
    }
    OnFail();
    m_index.SetSize(0, -1);
    m_openGate = 0;
}

RVA(0x0017b5a0, 0x48)
i32 CFecFile::OnFail() {
    if (m_openGate && (m_readOpen || m_writeOpen)) {
        m_stream.Close();
        m_readOpen = 0;
        m_writeOpen = 0;
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
    if (m_readOpen != 0) {
        return 0;
    }
    if (m_openGate == 0) {
        return 0;
    }
    if (m_stream.Open(name, 0, 0) == 0) {
        return 0;
    }
    m_readOpen = 1;

    char magic[3];
    if (m_stream.Read(magic, 3) != 3) {
        goto fail;
    }
    if (magic[0] != 'F' || magic[1] != 'E' || magic[2] != 'C') {
        goto fail;
    }
    if (m_stream.Read(&m_header, sizeof(m_header)) != sizeof(m_header)) {
        goto fail;
    }

    char buf[0x100];
    sprintf(buf, "Opened FEC File %s\n", name);
    sprintf(
        buf,
        "FEC File Version: %d.%d\nNumber of Files: %d\n",
        m_header.m_versionMajor,
        m_header.m_versionMinor,
        m_header.m_fileCount
    );

    if (m_stream.Read(&m_entry, sizeof(m_entry)) != 0x10c) {
        goto fail;
    }
    {
        if (m_stream.Seek(m_entry.m_scramble - 0x2b8, 1) != m_entry.m_scramble - 0x19d) {
            goto fail;
        }
        m_index.Add(m_entry.m_scramble - 0x19d);

        for (u16 i = 1; i < static_cast<u32>(m_header.m_fileCount); i++) {
            i32 stride = m_entry.m_payloadLen;
            if (m_stream.Seek(stride, 1) != static_cast<i32>(m_index[i - 1]) + stride) {
                goto fail;
            }
            memset(&m_entry, 0, 0x10c);
            if (m_stream.Read(&m_entry, sizeof(m_entry)) != 0x10c) {
                goto fail;
            }
            if (m_stream.Seek(m_entry.m_scramble - 0x2b8, 1)
                != static_cast<i32>(m_index[i - 1]) + stride + m_entry.m_scramble - 0x1ac) {
                goto fail;
            }
            m_index.Add(static_cast<i32>(m_index[i - 1]) + stride + m_entry.m_scramble - 0x1ac);
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
        if (m_stream.Seek(static_cast<i32>(*slot), 0) == static_cast<i32>(*slot)) {
            return m_stream.m_hFile;
        }
    }
    return 0;
}

RVA(0x0017b8a0, 0xa2)
i32 CFecFile::CreateArchive(const char* name) {
    if (name != NULL && m_writeOpen == 0 && m_openGate != 0
        && m_stream.Open(name, 0x1002, 0) != 0) {
        m_writeOpen = 1;

        char magic[3];
        magic[0] = 'F';
        magic[1] = 'E';
        magic[2] = 'C';
        m_stream.Write(magic, 3);

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

// @early-stop
RVA(0x0017b950, 0x380)
i32 CFecFile::AddFile(const char* name, i32* pCancel, void* pProgress) {
    if (m_writeOpen == 0 || m_openGate == 0) {
        return 0;
    }

    CFile file;
    if (file.Open(name, 0, 0) == 0) {
        return 0;
    }

    m_nextIndex++;

    CString base = name;
    i32 slash = base.Find('\\');
    if (slash != -1) {
        base = base.Right(base.GetLength() - slash - 1);
    }

    memset(&m_entry, 0, 0x10c);
    m_entry.m_index = m_nextIndex;
    m_entry.m_nameLen = static_cast<u16>(base.GetLength());

    char* enc = static_cast<char*>(operator new(base.GetLength() + 1));
    FecEncode(base, enc);
    memcpy(m_entry.m_name, enc, base.GetLength());
    operator delete(enc);

    if (base.GetLength() < 0x100) {

        char* p = m_entry.m_name + base.GetLength();
        i32 c = 0x100 - base.GetLength();
        do {
            *p++ = static_cast<char>((rand() % 0xff));
        } while (--c);
    }

    m_entry.m_scramble = static_cast<u16>((rand() % 0x400 + 0x2b8));
    m_entry.m_payloadLen = file.Seek(0, 2);
    if (file.Seek(0, 0) != 0) {
        m_nextIndex--;
        return 0;
    }

    m_stream.Seek(0, 2);
    m_stream.Write(&m_entry, sizeof(m_entry));

    char* pad = static_cast<char*>(operator new(m_entry.m_scramble - 0x2b8));
    for (i32 i = 0; i < m_entry.m_scramble - 0x2b8; i++) {
        pad[i] = static_cast<char>((rand() % 0xff));
    }
    m_stream.Write(pad, m_entry.m_scramble - 0x2b8);
    operator delete(pad);

    memset(m_copyBuf, 0, 0x8000);
    u32 copied = 0;
    i32 done = 0;
    while (done == 0) {
        if (pProgress != NULL) {
            MSG msg;
            if (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessageA(&msg);
            }
        }

        if (*pCancel != 0) {
            return 0;
        }
        u32 chunk = 0x8000;
        if (copied + 0x8000 > static_cast<u32>(m_entry.m_payloadLen)) {
            chunk = m_entry.m_payloadLen - copied;
        }
        file.Read(m_copyBuf, chunk);
        m_stream.Write(m_copyBuf, chunk);
        copied += chunk;
        if (copied == static_cast<u32>(m_entry.m_payloadLen)) {
            done = 1;
        }
    }

    m_stream.Seek(0xb, 0);
    m_stream.Write(&m_nextIndex, sizeof(m_nextIndex));
    m_stream.Flush();
    return 1;
}

// @early-stop
RVA(0x0017bcd0, 0x28b)
i32 CFecFile::ExtractArchive(const char* dir, i32* pCancel, void* pProgress) {
    if (m_readOpen == 0 || m_openGate == 0) {
        return 0;
    }
    if (m_header.m_versionMajor == 1 && m_header.m_versionMinor == 0) {
        return 0;
    }

    char cwd[0x104];
    if (_getcwd(cwd, 0x104) == NULL) {
        return 0;
    }
    if (_chdir(dir) != 0) {
        return 0;
    }

    CFile file;
    m_stream.Seek(0xf, 0);

    for (u16 i = 0; i < static_cast<u32>(m_header.m_fileCount); i++) {
        u32 copied = 0;
        if (m_stream.Read(&m_entry, sizeof(m_entry)) != 0x10c) {
            _chdir(cwd);
            return 0;
        }
        char decoded[0x100];
        FecDecode(m_entry.m_name, decoded, m_entry.m_nameLen);
        if (file.Open(decoded, 0x1002, 0) == 0) {
            _chdir(cwd);
            return 0;
        }
        if (m_stream.Seek(static_cast<i32>(m_index[i]), 0) != static_cast<i32>(m_index[i])) {
            _chdir(cwd);
            return 0;
        }
        i32 done = 0;
        while (done == 0) {
            if (pProgress != NULL) {
                MSG msg;
                if (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) {
                    TranslateMessage(&msg);
                    DispatchMessageA(&msg);
                }
            }
            if (*pCancel != 0) {
                return 0;
            }
            u32 chunk = m_entry.m_payloadLen;
            if (copied + 0x8000 > chunk) {
                chunk -= copied;
            } else {
                chunk = 0x8000;
            }
            m_stream.Read(m_copyBuf, chunk);
            file.Write(m_copyBuf, chunk);
            copied += chunk;
            if (copied == static_cast<u32>(m_entry.m_payloadLen)) {
                done = 1;
            }
        }
        file.Flush();
        file.Close();
    }

    _chdir(cwd);
    return 1;
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

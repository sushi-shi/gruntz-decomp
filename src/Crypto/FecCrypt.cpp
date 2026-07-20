#include <Ints.h>
#include <rva.h>
#include <string.h> // strlen (inlined as repnz scasb at /O2 /Oi), memset/memcpy
#include <stdio.h>  // sprintf (the "Opened FEC File" diagnostics, 0x11f890)
#include <stdlib.h> // rand (0x17bf60, obfuscation padding)
#include <direct.h> // _getcwd / _chdir (ExtractArchive dir save/restore)

#include <Crypto/FecCrypt.h> // the unified CFecFile (embedded MFC CFile stream) shape

void __stdcall FecEncode(const char* src, char* dst);
void __stdcall FecDecode(const char* src, char* dst, unsigned short len);

RVA(0x0017b510, 0x55)
i32 CFecFile::Init() {
    if (m_00) {
        return 0;
    }
    m_04 = 0;
    m_08 = 0;
    m_index.SetSize(0, -1);
    memset(&m_0c, 0, 12); // m_0c, m_10, m_14
    memset(&m_entry, 0, sizeof(m_entry));
    m_134 = 0;
    m_00 = 1;
    return 1;
}

RVA(0x0017b570, 0x24)
void CFecFile::Close() {
    if (!m_00) {
        return;
    }
    OnFail();
    m_index.SetSize(0, -1);
    m_00 = 0;
}

RVA(0x0017b5a0, 0x48)
i32 CFecFile::OnFail() {
    if (m_00 && (m_04 || m_08)) {
        m_stream.Close();
        m_04 = 0;
        m_08 = 0;
        m_134 = 0;
        return 1;
    }
    return 0;
}

// @early-stop
// ~82.8% regalloc wall: layout/logic byte-correct (CDWordArray index, single sprintf
// buffer, the two m_11e re-reads, (u32)m_14 loop bound), but retail colours `name`->ebp
// and `&m_stream`->ebx while cl swaps them, so every m_stream vtable dispatch differs by
// the base register (mov ecx,ebx vs ebp). A pure register-coloring tie-break; not
// source-steerable.
RVA(0x0017b5f0, 0x249)
i32 CFecFile::ReadArchive(const char* name) {
    if (name == 0) {
        return 0;
    }
    if (m_04 != 0) {
        return 0;
    }
    if (m_00 == 0) {
        return 0;
    }
    if (m_stream.Open(name, 0, 0) == 0) {
        return 0;
    }
    m_04 = 1;

    char magic[3];
    if (m_stream.Read(magic, 3) != 3) {
        goto fail;
    }
    if (magic[0] != 'F' || magic[1] != 'E' || magic[2] != 'C') {
        goto fail;
    }
    if (m_stream.Read(&m_0c, 0xc) != 0xc) {
        goto fail;
    }

    char buf[0x100];
    sprintf(buf, "Opened FEC File %s\n", name);
    sprintf(buf, "FEC File Version: %d.%d\nNumber of Files: %d\n", m_0c, m_10, m_14);

    if (m_stream.Read(&m_entry, 0x10c) != 0x10c) {
        goto fail;
    }
    {
        i32 tail = m_entry.m_scramble - 0x19d;
        if (m_stream.Seek(m_entry.m_scramble - 0x2b8, 1) != tail) {
            goto fail;
        }
        m_index.Add(tail);

        for (u16 i = 1; i < static_cast<u32>(m_14); i++) {
            i32 stride = m_entry.m_payloadLen;
            if (m_stream.Seek(stride, 1) != static_cast<i32>(m_index.GetData()[i - 1]) + stride) {
                goto fail;
            }
            memset(&m_entry, 0, 0x10c);
            if (m_stream.Read(&m_entry, 0x10c) != 0x10c) {
                goto fail;
            }
            i32 w2 = m_entry.m_scramble;
            if (m_stream.Seek(w2 - 0x2b8, 1)
                != static_cast<i32>(m_index.GetData()[i - 1]) + stride + w2 - 0x1ac) {
                goto fail;
            }
            m_index.Add(static_cast<i32>(m_index.GetData()[i - 1]) + stride + w2 - 0x1ac);
        }
    }
    return 1;

fail:
    OnFail();
    return 0;
}

RVA(0x0017b840, 0x53)
i32 CFecFile::Lookup(u32 idx) {
    if (m_04 && m_00 && idx <= static_cast<u32>(m_14) && idx != 0) {
        i32* slot = reinterpret_cast<i32*>(&m_index.GetData()[idx - 1]);
        if (m_stream.Seek(*slot, 0) == *slot) {
            return m_stream.m_hFile; // +0x128 - the Win32 file HANDLE
        }
    }
    return 0;
}

RVA(0x0017b8a0, 0xa2)
i32 CFecFile::CreateArchive(const char* name) {
    if (name != 0 && m_08 == 0 && m_00 != 0 && m_stream.Open(name, 0x1002, 0) != 0) {
        m_08 = 1;

        char magic[3];
        magic[0] = 'F';
        magic[1] = 'E';
        magic[2] = 'C';
        m_stream.Write(magic, 3);

        memset(&m_0c, 0, 0xc);
        m_14 = 0;
        m_0c = 1;
        m_10 = 1;
        m_stream.Write(&m_0c, 0xc);
        m_stream.Flush(); // +0x50 CFile::Flush (flush the header write)
        return 1;
    }
    return 0;
}

// ===========================================================================
// 0x17b950 - CFecFile::AddFile(name, pCancel, pProgress): append the disk file `name`
// to an open (m_08) write-archive. Builds the 0x10c entry record (index=++m_134,
// FecEncode'd basename + random padding, scramble word m_11e = rand()%0x400 + 0x2b8,
// payload length m_120), writes the record + (m_11e - 0x2b8) random bytes, then streams
// the file in 32 KB chunks (pumping messages while pProgress, aborting on *pCancel), and
// finally patches the header file-count at offset 0xb. __thiscall; 1 on success.
// ===========================================================================
// @early-stop
// ~82% regalloc/scheduling wall: full control flow + /GX EH frame + record build + copy
// loop + finalize are reconstructed and reloc-match retail. Residuals are the `this`
// register colouring (ebp vs ebx), the seek-fail/abort undo tail-merge layout, and the
// name-padding `mov dh,dl` byte-extract scheduling residue. Not source-steerable.
RVA(0x0017b950, 0x380)
i32 CFecFile::AddFile(const char* name, i32* pCancel, void* pProgress) {
    if (m_08 == 0 || m_00 == 0) {
        return 0;
    }

    CFile file;
    if (file.Open(name, 0, 0) == 0) {
        return 0;
    }

    m_134++;

    CString base = name;
    i32 slash = base.Find('\\');
    if (slash != -1) {
        base = base.Right(base.GetLength() - slash - 1);
    }

    memset(&m_entry, 0, 0x10c);
    m_entry.m_index = m_134;
    m_entry.m_nameLen = static_cast<u16>(base.GetLength());

    char* enc = static_cast<char*>(operator new(base.GetLength() + 1));
    FecEncode(base, enc);
    memcpy(m_entry.m_name, enc, base.GetLength());
    operator delete(enc);

    if (base.GetLength() < 0x100) {
        char* p = m_entry.m_name + base.GetLength();
        for (i32 c = 0x100 - base.GetLength(); c != 0; c--) {
            *p++ = static_cast<char>((rand() % 0xff));
        }
    }

    m_entry.m_scramble = static_cast<u16>((rand() % 0x400 + 0x2b8));
    m_entry.m_payloadLen = file.Seek(0, 2);
    if (file.Seek(0, 0) != 0) {
        m_134--;
        return 0;
    }

    m_stream.Seek(0, 2);
    m_stream.Write(&m_entry, 0x10c);

    char* pad = static_cast<char*>(operator new(m_entry.m_scramble - 0x2b8));
    for (i32 i = 0; i < m_entry.m_scramble - 0x2b8; i++) {
        pad[i] = static_cast<char>((rand() % 0xff));
    }
    m_stream.Write(pad, m_entry.m_scramble - 0x2b8);
    operator delete(pad);

    memset(m_14c, 0, 0x8000);
    u32 copied = 0;
    i32 done = 0;
    while (done == 0) {
        if (pProgress != 0) {
            MSG msg;
            if (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessageA(&msg);
            }
        }
        if (*pCancel != 0) {
            m_134--;
            return 0;
        }
        u32 chunk = 0x8000;
        if (copied + 0x8000 > static_cast<u32>(m_entry.m_payloadLen)) {
            chunk = m_entry.m_payloadLen - copied;
        }
        file.Read(m_14c, chunk);
        m_stream.Write(m_14c, chunk);
        copied += chunk;
        if (copied == static_cast<u32>(m_entry.m_payloadLen)) {
            done = 1;
        }
    }

    m_stream.Seek(0xb, 0);
    m_stream.Write(&m_134, 4);
    m_stream.Flush(); // +0x50 CFile::Flush (flush the appended entry)
    return 1;
}

// ===========================================================================
// 0x17bcd0 - CFecFile::ExtractArchive(dir, pCancel, pProgress): unpack an open (m_04)
// read-archive into directory `dir`. Saves the cwd, chdirs into `dir`, seeks the stream
// past the 0xf-byte header, then for each of m_14 entries reads the 0x10c record,
// FecDecode's the name, opens the output file, seeks the stream to the entry's recorded
// offset (m_index[i]) and streams m_120 bytes out in 32 KB chunks (message-pumping while
// pProgress, aborting on *pCancel). Restores the cwd on success/failure. __thiscall.
// ===========================================================================
// @early-stop
// ~84.5% regalloc wall: full logic + /GX EH frame + per-entry decode/extract + copy loop
// reconstructed and reloc-match retail. Residual is register colouring - retail reuses
// ebp for both the zero-constant (the m_04/m_00/version gates) and the `copied` counter,
// while cl splits them across esi/ebp - plus the chunk-branch polarity. Not
// source-steerable.
RVA(0x0017bcd0, 0x28b)
i32 CFecFile::ExtractArchive(const char* dir, i32* pCancel, void* pProgress) {
    if (m_04 == 0 || m_00 == 0) {
        return 0;
    }
    if (m_0c == 1 && m_10 == 0) {
        return 0;
    }

    char cwd[0x104];
    if (_getcwd(cwd, 0x104) == 0) {
        return 0;
    }
    if (_chdir(dir) != 0) {
        return 0;
    }

    CFile file;
    m_stream.Seek(0xf, 0);

    for (u16 i = 0; i < static_cast<u32>(m_14); i++) {
        if (m_stream.Read(&m_entry, 0x10c) != 0x10c) {
            goto fail;
        }
        char decoded[0x100];
        FecDecode(m_entry.m_name, decoded, m_entry.m_nameLen);
        if (file.Open(decoded, 0x1002, 0) == 0) {
            goto fail;
        }
        if (m_stream.Seek(static_cast<i32>(m_index.GetData()[i]), 0) != static_cast<i32>(m_index.GetData()[i])) {
            goto fail;
        }
        u32 copied = 0;
        i32 done = 0;
        while (done == 0) {
            if (pProgress != 0) {
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
            if (copied + 0x8000 <= chunk) {
                chunk = 0x8000;
            } else {
                chunk -= copied;
            }
            m_stream.Read(m_14c, chunk);
            file.Write(m_14c, chunk);
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

fail:
    _chdir(cwd);
    return 0;
}

RVA(0x0017bf70, 0x65)
void __stdcall FecEncode(const char* src, char* dst) {
    for (unsigned short i = 0; i < strlen(src); i++) {
        if (i % 2 == 0) {
            dst[i] = src[i] + 0x4f;
        } else {
            dst[i] = src[i] + 0x53;
        }
    }
}

// ===========================================================================
// Decode(src, dst, len): dst[i] = src[i] - (i odd ? 0x53 : 0x4f) for
// `len` bytes, then NUL-terminate dst[len]. `len` is a WORD. __stdcall.
// ===========================================================================
// @early-stop
// entropy tail (~99.75%): every instruction is byte-identical except the final
// `dst[len]=0` store, where retail bases the SIB on len (ebp) and indexes by dst,
// while cl bases on dst and indexes by len (same address, 1-byte-different
// encoding). MSVC canonicalizes `len[dst]` back to `dst+len`, so the base/index
// pick is not source-steerable.
RVA(0x0017bfe0, 0x5d)
void __stdcall FecDecode(const char* src, char* dst, unsigned short len) {
    for (unsigned short i = 0; i < len; i++) {
        if (i % 2 == 0) {
            dst[i] = src[i] - 0x4f;
        } else {
            dst[i] = src[i] - 0x53;
        }
    }
    dst[len] = 0;
}

SIZE(CFecFile, 0x814c);
SIZE(FecEntry, 0x10c); // the CFecFile+0x18 per-entry record (typed in FecCrypt.h)

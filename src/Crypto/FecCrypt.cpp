#include <Ints.h>
#include <rva.h>
#include <string.h> // strlen (inlined as repnz scasb at /O2 /Oi), memset/memcpy
#include <stdio.h>  // sprintf (the "Opened FEC File" diagnostics, 0x11f890)
#include <stdlib.h> // rand (0x17bf60, obfuscation padding)
#include <direct.h> // _getcwd / _chdir (ExtractArchive dir save/restore)

#include <Crypto/FecCrypt.h> // the unified CFecFile (embedded MFC CFile stream) shape

RVA(0x0017b510, 0x55)
i32 CFecFile::Init() {
    if (m_openGate) {
        return 0;
    }
    m_readOpen = 0;
    m_writeOpen = 0;
    m_index.SetSize(0, -1);
    memset(&m_versionMajor, 0, 12); // m_versionMajor, m_versionMinor, m_fileCount
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
// 88.6% (was 82.8). The old note ("retail colours `name`->ebp and `&m_stream`->ebx while
// cl swaps them") was WRONG - the colouring already agreed; the whole prologue/header
// half is now byte-exact after two source fixes:
//   - `m_index[i-1]` (CDWordArray::operator[] -> ElementAt, returns DWORD&) instead of
//     `m_index.GetData()[i-1]`: the reference form makes /O2 CSE the ELEMENT ADDRESS
//     (`lea edi,[ebp+eax-4]` before the Seek, `mov ecx,[edi]` after), which is what retail
//     does; the GetData() form CSEs the base pointer and re-indexes.
//   - the first Seek's `tail` local removed: retail reads m_entry.m_scramble TWICE
//     (`xor eax,eax; mov ax,[..]` / `xor ecx,ecx; mov cx,[..]`), which only happens when
//     both `- 0x2b8` and `- 0x19d` are spelled inline.
// Residual is ONE allocator tie-break in the loop: retail keeps `i*4` in ebp and spills
// `stride` to [esp+0x18]; cl keeps `stride` in ebp and spills `i*4`. Every downstream
// register/slot difference follows from that single choice. 11 source spellings tested
// (operand order, paren grouping, u16/u32/i32 stride+w2, k=i-1 local, reversed compare,
// stride hoisted, SetAtGrow-vs-Add, ElementAt) - all identical to the byte.
RVA(0x0017b5f0, 0x249)
i32 CFecFile::ReadArchive(const char* name) {
    if (name == 0) {
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
    if (m_stream.Read(&m_versionMajor, 0xc) != 0xc) {
        goto fail;
    }

    char buf[0x100];
    sprintf(buf, "Opened FEC File %s\n", name);
    sprintf(
        buf,
        "FEC File Version: %d.%d\nNumber of Files: %d\n",
        m_versionMajor,
        m_versionMinor,
        m_fileCount
    );

    if (m_stream.Read(&m_entry, 0x10c) != 0x10c) {
        goto fail;
    }
    {
        if (m_stream.Seek(m_entry.m_scramble - 0x2b8, 1) != m_entry.m_scramble - 0x19d) {
            goto fail;
        }
        m_index.Add(m_entry.m_scramble - 0x19d);

        for (u16 i = 1; i < static_cast<u32>(m_fileCount); i++) {
            i32 stride = m_entry.m_payloadLen;
            if (m_stream.Seek(stride, 1) != static_cast<i32>(m_index[i - 1]) + stride) {
                goto fail;
            }
            memset(&m_entry, 0, 0x10c);
            if (m_stream.Read(&m_entry, 0x10c) != 0x10c) {
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
    if (m_readOpen && m_openGate && idx <= static_cast<u32>(m_fileCount) && idx != 0) {
        const DWORD* slot = &m_index.GetData()[idx - 1];
        if (m_stream.Seek(static_cast<i32>(*slot), 0) == static_cast<i32>(*slot)) {
            return m_stream.m_hFile; // +0x128 - the Win32 file HANDLE
        }
    }
    return 0;
}

RVA(0x0017b8a0, 0xa2)
i32 CFecFile::CreateArchive(const char* name) {
    if (name != 0 && m_writeOpen == 0 && m_openGate != 0 && m_stream.Open(name, 0x1002, 0) != 0) {
        m_writeOpen = 1;

        char magic[3];
        magic[0] = 'F';
        magic[1] = 'E';
        magic[2] = 'C';
        m_stream.Write(magic, 3);

        memset(&m_versionMajor, 0, 0xc);
        m_fileCount = 0;
        m_versionMajor = 1;
        m_versionMinor = 1;
        m_stream.Write(&m_versionMajor, 0xc);
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
// 82.9% (was 82.0). The "`this` colouring ebp vs ebx" part of the old note was wrong -
// the colouring agreed; what was missing was that FecEncode is a __thiscall MEMBER
// (retail sets `mov ecx,this` right before the call; see the header note), and the
// name-padding loop is a do-while (retail has no zero-trip skip).
// 82.9 -> 96.9 (2026-08-01): the cancel arm had a SPURIOUS `m_nextIndex--`. It is a
// behaviour bug, not a codegen one, and `--branches --diff` is what exposed it - branch
// #6 read `base jne -> target je`, i.e. our fail block owned the wrong side of the
// fallthrough. Retail keeps `&m_nextIndex` (this+0x134) in a stack slot and the two
// bail-outs enter the SAME teardown at different points: the `file.Seek(0,0)` failure at
// 0x17bb05 runs `mov eax,[esp+0x18] / dec DWORD PTR [eax]` first, while `*pCancel` jumps
// to 0x17bb0b - one instruction PAST the `dec`. Only the seek failure rolls the counter
// back; a cancel leaves it. Ours decremented on both, so cl merged the two exits into one
// block and every branch below it inverted. That single statement was worth 14 points,
// and the branch sequences now AGREE end to end.
// (The old note's "retail does NOT tail-merge the two early-fail arms" was reading this
// same thing from the wrong end - retail DOES merge them, it just enters at two offsets.)
// Remaining residual, verified against retail's bytes:
//   - our /GX frame is 4 B short (0x38 vs 0x3c) - retail spills the `&m_nextIndex`
//     address CSE to a slot where cl rematerializes it, and every [esp+N] below shifts.
//     Not source-reachable: writing the pointer out as `i32* pIndex = &m_nextIndex;` and
//     routing the dec and/or the Write through it is byte-identical (96.504, size 886);
//   - the padding loop's `and edx,0xff` + `mov dh,dl` byte-duplication residue;
//   - the `[edi+esi-1]` vs `[esi+edi-1]` SIB base/index coin-flip (see MonoClear).
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
        // do-while: the guard above already proves c >= 1, and retail's loop has no
        // zero-trip skip (`mov edi,0x100; sub edi,eax` straight into the body).
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
    m_stream.Write(&m_entry, 0x10c);

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
        if (pProgress != 0) {
            MSG msg;
            if (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessageA(&msg);
            }
        }
        // NO `m_nextIndex--` here: retail's cancel arm branches to 0x17bb0b, which is
        // one instruction PAST the `dec DWORD PTR [eax]` at 0x17bb09 that the
        // file.Seek(0,0) failure arm runs. Only the seek failure rolls the counter back.
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
    m_stream.Write(&m_nextIndex, 4);
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
// 89.1% (was 85.3). Three real source bugs fixed so far:
//   - the chunk branch polarity was inverted. Retail is
//     `if (copied + 0x8000 > payloadLen) chunk = payloadLen - copied; else chunk = 0x8000;`
//     (`cmp eax,esi; jbe <chunk=0x8000>; sub esi,ebp`), not the `<=` form.
//   - `m_index[i]` (operator[] -> DWORD&) instead of `m_index.GetData()[i]`, so /O2 keeps
//     retail's element ADDRESS (`lea edi,[edx+ecx*4]`) across the Seek call.
//   - THE THREE FAILURE ARMS ARE NOT A SHARED `goto fail` - they are three inline
//     `_chdir(cwd); return 0;` copies. That is provable from retail's bytes: a shared
//     label emits ONE block, but retail has THREE (0x17bef9 / 0x17bf00 / 0x17bf07),
//     each `lea <reg>,[esp+0x44]; push <reg>; jmp 0x17bf0c`, with a DIFFERENT scratch
//     register (eax / ecx / edx) - which is precisely what stops cl cross-jumping them,
//     and precisely what you get when the allocator picks a free scratch independently
//     at three separate sites. Writing the three copies out reproduces retail's register
//     sequence exactly and merges the common `call _chdir` tail the same way.
// Residual is ONE allocator tie-break, and every remaining block-level difference is
// downstream of it: retail merges the entry zero-constant with `copied` in ebp
// (`xor ebp,ebp` at entry serves both, hence retail's loop-entry `jmp` over the loop-top
// `xor ebp,ebp`) and rematerializes `lea esi,[ebx+0x124]` per outer iteration plus
// `lea ecx,[ebx+0x124]` inside the inner loop. cl instead keeps &m_stream loop-carried in
// ebp, so the inner loop needs FOUR callee-saved values (this / &m_stream / chunk /
// &m_copyBuf) and `copied` is pushed to a stack home - hence our frame is 0x23c vs
// retail's 0x238. Tested and byte-identical across 339 cells: `copied` at fn top /
// before the loop / at the loop top, i32-vs-u32 copied+chunk+done, `register` on every
// candidate (MSVC5 /O2 ignores it), the chunk if/else as a ternary, a `char* buf` local,
// `!done`, and FIVE `CFile*` local-pointer spellings (per-outer-iteration, per-inner-
// iteration, and a scoped one for the pre-loop Seek) - cl folds every local pointer back
// into the same CSE. Only `m_stream.Seek(0xf, 0)` moved ahead of `CFile file;` changed
// anything, and it REGRESSED (83.2). Not source-steerable.
RVA(0x0017bcd0, 0x28b)
i32 CFecFile::ExtractArchive(const char* dir, i32* pCancel, void* pProgress) {
    if (m_readOpen == 0 || m_openGate == 0) {
        return 0;
    }
    if (m_versionMajor == 1 && m_versionMinor == 0) {
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

    for (u16 i = 0; i < static_cast<u32>(m_fileCount); i++) {
        u32 copied = 0;
        if (m_stream.Read(&m_entry, 0x10c) != 0x10c) {
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

// ===========================================================================
// Decode(src, dst, len): dst[i] = src[i] - (i odd ? 0x53 : 0x4f) for
// `len` bytes, then NUL-terminate dst[len]. `len` is a WORD. __stdcall.
// ===========================================================================
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

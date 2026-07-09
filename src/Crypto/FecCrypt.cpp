// FecCrypt.cpp - the two leaf string-obfuscation routines that sit between the
// Font cluster and the "FEC File" reader (0x17b5f0 references the "Opened FEC
// File %s" diagnostics). A simple alternating-byte cipher: even-index bytes are
// biased by 0x4f, odd-index bytes by 0x53. Encode/Decode are pure leaf functions
// (no relocs; inline strlen via the /Oi `repnz scasb` intrinsic), __stdcall.
//
// The loop index is a `unsigned short` kept in a full register and masked to
// 16-bit on use (the `and reg,0xffff`); `i % 2` promotes it to int, so the
// even/odd test lowers to MSVC's signed `% 2` idiom (cdq/xor/sub) even though the
// value is always >= 0. Names are placeholders; offsets + code bytes are the
// load-bearing fact.
#include <Ints.h>
#include <rva.h>
#include <string.h> // strlen (inlined as repnz scasb at /O2 /Oi)

// ---------------------------------------------------------------------------
// The "FEC File" archive reader (0x17b5f0): validates an FEC-format encrypted
// resource container and walks its index. The class owns a polymorphic byte
// stream at +0x124 (Open/Read/Seek through its vtable) and a name/checksum
// helper at +0x138. Field names are placeholders; only offsets + code bytes
// are load-bearing.
// ---------------------------------------------------------------------------

// The stream subobject at CFecFile+0x124: a polymorphic file/byte source. Its
// virtuals are DECLARED only (foreign vtable, reloc-masked dispatch). Slots:
// Open @+0x28, Seek @+0x30, Read @+0x3c (all __thiscall by MSVC default).
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
    virtual i32 Open(const char* name, i32 a2, i32 a3); // +0x28
    virtual void Slot2C();                              // +0x2c
    virtual i32 Seek(i32 off, i32 origin);              // +0x30
    virtual void Slot34();
    virtual void Slot38();
    virtual i32 Read(void* buf, i32 size);        // +0x3c
    virtual i32 Write(const void* buf, i32 size); // +0x40
    virtual void Slot44();
    virtual void Slot48();
    virtual void Slot4C();
    virtual void Close(); // +0x50
};

// The helper at CFecFile+0x138 (its Op at 0x1b4d7c is a reloc-masked rel32 call).
struct FecIndex {
    void Op(i32 a1, i32 a2); // 0x1b4d7c  __thiscall
    void* m_00;
    i32 m_04;
    i32 m_08; // +0x08  read as the first Op arg in the loop
};

// The diagnostics sink (0x11f890): variadic cdecl `(buf, fmt, ...)` logger.
extern "C" int FecLog(char* buf, const char* fmt, ...);

class CFecFile {
public:
    i32 CreateArchive(const char* name); // 0x17b8a0
    i32 ReadArchive(const char* name);   // 0x17b5f0
    void OnFail();                       // 0x17b5a0 (reloc-masked)

    i32 m_00;           // +0x00  open-gate (must be nonzero)
    i32 m_04;           // +0x04  already-open flag
    i32 m_08;           // +0x08
    i32 m_0c;           // +0x0c  version major (12-byte header word 0)
    i32 m_10;           // +0x10  version minor
    i32 m_14;           // +0x14  entry count
    char m_18[0x10c];   // +0x18  per-entry record (m_11e WORD + m_120 stride inside)
    FecStream m_stream; // +0x124
    char _pad128[0x138 - 0x128];
    FecIndex m_index; // +0x138
    char* m_13c;      // +0x13c  per-entry offset table (dwords)
    i32 m_140;        // +0x140
};

// The version WORD (0x11e) + per-entry seek stride (0x120) live inside the m_18
// record buffer (0x18 + 0x106 / 0x108).
#define FEC_W(p) (*(u16*)((char*)(p) + 0x11e))
#define FEC_STRIDE(p) (*(i32*)((char*)(p) + 0x120))

// @early-stop
// validation-reader wall: virtual stream dispatch + the per-entry index walk
// reproduce, but the loop's intricate stack-slot reuse around the two Seek
// checks is not source-steerable (intermediate live ranges differ).
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

    char buf1[0x80];
    char buf2[0x80];
    FecLog(buf1, "Opened FEC File %s", name);
    FecLog(buf2, "FEC File Version: %d.%d Number o", m_0c, m_10, m_14);

    if (m_stream.Read(m_18, 0x10c) != 0x10c) {
        goto fail;
    }
    {
        u16 w = FEC_W(this);
        if (m_stream.Seek(w - 0x2b8, 1) != w - 0x19d) {
            goto fail;
        }
        m_index.Op(m_140, w - 0x19d);

        for (u16 i = 1; i < (u16)m_14; i++) {
            i32 base = *(i32*)(m_13c + i * 4 - 4);
            i32 stride = FEC_STRIDE(this);
            if (m_stream.Seek(stride, 1) != base + stride) {
                goto fail;
            }
            memset(m_18, 0, 0x10c);
            if (m_stream.Read(m_18, 0x10c) != 0x10c) {
                goto fail;
            }
            u16 w2 = FEC_W(this);
            if (m_stream.Seek(w2 - 0x2b8, 1) != base + stride + w2 - 0x1ac) {
                goto fail;
            }
            m_index.Op(m_index.m_08, base + stride + *(i32*)(m_13c + i * 4 - 4) - 0x1ac);
        }
    }
    return 1;

fail:
    OnFail();
    return 0;
}

// ===========================================================================
// 0x17b8a0 - CFecFile::CreateArchive(name): open `name` for writing (mode 0x1002),
// emit the "FEC" magic + a 12-byte {major=1, minor=1, count=0} header, then flush
// the stream. Gated by the shared m_00 open-gate + the m_08 write-open flag (distinct
// from ReadArchive's m_04 read-open flag). __thiscall; returns 1 on success, 0 on any
// gate/open failure.
// ===========================================================================
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
        m_stream.Close();
        return 1;
    }
    return 0;
}

// ===========================================================================
// Encode(src, dst): dst[i] = src[i] + (i odd ? 0x53 : 0x4f), for the
// whole NUL-terminated src (strlen recomputed each iteration). __stdcall.
// ===========================================================================
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

SIZE_UNKNOWN(FecStream);
SIZE_UNKNOWN(FecIndex);
SIZE_UNKNOWN(CFecFile);

// --- vtable catalog ---

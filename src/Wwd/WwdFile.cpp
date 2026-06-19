// WwdFile.cpp - Gruntz WWD level-file loader (Monolith-faithful reconstruction).
//
// Functions matched in this TU:
//   WwdFile::IsValidWwd       - BYTE-EXACT
//   WwdFile::CheckHeader      - BYTE-EXACT
//   WwdFile::InflateMainBlock - ~88.7% fuzzy, entropy plateau
//   WwdFile::ReadPlane        - 99.19%
//                               (byte-exact modulo 11 reloc-masked operand bytes
//                                + a 6-byte 2-instruction MSVC5 scheduling swap)
//
// IsValidWwd / CheckHeader open a file by name, read the 1524-byte (0x5F4) WWD
// header, and validate it: the read must return exactly 0x5F4 bytes and the
// header signature (first u32 == sizeof(WwdHeader)) must be <= 0x5F4. IsValidWwd
// reads straight into the caller buffer; CheckHeader reads into a private 0x5F4
// stack buffer and then copies that header out to the caller (inline strcpy-like
// rep movs of the NUL-terminated... actually a header byte-copy of strlen+1).
//
// Both construct an engine binary-file stream on the stack (ctor, dtor, Open,
// Read). The stack object has a non-trivial
// destructor, so MSVC emits a C++ EH frame (__CxxFrameHandler + FuncInfo, the
// fs:0 registration + `push -1; push handler`): this TU is built with /GX. The
// stream's ctor/dtor/Open/Read are unmatched engine functions, but their call
// bytes are reloc-masked in objdiff, so the validators are still byte-exact.
//
// __stdcall (callee cleans 8 bytes; ret 0x8 in the binary): each takes two
// pointer args and uses callee-cleanup, so they reconstruct as __stdcall free
// functions. Returns are full-width eax (1 / 0), i.e. `int`, not bool.
#include <Wwd/WwdFile.h>
#include <rva.h>

#include <string.h>  // memcpy

// ---------------------------------------------------------------------------
// WwdFile::IsValidWwd
// Open(name) -> Read(headerBuf, 0x5F4) -> require read == 0x5F4 and first u32
// (the header signature) <= 0x5F4. The two null guards return BEFORE the stream
// is constructed (no destructor on those paths); the stream's ctor runs only
// after both guards, so its dtor unwinds the remaining exits.
RVA(0x160530, 0x125)
int __stdcall WwdFile_IsValidWwd(const char* name, void* headerBuf)
{
    if (name == 0)
        return 0;
    if (headerBuf == 0)
        return 0;

    WwdInputStream stream;

    if (stream.Open(name, 0, 0) == 0)        // Open returns 0 on failure
        return 0;

    if (stream.Read(headerBuf, 0x5f4) != 0x5f4)
        return 0;

    if (*(unsigned int*)headerBuf > 0x5f4)   // signature must be <= 1524
        return 0;

    return 1;
}

// ---------------------------------------------------------------------------
// WwdFile::CheckHeader
// Same validation as IsValidWwd but reads into a PRIVATE 0x5F4 stack buffer,
// then copies the header out to the caller (an inline strlen+rep-movs copy of
// the NUL-terminated leading bytes - the binary does `repnz scasb; rep movs`,
// i.e. a strcpy of the header buffer into the caller's output).
RVA(0x160660, 0x12b)
int __stdcall WwdFile_CheckHeader(const char* name, void* headerOut)
{
    char header[0x5f4];

    if (name == 0)
        return 0;
    if (headerOut == 0)
        return 0;

    WwdInputStream stream;

    if (stream.Open(name, 0, 0) == 0)        // Open returns 0 on failure
        return 0;

    if (stream.Read(header, 0x5f4) != 0x5f4)
        return 0;

    if (*(unsigned int*)header > 0x5f4)      // signature must be <= 1524
        return 0;

    strcpy((char*)headerOut, header);        // inline strlen + rep movs
    return 1;
}

// ---------------------------------------------------------------------------
// CGameLevelPlanes::ReadPlane (__thiscall ret 0xc).
// Build one plane: `new CPlane(this->m_field0c, this->m_planeCount, 0)` (operator
// new(0x158) under the C++ EH frame), then invoke the plane's block reader
// (vtable +0x28) on (planeData, blockBase, &this->m_planeCtx). On reader failure,
// delete the plane (scalar-deleting dtor, vtable +0x4) and return 0. On success,
// append the plane to m_planes (CArray::SetAtGrow) at index
// m_planeCount, and if it is the MAIN plane (m_flags bit0) cache it as m_mainPlane
// with m_mainIndex = m_planeCount - 1. Returns the new plane.
//
// The new CPlane and its virtuals are UNMATCHED engine code -> reloc-masked calls.
RVA(0x15d8d0, 0xc3)
CPlane* CGameLevelPlanes::ReadPlane(void* planeData, void* blockBase, void* /*unused*/)
{
    CPlane* plane = new CPlane(m_field0c, m_planeCount, 0);

    if (plane->Read(planeData, blockBase, &m_planeCtx) == 0)
    {
        if (plane)
            plane->dtor(1);                       // scalar-deleting dtor (vtable +0x4)
        return 0;
    }

    ((CPlanePtrArray*)&m_planes)->SetAtGrow(m_planeCount, plane);

    if (plane->m_flags & 1)                       // MAIN plane
    {
        m_mainPlane = plane;
        m_mainIndex = m_planeCount - 1;
    }

    return plane;
}

// ---------------------------------------------------------------------------
// WwdFile::InflateMainBlock
// Validates the header, copies the 0x5F4-byte header prefix into dest, then
// zlib-uncompresses the COMPRESS main block into the remainder. Returns dest on
// success, 0 on any validation/inflate failure. (~88.7% fuzzy, entropy plateau.)
RVA(0x160790, 0xd2)
int __stdcall WwdFile_InflateMainBlock(WwdHeader* src, Bytef* dest, unsigned int destLen)
{
    uLongf outLen;

    if (src == 0)
        return 0;
    if (dest == 0)
        return 0;

    if (src->wwdSignature > 0x5f4)           // header size (== 1524)
        return 0;
    if ((src->flags & 0x2) == 0)             // require COMPRESS
        return 0;
    if (src->mainBlockLength == 0)
        return 0;
    if (src->mainBlockLength > destLen + src->wwdSignature)
        return 0;

    memcpy(dest, src, src->wwdSignature);     // copy the 1524-byte header prefix
    outLen = (uLongf)(destLen - src->wwdSignature);
    if (uncompress(dest + src->wwdSignature, &outLen,
                   (Bytef*)src + src->wwdSignature, src->mainBlockLength) != 0)
        return 0;

    return outLen == src->mainBlockLength ? (int)dest : 0;
}

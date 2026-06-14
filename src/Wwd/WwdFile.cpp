// WwdFile.cpp - Gruntz WWD level-file loader (Monolith-faithful reconstruction).
//
// Functions matched in this TU:
//   WwdFile::IsValidWwd       @ RVA 0x160530 (293 B, __stdcall ret 8) - BYTE-EXACT
//   WwdFile::CheckHeader      @ RVA 0x160660 (299 B, __stdcall ret 8) - BYTE-EXACT
//   WwdFile::InflateMainBlock @ RVA 0x160790 (~88.7% fuzzy, entropy plateau)
//
// IsValidWwd / CheckHeader open a file by name, read the 1524-byte (0x5F4) WWD
// header, and validate it: the read must return exactly 0x5F4 bytes and the
// header signature (first u32 == sizeof(WwdHeader)) must be <= 0x5F4. IsValidWwd
// reads straight into the caller buffer; CheckHeader reads into a private 0x5F4
// stack buffer and then copies that header out to the caller (inline strcpy-like
// rep movs of the NUL-terminated... actually a header byte-copy of strlen+1).
//
// Both construct an engine binary-file stream on the stack (ctor @0x1befd7, dtor
// @0x1bf121, Open @0x1bf200, Read @0x1bf328). The stack object has a non-trivial
// destructor, so MSVC emits a C++ EH frame (__CxxFrameHandler + FuncInfo, the
// fs:0 registration + `push -1; push handler`): this TU is built with /GX. The
// stream's ctor/dtor/Open/Read are unmatched engine functions, but their call
// bytes are reloc-masked in objdiff, so the validators are still byte-exact.
//
// __stdcall (callee cleans 8 bytes; ret 0x8 in the binary): each takes two
// pointer args and uses callee-cleanup, so they reconstruct as __stdcall free
// functions. Returns are full-width eax (1 / 0), i.e. `int`, not bool.
#include "WwdFile.h"

#include <string.h>  // memcpy

// ---------------------------------------------------------------------------
// WwdFile::IsValidWwd  @ RVA 0x160530 (293 B).
// Open(name) -> Read(headerBuf, 0x5F4) -> require read == 0x5F4 and first u32
// (the header signature) <= 0x5F4. The two null guards return BEFORE the stream
// is constructed (no destructor on those paths); the stream's ctor runs only
// after both guards, so its dtor unwinds the remaining exits.
// ---------------------------------------------------------------------------
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
// WwdFile::CheckHeader  @ RVA 0x160660 (299 B).
// Same validation as IsValidWwd but reads into a PRIVATE 0x5F4 stack buffer,
// then copies the header out to the caller (an inline strlen+rep-movs copy of
// the NUL-terminated leading bytes - the binary does `repnz scasb; rep movs`,
// i.e. a strcpy of the header buffer into the caller's output).
// ---------------------------------------------------------------------------
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

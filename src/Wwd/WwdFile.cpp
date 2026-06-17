// WwdFile.cpp - Gruntz WWD level-file loader (Monolith-faithful reconstruction).
//
// Functions matched in this TU:
//   WwdFile::IsValidWwd           @ RVA 0x160530 (293 B, __stdcall ret 8) - BYTE-EXACT
//   WwdFile::CheckHeader          @ RVA 0x160660 (299 B, __stdcall ret 8) - BYTE-EXACT
//   WwdFile::InflateMainBlock     @ RVA 0x160790 (~88.7% fuzzy, entropy plateau)
//   WwdFile::ValidateMainBlock    @ RVA 0x03b470 (314 B, __stdcall ret 4) - target
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
// ValidateMainBlock takes an AfxString (MFC CString) by value, whose destructor
// runs on the return path, so MSVC emits the same C++ EH frame. It calls
// CheckHeader into a local buffer, scans the header for a digit string, parses
// it as a version number, and returns the result (or -1 on any failure).
//
// __stdcall (callee cleans 8 bytes; ret 0x8 in the binary): each takes two
// pointer args and uses callee-cleanup, so they reconstruct as __stdcall free
// functions. Returns are full-width eax (1 / 0), i.e. `int`, not bool.
// ValidateMainBlock takes 1 arg (by-value CString, 4-byte pointer) and is
// __stdcall ret 4 (callee cleans 4 bytes).
#include "WwdFile.h"

#include <string.h>  // memcpy, strcpy

// ---------------------------------------------------------------------------
// Minimal AfxString (MFC CString) for the by-value parameter in
// ValidateMainBlock. The engine destructor (@0x1b9cde) is external/no-body so
// its call displacement reloc-masks. The ctor / copy-ctor / operator= are not
// needed in the callee (the caller builds the CString; the callee only destroys
// it on the return path).
// ---------------------------------------------------------------------------
class AfxString {
public:
    ~AfxString();                      // @0x1b9cde (engine external)
    operator const char *() const { return m_pchData; }
    char *m_pchData;
};

// The global game registry pointer (binary @0x64556c, reloc-masked GPR32 load).
// Used to verify the engine is initialised before reading the WWD file.
// @data: 0x64556c
extern void *g_pGameRegistry_64556c;

// CRT atoi() helper (LIBCMT @0x11ffb0, reloc-masked rel32 call).
extern "C" int __cdecl atoi(const char *str);

// CheckHeader - declared in WwdFile.h.
int __stdcall WwdFile_CheckHeader(const char *name, void *headerOut);

// ---------------------------------------------------------------------------
// WwdFile::IsValidWwd
// Open(name) -> Read(headerBuf, 0x5F4) -> require read == 0x5F4 and first u32
// (the header signature) <= 0x5F4. The two null guards return BEFORE the stream
// is constructed (no destructor on those paths); the stream's ctor runs only
// after both guards, so its dtor unwinds the remaining exits.
//
// @address: 0x160530
// @size:    0x125
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
// WwdFile::CheckHeader
// Same validation as IsValidWwd but reads into a PRIVATE 0x5F4 stack buffer,
// then copies the header out to the caller (an inline strlen+rep-movs copy of
// the NUL-terminated leading bytes - the binary does `repnz scasb; rep movs`,
// i.e. a strcpy of the header buffer into the caller's output).
//
// @address: 0x160660
// @size:    0x12b
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

// ---------------------------------------------------------------------------
// WwdFile::InflateMainBlock
// Validates the header, copies the 0x5F4-byte header prefix into dest, then
// zlib-uncompresses the COMPRESS main block into the remainder. Returns dest on
// success, 0 on any validation/inflate failure. (~88.7% fuzzy, entropy plateau.)
//
// @address: 0x160790
// @size:    0xd2
// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
// WwdFile::ValidateMainBlock @0x03b470 (314B, __stdcall ret 4)
// Validates a WWD file by name (passed as an MFC CString / AfxString by value)
// and extracts a version number from the header. Returns the parsed version
// integer on success, -1 on any failure (empty name, uninitialised engine,
// file not found / invalid, no version digit found).
//
// The by-value CString parameter has a destructor that must run on every exit
// path, so MSVC emits a C++ EH frame (__CxxFrameHandler + FuncInfo, the fs:0
// registration + `push -1; push handler`): this TU is built with /GX.
// ---------------------------------------------------------------------------
// @address: 0x03b470
// @size:    0x13a
int __stdcall WwdFile_ValidateMainBlock(AfxString name)
{
    // Gate 1: the name CString must be non-empty (nDataLength != 0).
    // The CString internal header is at m_pchData[-8] for nDataLength.
    if (*(int *)((const char *)name - 8) == 0)
        return -1;

    // Gate 2: the engine must be initialised (registry + indirect sub-field).
    void *reg = *(void **)0x64556c;
    void *r30 = reg == 0 ? 0 : *(void **)((char *)reg + 0x30);
    if (r30 == 0 || *(void **)((char *)r30 + 0x24) == 0)
        return -1;

    // Read the WWD header into a local buffer via CheckHeader.
    // CheckHeader strcpy's only the short NUL-terminated prefix (the header
    // starts with 0xF4 0x05 0x00 ...), so only a few bytes are written.
    char headerBuf[0x100];
    if (WwdFile_CheckHeader((const char *)name, headerBuf) == 0)
        return -1;

    // Scan for the first digit sequence in the header, parse it as a decimal
    // version number. Skip non-digit characters (everything up to the first
    // NUL), then call atoi on the first digit found.
    const char *p = headerBuf;
    while (*p) {
        if (*p >= '0' && *p <= '9')
            return atoi(p);
        ++p;
    }

    // No digit found in the header.
    return -1;
}

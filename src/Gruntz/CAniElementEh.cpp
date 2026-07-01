// CAniElementEh.cpp - the /GX (eh) sibling of CAniElement.cpp, carrying the single
// member whose destructible file-reader local forces the C++ EH frame (the rest of
// CAniElement stays in the frameless base TU; split per
// docs/patterns/split-tu-eh-dtor-vs-frameless-cstring.md).
//
//   0x165620  LoadFile - open a file via a stack-local reader, slurp it into a
//             RezAlloc'd buffer, hand the buffer to Build_165460, free it, return
//             the build result.  /GX frame from the reader local's non-trivial dtor.
//
// The file-reader engine helpers (0x1bexxx-0x1bf5xx, a CRT/MFC file class) and
// RezAlloc/RezFree are reloc-masked external __thiscall/__cdecl callees.
#include <rva.h>

#include <Gruntz/CAniElement.h>

// Engine heap alloc/free.  Reloc-masked __cdecl externs.
extern "C" void* RezAlloc(u32 size); // 0x1b9b46
extern "C" void RezFree(void* p);    // 0x1b9b82

// The stack-local file reader: ctor opens nothing, Open binds a path, GetSize/Read
// pull the bytes, the dtor closes/releases.  Non-trivial ctor+dtor so MSVC emits
// the destructible-local /GX frame (docs/patterns/gx-frame-destructible-local.md).
// All reloc-masked external __thiscall callees (no body).
class CAniFileReader {
public:
    CAniFileReader();                   // 0x1befd7
    ~CAniFileReader();                  // 0x1bf121
    i32 Open(void* name, i32 a, i32 b); // 0x1bf200  nonzero on success
    i32 GetSize();                      // 0x1bf505
    i32 Read(void* buf, i32 len);       // 0x1bf328  nonzero on success
};

// ---------------------------------------------------------------------------
// 0x165620: load + build the element from a file.  Open the reader on `filename`;
// on failure return 0.  Otherwise read the whole file into a RezAlloc'd buffer,
// hand it to Build_165460(ctx, buf, 0), free the buffer, and return the build
// result (the reader local is destroyed on every exit).  __thiscall, ret 0xc.
// @early-stop
// 97.29% — the whole body is byte-faithful (Open/GetLength/RezAlloc/Read/Build/
// RezFree + the three reader-dtor cleanup tails + the /GX frame).  Residual is the
// EH scope-table cookie (retail push 0x8 / Unwind@005e2410 vs our push 0x0 / $L
// funclet) + the reloc-masked names (retail's reader is CFileIO with a virtual
// dtor; modeling that is matching-neutral, tested).  docs/patterns/gx-scoped-local-
// eh-frame-size.md.
RVA(0x00165620, 0x101)
i32 CAniElement::LoadFile_165620(void* ctx, void* filename, i32 a3) {
    CAniFileReader fr;
    if (fr.Open(filename, 0, 0) == 0) {
        return 0;
    }
    i32 size = fr.GetSize();
    void* buf = RezAlloc(size);
    if (fr.Read(buf, size) == 0) {
        RezFree(buf);
        return 0;
    }
    i32 r = Build_165460(ctx, (CAniSource*)buf, 0);
    RezFree(buf);
    return r;
}

SIZE_UNKNOWN(CAniFileReader);

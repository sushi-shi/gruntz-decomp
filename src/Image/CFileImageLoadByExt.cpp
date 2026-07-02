// CFileImageLoadByExt.cpp - CFileImage::LoadByExt (0x148940), the .BMP/.PCX/.PID
// file-extension dispatcher (== the DIRSURF.CPP surface). Re-homed from
// src/Stub/CFileImageDecode.cpp.
//
// Kept in its own TU (NOT folded into CFileImage.cpp) with a minimal CFileImage view
// to avoid disturbing CFileImage.cpp's byte-exact neighbors. The default-loader branch
// is just CFileImage::Load (the RT_BITMAP loader at 0x144270, body in the ApiCallers
// TU) - the same __thiscall `this`, no separate wrapper class.
// Only offsets / code bytes are load-bearing; helpers are reloc-masked externals.
#include <rva.h>

class CFileImageInfo; // the decode-target info block (reloc-masked param type)

class CFileImage {
public:
    i32 LoadByExt(CFileImageInfo* info, char* path, i32 flags, i32 a4);
    i32 LoadFile2(CFileImageInfo* info, const char* path, i32 flags); // 0x143e60 .BMP
    i32 LoadFile(CFileImageInfo* info, const char* path, i32 flags);  // 0x144d80 .PCX
    i32 DecodePcxEx(char* a, char* b, void* c, void* d);              // 0x1459d0 .PID
    // The RT_BITMAP resource loader (its `this` is the same CFileImage; external
    // no-body/reloc-masked - the body lives in the ApiCallers TU).
    i32 Load(i32 a, char* name, i32 c); // 0x144270 (thiscall, default loader)
    void FillPalette(void* pal);        // 0x13eb40
};

// The engine's own strrchr / case-insensitive compare (cdecl C-linkage helpers).
extern "C" char* RezStrrchr(const char* s, i32 c);       // FUN_00120680 (_RezStrrchr)
extern "C" i32 RezStricmp(const char* a, const char* b); // FUN_0011fdf0 (_RezStricmp)

// CFileImage::LoadByExt - load an image by inspecting its file
// extension. Forces the IMAGEZ flag (|0x40), finds the extension, and dispatches
// to LoadFile2 (.BMP) / LoadFile (.PCX) / DecodePcxEx (.PID) or the default loader
// (ResLoad_144270::Load, the same __thiscall `this`). On a successful load (except
// the .PID path) it fills the palette from a4 when a4 != -1.
// @source: string-xref (.BMP/.PCX/.PID extension table)
RVA(0x00148940, 0x102)
i32 CFileImage::LoadByExt(CFileImageInfo* info, char* path, i32 flags, i32 a4) {
    flags |= 0x40;
    i32 doFill = 1;
    char* ext = RezStrrchr(path, '.');
    if (ext != 0 && RezStricmp(ext, ".BMP") == 0) {
        if (LoadFile2(info, path, flags) == 0) {
            return 0;
        }
    } else if (ext != 0 && RezStricmp(ext, ".PCX") == 0) {
        if (LoadFile(info, path, flags) == 0) {
            return 0;
        }
    } else if (ext != 0 && RezStricmp(ext, ".PID") == 0) {
        if (DecodePcxEx((char*)info, path, (void*)flags, (void*)a4) == 0) {
            return 0;
        }
        doFill = 0;
    } else if (this->Load((i32)info, path, flags) == 0) {
        return 0;
    }
    if (a4 != -1 && doFill != 0) {
        FillPalette((void*)a4);
    }
    return 1;
}

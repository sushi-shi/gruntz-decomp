// CFileImageLoadByExt.cpp - CFileImage::LoadByExt (0x148940), the .BMP/.PCX/.PID
// file-extension dispatcher (== the DIRSURF.CPP surface). Re-homed from
// src/Stub/CFileImageDecode.cpp.
//
// Kept in its own TU (NOT folded into CFileImage.cpp): the default-loader branch
// dispatches through a `this`-cast to the RT_BITMAP loader at 0x144270 (defined in
// the ApiCallers grab-bag as ResLoad_144270::Load); modeling that cast against the
// full CFileImage class in CFileImage.cpp risks its byte-exact neighbors, so the
// self-contained minimal model is preserved here (byte-identical to the former stub).
// Only offsets / code bytes are load-bearing; helpers are reloc-masked externals.
#include <rva.h>

class CFileImageInfo; // the decode-target info block (reloc-masked param type)

// The RT_BITMAP resource loader at 0x144270 (its `this` is the same CFileImage;
// declared minimally here so its __thiscall `call` reloc-masks - defined in the
// ApiCallers TU as ResLoad_144270::Load).
struct ResLoad_144270 {
    i32 Load(i32 a, char* name, i32 c); // 0x144270 (thiscall)
};

class CFileImage {
public:
    i32 LoadByExt(CFileImageInfo* info, char* path, i32 flags, i32 a4);
    i32 LoadFile2(CFileImageInfo* info, const char* path, i32 flags); // 0x143e60 .BMP
    i32 LoadFile(CFileImageInfo* info, const char* path, i32 flags);  // 0x144d80 .PCX
    i32 DecodePcxEx(char* a, char* b, void* c, void* d);              // 0x1459d0 .PID
    void FillPalette(void* pal);                                      // 0x13eb40
};

// The engine's own strrchr / case-insensitive compare (cdecl C-linkage helpers).
extern "C" char* RezStrrchr(const char* s, i32 c);       // FUN_00120680 (_RezStrrchr)
extern "C" i32 RezStricmp(const char* a, const char* b); // FUN_0011fdf0 (_RezStricmp)

// CFileImage::LoadByExt (0x148940) - load an image by inspecting its file
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
    } else if (((ResLoad_144270*)this)->Load((i32)info, path, flags) == 0) {
        return 0;
    }
    if (a4 != -1 && doFill != 0) {
        FillPalette((void*)a4);
    }
    return 1;
}

SIZE_UNKNOWN(ResLoad_144270);

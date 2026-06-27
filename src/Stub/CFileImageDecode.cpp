#include <rva.h>
// CFileImageDecode.cpp - reloc-correlation stubs for the remaining CFileImage
// (== the DIRSURF.CPP surface) decode helpers. The matched surface blitters
// FillPalette/BlitSurf/Blit/BlitDirect now live as real bodies in
// src/Image/Image.cpp; what remains stubbed here:
//   * the four run-decoders DecodeRun8/DecodeRun24/RunDecode1/RunDecode3 - retail
//     compiled these FRAMED/UNOPTIMIZED (push ebp; mov ebp,esp; sub esp,N - the
//     MSVC 5.0 optimizer-bailout codegen, NOT /O2), so the /O2 image TU can't
//     reproduce them; they need an /O1-or-bailout path (follow-up).
//   * the six Blit<dest><src> specializations (0x13fbb0..0x140420) the Blit
//     dispatcher tail-calls - genuinely-new leaf blitters, stubbed so Blit's
//     dispatch calls reloc-mask against named symbols.
// All are unnamed in retail (FUN_*); the names are placeholders, so only the RVA
// + the (class+name+param) mangling are load-bearing. Empty bodies (real RVA())
// so each caller's call reloc-masks against a named symbol.
//
// Minimal inline CFileImage decl (the stub convention - the All.cpp aggregation
// requires each stub to NOT pull a shared class header, so the methods are
// declared here only enough to mangle identically to the Image.cpp callers).
class CFileImageInfo; // the decode-target info block (reloc-masked param type)
class CFileImage {
public:
    i32 DecodeRun8(void*);
    i32 DecodeRun24(void*);
    i32 RunDecode1(void*, void*, i32, i32);
    i32 RunDecode3(void*, void*, i32, i32);
    // The per-(dest,src) bpp blit specializations Blit() dispatches to.
    i32 Blit248(void*, void*, i32);
    i32 Blit2416(void*, i32);
    i32 Blit1624(void*, i32);
    i32 Blit168(void*, void*, i32);
    i32 Blit824(void*, void*, i32);
    i32 Blit816(void*, void*, i32);
    // The .BMP/.PCX/.PID format dispatcher (0x148940) + the per-format decoders
    // it forwards to (reloc-masked externals defined in cfileimage/image units).
    i32 LoadByExt(CFileImageInfo* info, char* path, i32 flags, i32 a4);
    i32 LoadFile2(CFileImageInfo* info, const char* path, i32 flags);  // 0x143e60 .BMP
    i32 LoadFile(CFileImageInfo* info, const char* path, i32 flags);   // 0x144d80 .PCX
    i32 DecodePcxEx(char* a, char* b, void* c, void* d);               // 0x1459d0 .PID
    void FillPalette(void* pal);                                       // 0x13eb40
};
// The engine's own strrchr / case-insensitive compare (cdecl C-linkage helpers).
extern "C" char* RezStrrchr(const char* s, i32 c); // FUN_00120680 (_RezStrrchr)
extern "C" i32 RezStricmp(const char* a, const char* b); // FUN_0011fdf0 (_RezStricmp)

// CFileImage::LoadByExt (0x148940) - load an image by inspecting its file
// extension. Forces the IMAGEZ flag (|0x40), finds the extension, and dispatches
// to LoadFile2 (.BMP) / LoadFile (.PCX) / DecodePcxEx (.PID) or the default
// loader (ResLoad_144270::Load, the same __thiscall `this` - a CFileImage method
// the delinker placeheld under a per-address name). On a successful load (except
// the .PID path) it fills the palette from a4 when a4 != -1. Only offsets / code
// bytes are load-bearing; helpers are reloc-masked externals.
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
    } else if (((ApiCallerStubs::ResLoad_144270*)this)->Load((i32)info, path, flags) == 0) {
        return 0;
    }
    if (a4 != -1 && doFill != 0) {
        FillPalette((void*)a4);
    }
    return 1;
}

// @confidence: high
// @source: reloc-correlation (CFileImage::Blit dispatcher)
// @stub
RVA(0x0013fbb0, 0x126)
i32 CFileImage::Blit168(void*, void*, i32) {
    return 0;
}
// @confidence: high
// @source: reloc-correlation (CFileImage::Blit dispatcher)
// @stub
RVA(0x0013fce0, 0x17f)
i32 CFileImage::Blit1624(void*, i32) {
    return 0;
}
// @confidence: high
// @source: reloc-correlation (CFileImage::Blit dispatcher)
// @stub
RVA(0x0013fe60, 0x11e)
i32 CFileImage::Blit248(void*, void*, i32) {
    return 0;
}
// @confidence: high
// @source: reloc-correlation (CFileImage::Blit dispatcher)
// @stub
RVA(0x0013ff80, 0x184)
i32 CFileImage::Blit2416(void*, i32) {
    return 0;
}
// @confidence: high
// @source: reloc-correlation (CFileImage::Blit dispatcher)
// @stub
RVA(0x00140110, 0x30b)
i32 CFileImage::Blit824(void*, void*, i32) {
    return 0;
}
// @confidence: high
// @source: reloc-correlation (CFileImage::Blit dispatcher)
// @stub
RVA(0x00140420, 0x34f)
i32 CFileImage::Blit816(void*, void*, i32) {
    return 0;
}
// @confidence: high
// @source: reloc-correlation (CFileImage decoders)
// @stub
RVA(0x00140aa0, 0x1a3)
i32 CFileImage::DecodeRun8(void*) {
    return 0;
}
// @confidence: high
// @source: reloc-correlation (CFileImage::DecodePcx)
// @stub
RVA(0x00140c50, 0x3e2)
i32 CFileImage::DecodeRun24(void*) {
    return 0;
}
// @confidence: high
// @source: reloc-correlation (CFileImage decoders)
// @stub
RVA(0x00145270, 0x17a)
i32 CFileImage::RunDecode1(void*, void*, i32, i32) {
    return 0;
}
// @confidence: high
// @source: reloc-correlation (CFileImage::DecodePcx)
// @stub
RVA(0x001453f0, 0x3ac)
i32 CFileImage::RunDecode3(void*, void*, i32, i32) {
    return 0;
}

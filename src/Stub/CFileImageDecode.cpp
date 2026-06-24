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
};

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

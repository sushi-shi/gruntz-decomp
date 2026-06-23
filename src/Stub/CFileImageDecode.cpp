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
    int DecodeRun8(void*);
    int DecodeRun24(void*);
    int RunDecode1(void*, void*, int, int);
    int RunDecode3(void*, void*, int, int);
    // The per-(dest,src) bpp blit specializations Blit() dispatches to.
    int Blit248(void*, void*, int);
    int Blit2416(void*, int);
    int Blit1624(void*, int);
    int Blit168(void*, void*, int);
    int Blit824(void*, void*, int);
    int Blit816(void*, void*, int);
};

// @confidence: high
// @source: reloc-correlation (CFileImage::Blit dispatcher)
// @stub
RVA(0x13fbb0, 0x126)
int CFileImage::Blit168(void*, void*, int) {
    return 0;
}
// @confidence: high
// @source: reloc-correlation (CFileImage::Blit dispatcher)
// @stub
RVA(0x13fce0, 0x17f)
int CFileImage::Blit1624(void*, int) {
    return 0;
}
// @confidence: high
// @source: reloc-correlation (CFileImage::Blit dispatcher)
// @stub
RVA(0x13fe60, 0x11e)
int CFileImage::Blit248(void*, void*, int) {
    return 0;
}
// @confidence: high
// @source: reloc-correlation (CFileImage::Blit dispatcher)
// @stub
RVA(0x13ff80, 0x184)
int CFileImage::Blit2416(void*, int) {
    return 0;
}
// @confidence: high
// @source: reloc-correlation (CFileImage::Blit dispatcher)
// @stub
RVA(0x140110, 0x30b)
int CFileImage::Blit824(void*, void*, int) {
    return 0;
}
// @confidence: high
// @source: reloc-correlation (CFileImage::Blit dispatcher)
// @stub
RVA(0x140420, 0x34f)
int CFileImage::Blit816(void*, void*, int) {
    return 0;
}
// @confidence: high
// @source: reloc-correlation (CFileImage decoders)
// @stub
RVA(0x140aa0, 0x1a3)
int CFileImage::DecodeRun8(void*) {
    return 0;
}
// @confidence: high
// @source: reloc-correlation (CFileImage::DecodePcx)
// @stub
RVA(0x140c50, 0x3e2)
int CFileImage::DecodeRun24(void*) {
    return 0;
}
// @confidence: high
// @source: reloc-correlation (CFileImage decoders)
// @stub
RVA(0x145270, 0x17a)
int CFileImage::RunDecode1(void*, void*, int, int) {
    return 0;
}
// @confidence: high
// @source: reloc-correlation (CFileImage::DecodePcx)
// @stub
RVA(0x1453f0, 0x3ac)
int CFileImage::RunDecode3(void*, void*, int, int) {
    return 0;
}

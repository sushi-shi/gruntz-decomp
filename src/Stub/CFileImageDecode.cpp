#include <rva.h>
// CFileImageDecode.cpp - reloc-correlation stubs for the CFileImage decode
// helpers the per-format decoders (DecodeBmp/DecodePcx/DecodePcxData/DecodePid in
// src/Image/Image.cpp) call: the surface blitters (Blit/BlitDirect/BlitSurf) and
// the raw run-decoders (DecodeRun8/DecodeRun24/RunDecode1/RunDecode3) plus the
// transparency installer (FillPalette). All are unnamed in retail (FUN_*); the
// names are placeholders, so only the RVA + the (class+name+param) mangling are
// load-bearing. Empty bodies (real RVA()) so each caller's call reloc-masks
// against a named symbol; reconstructing them is a later step.
//
// Minimal inline CFileImage decl (the stub convention - the All.cpp aggregation
// requires each stub to NOT pull a shared class header, so the methods are
// declared here only enough to mangle identically to the Image.cpp callers).
class CFileImage {
public:
    int Blit(void*, int, void*, int);
    int BlitDirect(void*, int);
    int BlitSurf(void*, int, int, int, int);
    int DecodeRun8(void*);
    int DecodeRun24(void*);
    int RunDecode1(void*, void*, int, int);
    int RunDecode3(void*, void*, int, int);
    void FillPalette(void*);
};

// @confidence: high
// @source: reloc-correlation (CFileImage decoders)
// @stub
RVA(0x13faa0, 0x108)
int CFileImage::Blit(void*, int, void*, int) {
    return 0;
}
// @confidence: high
// @source: reloc-correlation (CFileImage::DecodeBmp)
// @stub
RVA(0x13ece0, 0xc7)
int CFileImage::BlitDirect(void*, int) {
    return 0;
}
// @confidence: high
// @source: reloc-correlation (CFileImage::DecodePcxData)
// @stub
RVA(0x13e0d0, 0x66)
int CFileImage::BlitSurf(void*, int, int, int, int) {
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
// @confidence: high
// @source: reloc-correlation (CFileImage decoders)
// @stub
RVA(0x13eb40, 0x3c)
void CFileImage::FillPalette(void*) {}

#include <rva.h>
// CImageDecode.cpp - engine-label stubs for CImage's per-format decode helpers.
//
// These are the __thiscall CImage methods the five per-extension loaders
// (CImage::Load{Bmp,Pcx,Rid,Pid,Default} in src/Image/Image.cpp) call to
// actually build the decoded image. They carry no leaked names (Ghidra labels
// them FUN_*), so the names are placeholders; only their RVAs + signatures are
// load-bearing. Stubbed here (empty bodies, real RVA()) so the loaders' calls
// resolve to named symbols and can flip exact. Reconstructing them is the next
// step. Minimal inline CImage decl (the stub convention); the mangling matches
// the callers (depends only on class name + method name + param types).

class CImage {
public:
    int DecodeBmpHeader(void*, int, int, int, void*);
    int DecodePcxData(void*, void*, void*);
    int DecodeRidData(void*, void*, void*);
    int DecodePidData(void*, void*, void*);
    int DecodeResData(void*, void*, void*);
};
// @confidence: med
// @source: reloc-correlation (CImage::LoadBmp + the PCX/RID/PID/default decoders)
// @stub
RVA(0x1757c0, 0x16f)
int CImage::DecodeBmpHeader(void*, int, int, int, void*) {
    return 0;
}
// @confidence: med
// @source: reloc-correlation (CImage::LoadPcx)
// @stub
RVA(0x176000, 0x18f)
int CImage::DecodePcxData(void*, void*, void*) {
    return 0;
}
// @confidence: med
// @source: reloc-correlation (CImage::LoadRid)
// @stub
RVA(0x1762c0, 0x42)
int CImage::DecodeRidData(void*, void*, void*) {
    return 0;
}
// @confidence: med
// @source: reloc-correlation (CImage::LoadPid)
// @stub
RVA(0x176440, 0x25d)
int CImage::DecodePidData(void*, void*, void*) {
    return 0;
}
// @confidence: med
// @source: reloc-correlation (CImage::LoadDefault)
// @stub
RVA(0x175e00, 0x3d)
int CImage::DecodeResData(void*, void*, void*) {
    return 0;
}

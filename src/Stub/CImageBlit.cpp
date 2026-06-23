#include <rva.h>
// CImageBlit.cpp - the shared CImage plane blitter (FUN_00575930), a __thiscall
// CImage method the format decoders (DecodeResData/DecodeRidData and, indirectly,
// the others) call to (re)allocate the plane via DecodeBmpHeader and copy the
// decoded scanlines into it (flat rep-movs when m_448==0, else row-by-row through
// the +0x430 offset table). No leaked name -> placeholder DecodeBlit; only the
// RVA + signature are load-bearing. Stubbed here (empty body, real RVA()) so the
// decoders' calls resolve to a named symbol; reconstructing it is a later step.
// Minimal inline CImage decl (the stub convention); the mangling matches the
// callers (it depends only on the class + method name + param types).

class CImage {
public:
    int DecodeBlit(void*, void*, int, int, int, void*);
};
// @confidence: high
// @source: reloc-correlation (CImage::DecodeResData + DecodeRidData)
// @stub
RVA(0x00175930, 0xc6)
int CImage::DecodeBlit(void*, void*, int, int, int, void*) {
    return 0;
}

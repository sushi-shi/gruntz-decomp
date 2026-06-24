#include <rva.h>
// CSBI_Image.cpp - engine-label stubs for CSBI_Image.
//
// NOTE: 0x10a340 was mislabeled `~CSBI_Image` by the rtti-vptr heuristic; the real
// scalar destructor is reconstructed at 0x100870 (src/Gruntz/SBI_ImageEh.cpp).
// 0x10a340 is a large /GX method that walks the CSBI_RectOnly +0x550 region (a
// save/serialize-style method), not the destructor. Relabeled `Method_10a340` to
// free the ??1 name for the real dtor.

class CSBI_Image {
public:
    void Method_10a340();
};

// @confidence: low
// @source: rtti-vptr
// @stub
RVA(0x0010a340, 0xbcb)
void CSBI_Image::Method_10a340() {}

// ImageSet3Eh.cpp - the /GX (eh) sibling of the imageset3 unit, hosting CImageSet3's
// standalone OUT-OF-LINE destructor (0x161500). Split from imageset3 (flags="base")
// because a /GX EH-frame funclet loses its unwind prologue in a non-/GX unit - the
// same SBI sbi_image / sbi_image_eh split. Re-homed from src/Stub/BoundaryUpperEh.cpp
// (matcher-2). Only OFFSETS + code shape are load-bearing.
#include <rva.h>

#include <Ints.h>

// The Rez heap free (0x1b9b82). C++ linkage (NOT extern "C") so cl treats it as
// potentially-throwing and keeps the /GX base-subobject unwind frame.
void RezFree(void* p);

// 0x161500 - the out-of-line ~CImageSet3: stamp derived (0x5f0228), free the +0x14
// pixel buffer and zero it, fold the CObject base dtor (0x5e8cb4). Kept a DISTINCT
// placeholder identity (C161500): folding onto the real CImageSet3 (imageset3 unit)
// needs its CLoadable base modeled as a non-trivial subobject to emit the EH frame
// (the deferred archetype on CDDrawSurfacePair::~). 0x5f0228 == ??_7CImageSet3
// (bound by VTBL in GameLevel.cpp).
struct Sev161500 {
    virtual ~Sev161500();
};
SIZE_UNKNOWN(Sev161500);
inline Sev161500::~Sev161500() {}
struct C161500 : Sev161500 {
    char _4[0x14 - 0x4];
    char* m_14; // +0x14  pixel buffer
    virtual ~C161500() OVERRIDE;
};
SIZE_UNKNOWN(C161500);
RELOC_VTBL(C161500, 0x001f0228); // aliases CImageSet3 (dtor-stamp verified)
RVA(0x00161500, 0x58)
C161500::~C161500() {
    if (m_14) {
        RezFree(m_14);
    }
    m_14 = 0;
}

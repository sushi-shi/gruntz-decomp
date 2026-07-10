// AniAdvanceCursorEh.cpp - the /GX (eh) sibling of the aniadvancecursor unit, hosting
// CAniAdvanceCursor's standalone OUT-OF-LINE destructor (0x15b6d0). Split from
// aniadvancecursor (flags="base") because a /GX EH-frame funclet loses its unwind
// prologue in a non-/GX unit (SBI sbi_image / sbi_image_eh split). Re-homed from
// src/Stub/BoundaryUpperEh.cpp (matcher-2). Only OFFSETS + code shape are load-bearing.
#include <rva.h>

#include <Mfc.h> // CObject grand-base (folds ??_7CObject @0x5e8cb4)
#include <Ints.h>
#include <DDrawMgr/DDrawBlitParam.h> // Reset_15c2c0 (0x15c2c0; reloc-masked)

// 0x15b6d0 - the out-of-line ~CAniAdvanceCursor: stamp derived (0x5f0128), run the
// CLoadable slot-7 Unload/Reset (0x15c2c0 == CAniAdvanceCursor's own Reset; the
// (CDDrawBlitParam*) cast is a reloc-masked placeholder for it), then reset
// m_4/m_8/m_c. Kept a DISTINCT placeholder identity (C15b6d0): the real
// CAniAdvanceCursor (aniadvancecursor unit) has an INLINE dtor, so this out-of-line ??1
// can't be it (inline XOR out-of-line). 0x5f0128 == ??_7CAniAdvanceCursor (bound by
// VTBL in <Gruntz/AniAdvanceCursor.h>).
// Grand-base fold @0x15b71b is the REAL ??_7CObject (0x5e8cb4, disasm-verified) - so
// C15b6d0 derives from the real CObject (no Sev shell). Distinct placeholder identity
// kept: name-injectivity forbids a 2nd ??1CAniAdvanceCursor (one-source/N-COMDAT wall).
struct C15b6d0 : CObject {
    i32 m_4; // +0x4
    i32 m_8; // +0x8
    i32 m_c; // +0xc
    virtual ~C15b6d0() OVERRIDE;
};
SIZE_UNKNOWN(C15b6d0);
RELOC_VTBL(C15b6d0, 0x001f0128); // aliases CAniAdvanceCursor (dtor-stamp verified)
RVA(0x0015b6d0, 0x5b)
C15b6d0::~C15b6d0() {
    ((CDDrawBlitParam*)this)->Reset_15c2c0();
    m_4 = -1;
    m_8 = 0;
    m_c = 0;
}

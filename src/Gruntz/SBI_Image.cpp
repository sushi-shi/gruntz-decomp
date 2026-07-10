#define SBI_DTOR_CHAIN     // enable the inline base-dtor bodies (see StatusBarItem.h)
#define SBI_OWN_IMAGE_DTOR // this TU supplies the out-of-line ~CSBI_Image (0x100870)
#include <rva.h>
#include <Mfc.h>
#include <Ints.h>
#include <Gruntz/SbiConfig.h> // canonical config-host family (one shape)
#include <Gruntz/SBI_Image.h> // canonical frameless CSBI_Image (: CSBI_RectOnly : CStatusBarItem)
// SBI_Image.cpp - Gruntz CSBI_Image (C:\Proj\Gruntz), the frameless methods.
// RTTI .?AVCSBI_Image@@; in the SBI family
//   CSBI_Image : CSBI_RectOnly : CStatusBarItem  (CSBI_ImageSet derives from this).
// Vtable @0x5eac0c. The /GX chain destructor (0x100870) is defined below - the
// former SBI_ImageEh.cpp companion split is collapsed (retail's one TU was /GX).
//
// These are concrete virtual-slot methods modeled with the SBI family's
// manual-vtable-stamp device (no real `virtual`); sibling/engine callees are
// ILT-reloc-masked.

// ---------------------------------------------------------------------------
// Shared engine views (modeled minimally; only the touched members/methods are
// load-bearing; every call through them is reloc-masked).

// The config host + its lookup map + record now come from the shared canonical
// family (<Gruntz/SbiConfig.h>): CSbiConfigHost / CSbiConfigMap / CSbiConfigRecord.

// CSBI_Image (+ its CSBI_RectOnly intermediate) now come from the canonical
// frameless header <Gruntz/SBI_Image.h>. SetupImage is defined below.

// vtable slot 11 (0xe6c80): store the live config args into the base-region
// fields, then (if a key is supplied) look up the config record through the host
// map and latch its value into m_30. Returns whether a non-zero value was latched.
// @early-stop
// ~65% (zero-register-pinning INVERSE wall): logic + every field store/value/guard
// is correct, but with four `== 0` null tests and two `field = 0` stores MSVC5 here
// PINS 0 in edi (extra push edi/pop edi + `cmp edi,reg` everywhere) while retail
// uses `test reg,reg` + immediate `mov [field],0` stores. Documented coin-flip
// regalloc wall (docs/patterns/zero-register-pinning.md, inverse case) - "no
// init-list/assignment/reorder lever flips it". Also a reloc-masked `call Lookup`.
// Deferred to the final sweep.
RVA(0x000e6c80, 0xc3)
i32 CSBI_Image::SetupImage(
    i32 a1,
    CSbiConfigHost* host,
    i32 a3,
    i32 a4,
    i32 a5,
    i32 a6,
    i32 a7,
    i32 a8,
    i32 key,
    i32 a10,
    i32 a11
) {
    if (host == 0) {
        return 0;
    }
    if (a1 == 0) {
        return 0;
    }
    m_2c = a1;
    m_10 = a4;
    m_24 = (i32)host;
    m_28 = 0;
    m_4 = 0;
    m_rect14.m_0 = a5;
    m_rect14.m_4 = a6;
    m_rect14.m_8 = a7;
    m_rect14.m_c = a8;
    m_c = a3;
    if (key == 0) {
        m_30 = 0;
        return 0 != 0;
    }
    CSbiConfigRecord* rec = 0;
    ((CMapStringToPtr*)&host->m_10->m_10map)->Lookup((const char*)key, (void*&)rec);
    if (rec == 0 || rec->m_64 > 1 || rec->m_68 < 1) {
        m_30 = 0;
        return 0 != 0;
    }
    i32 val = rec->m_14[1];
    m_30 = val;
    return val != 0;
}

// ---------------------------------------------------------------------------
// ~CSBI_Image (0x100870): the /GX chain destructor - stamp ??_7CSBI_Image, run
// DtorImage (0xe6d90; menu-item-TU-resident, reloc-masked), then MSVC folds the
// two inline base dtors in (??_7CSBI_RectOnly + DtorRect, ??_7CStatusBarItem +
// DtorStatus - the SBI_DTOR_CHAIN device; this TU owns ~CSBI_Image itself via
// SBI_OWN_IMAGE_DTOR) behind the /GX SEH frame with 0/1/-1 trylevels. Collapsed
// from SBI_ImageEh.cpp (3-level case of
// docs/patterns/eh-dtor-multilevel-polymorphic-chain.md).
RVA(0x00100870, 0x6a)
CSBI_Image::~CSBI_Image() {
    DtorImage();
}

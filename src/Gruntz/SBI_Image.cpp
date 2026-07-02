#include <rva.h>
#include <Mfc.h>
#include <Ints.h>
#include <Gruntz/SbiConfig.h> // canonical config-host family (one shape)
#include <Gruntz/StatusBarItem.h>
// SBI_Image.cpp - Gruntz CSBI_Image (C:\Proj\Gruntz), the frameless methods.
// RTTI .?AVCSBI_Image@@; in the SBI family
//   CSBI_Image : CSBI_RectOnly : CStatusBarItem  (CSBI_ImageSet derives from this).
// Vtable @0x5eac0c. The /GX-framed scalar destructor (0x100870) lives in
// SBI_ImageEh.cpp.
//
// These are concrete virtual-slot methods modeled with the SBI family's
// manual-vtable-stamp device (no real `virtual`); sibling/engine callees are
// ILT-reloc-masked.

// ---------------------------------------------------------------------------
// Shared engine views (modeled minimally; only the touched members/methods are
// load-bearing; every call through them is reloc-masked).

// The config host + its lookup map + record now come from the shared canonical
// family (<Gruntz/SbiConfig.h>): CSbiConfigHost / CSbiConfigMap / CSbiConfigRecord.

// ---------------------------------------------------------------------------
// CSBI_Image - the image status-bar item. Inherits the CSBI_RectOnly base-region
// fields; adds the vslot-11 image setup. Fields are placeholders; offsets are the
// load-bearing fact.
class CSBI_Image : public CStatusBarItem {
public:
    // vtable slot 11 (0xe6c80): the 11-arg image setup; arg1 = id, arg2 = config
    // host, args 3..8 = the rect block, arg9 = the lookup key (args 10/11 unused).
    i32 SetupImage(
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
    );

    i32 m_2c; // +0x2c  Setup id (== a1)
    i32 m_30; // +0x30  latched config value
};

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
    host->m_10->m_10map.Lookup(key, &rec);
    if (rec == 0 || rec->m_64 > 1 || rec->m_68 < 1) {
        m_30 = 0;
        return 0 != 0;
    }
    i32 val = rec->m_14[1];
    m_30 = val;
    return val != 0;
}

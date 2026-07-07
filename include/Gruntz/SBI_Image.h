// SBI_Image.h - Gruntz CSBI_Image (C:\Proj\Gruntz), the FRAMELESS method view.
// RTTI .?AVCSBI_Image@@; the real single-inheritance chain (RTTI-confirmed):
//   CSBI_Image : CSBI_RectOnly : CStatusBarItem   (vtable 0x5eac0c).
//
// This is the ONE canonical frameless spelling of CSBI_Image + its RectOnly
// intermediate, shared by the concrete method TUs (SBI_Image.cpp defines
// SetupImage; SBI_ImageSet.h derives CSBI_ImageSet from it). It is the FRAMELESS
// counterpart of two deliberate, NEVER-co-included views of the SAME retail class:
//   * <Gruntz/SbiDtorChain.h> - the CHAIN view (polymorphic /GX dtor chain, size
//     0x60 grand-base) the *Eh.cpp dtor TUs fold; and
//   * the builder-facet views (CSbConfigItem/CSbDialogItem/CSbMenuItem in the
//     CStatusBarMgr.cpp / SBI_TabzDialogEh.cpp / StatusBarGameMenu.cpp builders) -
//     the same CSBI_Image level modeled as a native-dispatch polymorphic base so
//     `new CSBI_X` auto-stamps the retail vtable and Configure lowers to
//     `call [edx+0x2c]`. One MSVC5 spelling emits only one of these shapes, so the
//     views stay split (see the two-view-split note atop StatusBarItem.h).
//
// Fields are placeholders; the offsets + total 0x34 size are the load-bearing fact.
#ifndef GRUNTZ_SBI_IMAGE_H
#define GRUNTZ_SBI_IMAGE_H

#include <Ints.h>
#include <rva.h>
#include <Gruntz/StatusBarItem.h> // canonical frameless CStatusBarItem base

// The +0x24 config host is the shared canonical CSbiConfigHost (SbiConfig.h, pulled
// in the .cpp); only a pointer is needed here, so forward-declare it.
struct CSbiConfigHost;

// CSBI_RectOnly - the empty intermediate between CStatusBarItem and CSBI_Image
// (RTTI: CSBI_Image : CSBI_RectOnly : CStatusBarItem). In retail it overrides only
// the virtual destructor and adds no fields; the frameless method view constructs
// nothing here, so modeling it as a field-less intermediate is layout-identical to
// deriving CStatusBarItem directly (matching-neutral) while recovering the real level.
class CSBI_RectOnly : public CStatusBarItem {
public:
    virtual ~CSBI_RectOnly() OVERRIDE; // slot 0
    virtual i32 SbiVfunc0() OVERRIDE;  // slot 1
    virtual void SbiSlot2() OVERRIDE;  // slot 2
    virtual void SbiSlot3() OVERRIDE;  // slot 3
    virtual void SbiSlot4() OVERRIDE;  // slot 4
    void InsertPtr(i32 a, i32 b);      // 0x... (plane-scan object sink)
};
SIZE_UNKNOWN(CSBI_RectOnly);

// CSBI_Image - the image status-bar item. Inherits the CStatusBarItem base-region
// fields (via CSBI_RectOnly); adds the vslot-11 image setup (ConfigureRect @ +0x2c)
// plus the id/latched-value pair.
class CSBI_Image : public CSBI_RectOnly {
public:
    virtual ~CSBI_Image() OVERRIDE;   // slot 0
    virtual i32 SbiVfunc0() OVERRIDE; // slot 1
    virtual void SbiSlot3() OVERRIDE; // slot 3
    virtual void SbiSlot4() OVERRIDE; // slot 4
    virtual void SbiSlot5() OVERRIDE; // slot 5
    // vtable slot 11 (0xe6c80): the 11-arg image setup; arg1 = id, arg2 = config
    // host, args 3..8 = the rect block, arg9 = the lookup key (args 10/11 unused).
    virtual i32 SetupImage( // slot 11 (new)

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
SIZE(CSBI_Image, 0x34);

#endif // GRUNTZ_SBI_IMAGE_H

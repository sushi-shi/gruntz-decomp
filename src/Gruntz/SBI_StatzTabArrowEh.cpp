#include <rva.h>
// SBI_StatzTabArrowEh.cpp - the /GX EH-framed CSBI_StatzTabArrow scalar destructor
// (C:\Proj\Gruntz). RTTI walked from the dtor's vtable-restore chain:
//   CSBI_StatzTabArrow : CSBI_ImageSetAni : CSBI_ImageSet : CSBI_Image
//                      : CSBI_RectOnly : CStatusBarItem
// (the six .?AVCSBI_*@@ / .?AVCStatusBarItem@@ type descriptors the dtor's six
// vptr-stamps walk; CSBI_StatzTabArrow @0x5eac94 is the most-derived). The labeler
// grouped this (with the CSBI_ImageSetAni dtor) as "ClassUnknown_116".
//
// The destructor walks the base-subobject chain bottom-up: before tearing down
// each base it re-stamps the vptr to that base's retail vtable, then calls that
// base's member teardown (all reloc-masked ILT/engine callees). /GX frames the
// whole walk (the auto-generated 0/1/2/3/4/-1 trylevel writes are the EH-state
// machine's). The vtable VALUES are reproduced by address (DATA() externs,
// reloc-masked) - the transitional manual-stamp device while the full hierarchy's
// vtables are not yet modeled. Same symbols as SBI_RectOnlyEh.cpp's g_vtbl_t3/t4.

DATA(0x001eac94)
extern void* g_vtbl_statzTabArrow[]; // 0x5eac94 (CSBI_StatzTabArrow most-derived subobject)
DATA(0x001ead6c)
extern void* g_vtbl_imageSetAni[]; // 0x5ead6c (CSBI_ImageSetAni subobject)
DATA(0x001eac4c)
extern void* g_vtbl_t4[]; // 0x5eac4c (CSBI_ImageSet subobject)
DATA(0x001eac0c)
extern void* g_vtbl_t3[]; // 0x5eac0c (CSBI_Image subobject)
DATA(0x001eab8c)
extern void* g_vtbl_rectBase[]; // 0x5eab8c (CSBI_RectOnly subobject)
DATA(0x001eabcc)
extern void* g_vtbl_statusBase[]; // 0x5eabcc (CStatusBarItem base subobject)

class CSBI_StatzTabArrow {
public:
    ~CSBI_StatzTabArrow();
    void DtorStatzTabArrow(); // most-derived (StatzTabArrow) member teardown
    void DtorImageSetAni();   // CSBI_ImageSetAni base teardown
    void DtorImageSet();      // CSBI_ImageSet base teardown
    void DtorImage();         // CSBI_Image base teardown
    void DtorRect();          // CSBI_RectOnly base teardown
    void DtorStatus();        // CStatusBarItem base teardown
    char m_pad[0x60];
};

// 0x1048f0: walk the full 6-level chain, re-stamping each base's vtable before its
// teardown call. /GX frames the whole walk (trylevel 0/1/2/3/4/-1 auto-generated).
// @early-stop
// eh-dtor-needs-base-subobject wall (docs/patterns/eh-dtor-needs-base-subobject.md):
// the six vptr-stamp + base-dtor-call pairs are byte-exact, but the whole /GX SEH
// frame (push -1/handler/fs:0 + the 0/1/2/3/4/-1 trylevel stamps) is MISSING - MSVC
// only frames a dtor whose base SUBOBJECT has a non-trivial dtor, which the
// manual-vptr non-polymorphic model can't express. Same ~43% plateau as the
// sibling ~CSBI_ImageSet (0x102000); deferred to the final sweep (whole-class
// model). Logic complete.
RVA(0x001048f0, 0xa9)
CSBI_StatzTabArrow::~CSBI_StatzTabArrow() {
    *(void**)this = g_vtbl_statzTabArrow;
    DtorStatzTabArrow();
    *(void**)this = g_vtbl_imageSetAni;
    DtorImageSetAni();
    *(void**)this = g_vtbl_t4;
    DtorImageSet();
    *(void**)this = g_vtbl_t3;
    DtorImage();
    *(void**)this = g_vtbl_rectBase;
    DtorRect();
    *(void**)this = g_vtbl_statusBase;
    DtorStatus();
}

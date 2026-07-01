#include <rva.h>
#include <Ints.h>
// SBI_ImageEh.cpp - the /GX EH-framed CSBI_Image destructor, split off the
// frameless sbi_image TU (C:\Proj\Gruntz). MSVC5's /GX frames the dtor's
// base-subobject teardown walk; it cannot share the base TU's frameless flags.
// The split is matching-neutral (each function is RVA-keyed).
//
// REAL polymorphic hierarchy now:  CSBI_Image : CSBI_RectOnly : CStatusBarItem.
// Each level has a non-trivial (inline) virtual destructor that calls its member
// teardown helper; MSVC folds the two base dtors into the most-derived ~CSBI_Image
// and - because the base subobjects are non-trivial - emits the full /GX SEH frame
// (push -1/handler/fs:0 + the 0/1/-1 trylevel stamps) plus the per-level vptr
// re-stamps. The three vftables ??_7CStatusBarItem / ??_7CSBI_RectOnly /
// ??_7CSBI_Image auto-derive on the target (RTTI; config/vtable_names.csv),
// replacing the transitional manual `*(void**)this = &g_vtbl_*` stamps.

// CStatusBarItem grand-base (vtable 0x5eabcc, 11 slots = vdtor + 10 virtuals).
struct CStatusBarItem {
    virtual ~CStatusBarItem();
    virtual void Vf1();
    virtual void Vf2();
    virtual void Vf3();
    virtual void Vf4();
    virtual void Vf5();
    virtual void Vf6();
    virtual void Vf7();
    virtual void Vf8();
    virtual void Vf9();
    virtual void Vf10();
    void DtorStatus(); // 0x10bfa0  CStatusBarItem base teardown
    char m_pad[0x60 - 0x04];
};
inline CStatusBarItem::~CStatusBarItem() {
    DtorStatus();
}

// CSBI_RectOnly base (vtable 0x5eab8c, 11 slots; overrides the vdtor -> own vtable).
struct CSBI_RectOnly : CStatusBarItem {
    virtual ~CSBI_RectOnly();
    void DtorRect(); // 0xe8760  CSBI_RectOnly base teardown
};
SIZE_UNKNOWN(CSBI_RectOnly);
inline CSBI_RectOnly::~CSBI_RectOnly() {
    DtorRect();
}

// CSBI_Image most-derived (vtable 0x5eac0c, 12 slots = vdtor + 10 + 1 new).
struct CSBI_Image : CSBI_RectOnly {
    virtual ~CSBI_Image();
    virtual void Vf11();
    void DtorImage(); // 0xe6d90  most-derived (Image) member teardown
};
SIZE_UNKNOWN(CSBI_Image);

// The most-derived destructor: stamp ??_7CSBI_Image, run DtorImage, then MSVC
// folds the base dtors (stamp ??_7CSBI_RectOnly + DtorRect, stamp
// ??_7CStatusBarItem + DtorStatus). The non-trivial base subobjects supply the /GX
// frame and the 0/1/-1 trylevel stamps.
RVA(0x00100870, 0x6a)
CSBI_Image::~CSBI_Image() {
    DtorImage();
}

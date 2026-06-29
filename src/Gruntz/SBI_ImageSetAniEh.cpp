#include <rva.h>
#include <Ints.h>
// SBI_ImageSetAniEh.cpp - the /GX EH-framed CSBI_ImageSetAni destructor (C:\Proj\Gruntz). The split
// off the frameless base TU is matching-neutral (each function is RVA-keyed).
//
// REAL polymorphic hierarchy:  CSBI_ImageSetAni : CSBI_ImageSet : CSBI_Image : CSBI_RectOnly : CStatusBarItem.
// Each base subobject has a non-trivial (inline) virtual dtor, so MSVC folds the
// base teardown chain into the most-derived ~CSBI_ImageSetAni and emits the full /GX SEH
// frame (push -1/handler/fs:0 + the descending trylevel stamps) plus the per-level
// vptr re-stamps. The ??_7 vftables auto-derive on the target (RTTI;
// config/vtable_names.csv), replacing the manual `*(void**)this = &g_vtbl_*` stamps.

struct CStatusBarItem {
    virtual ~CStatusBarItem();
    virtual void Sf1();
    virtual void Sf2();
    virtual void Sf3();
    virtual void Sf4();
    virtual void Sf5();
    virtual void Sf6();
    virtual void Sf7();
    virtual void Sf8();
    virtual void Sf9();
    virtual void Sf10();
    void DtorStatus(); // reloc-masked member teardown
    char m_pad[0x60 - 0x04];
};
inline CStatusBarItem::~CStatusBarItem() {
    DtorStatus();
}

struct CSBI_RectOnly : CStatusBarItem {
    virtual ~CSBI_RectOnly();
    void DtorRect(); // reloc-masked member teardown
};
inline CSBI_RectOnly::~CSBI_RectOnly() {
    DtorRect();
}

struct CSBI_Image : CSBI_RectOnly {
    virtual ~CSBI_Image();
    virtual void Imf1();
    void DtorImage(); // reloc-masked member teardown
};
inline CSBI_Image::~CSBI_Image() {
    DtorImage();
}

struct CSBI_ImageSet : CSBI_Image {
    virtual ~CSBI_ImageSet();
    virtual void Isf1();
    void DtorImageSet(); // reloc-masked member teardown
};
inline CSBI_ImageSet::~CSBI_ImageSet() {
    DtorImageSet();
}

struct CSBI_ImageSetAni : CSBI_ImageSet {
    virtual ~CSBI_ImageSetAni();
    virtual void Ianf1();
    virtual void Ianf2();
    void DtorImageSetAni(); // most-derived member teardown (reloc-masked)
};

RVA(0x001047f0, 0x94)
CSBI_ImageSetAni::~CSBI_ImageSetAni() {
    DtorImageSetAni();
}

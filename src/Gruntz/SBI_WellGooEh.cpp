#include <rva.h>
#include <Ints.h>
// SBI_WellGooEh.cpp - the /GX EH-framed CSBI_WellGoo destructor (C:\Proj\Gruntz).
// The split off the frameless sbi_wellgoo TU is matching-neutral (RVA-keyed).
//
// REAL polymorphic hierarchy:  CSBI_WellGoo : CSBI_Image : CSBI_RectOnly :
// CStatusBarItem.  The most-derived dtor frees the goo source surface (its own
// member teardown), then MSVC folds the non-trivial base subobject dtors in and
// emits the /GX SEH frame + per-level vptr re-stamps.  The ??_7 vftables auto-
// derive on the target (RTTI), replacing the manual `*(void**)this = &g_vtbl_*`.

// CStatusBarItem base (vtable 0x5eabcc, 11 slots = vdtor + 10 virtuals).
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
    void DtorStatus(); // 0x10bfa0
};
inline CStatusBarItem::~CStatusBarItem() {
    DtorStatus();
}

struct CSBI_RectOnly : CStatusBarItem {
    virtual ~CSBI_RectOnly();
    void DtorRect(); // 0xe8760
};
inline CSBI_RectOnly::~CSBI_RectOnly() {
    DtorRect();
}

struct CSBI_Image : CSBI_RectOnly {
    virtual ~CSBI_Image();
    virtual void Imf1();
    void DtorImage(); // 0xe6d90
};
inline CSBI_Image::~CSBI_Image() {
    DtorImage();
}

// The pool collection the goo surface is returned to: RemoveItemA (0x142160,
// __thiscall) frees one held surface. Reached via m_24->m_1c (the host pool).
struct CGooPool {
    void RemoveItemA(void* item); // 0x142160
};
struct CGooHost {
    char m_pad0[0x1c];
    CGooPool* m_1c; // +0x1c  surface pool
};

// CSBI_WellGoo most-derived (vtable 0x5eadfc, 12 slots; overrides the vdtor).
struct CSBI_WellGoo : CSBI_Image {
    virtual ~CSBI_WellGoo();
    char m_pad0[0x24 - 0x04]; // after the +0x00 vptr
    CGooHost* m_24;           // +0x24  config host (its m_1c is the surface pool)
    char m_pad28[0x34 - 0x28];
    void* m_34; // +0x34  goo source surface (freed back to the pool)
    char m_pad38[0x60 - 0x38];
};

RVA(0x00104bb0, 0x94)
CSBI_WellGoo::~CSBI_WellGoo() {
    if (m_34) {
        m_24->m_1c->RemoveItemA(m_34);
        m_34 = 0;
    }
}

#ifndef GRUNTZ_SBI_IMAGE_H
#define GRUNTZ_SBI_IMAGE_H

#include <Ints.h>
#include <rva.h>
#include <Gruntz/SbRect.h>        // SetupImage args 5..8 - ONE by-value geometry rect
#include <Gruntz/StatusBarItem.h> // canonical frameless CStatusBarItem base

class CDDrawSurfaceMgr;
class CStatusBarMgr;
class CImage; // the latched/resolved m_30 frame (<Image/CImage.h>)

class CSBI_RectOnly : public CStatusBarItem {
public:
    // INLINE in the header (like CStatusBarItem's ctor): MSVC5's per-site inline
    // BUDGET reproduces retail's split for free - it INLINES this tiny body at the
    // simple leaf `new` sites (BuildStatusBarTabs) and CALLS the out-of-line COMDAT
    // where the enclosing ctor's budget is exhausted (the CSBI_MenuItem base chain).
    // SBI_RectOnlyBase.cpp #defines SBI_RECTONLY_OWN_CTOR to emit the 0x101fa0 COMDAT.
#ifdef SBI_RECTONLY_OWN_CTOR
    CSBI_RectOnly(); // 0x00101fa0 (out-of-line COMDAT, SBI_RectOnlyBase.cpp)
#else
    CSBI_RectOnly() { m_kind = 1; }
#endif
    virtual ~CSBI_RectOnly() OVERRIDE; // slot 0
    // (NO slot-1 override: sema class says vtbl 0x1eab8c slot [1] is INHERITED
    // (CStatusBarItem::SerializeFields, thunk 0x1848). The `SbiVfunc0` the old merged TU
    // defined under this class name belonged to the host's fabricated vtable, not here.)
    // slot 2 (0xe86e0). Args 5..8 are ONE by-value SbRect - see StatusBarItem.h.
    virtual i32 Setup(CStatusBarMgr* owner, CDDrawSurfaceMgr* host, i32 a3, i32 a4, SbiRect rc, i32 a9, i32 a10) OVERRIDE;
    virtual void SbiSlot3() OVERRIDE; // slot 3
    virtual void SbiSlot4() OVERRIDE; // slot 4
    // Member teardown run by the CHAIN-DTOR device (see StatusBarItem.h).
    void DtorRect(); // 0xe8760
    // (InsertPtr 0x108410 / ClearTabGroup 0x100b00 / Deactivate 0x100cb0 were declared
    // here on the strength of the old CSBI_RectOnly/CStatusBarMgr name conflation. They
    // are methods of the 0x630 status-bar HOST (CStatusBarMgr) and moved there with the
    // split - this thin sub-widget never had them.)
};
SIZE_UNKNOWN();

#if defined(SBI_DTOR_CHAIN) && !defined(SBI_OWN_RECTONLY_DTOR)
inline CSBI_RectOnly::~CSBI_RectOnly() {
    DtorRect();
}
#endif

class CSBI_Image : public CSBI_RectOnly {
public:
    // The type tag (m_8 = 3) is CSBI_Image's own ctor leg: every `new CSBI_Image` in the
    // tab builders (CStatusBarMgr::LoadTabSprites, CGameMenuMgr::BuildGameMenu) produces
    // tag 3 with m_30 cleared, on top of the out-of-line CSBI_RectOnly base ctor (0x101fa0)
    // retail calls first. Inline, so the builders fold it at the new-site like retail.
    CSBI_Image() {
        m_kind = 3;
        m_frame = 0;
    }
    virtual ~CSBI_Image() OVERRIDE; // slot 0
    // slot 1 (vtbl 0x1eac0c thunk 0x2077 -> 0xe6e40): the CSBI_Image serialize leg;
    // tail-chains the base CStatusBarItem::SerializeFields. Re-attributed from
    // CSBI_MenuItem (dossier #16); body in SBI_Image.cpp. This IS the slot - it used to
    // be a non-virtual `SerializeChain` sitting beside a fabricated 0-arg `SbiVfunc0`
    // that held the slot instead.
    virtual i32 SerializeFields(CSerialArchive* ar, i32 kind, i32 a, i32 b) OVERRIDE; // 0xe6e40
    virtual void SbiSlot3() OVERRIDE;                                                 // slot 3
    virtual void SbiSlot4() OVERRIDE; // slot 4
    virtual void SbiSlot5() OVERRIDE; // slot 5
    // vtable slot 11 (0xe6c80): the image setup, 11 dwords of args. The RETAIL BODY pins
    // the arg types (disasm 0xe6c80): entry `mov eax,[esp+8]` reads arg2 and later
    // `mov ecx,[eax+0x10]` DEREFERENCES it => arg2 is the CDDrawSurfaceMgr*; arg1 is only
    // null-tested and stored (m_2c = owner), and every call site passes the building
    // manager's `this`. Args 5..8 are ONE by-value rect (the builders fill an SbRect and
    // pass it), arg9 the asset key string. This is the ONE signature: the tab builders
    // used to call it through a fabricated 15-slot `CSbConfigItem::Configure` view, which
    // made cl emit a 60 B ??_7CSBI_Image@@6B@ in statusbarmgr against retail's 48 B.
    virtual i32 SetupImage( // slot 11 (new)
        CStatusBarMgr* owner,
        CDDrawSurfaceMgr* host,
        i32 a3,
        i32 a4,
        SbRect rc,
        const char* key,
        i32 a10,
        i32 a11
    );

    // slot-3 body AND the dtor's member teardown (ONE retail body, 0xe6d90 - the
    // chain dtors call it; the vtable slot-3 thunk 0x1b59 jmps to it). Re-attributed
    // from CSBI_MenuItem (dossier #16: vtbl 0x1eac0c slot [3]); body in SBI_Image.cpp.
    void ClearFrame(); // 0xe6d90
    // slot-5 body (vtbl 0x1eac0c slot [5], thunk 0x16e5): one play step rendering the
    // CURRENT resolved frame m_30 (no table re-lookup). Ex CAniPlayer view (dossier #16).
    i32 TickRenderCurrent(); // 0xe6dd0

    // +0x2c is the inherited base CStatusBarItem::m_2c (the id slot SetupImage latches).
    CImage* m_frame; // +0x30  latched/resolved frame (the config value; ex i32)
};
SIZE(0x34);

#if defined(SBI_DTOR_CHAIN) && !defined(SBI_OWN_IMAGE_DTOR)
inline CSBI_Image::~CSBI_Image() {
    ClearFrame();
}
#endif

#endif // GRUNTZ_SBI_IMAGE_H

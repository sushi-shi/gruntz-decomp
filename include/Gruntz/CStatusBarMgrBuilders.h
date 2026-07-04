// CStatusBarMgrBuilders.h - the CStatusBarMgr per-tab builder (LoadTabSprites TU,
// CStatusBarMgr.cpp) shapes: the builder-facet base CSbConfigItem + its concrete
// SBI leaves + the icon/factory helpers + the CStatusBarMgr class. Moved here from
// the per-TU inline defs so each carries a single shared definition (matching-neutral;
// only offsets + code bytes are load-bearing, engine callees are reloc-masked).
//
// BUILDER-FACET VIEW: CSbConfigItem is the per-TU builder VIEW of the ONE retail
// CSBI_Image level (Configure lands at vtable slot +0x2c = CSBI_Image's own virtual).
// Its two sibling builder views model the SAME level with the SAME slot -
// CSbDialogItem (SBI_TabzDialogEh.cpp) and CSbMenuItem (StatusBarGameMenu.cpp) - but
// with an OUT-OF-LINE base ctor; one MSVC5 spelling emits only one shape so they stay
// renamed apart, NOT merged (measured-regression wall,
// docs/patterns/gx-frame-outofline-ctor.md).
#ifndef GRUNTZ_CSTATUSBARMGR_BUILDERS_H
#define GRUNTZ_CSTATUSBARMGR_BUILDERS_H

#include <Ints.h>
#include <rva.h>
#include <Mfc.h>           // CPtrList (embedded tab lists in CStatusBarMgr)
#include <Gruntz/SbRect.h> // the by-value geometry rect the Configure virtuals take

class CStatusBarMgr;

// CSbConfigItem is the shared base "view": polymorphic so MSVC emits native
// __thiscall vtable dispatch (call [edx+0x2c] etc.). The eleven leading placeholder
// virtuals line Configure up to slot +0x2c and ConfigureEx to +0x34; the deleting
// dtor is slot 0. Each concrete tab widget is a REAL derived class (CSBI_Image /
// CSBI_ImageSet / CSBI_WellGoo below), so `new CSBI_Image` makes cl auto-stamp the
// retail ??_7CSBI_Image@@6B@ vtable - no manual vtable stamp. The inline base ctor
// zeroes the base fields the retail ??0CStatusBarItem (0x1005d0) cleared.
class CSbConfigItem {
public:
    // INLINE ctor (deliberately). The retail /GX frame here comes from `new CSBI_X`
    // CALLING the base ctor OUT OF LINE (call 0x1005d0/0x101fa0): the opaque may-throw
    // call makes cl register the operator-delete-on-ctor-throw cleanup and raise the
    // frame (proven on the sibling CGameMenuMgr::BuildGameMenu, 0x101580, which is a
    // COMPLETE body: declaring its base ctor out-of-line took it 37%->63% by emitting
    // the exact Order-A /GX prologue). LoadTabSprites, however, is still a large PARTIAL
    // (only the Gruntz/Resource/title cases), so adding the frame here allocates the
    // /GX prologue in the wrong Order-B register layout (this->ebp instead of ->esi)
    // and REGRESSES the partial (23.7%->20.6%). The out-of-line-ctor unblock only pays
    // off once the WHOLE 7629 B body is reconstructed (register pressure then matches
    // retail's this->esi/Order-A). Kept inline until that full redo; see
    // docs/patterns/gx-frame-outofline-ctor.md.
    CSbConfigItem() {
        m_4 = 0;
        m_24 = 0;
        m_28 = 0;
    }
    virtual ~CSbConfigItem();   // +0x00 (scalar deleting dtor)
    virtual void Serialize();   // +0x04 (slot 1)
    virtual void Setup();       // +0x08 (slot 2)
    virtual void ClearFrame();  // +0x0c (slot 3)
    virtual void Poll();        // +0x10 (slot 4)
    virtual void Tick();        // +0x14 (slot 5)
    virtual void HitHandlerA(); // +0x18 (slot 6)
    virtual void HitHandlerB(); // +0x1c (slot 7)
    virtual void HitHandlerC(); // +0x20 (slot 8)
    virtual void HitHandlerD(); // +0x24 (slot 9)
    virtual void Refresh();     // +0x28 (slot 10)
    virtual i32 Configure(
        CStatusBarMgr* mgr,
        i32 a,
        i32 b,
        i32 c,
        SbRect rect,
        const char* key,
        i32 d,
        i32 e
    ); // +0x2c
    virtual i32 ConfigureEx(
        CStatusBarMgr* mgr,
        i32 a0,
        SbRect rect,
        const char* key,
        i32 b,
        i32 c,
        i32 d,
        i32 e,
        i32 f
    );                                                             // +0x34
    virtual void s34_pad();                                        // +0x34 (filler)
    virtual void ApplyDir(i32 p0, i32 p1, i32 p2, i32 p3, i32 p4); // +0x38 (SetDirection sink)

    // SetDirection (0xea0f0): pick one of four direction tuples from the two
    // boolean selectors and forward to the +0x38 virtual.
    void SetDirection(i32 a, i32 b); // 0x0ea0f0

    i32 m_4; // +0x04
    i32 m_8; // +0x08 type tag (3/4/5/6/7/8/9/0xb)
    char pad[0x24 - 0x0c];
    i32 m_24; // +0x24
    i32 m_28; // +0x28
    i32 m_2c; // +0x2c  Setup id (filled by Configure)
    i32 m_30; // +0x30
};
SIZE_UNKNOWN(CSbConfigItem);

// The concrete tab-widget subclasses. `new CSBI_Image` makes MSVC auto-stamp the
// retail ??_7CSBI_Image@@6B@ vtable (0x5eac0c) - the vtable-name catalog in
// config/vtable_names.csv names it on the target side (reloc-masked). The inline
// ctor sets the per-tag fields the retail Construct wrote after the base ctor
// (m_8 = tag, m_30 = 0). Trailing padding pins each operator-new size to retail.
class CSBI_Image : public CSbConfigItem { // vtable 0x5eac0c, size 0x34
public:
    CSBI_Image() {
        m_8 = 3;
        m_30 = 0;
    }
};
SIZE(CSBI_Image, 0x34);

class CSBI_ImageSet : public CSbConfigItem { // vtable 0x5eac4c, size 0x3c
public:
    CSBI_ImageSet() {
        m_30 = 0;
        m_8 = 4;
        m_34 = 0;
    }
    i32 m_34; // +0x34
    char _pad38[0x3c - 0x38];
};
SIZE(CSBI_ImageSet, 0x3c);

class CSBI_WellGoo : public CSbConfigItem { // vtable 0x5eadfc, size 0x6c
public:
    CSBI_WellGoo() {
        m_8 = 7;
        m_30 = 0;
    }
    char _pad34[0x6c - 0x34];
};
SIZE(CSBI_WellGoo, 0x6c);

// The shared item helpers driven on a freshly created icon-set item.
class CSbItemHelp {
public:
    void Init(i32 n); // FUN_00552480  @0x152480
    void Push(i32 v); // FUN_00552520  @0x152520
};
SIZE_UNKNOWN(CSbItemHelp);

// The icon/sprite factory the resource/game tabs pull chip + warpstone sprites
// from (g_gameReg.m_74 / m_68); __thiscall on the factory.
class CSbFactory {
public:
    void* GetByIndex(i32 idx, i32 z); // thunk 0x4165 -> FUN_004e23c0
};
SIZE_UNKNOWN(CSbFactory);
class CSbIconSet {
public:
    i32 Probe(i32 a);        // thunk 0x1582 -> FUN_00479b30
    void SetA(i32 v);        // thunk 0x11e5 -> FUN_004eb830
    void SetB(i32 a, i32 b); // thunk 0x23dd -> FUN_004eb740
    void AddRef(i32 v);      // thunk 0x3b98 -> FUN_004ea170
};
SIZE_UNKNOWN(CSbIconSet);

// CStatusBarMgr layout (placeholder fields; only offsets are load-bearing).
class CStatusBarMgr {
public:
    i32 LoadTabSprites();

    char m_pad00[0xc];
    void* m_c; // +0x0c  the configure-virtual `this`
    i32 m_10;  // +0x10  base x
    i32 m_14;  // +0x14  base y
    char m_pad18[0x48 - 0x18];
    CPtrList m_48; // +0x48  Statz tab list
    CPtrList m_64; // +0x64  Gruntz tab list
    CPtrList m_80; // +0x80  Resource tab list
    CPtrList m_9c; // +0x9c  Multiplayer tab list
    CPtrList m_b8; // +0xb8  Game tab list
    char m_padd4[0x10c - 0xd4];
    i32 m_10c; // +0x10c  current tab selector (1..5)
};
SIZE_UNKNOWN(CStatusBarMgr);

#endif // GRUNTZ_CSTATUSBARMGR_BUILDERS_H

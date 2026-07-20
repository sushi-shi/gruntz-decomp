#ifndef GRUNTZ_SBI_SIDETAB_H
#define GRUNTZ_SBI_SIDETAB_H

#include <Ints.h>
#include <rva.h>
#include <DDrawMgr/DDrawSurfaceMgr.h> // canonical g_gameReg->m_world (CDDrawSurfaceMgr + draw chain)
#include <Gruntz/StatusBarItem.h>     // canonical frameless CStatusBarItem base
#include <Gruntz/SbiConfig.h>         // canonical CDDrawSurfaceMgr (the configure's arg2)
#include <Image/CImage.h> // the m_30/m_34 frame handles ARE CImage (RenderFrame @0x153790)

class CStatusBarMgr; // the mgr that `new`s + configures these children (ex 'CStatzTabBuilder')

class CSBI_SideTab : public CStatusBarItem {
public:
    // The field init retail's inline ctor did at the BuildSideTabs new-site (the
    // base CStatusBarItem ctor already zeroes m_4/m_8/m_24/m_28).
    CSBI_SideTab() {
        m_topFrame = 0;
        m_bottomFrame = 0;
        m_sampledValue = -1;
        m_44 = -1;
    }
    virtual ~CSBI_SideTab() OVERRIDE; // slot 0
    // slot 1 (vtbl 0x1eae3c thunk 0x1ef1 -> 0xe9a30): the side-tab serialize leg,
    // tail-chaining CStatusBarItem::SerializeFields. 4 args, proven by `ret 0x10` + the
    // body's `mov esi,[esp+0x9c]` archive read / `[esp+0xa4]` kind switch (case 4/7).
    virtual i32 SerializeFields(CSerialArchive* ar, i32 kind, i32 a, i32 b) OVERRIDE; // 0xe9a30
    virtual void SbiSlot3() OVERRIDE;                                                 // slot 3
    virtual void SbiSlot4() OVERRIDE; // slot 4
    virtual void SbiSlot5() OVERRIDE; // slot 5

    // 0xe9600: the side tab's own configure, run on the freshly-`new`ed child by
    // CStatusBarMgr::BuildSideTabs. `parent` is the mgr - the body reads parent->m_10 /
    // parent->m_rect14.m_4, its geometry anchors - and `host` the config
    // host, the same arg2 every setup in this family takes. It was DEFINED as
    // `CSbTab::BuildStatzTabStatusBar` while the caller referenced it on a 1-slot
    // CSBI_SideTab view: two mangled names, so the call resolved to no definition at link.
    i32 BuildStatzTabStatusBar(
        CStatusBarMgr* parent,
        CDDrawSurfaceMgr* host,
        i32 p3,
        i32 p4,
        i32 p5,
        i32 p6,
        i32 p7,
        i32 p8,
        const char* p9, // the asset key - ACCEPTED BUT UNUSED (the two lookups below are
                        // keyed on hardcoded TABONLEFT/TABONRIGHT literals)
        i32 p10,
        i32 p11,
        i32 p12,
        i32 onLeft
    ); // 0xe9600

    void Reset();            // 0xe9800 (out-of-line)
    i32 Refresh(i32 unused); // vslot 4 (0xe9820)  rebuild the +0x58 draw gate (ret int 0)
    i32 Render(); // vslot 5 (0xe99c0) draw the two side frames. 0-arg: the body ends `retl` (cleans 0); the ex-`i32 z` param was fabricated and unused.
    i32 BuildHandle();       // 0xe9850  sibling: build the +0x58 draw gate

    // base region m_0..0x2f comes from CStatusBarItem (incl. m_2c, the owner slot that
    // inherited slot-2 Setup fills - BuildHandle reads it as the empty-slot fallback
    // notify target via ((CSBI_RectOnly*)m_2c)); leaf fields start at +0x30.
    CImage* m_topFrame;    // +0x30  top frame handle
    CImage* m_bottomFrame; // +0x34  bottom frame handle (resolved glyph)
    i32 m_sampledValue;    // +0x38  tracked sampled value
    i32 m_rowIndex;        // +0x3c  unit-table row index (stride 15)
    i32 m_40;              // +0x40  unit-table column index
    i32 m_44;              // +0x44  sample mode (0 idle / 2 ability / 3 badge / 1 health)
    i32 m_48;              // +0x48  draw x
    i32 m_4c;              // +0x4c  draw y
    i32 m_50;              // +0x50  bottom-frame y delta
    i32 m_54; // +0x54  side latch (BuildStatzTabStatusBar's `onLeft`); was an unnamed pad
    i32 m_58; // +0x58  draw gate (0 => not built)
};
SIZE_UNKNOWN(CSBI_SideTab);
VTBL(CSBI_SideTab, 0x001eae3c); // vtable_names -> code (RTTI game class)

#endif // GRUNTZ_SBI_SIDETAB_H

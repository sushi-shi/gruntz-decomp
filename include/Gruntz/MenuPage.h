// MenuPage.h - a named menu/list page node (C:\Proj\Gruntz).
//
// Recovered from the tomalla-45 trace cluster (0x183250..0x1844d0). The
// main-menu builder (src/Gruntz/MainMenuBuilder.cpp, 0xa11d0) constructs one of
// these per menu page ("MAINMENU", "SINGLEPLAYER", "MULTIPLAYER", "MOVIEZ",
// "QUESTZ", the AREAS sub-pages) and fills it with named items; the on-screen
// menu host (ChatBox.cpp region, 0x182ab0) drives it with Draw + focus
// navigation (FocusNext / FocusPrev / SetFocus / Activate).
//
// Layout (offsets + code bytes are load-bearing):
//   +0x00 m_owner     - owning menu system (Configure sets it from template link)
//   +0x04 m_host      - render host (Draw/SwitchToPage; +0x20 byte gates wrapping)
//   +0x08 m_switchKey - CString: page-switch target key (Switch)
//   +0x0c m_key       - CString: this page/item key returned by GetKey
//   +0x10 m_focusName - CString: saved focus item name (RestoreFocus)
//   +0x14 m_items     - CPtrList of child items (m_pNodeHead @+0x18; node {next,prev,data@+8})
//   +0x30 m_flags     - flag bits (0x1 wrap-on, 0x2 wrap-off, 0x4 grid, 0x8 no-draw)
//   +0x34..+0x54 - layout scalars: rect{L,T,R,B}, headGap, rowSpacing,
//                  colWidth, rowsPerCol, colOffset (named)
//   +0x58 m_offsetX   - layout x-offset (zeroed in Configure)
//   +0x5c m_offsetY   - layout y-offset (zeroed in Configure)
//   +0x60 m_subPage   - sub-page / name-cache pointer (CMapStringToPtr::Lookup result)
//   +0x64 m_focus     - current focused child item
//
// The child-item classes (CMenuItem / CMenuItem2) are the polymorphic leaves in
// MenuItem.h / MenuItem2.h; the page owns a CPtrList of them, reads a child's kind
// (+0x24) and back-link (+0x1c), caches its list POSITION (+0x2c), and drives it
// through its vtable (Init / GetWidth / Detach / Notify / Place / Configure /
// Release / Trigger). __thiscall.
#ifndef GRUNTZ_MENUPAGE_H
#define GRUNTZ_MENUPAGE_H

#include <Ints.h>
#include <rva.h>

#include <Mfc.h>

#include <Gruntz/MenuItem.h>
#include <Gruntz/MenuItem2.h>

// The owning menu system (catalog holder) IS the canonical CDDrawSurfaceMgr
// (== CState::m_c / CChatBox::m_page; GameRegistry.h): the catalog is its +0x10
// m_10 CImageRegistry, whose +0x10 m_10map is the name->page CMapStringToOb.
class CDDrawSurfaceMgr;
class CChatBox; // the render host (Draw/ReplaceNode/ScrollRow1; +0x20 wrap flag)

class CMenuPage {
public:
    // Inline (in-class) ctor - retail INLINES it at BuildMainMenuTree's 14 `new
    // CMenuPage` sites (0xa1208...: member ctors +0x08/+0x0c/+0x10 CString +
    // +0x14 CPtrList, then the body's zero stores in exactly this order:
    // +0x00, +0x04, +0x60, +0x64, +0x30). No out-of-line ??0CMenuPage exists in
    // retail (no other new-site), so the COMDAT never materializes.
    CMenuPage() {
        m_owner = 0;
        m_host = 0;
        m_subPage = 0;
        m_focus = 0;
        m_flags = 0;
    }
    // Inline (in-class) dtor - retail INLINES it at the builder's `delete page`
    // failure paths (call InitDefaults 0x1833a0 at the whole-object EH state,
    // then the member teardown at descending states, then operator delete) and
    // keeps the out-of-line COMDAT copy at 0x183250 for the non-EH-framed
    // caller CChatBox::Clear (`call 0x183250`); that standalone copy is pinned
    // by @rva-symbol in ChatBox.cpp (an inline dtor cannot carry RVA()).
    ~CMenuPage() {
        InitDefaults();
    }
    CString GetKey(); // 0x1832d0
    // 0x1832f0: seed this page from the owning chat/menu box: m_owner <- host->m_page,
    // m_host <- host, the label/parent CStrings, m_rect <- host->m_rect8 (16-byte block
    // copy), headGap/rowSpacing <- host->m_18/m_1c, then resolve the catalog sub-page.
    i32 Configure(
        CChatBox* host,
        const char* label,
        const char* key,
        const char* parent,
        i32 flags
    );                                   // 0x1832f0
    void InitDefaults();                 // 0x1833a0  Clear + zero scalars
    void Clear();                        // 0x1833c0  free child items + RemoveAll
    i32 ResolveSubPage(const char* key); // 0x1833f0  catalog Lookup -> cache m_subPage
    void* Append(CMenuItem* item);       // 0x183430  AddTail(item) -> item->m_2c
    // 0x183460 (ret 0x14 = 5 args, __thiscall): alloc + construct a CMenuItem,
    // Init(this, a0..a4), append. Semantic sig (binary-proven: BuildMainMenuTree's
    // pushes are $SG string relocs; Init routes label -> m_name, spriteKey -> the
    // catalog Lookup, key -> m_key CStrings). Byte-neutral vs the old i32 x5
    // typing (5 4-byte pushes either way); CMenuItem::Init itself keeps the
    // mangling-pinned i32 slots (it is a virtual - retyping its params would
    // rewrite the slot's mangled name), so the defs cast at the forward.
    CMenuItem*
    AddItem(const char* label, const char* spriteKey, i32 cmdId, const char* key, i32 flags);
    // 0x1835a0 (ret 0x1c = 7 args): like AddItem but two extra params seed the new
    // item's m_1c (tag) and m_cmdParam/+0x30 (cmdParam); Init gets (this, a0,a1,a2,a5,a6).
    CMenuItem* AddSubItem(
        const char* label,
        const char* spriteKey,
        i32 cmdId,
        i32 cmdParam,
        i32 tag,
        const char* key,
        i32 flags
    ); // 0x1835a0
    i32 ReleaseAll();                                         // 0x183990  release focus + items
    i32 RestoreFocus();                        // 0x1839d0  focus saved name / first focusable
    i32 SetFocus(CMenuItem* item, i32 notify); // 0x183ad0
    i32 NotifyAll(void* arg);                  // 0x183b30
    i32 Layout(i32 ctx);                       // 0x183b60  measure/place children
    i32 FocusNext();                           // 0x183c50
    i32 FocusPrev();                           // 0x183d10
    i32 Activate();                            // 0x183dd0  focus->vtable[+0x30]
    i32 FocusAndSelect(i32 a0, i32 a1);        // 0x184070
    i32 Click(i32 a0, i32 a1);                 // 0x1840a0
    CMenuItem* HitTest(i32 x, i32 y);          // 0x184100
    CMenuItem* FindByName(const char* s);      // 0x184150  /GX; walk + strcmp
    i32 SelectForward();                       // 0x1843f0  /GX
    i32 SelectBackward();                      // 0x1844d0  /GX
    i32 LayoutOne(i32 ctx);                    // 0x183e50  single-column measure/place

    // 0x74-item factories (derived item @0x5f08f8) - mirror AddItem/AddSubItem:
    CMenuItem2* AddItem2(i32, i32, i32, i32, i32);                   // 0x1836f0
    CMenuItem2* AddSubItem2(i32, i32, i32, i32, i32, i32, i32, i32); // 0x183850
    i32 Switch(i32 refocus); // 0x183df0  host SwitchToPage + refocus
    i32 CanWrap();           // 0x183e30  focus-wrap gate
    i32 FocusForwardN();     // 0x183f70  step focus +m_rowsPerCol nodes
    i32 FocusBackwardN();    // 0x183ff0  step focus -m_rowsPerCol nodes
    i32 SelectFwd2();        // 0x184230  /GX  m_focus GetField4c -> FindByName
    i32 SelectBack2();       // 0x184310  /GX  m_focus GetField50 -> FindByName

    CDDrawSurfaceMgr* m_owner; // +0x00 owning holder (catalog via ->m_10->m_10map)
    CChatBox* m_host;          // +0x04 render host (Draw/ReplaceNode/ScrollRow1; wrap flag @+0x20)
    CString m_switchKey;       // +0x08 page-switch target key (Switch -> host SwitchToPage)
    CString m_key;             // +0x0c this page/item key (GetKey)
    CString m_focusName;       // +0x10 saved focus item name (RestoreFocus)
    CPtrList m_items;          // +0x14 child items (m_pNodeHead @+0x18; node {next,prev,data@+8})
    i32 m_flags;               // +0x30 flag bits: 0x1 wrap-on, 0x2 wrap-off, 0x4 grid, 0x8 no-draw
    RECT m_rect;               // +0x34  page rect (block-copied from CChatBox::m_rect8;
                               //        initial y = m_offsetY + m_rect.top; x center =
                               //        (right-left+1)/2 + m_offsetX + left)
    i32 m_headGap;             // +0x44  gap after sub-page head item (template +0x18)
    i32 m_rowSpacing;          // +0x48  per-item vertical advance (template +0x1c)
    i32 m_colWidth;            // +0x4c  column width (grid wrap step)
    i32 m_rowsPerCol;          // +0x50  rows per column / focus stride
    i32 m_colOffset;           // +0x54  column x-offset
    i32 m_offsetX;             // +0x58  layout x-offset (added to centered column x)
    i32 m_offsetY;             // +0x5c  layout y-offset (added to initial row y)
    CMenuPage* m_subPage;      // +0x60 sub-page/name-cache (catalog Lookup result)
    CMenuItem* m_focus;        // +0x64 current focused child item
};
SIZE(CMenuPage, 0x68); // op-new ground truth: `push 0x68` at the builder's 14 new-sites

#endif // GRUNTZ_MENUPAGE_H

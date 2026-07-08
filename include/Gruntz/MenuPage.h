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

// The owning menu system (catalog holder) and the render host (Draw/SwitchToPage/
// wrap flag) reached through m_owner/m_host; minimal views defined in MenuPage.cpp.
struct CMenuHost;
struct CMenuRenderHost;

class CMenuPage {
public:
    ~CMenuPage();     // 0x183250  /GX member teardown
    RVA(0x001832d0, 0x20)
    CString GetKey() {
        return m_key;
    }
    i32 Configure(
        CMenuItem* tmpl,
        const char* label,
        const char* key,
        const char* parent,
        i32 flags
    );                                                   // 0x1832f0
    void InitDefaults();                                 // 0x1833a0  Clear + zero scalars
    void Clear();                                        // 0x1833c0  free child items + RemoveAll
    void* Append(CMenuItem* item);                       // 0x183430  AddTail(item) -> item->m_2c
    CMenuItem* AddItem(i32, i32, i32, i32, i32, i32);    // 0x183460
    CMenuItem* AddSubItem(i32, i32, i32, i32, i32, i32); // 0x1835a0
    i32 ReleaseAll();                                    // 0x183990  release focus + items
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

    CMenuHost* m_owner;      // +0x00 owning menu system (catalog via ->m_catalog->m_map)
    CMenuRenderHost* m_host; // +0x04 render host (Draw/SwitchToPage/wrap flag @+0x20)
    CString m_switchKey;     // +0x08 page-switch target key (Switch -> host SwitchToPage)
    CString m_key;           // +0x0c this page/item key (GetKey)
    CString m_focusName;     // +0x10 saved focus item name (RestoreFocus)
    CPtrList m_items;        // +0x14 child items (m_pNodeHead @+0x18; node {next,prev,data@+8})
    i32 m_flags;             // +0x30 flag bits: 0x1 wrap-on, 0x2 wrap-off, 0x4 grid, 0x8 no-draw
    i32 m_rectLeft;          // +0x34  page rect (block-copied from template +0x8): left
    i32 m_rectTop;           // +0x38  top (initial y = m_offsetY + m_rectTop)
    i32 m_rectRight;         // +0x3c  right (x center = (right-left+1)/2 + m_offsetX + left)
    i32 m_rectBottom;        // +0x40  bottom
    i32 m_headGap;           // +0x44  gap after sub-page head item (template +0x18)
    i32 m_rowSpacing;        // +0x48  per-item vertical advance (template +0x1c)
    i32 m_colWidth;          // +0x4c  column width (grid wrap step)
    i32 m_rowsPerCol;        // +0x50  rows per column / focus stride
    i32 m_colOffset;         // +0x54  column x-offset
    i32 m_offsetX;           // +0x58  layout x-offset (added to centered column x)
    i32 m_offsetY;           // +0x5c  layout y-offset (added to initial row y)
    CMenuPage* m_subPage;    // +0x60 sub-page/name-cache (catalog Lookup result)
    CMenuItem* m_focus;      // +0x64 current focused child item
};
SIZE_UNKNOWN(CMenuPage);

#endif // GRUNTZ_MENUPAGE_H

// MenuPage.h - a named menu/list page node (C:\Proj\Gruntz).
//
// Recovered from the ClassUnknown_45 trace cluster (0x183250..0x1844d0). The
// main-menu builder (src/Gruntz/MainMenuBuilder.cpp, 0xa11d0) constructs one of
// these per menu page ("MAINMENU", "SINGLEPLAYER", "MULTIPLAYER", "MOVIEZ",
// "QUESTZ", the AREAS sub-pages) and fills it with named items; the on-screen
// menu host (ChatBox.cpp region, 0x182ab0) drives it with Draw + focus
// navigation (FocusNext / FocusPrev / SetFocus / Activate).
//
// Layout (offsets + code bytes are load-bearing; field names are placeholders):
//   +0x00 m_0   - link / owner pointer (set in Configure from the template)
//   +0x04 m_4   - parent page back-pointer (its +0x20 byte gates focusability)
//   +0x08 m_8   - CString
//   +0x0c m_c   - CString (the page/item key returned by GetKey)
//   +0x10 m_10  - CString
//   +0x14 m_14  - CPtrList of child items (m_pNodeHead lands at +0x18; node {next,prev,data@+8})
//   +0x30 m_30  - flag byte (0x1 enabled, 0x2 hidden, 0x4 mode, 0x8 sub-mode)
//   +0x34..+0x48 - layout / geometry scalars
//   +0x58 m_58  - geometry accumulator (zeroed in ctor)
//   +0x5c m_5c  - geometry accumulator (zeroed in ctor)
//   +0x60 m_60  - sub-page / name-cache pointer (CMapStringToPtr::Lookup result)
//   +0x64 m_64  - current focused child item
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

class CMenuPage {
public:
    ~CMenuPage();     // 0x183250  /GX member teardown
    CString GetKey(); // 0x1832d0  returns m_c by value
    i32 Configure(CMenuItem* tmpl, i32 a1, i32 a2, i32 a3, i32 a4); // 0x1832f0
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
    i32 FocusForwardN();     // 0x183f70  step focus +m_50 nodes
    i32 FocusBackwardN();    // 0x183ff0  step focus -m_50 nodes
    i32 SelectFwd2();        // 0x184230  /GX  m_64 GetField4c -> FindByName
    i32 SelectBack2();       // 0x184310  /GX  m_64 GetField50 -> FindByName

    void* m_0;
    void* m_4;
    CString m_8;
    CString m_c;
    CString m_10;
    CPtrList m_14;
    i32 m_30;
    char m_pad34[0x58 - 0x34];
    i32 m_58;
    i32 m_5c;
    CMenuPage* m_60;
    CMenuItem* m_64;
};
SIZE_UNKNOWN(CMenuPage);

#endif // GRUNTZ_MENUPAGE_H

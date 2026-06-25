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
// The child-item class (0x5c bytes, vtable 0x5f08c0, methods 0x184670+) is a
// SEPARATE cluster - here an opaque CMenuItem reached through the child list;
// its accessors are no-body, reloc-masked rel32 callees. A child node caches its
// AddTail POSITION at item+0x2c and exposes its kind at item+0x24 (1/2 focusable).
#ifndef GRUNTZ_MENUPAGE_H
#define GRUNTZ_MENUPAGE_H

#include <Ints.h>

#include <Mfc.h>

struct CMenuPage;

// A child menu item. The page reads its kind (+0x24) and back-link (+0x1c),
// caches its list POSITION (+0x2c), and dispatches through its vtable
// (slot +0x04 Init, +0x08 Configure, +0x10 GetWidth, +0x1c Detach, +0x20 Notify,
// +0x24 Place, +0x28 Activate, +0x2c Release, +0x30 Trigger). __thiscall.
// CMenuItem - polymorphic leaf so the page's `mov eax,[item]; call [eax+N]`
// dispatch falls out with `this` in ecx (__thiscall, no stack cleanup). The 13
// vtable slots are at +0x00..+0x30 (dtor, Init, two spare, GetWidth, two spare,
// Detach, Notify, Place, Configure, Release, Trigger). The vtable CONTENTS live
// in another TU (the 0x184670+ cluster), so the inlined ctor stamps 0x5f08c0 by
// address rather than letting the compiler emit a (divergent) vtable.
class CMenuItem {
public:
    virtual ~CMenuItem();                           // +0x00 scalar-deleting dtor
    virtual i32 Init(i32, i32, i32, i32, i32, i32); // +0x04
    virtual void Vf08();                            // +0x08
    virtual void Vf0c();                            // +0x0c
    virtual i32 GetWidth();                         // +0x10
    virtual void Vf14();                            // +0x14
    virtual void Vf18();                            // +0x18
    virtual void Detach();                          // +0x1c
    virtual void Notify(void* arg);                 // +0x20
    virtual void Place(void* ctx, i32 x, i32 y);    // +0x24
    virtual i32 Configure(void* notify);            // +0x28
    virtual void Release();                         // +0x2c
    virtual i32 Trigger();                          // +0x30

    i32 m_4;      // +0x04
    void* m_8;    // +0x08
    void* m_c;    // +0x0c
    CString m_10; // +0x10
    CString m_14; // +0x14
    char m_pad18[0x1c - 0x18];
    i32 m_1c; // +0x1c  position / row
    char m_pad20[0x24 - 0x20];
    i32 m_24;   // +0x24  kind (1/2 = focusable)
    i32 m_28;   // +0x28
    void* m_2c; // +0x2c  cached AddTail POSITION
    i32 m_30;   // +0x30
    i32 m_34;   // +0x34  sentinel (0xeeeeeeee)
    char m_pad38[0x44 - 0x38];
    i32 m_44; // +0x44  sentinel (0xeeeeeeee)
    char m_pad48[0x4c - 0x48];
    CString m_4c; // +0x4c
    CString m_50; // +0x50
    CString m_54; // +0x54
    CString m_58; // +0x58

    // Non-virtual __thiscall helpers (bodies in the 0x184670+ TU):
    i32 Hit(i32 x, i32 y); // 0x185700
    CString GetName();     // 0x1845b0 (returns the item's name by value)

    void Construct(); // the inlined leaf ctor (CStrings + vtable + sentinels)

    // The page's two by-value key accessors used by SelectFwd2/SelectBack2:
    // each returns one of the item's strings (vtable 0x5f08f8 sibling factories).
    CString GetKey1(); // 0x1845d0
    CString GetKey2(); // 0x1845f0

    // The shared field-reset tail of the leaf ctor (scalars + CString Empty()s,
    // NO vtable stamp): 0x184730, called by AddSubItem2 between the two stamps.
    void ResetFields();
};

// The 0x74-byte derived menu item (base CMenuItem @0x5f08c0 -> own vtable
// 0x5f08f8). AddItem2/AddSubItem2 allocate 0x74, run the base ctor, re-stamp the
// derived vtable, then zero +0x5c..+0x6c and seed +0x70 = 0x64.
class CMenuItem2 : public CMenuItem {
public:
    i32 m_5c; // +0x5c
    i32 m_60; // +0x60
    i32 m_64; // +0x64
    i32 m_68; // +0x68
    i32 m_6c; // +0x6c
    i32 m_70; // +0x70  seeded to 0x64
};

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
    i32 FocusByName(const char* s);            // 0x1839d0  walk + strcmp -> SetFocus
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
    i32 Switch(i32 refocus);                   // 0x183df0  host SwitchToPage + refocus
    i32 CanWrap();                             // 0x183e30  focus-wrap gate
    i32 FocusForwardN();                       // 0x183f70  step focus +m_50 nodes
    i32 FocusBackwardN();                      // 0x183ff0  step focus -m_50 nodes
    i32 SelectFwd2();                          // 0x184230  /GX  m_64 GetKey1 -> FindByName
    i32 SelectBack2();                         // 0x184310  /GX  m_64 GetKey2 -> FindByName

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

#endif // GRUNTZ_MENUPAGE_H

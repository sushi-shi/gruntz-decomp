// MenuItem.h - the polymorphic menu-item leaf (C:\Proj\Gruntz).
//
// The 0x5c-byte child-item object a CMenuPage owns (a CPtrList of these) and
// drives through its vtable (0x5f08c0). The page's main-menu builder constructs
// one per named entry (six CStrings: name/key/label + three more), Init()s it
// from a template + key/label strings, and dispatches Place / Notify / Trigger /
// Hit through the vtable. Recovered from the 0x1845b0..0x185700 cluster.
//
// Layout (offsets + code bytes are load-bearing; field names are placeholders):
//   +0x00 vptr  -> 0x5f08c0
//   +0x04 m_4   - template->[0] (the owner / catalog host)
//   +0x08 m_8   - template->[4] (the on-screen chatbox host)
//   +0x0c m_c   - the template pointer itself
//   +0x10 m_10  - CString (item name; GetName returns it)
//   +0x14 m_14  - CString (the key string Init stores)
//   +0x18 m_18  - i32 (Init arg a3; Place x base)
//   +0x1c m_1c  - i32 (row, zeroed by Init)
//   +0x20 m_20  - i32 flags (Init arg a5; bit0 -> kind 3 vs 1)
//   +0x24 m_24  - i32 kind (1 or 3; 1/2 focusable to the page)
//   +0x28 m_28  - sub-page pointer (catalog Lookup result)
//   +0x2c m_2c  - cached AddTail POSITION (set by the page)
//   +0x30 m_30  - i32
//   +0x34..+0x40 - geometry rect written by Place (m_34 sentinel 0xeeeeeeee)
//   +0x44 m_44  - i32 (sentinel 0xeeeeeeee; placed-x cache)
//   +0x48 m_48  - i32 (placed-y cache)
//   +0x4c..+0x58 - CString m_4c/m_50/m_54/m_58
#ifndef GRUNTZ_MENUITEM_H
#define GRUNTZ_MENUITEM_H

#include <Ints.h>
#include <rva.h>

#include <Mfc.h>

struct CMenuItem;

// A polymorphic view of the item's vtable so `mov eax,[this]; call [eax+N]`
// dispatch falls out WITHOUT emitting a ??_7 here (none of these virtuals is
// defined in this TU -> no vtable materialized -> no clash with MenuPage's
// DATA(0x005f08c0)). The slots Init/dtor/Place/... call live in other clusters.
// Slot order (offset = index*4): +0x0c Reset, +0x34 OnInit are the two reached.
struct CMenuItemView {
    virtual void Vf00();  // +0x00 dtor
    virtual void Vf04();  // +0x04 Init
    virtual void Vf08();  // +0x08
    virtual void Reset(); // +0x0c
    virtual void Vf10();  // +0x10
    virtual void Vf14();  // +0x14
    virtual void Vf18();  // +0x18
    virtual void Vf1c();  // +0x1c
    virtual void Vf20();  // +0x20
    virtual void Vf24();  // +0x24
    virtual void Vf28();  // +0x28
    virtual void Vf2c();  // +0x2c
    virtual void Vf30();  // +0x30
    virtual i32 OnInit(); // +0x34 (Init's post-config hook)
};
SIZE_UNKNOWN(CMenuItemView);

// The sub-page row placer reached through m_28 (CMenuItem::Place at 0x153790).
struct CMenuItemPlacer {
    i32 Place(i32 ctx, i32 a, i32 b, i32 z); // 0x153790 __thiscall
};
SIZE_UNKNOWN(CMenuItemPlacer);

// The chatbox host (m_8) Trigger drives.
struct CMenuItemHost {
    i32 Scroll();                // 0x1830b0 __thiscall
    i32 ReplaceNode(void* node); // 0x182dd0 __thiscall
};
SIZE_UNKNOWN(CMenuItemHost);

// The string->item catalog reached through m_4->m_10->m_10 (CMapStringToPtr::Lookup,
// 0x1b8008) - the same two-hop the page uses (m_0 -> +0x10 ptr -> +0x10 map base).
struct CMenuItemMap {
    i32 Lookup(const char* key, void*& out); // 0x1b8008
};
SIZE_UNKNOWN(CMenuItemMap);
struct CMenuItemCatalog {
    char pad0[0x10];
    CMenuItemMap m_10; // +0x10 the string->item map base
};
SIZE_UNKNOWN(CMenuItemCatalog);
struct CMenuItemHostOwner {
    char pad0[0x10];
    CMenuItemCatalog* m_10; // +0x10 -> the catalog
};
SIZE_UNKNOWN(CMenuItemHostOwner);

// The template Init reads (its [0] is the catalog host, [4] the chatbox host).
struct CMenuItemTemplate {
    CMenuItemHostOwner* m_0; // +0x00 -> owner/catalog host
    void* m_4;               // +0x04 -> chatbox host
};
SIZE_UNKNOWN(CMenuItemTemplate);

struct CMenuItem {
    void* m_vptr;       // +0x00
    void* m_4;          // +0x04
    CMenuItemHost* m_8; // +0x08
    void* m_c;          // +0x0c
    CString m_10;       // +0x10
    CString m_14;       // +0x14
    i32 m_18;           // +0x18
    i32 m_1c;           // +0x1c
    i32 m_20;           // +0x20
    i32 m_24;           // +0x24
    void* m_28;         // +0x28
    void* m_2c;         // +0x2c
    i32 m_30;           // +0x30
    i32 m_34;           // +0x34
    i32 m_38;           // +0x38
    i32 m_3c;           // +0x3c
    i32 m_40;           // +0x40
    i32 m_44;           // +0x44
    i32 m_48;           // +0x48
    CString m_4c;       // +0x4c
    CString m_50;       // +0x50
    CString m_54;       // +0x54
    CString m_58;       // +0x58

    CString GetName();    // 0x1845b0  return m_10
    CString GetField4c(); // 0x1845d0 return m_4c
    CString GetField50(); // 0x1845f0 return m_50
    CString GetField54(); // 0x184610 return m_54
    CString GetField58(); // 0x184630 return m_58
    ~CMenuItem();         // 0x184690  /GX teardown
    void Reset();         // 0x184730  zero scalars + Empty the four trailing CStrings
    i32 Init(i32, i32, i32, i32, i32, i32); // 0x185460
    void Dispatch0c();                      // 0x185510  mov eax,[ecx]; jmp [eax+0xc]
    i32 Notify();                           // 0x185580  PostMessage WM_COMMAND
    i32 Place(i32 ctx, i32 x, i32 y);       // 0x1855f0
    i32 Trigger();                          // 0x1856d0
    i32 Hit(i32 x, i32 y);                  // 0x185700  bounds test
};
SIZE_UNKNOWN(CMenuItem);

#endif // GRUNTZ_MENUITEM_H

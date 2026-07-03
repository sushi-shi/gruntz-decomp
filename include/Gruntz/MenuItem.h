// MenuItem.h - the polymorphic menu-item leaf (C:\Proj\Gruntz).
//
// The 0x5c-byte child-item object a CMenuPage owns (a CPtrList of these) and
// drives through its vtable (0x5f08c0). The page's main-menu builder constructs
// one per named entry (six CStrings: name/key/label + three more), Init()s it
// from a template + key/label strings, and dispatches Place / Notify / Trigger /
// Hit through the vtable. Recovered from the 0x1845b0..0x185700 cluster.
//
// CMenuItem is a REAL polymorphic class (vftable @0x5f08c0, 14 slots): declaring
// the 14 virtuals in slot order makes MSVC emit ??_7CMenuItem@@6B@ + the scalar
// deleting destructor (slot 0) + the implicit vptr stamp in the ctor/dtor. VTBL()
// (in MenuItem.cpp) catalogs the 0x1f08c0 datum (was vtbl-placeholders
// vtbl-cluster-74 / g_menuItemVtbl); the slot relocs + the stamp
// reloc-mask against the (differently-named) retail symbols. No manual g_*Vtbl
// stamp needed. The eight game slots without a reconstructed body (4/5/6/7/8/10/
// 11/13) are declared-only virtuals -> external reloc-masked references.
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

class CMenuItem;

// The sub-page row placer reached through m_28 (CMenuItem::Place at 0x153790).
struct CMenuItemPlacer {
    i32 Place(i32 ctx, i32 a, i32 b, i32 z); // 0x153790 __thiscall
};
SIZE_UNKNOWN(CMenuItemPlacer);

// The chatbox host (m_8) Trigger drives; its +0x04 is the command-target window.
struct CMenuItemHost {
    char pad0[0x4];
    HWND m_wnd;                  // +0x04 command-target window (NotifyCmd)
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
    CMenuItemHost* m_4;      // +0x04 -> chatbox host
};
SIZE_UNKNOWN(CMenuItemTemplate);

class CMenuItem {
public:
    CMenuItem();          // inlined leaf ctor (CStrings + implicit vptr + sentinels)
    virtual ~CMenuItem(); // 0x184690  slot 0 (scalar-deleting-dtor thunk @0x184670)
    virtual i32 Init(i32, i32, i32, i32, i32, i32); // 0x185460  slot 1
    virtual void Dispatch0c();                      // 0x185510  slot 2  (tail -> Reset)
    virtual void Reset();                           // 0x184730  slot 3
    virtual i32 GetWidth();                         // 0x185550  slot 4  (declared-only)
    virtual void Vf5();                             // 0x185520  slot 5  (declared-only)
    virtual void Vf6();                             // 0x184650  slot 6  (declared-only)
    virtual void Detach();                          // 0x1855d0  slot 7  (declared-only)
    virtual void Notify(void* arg);                 // 0x1855e0  slot 8  (declared-only)
    virtual i32 Place(i32 ctx, i32 x, i32 y);       // 0x1855f0  slot 9
    virtual i32 Configure(void* notify);            // 0x185690  slot 10 (declared-only)
    virtual void Release();                         // 0x1856c0  slot 11 (declared-only)
    virtual i32 Trigger();                          // 0x1856d0  slot 12
    virtual i32 OnInit();                           // 0x184660  slot 13 (declared-only)

    // Non-virtual __thiscall helpers/accessors (bodies in MenuItem.cpp):
    CString GetName();     // 0x1845b0  return m_10
    CString GetField4c();  // 0x1845d0  return m_4c
    CString GetField50();  // 0x1845f0  return m_50
    CString GetField54();  // 0x184610  return m_54
    CString GetField58();  // 0x184630  return m_58
    i32 NotifyCmd();       // 0x185580  PostMessage WM_COMMAND (called by Trigger)
    i32 Hit(i32 x, i32 y); // 0x185700  bounds test

    // implicit vptr           // +0x00
    CMenuItemHostOwner* m_4; // +0x04  owner / catalog host (template->[0])
    CMenuItemHost* m_8;      // +0x08
    void* m_c;               // +0x0c
    CString m_10;            // +0x10
    CString m_14;            // +0x14
    i32 m_18;                // +0x18
    i32 m_1c;                // +0x1c
    i32 m_20;                // +0x20
    i32 m_24;                // +0x24
    void* m_28;              // +0x28
    void* m_2c;              // +0x2c
    i32 m_30;                // +0x30
    i32 m_34;                // +0x34
    i32 m_38;                // +0x38
    i32 m_3c;                // +0x3c
    i32 m_40;                // +0x40
    i32 m_44;                // +0x44
    i32 m_48;                // +0x48
    CString m_4c;            // +0x4c
    CString m_50;            // +0x50
    CString m_54;            // +0x54
    CString m_58;            // +0x58
};

// The leaf ctor MSVC inlines into the page's AddItem/AddSubItem: the six CString
// members + implicit vptr stamp fall out of the ctor prologue; the body zeroes the
// scalar fields, sets the two sentinels, and clears the four trailing CStrings.
inline CMenuItem::CMenuItem() {
    m_8 = 0;
    m_c = 0;
    m_28 = 0;
    m_4 = 0;
    m_2c = 0;
    m_34 = (i32)0xeeeeeeee;
    m_44 = (i32)0xeeeeeeee;
    m_4c.Empty();
    m_50.Empty();
    m_54.Empty();
    m_58.Empty();
}

#endif // GRUNTZ_MENUITEM_H

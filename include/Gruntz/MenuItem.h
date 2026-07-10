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
// Layout (offsets + code bytes are load-bearing; recovered from usage):
//   +0x00 vptr        -> 0x5f08c0
//   +0x04 m_owner     - template->[0] (the owner / catalog host)
//   +0x08 m_host      - template->[4] (the on-screen chatbox host)
//   +0x0c m_template  - the template pointer itself
//   +0x10 m_name      - CString (item name; GetName)
//   +0x14 m_key       - CString (the key string Init stores; Trigger payload)
//   +0x18 m_cmdId     - i32 primary WM_COMMAND id (NotifyCmd wParam)
//   +0x1c m_1c        - i32 secondary cmd / sub-index (role dual; left placeholder)
//   +0x20 m_flags     - i32 flags (Init arg a5; bit0 -> disabled, 0x10000 -> loop)
//   +0x24 m_state     - i32 visual state (1 normal, 2 selected, 3 disabled)
//   +0x28 m_sprite    - resolved sprite/placer (catalog Lookup result)
//   +0x2c m_listPos   - cached AddTail POSITION (set by the page)
//   +0x30 m_cmdParam  - i32 WM_COMMAND lParam
//   +0x34..+0x40 - placed hit rect (m_hitLeft/Top/Right/Bottom; m_hitLeft sentinel 0xeeeeeeee)
//   +0x44 m_fixedX    - i32 placement x override (sentinel 0xeeeeeeee = use arg)
//   +0x48 m_fixedY    - i32 placement y override
//   +0x4c..+0x58 - CString m_navFwdName/m_navBackName/m_54/m_58
#ifndef GRUNTZ_MENUITEM_H
#define GRUNTZ_MENUITEM_H

#include <Ints.h>
#include <rva.h>

#include <Mfc.h>

class CMenuItem;

// The sub-page row placer reached through m_28 (CMenuItem::Place at 0x153790).
struct CMenuItemPlacer {};
SIZE_UNKNOWN(CMenuItemPlacer);

// The chatbox host (m_8) Trigger drives; its +0x04 is the command-target window.
struct CMenuItemHost {
    char pad0[0x4];
    HWND m_wnd; // +0x04 command-target window (NotifyCmd)
};
SIZE_UNKNOWN(CMenuItemHost);

// The string->item catalog reached through m_4->m_10->m_10 (CMapStringToPtr::Lookup,
// 0x1b8008) - the same two-hop the page uses (m_0 -> +0x10 ptr -> +0x10 map base).
struct CMenuItemCatalog {
    char pad0[0x10];
    CMapStringToPtr m_10; // +0x10 the string->item map base (real MFC)
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
// The 0x185510 label is emitted ONLY from MenuItem.cpp (which defines
// GRUNTZ_MENUITEM_TU before including this header): the inline body below is a
// COMDAT every includer emits, and the labels merge keeps the LAST duplicate row
// (menupage would win by unit-name sort). Retail's linked instance belongs to the
// MenuItem obj (interval dossier 0x1832d0 seam: it sits between Init@0x185460 and
// the CMenuItem vfunc block). Guarding the annotation is tooling-only (the
// attribute never affects codegen; MSVC sees nothing either way).
#ifdef GRUNTZ_MENUITEM_TU
    RVA(0x00185510, 0x5)
#endif
    virtual void Dispatch0c() {
        Reset();
    }
    virtual void Reset();                     // 0x184730  slot 3
    virtual i32 GetWidth();                   // 0x185550  slot 4  (frame[2] m_height)
    virtual i32 Vf5();                        // 0x185520  slot 5  (frame[2] m_width)
    virtual void Vf6(i32);                    // 0x184650  slot 6  (declared-only)
    virtual void Detach();                    // 0x1855d0  slot 7  (declared-only)
    virtual i32 Notify(void* arg);            // 0x1855e0  slot 8  (declared-only)
    virtual i32 Place(i32 ctx, i32 x, i32 y); // 0x1855f0  slot 9
    virtual i32 Configure(void* notify);      // 0x185690  slot 10 (0x185690)
    virtual void Release();                   // 0x1856c0  slot 11 (declared-only)
    virtual i32 Trigger();                    // 0x1856d0  slot 12
    virtual i32 OnInit();                     // 0x184660  slot 13 (declared-only)

    // Non-virtual __thiscall helpers/accessors (bodies in MenuItem.cpp):
    RVA(0x001845b0, 0x20)
    CString GetName() {
        return m_name;
    }
    RVA(0x001845d0, 0x20)
    CString GetNavFwdName() {
        return m_navFwdName;
    }
    RVA(0x001845f0, 0x20)
    CString GetNavBackName() {
        return m_navBackName;
    }
    CString GetField54();  // 0x184610
    CString GetField58();  // 0x184630
    i32 NotifyCmd();       // 0x185580  PostMessage WM_COMMAND (called by Trigger)
    i32 Hit(i32 x, i32 y); // 0x185700  bounds test

    // implicit vptr                  // +0x00
    CMenuItemHostOwner* m_owner;   // +0x04  owner / catalog host (template->[0])
    CMenuItemHost* m_host;         // +0x08  chatbox host (command window + Scroll/ReplaceNode)
    CMenuItemTemplate* m_template; // +0x0c  the source template (Init arg a0)
    CString m_name;                // +0x10  item name (GetName)
    CString m_key;                 // +0x14  key string (Trigger ReplaceNode payload)
    i32 m_cmdId;                   // +0x18  primary WM_COMMAND id (NotifyCmd wParam)
    i32 m_1c;                      // +0x1c  secondary cmd / sub-index (role dual; unproven)
    i32 m_flags;                   // +0x20  flags: bit0 -> disabled state, 0x10000 -> loop
    i32 m_state;                   // +0x24  visual state: 1 normal, 2 selected, 3 disabled
    void* m_sprite;                // +0x28  resolved sprite/placer (catalog Lookup result)
    void* m_listPos;               // +0x2c  cached POSITION in the page's item list
    i32 m_cmdParam;                // +0x30  WM_COMMAND lParam (NotifyCmd)
    i32 m_hitLeft;                 // +0x34  placed hit rect left (sentinel 0xeeeeeeee = unplaced)
    i32 m_hitTop;                  // +0x38  placed hit rect top
    i32 m_hitRight;                // +0x3c  placed hit rect right
    i32 m_hitBottom;               // +0x40  placed hit rect bottom
    i32 m_fixedX;                  // +0x44  placement x override (sentinel 0xeeeeeeee = use arg)
    i32 m_fixedY;                  // +0x48  placement y override
    CString m_navFwdName;          // +0x4c  forward-nav target item name (SelectFwd2)
    CString m_navBackName;         // +0x50  backward-nav target item name (SelectBack2)
    CString m_54;                  // +0x54  (GetField54 only; role unproven)
    CString m_58;                  // +0x58  (GetField58 only; role unproven)
};

// The leaf ctor MSVC inlines into the page's AddItem/AddSubItem: the six CString
// members + implicit vptr stamp fall out of the ctor prologue; the body zeroes the
// scalar fields, sets the two sentinels, and clears the four trailing CStrings.
inline CMenuItem::CMenuItem() {
    m_host = 0;
    m_template = 0;
    m_sprite = 0;
    m_owner = 0;
    m_listPos = 0;
    m_hitLeft = (i32)0xeeeeeeee;
    m_fixedX = (i32)0xeeeeeeee;
    m_navFwdName.Empty();
    m_navBackName.Empty();
    m_54.Empty();
    m_58.Empty();
}

#endif // GRUNTZ_MENUITEM_H

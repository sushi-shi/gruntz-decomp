#ifndef GRUNTZ_MENUITEM_H
#define GRUNTZ_MENUITEM_H

#include <Ints.h>
#include <rva.h>

#include <Mfc.h>

class CMenuItem;

struct CMenuItemCatalog {
    char pad0[0x10];
    // ::CMapStringToOb - retail's Lookup here is 0x1b8008, which lies in
    // [0x1b7e17, 0x1b8247), the band whose ctor stamps ??_7CMapStringToOb@@6B@.
    // (There is NO fold: CMapStringToPtr's Lookup is a SEPARATE body at 0x1b8438.)
    CMapStringToOb m_10; // +0x10 the string->item map base (real MFC)
};
SIZE_UNKNOWN(CMenuItemCatalog);
struct CMenuItemHostOwner {
    char pad0[0x10];
    CMenuItemCatalog* m_catalog; // +0x10 -> the catalog
};
SIZE_UNKNOWN(CMenuItemHostOwner);

struct CMenuItemTemplate {
    CMenuItemHostOwner* m_0; // +0x00 -> owner/catalog host
    class CChatBox* m_4;     // +0x04 -> the chatbox
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
    virtual i32 GetFrameWidth();              // 0x185520  slot 5  (frame[2] m_width)
    virtual void Disable(i32 mode);           // 0x184650  slot 6  (disable/state hook: the
                                              // main-menu builder Disables gated items w/ 3;
                                              // Configure chains it w/ 2)
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
    CMenuItemHostOwner* m_owner; // +0x04  owner / catalog host (template->[0])
    class CChatBox* m_host; // +0x08  the on-screen chatbox (command window + Scroll/ReplaceNode)
    CMenuItemTemplate* m_template; // +0x0c  the source template (Init arg a0)
    CString m_name;                // +0x10  item name (GetName)
    CString m_key;                 // +0x14  key string (Trigger ReplaceNode payload)
    i32 m_cmdId;                   // +0x18  primary WM_COMMAND id (NotifyCmd wParam)
    i32 m_1c;                      // +0x1c  secondary cmd / sub-index (role dual; unproven)
    i32 m_flags;                   // +0x20  flags: bit0 -> disabled state, 0x10000 -> loop
    i32 m_state;                   // +0x24  visual state: 1 normal, 2 selected, 3 disabled
    CObject* m_sprite;             // +0x28  resolved sprite/placer: the CMapStringToOb
                                   //        catalog Lookup value (a CObject*; consumers
                                   //        downcast to CImageSet / the placer page)
    POSITION m_listPos;            // +0x2c  cached POSITION in the page's item list
                                   //        (= CMenuPage::m_items.AddTail return)
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

inline CMenuItem::CMenuItem() {
    m_host = 0;
    m_template = 0;
    m_sprite = 0;
    m_owner = 0;
    m_listPos = 0;
    m_hitLeft = static_cast<i32>(0xeeeeeeee);
    m_fixedX = static_cast<i32>(0xeeeeeeee);
    m_navFwdName.Empty();
    m_navBackName.Empty();
    m_54.Empty();
    m_58.Empty();
}

#endif // GRUNTZ_MENUITEM_H

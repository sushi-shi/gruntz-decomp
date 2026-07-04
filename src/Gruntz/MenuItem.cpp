// MenuItem.cpp - the polymorphic menu-item leaf (C:\Proj\Gruntz).
//
// The 0x5c-byte child item a CMenuPage owns and dispatches through its vtable
// (0x5f08c0). Recovered from the 0x1845b0..0x185700 cluster (was tomalla-46).
// The page (src/Gruntz/MenuPage.cpp) constructs one per named menu entry, Init()s
// it from a template + key/label strings, and drives Place / Trigger / Hit / the
// teardown through the vtable. CMenuItem / CMenuItem2 are now REAL polymorphic
// classes (14 / 15 virtuals declared in slot order): MSVC emits ??_7CMenuItem@@6B@
// (@0x5f08c0) and ??_7CMenuItem2@@6B@ (@0x5f08f8) + the scalar-deleting-dtor thunks
// + the implicit vptr stamps; the VTBL() rows below catalog those retail data so
// the slot relocs + stamps reloc-mask (no manual g_*Vtbl needed). The /GX EH frame
// on the dtor comes from its six destructible CString members; the scalar helpers
// stay frameless (no destructible local), so they coexist in this eh TU like
// MenuPage.
#include <rva.h>

#include <Gruntz/MenuItem.h>
#include <Gruntz/MenuItem2.h>

// Own base vtable (14 slots): dtor + Init + Dispatch0c + Reset + 8 game slots +
// Place + Trigger. cl emits ??_7CMenuItem@@6B@; VTBL pairs the 0x1f08c0 datum (was
// vtbl-placeholders vtbl-cluster-74 / g_menuItemVtbl).
SIZE(CMenuItem, 0x5c);
VTBL(CMenuItem, 0x001f08c0);

// Derived vtable (15 slots): the visual overrides + one new setter slot. cl emits
// ??_7CMenuItem2@@6B@; VTBL pairs the 0x1f08f8 datum (was vtbl-cluster-75
// / g_menuItem2Vtbl).
SIZE(CMenuItem2, 0x74);
VTBL(CMenuItem2, 0x001f08f8);

// ===========================================================================

// return the item name (m_name) by value.
RVA(0x001845b0, 0x20)
CString CMenuItem::GetName() {
    return m_name;
}

// return the forward-nav target name (m_navFwdName) by value.
RVA(0x001845d0, 0x20)
CString CMenuItem::GetNavFwdName() {
    return m_navFwdName;
}

// return the backward-nav target name (m_navBackName) by value.
RVA(0x001845f0, 0x20)
CString CMenuItem::GetNavBackName() {
    return m_navBackName;
}

// return m_54 by value.
RVA(0x00184610, 0x20)
CString CMenuItem::GetField54() {
    return m_54;
}

// return m_58 by value.
RVA(0x00184630, 0x20)
CString CMenuItem::GetField58() {
    return m_58;
}

// destructor (100%): the compiler re-stamps the vptr (mov [this],&??_7CMenuItem@@6B@),
// then we run the slot-0xc teardown hook, then the six CString members are
// destroyed (auto, reverse declaration order). Now that the vtable is realized
// (VTBL binds ??_7CMenuItem@@6B@ at 0x5f08c0), the vptr-store reloc names the SAME
// symbol on both sides -> exact (was the ~96.6% reloc-masking artifact under the
// manual g_menuItemVtbl stamp). Marked `inline` so the derived ~CMenuItem2
// (0x1847e0) inlines this base teardown like retail did (/Ob1 only inlines
// inline-marked fns); MSVC still emits this out-of-line COMDAT because the derived
// dtor odr-uses it (and slot 0's scalar-deleting thunk references it). Keep `inline`.
RVA(0x00184690, 0x91)
inline CMenuItem::~CMenuItem() {
    Dispatch0c();
}

// reset the scalar fields and clear the four trailing CStrings.
RVA(0x00184730, 0x41)
void CMenuItem::Reset() {
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

// CMenuItem2 destructor (100%): the compiler stamps the derived vtable, we run its
// slot-0xc teardown hook, then the inlined base ~CMenuItem re-stamps the base
// vtable, runs its own slot-0xc hook, and destroys the six CString members. Defined
// in this TU (not menuitem2) because retail emitted it adjacent to ~CMenuItem
// (0x184690) so MSVC could inline the base teardown; the /GX EH frame falls out of
// the destructible CString members the base owns. Realizing both vtables (VTBL binds
// ??_7CMenuItem2@@6B@ @0x5f08f8 and ??_7CMenuItem@@6B@ @0x5f08c0) makes the two
// entry vptr-store relocs name the SAME symbols on both sides -> exact (was the
// ~92.3% vptr-stamp-vs-trylevel-order plateau under the manual g_*Vtbl stamps).
RVA(0x001847e0, 0xa6)
CMenuItem2::~CMenuItem2() {
    Dispatch0c();
    // compiler stamps ??_7CMenuItem2@@6B@ at entry, then the base ~CMenuItem is
    // inlined here: it stamps ??_7CMenuItem@@6B@, runs its Dispatch0c hook, and
    // destroys m_58/m_54/m_50/m_4c/m_14/m_10 (reverse declaration order).
}

// configure the item from a template (a0) + strings; resolve the
// sub-page via the catalog Lookup; the post-config hook (slot 0x34) can short it.
// The args stay i32 (with (const char*)/(template) casts) DELIBERATELY: unlike
// CMenuPage::Configure, Init's callers forward its params with INCONSISTENT order
// AND semantics - AddItem/AddSubItem pass a0..a5 in order, but AddItem2/AddSubItem2
// call Init(a4,a3,a2,a1,a0,(i32)this) (reversed, with `this` landing in a5). No one
// typed signature is correct across those call sites, so the int-model is the honest
// shape; typing the params would just relocate the casts to the tangled callers.
RVA(0x00185460, 0xa9)
i32 CMenuItem::Init(i32 a0, i32 a1, i32 a2, i32 a3, i32 a4, i32 a5) {
    CMenuItemTemplate* t = (CMenuItemTemplate*)a0;
    if (!t) {
        return 0;
    }
    m_flags = a5;
    m_owner = t->m_0;
    m_host = t->m_4;
    m_template = t;
    m_name = (const char*)a1;
    m_key = (const char*)a4;
    m_cmdId = a3;
    m_1c = 0;
    m_cmdParam = 0;
    if (m_flags & 1) {
        m_state = 3;
    } else {
        m_state = 1;
    }
    if (!OnInit()) {
        void* slot = 0;
        m_owner->m_10->m_10.Lookup((const char*)a2, slot);
        m_sprite = slot;
        if (!slot) {
            return 0;
        }
    }
    return 1;
}

// tail-dispatch through vtable slot +0x0c (mov eax,[ecx]; jmp [eax+0xc]).
RVA(0x00185510, 0x5)
void CMenuItem::Dispatch0c() {
    Reset();
}

// notify: PostMessage WM_COMMAND to the host window(s) keyed off m_8->m_4.
// (Non-virtual internal helper called by Trigger; NOT the slot-8 virtual Notify.)
RVA(0x00185580, 0x4a)
i32 CMenuItem::NotifyCmd() {
    i32 id = m_cmdId;
    if (!id) {
        return id;
    }
    HWND wnd = m_host->m_wnd;
    if (wnd) {
        PostMessageA(wnd, WM_COMMAND, id, m_cmdParam);
    }
    if (m_1c && wnd) {
        PostMessageA(wnd, WM_COMMAND, m_1c, 0);
    }
    return 1;
}

// place the item: pick the row out of the sub-page (m_28), have it lay
// itself out at the cached or argument coordinates, then cache the placed rect.
// @early-stop
// regalloc tie (~99.1%): the whole body is byte-aligned; the residual is which of
// the two callee-saved regs (ebx vs ebp) holds the m_44/m_48 coordinate pair --
// retail pins the cmp operand m_44 in ebp, the recompile in ebx. The two are
// interchangeable and the choice is not source-steerable (operand/decl reorderings
// all canonicalize to the same pick). Logic complete; deferred to the final sweep.
RVA(0x001855f0, 0x94)
i32 CMenuItem::Place(i32 ctx, i32 x, i32 y) {
    void* page = m_sprite;
    if (!page) {
        return 0;
    }
    i32 py, px;
    if (m_fixedX != (i32)0xeeeeeeee) {
        py = m_fixedX;
        px = m_fixedY;
    } else {
        py = x;
        px = y;
    }
    i32 idx = m_state;
    CMenuItemPlacer* row;
    if (idx >= *(i32*)((char*)page + 0x64) && idx <= *(i32*)((char*)page + 0x68)) {
        row = ((CMenuItemPlacer**)(*(void**)((char*)page + 0x14)))[idx];
    } else {
        row = 0;
    }
    if (!row) {
        return 0;
    }
    row->Place(ctx, py, px, 0);
    m_hitLeft = py - *(i32*)((char*)row + 0x18);
    m_hitRight = py + *(i32*)((char*)row + 0x18);
    m_hitTop = px - *(i32*)((char*)row + 0x1c);
    m_hitBottom = px + *(i32*)((char*)row + 0x1c);
    return 1;
}

// trigger: scroll the host row, notify, then re-activate the host node.
RVA(0x001856d0, 0x25)
i32 CMenuItem::Trigger() {
    m_host->Scroll();
    NotifyCmd();
    m_host->ReplaceNode(*(void**)&m_key);
    return 1;
}

// hit-test: is (x,y) inside the cached placed rect?
RVA(0x00185700, 0x4b)
i32 CMenuItem::Hit(i32 x, i32 y) {
    if (m_hitLeft == (i32)0xeeeeeeee) {
        return 0;
    }
    if (x < m_hitLeft) {
        return 0;
    }
    if (x > m_hitRight) {
        return 0;
    }
    if (y < m_hitTop) {
        return 0;
    }
    return y <= m_hitBottom;
}

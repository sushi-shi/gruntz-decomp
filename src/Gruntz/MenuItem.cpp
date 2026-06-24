// MenuItem.cpp - the polymorphic menu-item leaf (C:\Proj\Gruntz).
//
// The 0x5c-byte child item a CMenuPage owns and dispatches through its vtable
// (0x5f08c0). Recovered from the 0x1845b0..0x185700 cluster (was ClassUnknown_46).
// The page (src/Gruntz/MenuPage.cpp) constructs one per named menu entry, Init()s
// it from a template + key/label strings, and drives Place / Trigger / Hit / the
// teardown through the vtable (modeled as the polymorphic CMenuItemView so the two
// reached slots dispatch without emitting a clashing ??_7). The /GX EH frame on the
// dtor comes from its six destructible CString members; the scalar helpers stay
// frameless (no destructible local), so they coexist in this eh TU like MenuPage.
#include <rva.h>

#include <Gruntz/MenuItem.h>

// The vtable is stamped by address (its full contents span other clusters, so we
// don't emit a ??_7 here that would collide with MenuPage's DATA(0x005f08c0)).
DATA(0x005f08c0)
extern void* g_menuItemVtbl;

// ===========================================================================

// 0x1845b0 - return the item name (m_10) by value.
RVA(0x001845b0, 0x20)
CString CMenuItem::GetName() {
    return m_10;
}

// 0x184610 - return m_54 by value.
RVA(0x00184610, 0x20)
CString CMenuItem::GetField54() {
    return m_54;
}

// 0x184630 - return m_58 by value.
RVA(0x00184630, 0x20)
CString CMenuItem::GetField58() {
    return m_58;
}

// 0x184690 - destructor: re-stamp the vtable, run the slot-0xc teardown hook,
// then destroy the six CString members (auto, reverse declaration order).
// @early-stop
// reloc-masking scoring artifact (~96.6%): every instruction is byte-identical to
// retail (verified mnemonic-for-mnemonic, base vs delinked target) -- the only
// residual is the vptr store's masked operand (our DIR32 to g_menuItemVtbl vs
// retail's ??_7 at 0x5f08c0, a differently-named symbol). Code is a full match;
// see docs/patterns/reloc-typing-vptr-global.md (topic:scoring-artifact).
RVA(0x00184690, 0x91)
CMenuItem::~CMenuItem() {
    m_vptr = &g_menuItemVtbl;
    Dispatch0c();
}

// 0x184730 - reset the scalar fields and clear the four trailing CStrings.
RVA(0x00184730, 0x41)
void CMenuItem::Reset() {
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

// 0x185460 - configure the item from a template (a0) + strings; resolve the
// sub-page via the catalog Lookup; the post-config hook (slot 0x34) can short it.
RVA(0x00185460, 0xa9)
i32 CMenuItem::Init(i32 a0, i32 a1, i32 a2, i32 a3, i32 a4, i32 a5) {
    CMenuItemTemplate* t = (CMenuItemTemplate*)a0;
    if (!t) {
        return 0;
    }
    m_20 = a5;
    m_4 = t->m_0;
    m_8 = (CMenuItemHost*)t->m_4;
    m_c = t;
    m_10 = (const char*)a1;
    m_14 = (const char*)a4;
    m_18 = a3;
    m_1c = 0;
    m_30 = 0;
    if (m_20 & 1) {
        m_24 = 3;
    } else {
        m_24 = 1;
    }
    if (!((CMenuItemView*)this)->OnInit()) {
        void* slot = 0;
        ((CMenuItemHostOwner*)m_4)->m_10->m_10.Lookup((const char*)a2, slot);
        m_28 = slot;
        if (!slot) {
            return 0;
        }
    }
    return 1;
}

// 0x185510 - tail-dispatch through vtable slot +0x0c (mov eax,[ecx]; jmp [eax+0xc]).
RVA(0x00185510, 0x5)
void CMenuItem::Dispatch0c() {
    ((CMenuItemView*)this)->Reset();
}

// 0x185580 - notify: PostMessage WM_COMMAND to the host window(s) keyed off m_8->m_4.
RVA(0x00185580, 0x4a)
i32 CMenuItem::Notify() {
    i32 id = m_18;
    if (!id) {
        return id;
    }
    HWND wnd = *(HWND*)((char*)m_8 + 4);
    if (wnd) {
        PostMessageA(wnd, WM_COMMAND, id, m_30);
    }
    if (m_1c && wnd) {
        PostMessageA(wnd, WM_COMMAND, m_1c, 0);
    }
    return 1;
}

// 0x1855f0 - place the item: pick the row out of the sub-page (m_28), have it lay
// itself out at the cached or argument coordinates, then cache the placed rect.
// @early-stop
// regalloc tie (~99.1%): the whole body is byte-aligned; the residual is which of
// the two callee-saved regs (ebx vs ebp) holds the m_44/m_48 coordinate pair --
// retail pins the cmp operand m_44 in ebp, the recompile in ebx. The two are
// interchangeable and the choice is not source-steerable (operand/decl reorderings
// all canonicalize to the same pick). Logic complete; deferred to the final sweep.
RVA(0x001855f0, 0x94)
i32 CMenuItem::Place(i32 ctx, i32 x, i32 y) {
    void* page = m_28;
    if (!page) {
        return 0;
    }
    i32 py, px;
    if (m_44 != (i32)0xeeeeeeee) {
        py = m_44;
        px = m_48;
    } else {
        py = x;
        px = y;
    }
    i32 idx = m_24;
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
    m_34 = py - *(i32*)((char*)row + 0x18);
    m_3c = py + *(i32*)((char*)row + 0x18);
    m_38 = px - *(i32*)((char*)row + 0x1c);
    m_40 = px + *(i32*)((char*)row + 0x1c);
    return 1;
}

// 0x1856d0 - trigger: scroll the host row, notify, then re-activate the host node.
RVA(0x001856d0, 0x25)
i32 CMenuItem::Trigger() {
    m_8->Scroll();
    Notify();
    m_8->ReplaceNode(*(void**)&m_14);
    return 1;
}

// 0x185700 - hit-test: is (x,y) inside the cached placed rect?
RVA(0x00185700, 0x4b)
i32 CMenuItem::Hit(i32 x, i32 y) {
    if (m_34 == (i32)0xeeeeeeee) {
        return 0;
    }
    if (x < m_34) {
        return 0;
    }
    if (x > m_3c) {
        return 0;
    }
    if (y < m_38) {
        return 0;
    }
    return y <= m_40;
}

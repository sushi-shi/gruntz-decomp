// MenuItem.cpp - the polymorphic menu-item leaves (C:\Proj\Gruntz): ONE original
// TU (wave1-E; interval dossier 0x1832d0: MenuItem.cpp == [0x185460..0x185a0e],
// CMenuItem out-of-line Init/virtuals + the derived CMenuItem2). Absorbed the
// former MenuItem2.cpp; strict retail-RVA order. The part-1 accessor/dtor cluster
// (0x184610-0x1848a6) is this classes' inline-in-header COMDAT-at-usage emissions
// linked inside MenuPage's obj - their definitions stay here (the class's file).
//
// The 0x5c-byte child item a CMenuPage owns and dispatches through its vtable
// (0x5f08c0). The page (src/Gruntz/MenuPage.cpp) constructs one per named menu
// entry, Init()s it from a template + key/label strings, and drives Place /
// Trigger / Hit / the teardown through the vtable. CMenuItem / CMenuItem2 are REAL
// polymorphic classes (14 / 15 virtuals declared in slot order): MSVC emits
// ??_7CMenuItem@@6B@ (@0x5f08c0) and ??_7CMenuItem2@@6B@ (@0x5f08f8) + the
// scalar-deleting-dtor thunks + the implicit vptr stamps; the VTBL() rows below
// catalog those retail data so the slot relocs + stamps reloc-mask (no manual
// g_*Vtbl needed). The /GX EH frame on the dtor comes from its six destructible
// CString members; the scalar helpers stay frameless (no destructible local), so
// they coexist in this eh TU like MenuPage.
#define GRUNTZ_MENUITEM_TU // own the 0x185510 Dispatch0c label (see MenuItem.h)
#include <rva.h>
#include <Gruntz/ChatBox.h>
#include <Gruntz/ChatBoxOwner.h>
#include <Image/CImage.h>

#include <Gruntz/MenuItem.h>
#include <Gruntz/MenuItem2.h>

#include <stdio.h> // engine sprintf (reloc-masked; CMenuItem2::Init)

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

// CMenuItem::GetName (0x001845b0) is now an inline member in the header.

// CMenuItem::GetNavFwdName (0x001845d0) is now an inline member in the header.

// CMenuItem::GetNavBackName (0x001845f0) is now an inline member in the header.

// CMenuItem::GetField54 (0x184610) - return m_54 CString by value.
RVA(0x00184610, 0x20)
CString CMenuItem::GetField54() {
    return m_54;
}
// CMenuItem::GetField58 (0x184630) - return m_58 CString by value.
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
// CMenuItem2::SetFrame (0x1847a0, vtable slot 14): trivial frame-index setter.
// Folded from Stub/BoundaryUpper.cpp (B_1847a0::Set - ~??_7CMenuItem2@@6B@+0x38);
// declared in MenuItem2.h slot 14, m_70 already a CMenuItem2 field.
RVA(0x001847a0, 0xa)
void CMenuItem2::SetFrame(i32 v) {
    m_70 = v;
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
// CMenuItem::Dispatch0c (0x00185510) is an inline member in the header; this TU
// owns its label (GRUNTZ_MENUITEM_TU above - the retail instance sits here,
// between Init and the vfunc block).
// slot 5 (0x185520): the m_width of the sprite's representative frame (index 2).
RVA(0x00185520, 0x2c)
i32 CMenuItem::Vf5() {
    CMenuSprite* s = (CMenuSprite*)m_sprite;
    if (!s) {
        return 0;
    }
    CImage* f = s->GetAt(2);
    if (!f) {
        return 0;
    }
    return f->m_width;
}
// slot 4 (0x185550): the m_height of the sprite's representative frame (index 2);
// the menu-page layout uses this as each row's vertical step (y += GetWidth()/2).
RVA(0x00185550, 0x2c)
i32 CMenuItem::GetWidth() {
    CMenuSprite* s = (CMenuSprite*)m_sprite;
    if (!s) {
        return 0;
    }
    CImage* f = s->GetAt(2);
    if (!f) {
        return 0;
    }
    return f->m_height;
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
    ((CImage*)row)->RenderFrame((void*)ctx, (void*)py, (void*)px, (void*)0);
    m_hitLeft = py - *(i32*)((char*)row + 0x18);
    m_hitRight = py + *(i32*)((char*)row + 0x18);
    m_hitTop = px - *(i32*)((char*)row + 0x1c);
    m_hitBottom = px + *(i32*)((char*)row + 0x1c);
    return 1;
}
// slot 10 (0x185690): scroll the host row when notified, then run the slot-6 hook.
RVA(0x00185690, 0x25)
i32 CMenuItem::Configure(void* notify) {
    if (notify) {
        ((CChatBox*)m_host)->ScrollRow0();
    }
    Vf6(2);
    return 1;
}
// trigger: scroll the host row, notify, then re-activate the host node.
RVA(0x001856d0, 0x25)
i32 CMenuItem::Trigger() {
    ((CChatBox*)m_host)->ScrollRow1();
    NotifyCmd();
    ((CChatBox*)m_host)->ReplaceNode(*(void**)&m_key);
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
// configure from a template + strings (chaining CMenuItem::Init), then
// resolve the three per-state sprites by "<key>_NORMAL/_SELECTED/_DISABLED".
// @early-stop
// scheduling artifact (~91.6%): every instruction is byte-identical (incl. the base
// Init forwarding, all three sprintf+Lookup blocks, stack layout) -- the only residual
// is where MSVC schedules the `sprite = 0` store: retail interleaves the `mov [out],0`
// after both Lookup arg-pushes, the recompile hoists it adjacent to the &out lea. Not
// source-steerable (no statement order forces a plain store past the call arg setup).
RVA(0x00185750, 0x123)
i32 CMenuItem2::Init(i32 a0, i32 a1, i32 a2, i32 a3, i32 a4, i32 a5) {
    if (!a0) {
        return 0;
    }
    if (!CMenuItem::Init(a0, a1, a2, a3, a4, a5)) {
        return 0;
    }
    m_frameIdx = 0;
    m_6c = 0;
    m_70 = 0x64;

    char name[0x80];
    void* sprite;

    sprintf(name, "%s_NORMAL", (const char*)a2);
    sprite = 0;
    m_owner->m_10->m_10.Lookup(name, sprite);
    m_spriteNormal = (CMenuSprite*)sprite;

    sprintf(name, "%s_SELECTED", (const char*)a2);
    sprite = 0;
    m_owner->m_10->m_10.Lookup(name, sprite);
    m_spriteSelected = (CMenuSprite*)sprite;

    sprintf(name, "%s_DISABLED", (const char*)a2);
    sprite = 0;
    m_owner->m_10->m_10.Lookup(name, sprite);
    m_spriteDisabled = (CMenuSprite*)sprite;

    return 1;
}
// slot 8 (0x1858a0): the frame-cursor countdown. Decrement the pending count by
// `arg`; when it runs out, reload it from m_70 and advance one animation frame.
RVA(0x001858a0, 0x2b)
i32 CMenuItem2::Notify(void* arg) {
    u32 a = (u32)arg;
    if (a >= (u32)m_6c) {
        m_6c = m_70;
        NextFrame();
        return 1;
    }
    m_6c = m_6c - a;
    return 1;
}
// the slot-9 Place override: draw the current animation frame at the placed (or
// argument) coordinates, then cache the resulting hit rect.
// @early-stop
// regalloc tie (~98.8%): body byte-aligned; the residual is which callee-saved reg
// (ebx vs ebp) holds the py/px coordinate pair -- retail pins py(m_44) in ebp, the
// recompile in ebx. Identical not-source-steerable tie as CMenuItem::Place (0x1855f0).
RVA(0x001858d0, 0x72)
i32 CMenuItem2::Place(i32 ctx, i32 x, i32 y) {
    i32 py, px;
    if (m_fixedX != (i32)0xeeeeeeee) {
        py = m_fixedX;
        px = m_fixedY;
    } else {
        py = x;
        px = y;
    }
    CImage* f = GetCurrentFrame();
    if (!f) {
        return 0;
    }
    f->RenderFrame((void*)ctx, (void*)py, (void*)px, 0);
    m_hitLeft = py - f->m_anchorX;
    m_hitRight = py + f->m_anchorX;
    m_hitTop = px - f->m_anchorY;
    m_hitBottom = px + f->m_anchorY;
    return 1;
}
// CMenuItem2::GetCurrentSprite (0x185950) - the sprite for the current state.
RVA(0x00185950, 0x1b)
CMenuSprite* CMenuItem2::GetCurrentSprite() {
    switch (m_state) {
        case 1:
            return m_spriteNormal;
        case 2:
            return m_spriteSelected;
        case 3:
            return m_spriteDisabled;
    }
    return 0;
}
// resolve the frame at the current cursor; if absent, rewind the cursor
// to the sprite's first index and try once more.
// @early-stop
// shrink-wrapped callee-save wall (~60.7%): body byte-identical to retail (both GetAt
// inlines, the eax-reuse on the retry) -- retail defers `push edi` past the early
// `if(!s) return 0` guard (saving only esi there), the recompile pushes edi upfront so
// every return epilogue differs by a pop. docs/patterns/shrink-wrapped-callee-save-push.md.
RVA(0x00185970, 0x4d)
CImage* CMenuItem2::GetCurrentFrame() {
    CMenuSprite* s = GetCurrentSprite();
    if (!s) {
        return 0;
    }
    CImage* f = s->GetAt(m_frameIdx);
    if (f) {
        return f;
    }
    m_frameIdx = s->m_firstFrame;
    return s->GetAt(m_frameIdx);
}
// advance the cursor one frame; when the looping flag (0x10000) is clear
// or the sprite still has a frame, report whether a frame remains.
RVA(0x001859c0, 0x4e)
i32 CMenuItem2::NextFrame() {
    if (!GetCurrentFrame()) {
        return 0;
    }
    m_frameIdx = m_frameIdx + 1;
    if (m_flags & 0x10000) {
        CMenuSprite* s = GetCurrentSprite();
        if (s) {
            if (m_frameIdx > s->m_lastFrame) {
                m_frameIdx = m_frameIdx - 1;
                return 1;
            }
        }
    }
    return GetCurrentFrame() != 0;
}

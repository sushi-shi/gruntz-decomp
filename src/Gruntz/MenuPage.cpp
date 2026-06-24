// MenuPage.cpp - a named menu/list page node (C:\Proj\Gruntz).
//
// Recovered from the ClassUnknown_45 trace cluster (0x183250..0x1844d0): the
// per-page object the main-menu builder fills with named items and the menu host
// drives with Draw + focus navigation. It owns three CStrings (name/key/label),
// a CPtrList of child items (m_14, head @+0x18), a flag byte (+0x30), layout
// scalars, a sub-page/name-cache pointer (+0x60) and a current-focus item (+0x64).
//
// The child-item class (0x5c bytes, vtable 0x5f08c0, methods 0x184670+) lives in
// another TU; here it is opaque (CMenuItem) and its accessors are no-body,
// reloc-masked rel32 callees. Only offsets + code bytes are load-bearing; field
// names are placeholders. The /GX EH frame on the dtor + the FindByName helpers
// comes from the destructible CString temps.
#include <rva.h>

#include <Gruntz/MenuPage.h>

// The engine heap allocator (0x1b9b46), reached as the item's `operator new`.
// Declared locally (not via RezMgr.h) to keep this TU's include set minimal.
extern "C" void* RezAlloc(u32 size); // 0x1b9b46

// Placement new (MSVC5's <new.h> predates the standard declaration).
inline void* operator new(size_t, void* p) {
    return p;
}

// ---------------------------------------------------------------------------
// External engine callees / globals (no body -> reloc-masked rel32).
// ---------------------------------------------------------------------------

// A view of the CPtrList node layout (CPtrList::CNode is protected): next/prev/data.
struct CMenuNode {
    CMenuNode* pNext;
    CMenuNode* pPrev;
    CMenuItem* data;
};

// The catalog map reached through m_0->m_10->m_10 (CMapStringToPtr::Lookup, 0x1b8008).
struct CMenuMap {
    i32 Lookup(const char* key, void*& out);
};
struct CMenuCatalog {
    char pad0[0x10];
    CMenuMap m_10; // +0x10 the string->item map base
};
struct CMenuHost {
    char pad0[0x10];
    CMenuCatalog* m_10; // +0x10 -> the catalog
};

// Sibling-page helpers (other TUs), reached by name:
extern "C" i32 MenuPage_CanWrap(CMenuPage* p);                // 0x183e30
extern CString* MenuPage_KeyFwd(CMenuPage* p, CString* out);  // 0x184610
extern CString* MenuPage_KeyBack(CMenuPage* p, CString* out); // 0x184630

// The host (m_4) the page renders selected items through (CChatBox::Draw, __thiscall).
struct CMenuRenderHost {
    i32 Draw(i32 ctx, CMenuItem* item, i32 x, i32 y); // 0x182f90
};
// The sub-page's current item placer (0x153790, __thiscall on the head item).
struct CMenuPlacer {
    i32 Place(i32 ctx, i32 x, i32 y, i32 z); // 0x153790
};

// The child item's vtable (its contents live in the 0x184670+ TU). Referenced by
// address so the inlined ctor's `mov [item],&vtbl` reloc-masks against retail's.
DATA(0x005f08c0)
extern void* g_menuItemVtbl;

// The inlined leaf ctor the page's AddItem/AddSubItem reproduce: default-construct
// the six CStrings, stamp the vtable, zero the scalar fields, set the sentinels.
void CMenuItem::Construct() {
    new (&m_10) CString();
    new (&m_14) CString();
    new (&m_4c) CString();
    new (&m_50) CString();
    new (&m_54) CString();
    new (&m_58) CString();
    m_8 = 0;
    m_c = 0;
    m_28 = 0;
    m_4 = 0;
    m_2c = 0;
    *(void**)this = &g_menuItemVtbl;
    m_34 = (i32)0xeeeeeeee;
    m_44 = (i32)0xeeeeeeee;
    m_4c.Empty();
    m_50.Empty();
    m_54.Empty();
    m_58.Empty();
}

// ===========================================================================

// 0x183250 - destructor: Clear() then tear down the CPtrList + three CStrings.
RVA(0x00183250, 0x71)
CMenuPage::~CMenuPage() {
    Clear();
}

// 0x1832d0 - return the page/item key (m_c) by value.
RVA(0x001832d0, 0x20)
CString CMenuPage::GetKey() {
    return m_c;
}

// 0x1832f0 - configure this page from a template item, then resolve its catalog
// slot via m_0->m_10's CMapStringToPtr::Lookup.
RVA(0x001832f0, 0xa5)
i32 CMenuPage::Configure(CMenuItem* tmpl, i32 a1, i32 a2, i32 a3, i32 a4) {
    if (!tmpl) {
        return 0;
    }
    i32* t = (i32*)tmpl;
    m_0 = (void*)t[0];
    m_4 = tmpl;
    m_c = (const char*)a1;
    m_8 = (const char*)a3;
    *(i32*)((char*)this + 0x48) = t[7]; // tmpl+0x1c
    *(i32*)((char*)this + 0x44) = t[6]; // tmpl+0x18
    *(i32*)((char*)this + 0x30) = a4;
    // 4-dword block copy tmpl+0x8 -> this+0x34 (the layout/geometry rect).
    struct Geom4 {
        i32 a, b, c, d;
    };
    *(Geom4*)((char*)this + 0x34) = *(Geom4*)((char*)tmpl + 0x8);
    m_58 = 0;
    m_5c = 0;
    void* slot = 0;
    ((CMenuHost*)m_0)->m_10->m_10.Lookup((const char*)a2, slot);
    m_60 = (CMenuPage*)slot;
    return slot != 0;
}

// 0x1833a0 - reset to defaults: Clear() then zero link/back/focus/cache/flags.
RVA(0x001833a0, 0x1a)
void CMenuPage::InitDefaults() {
    Clear();
    m_0 = 0;
    m_4 = 0;
    m_60 = 0;
    m_64 = 0;
    m_30 = 0;
}

// 0x1833c0 - free every child item (its deleting dtor), then RemoveAll the list.
RVA(0x001833c0, 0x2b)
void CMenuPage::Clear() {
    CMenuNode* node = (CMenuNode*)m_14.GetHeadPosition();
    while (node) {
        CMenuNode* cur = node;
        node = node->pNext;
        CMenuItem* item = cur->data;
        if (item) {
            delete item;
        }
    }
    m_14.RemoveAll();
}

// 0x183430 - append an item to the list; cache its POSITION at item+0x2c.
RVA(0x00183430, 0x24)
void* CMenuPage::Append(CMenuItem* item) {
    if (!item) {
        return 0;
    }
    item->m_2c = m_14.AddTail(item);
    return (void*)1;
}

// 0x183990 - release the current focus item, then detach every child item.
RVA(0x00183990, 0x38)
i32 CMenuPage::ReleaseAll() {
    if (m_64) {
        m_64->Release();
        m_64 = 0;
    }
    CMenuNode* node = (CMenuNode*)m_14.GetHeadPosition();
    while (node) {
        CMenuNode* cur = node;
        node = node->pNext;
        CMenuItem* item = cur->data;
        if (item) {
            item->Detach();
        }
    }
    return 1;
}

// 0x1839d0 - restore focus: if a name (m_10) was saved, focus the item matching
// it; otherwise (or if not found) focus the first focusable item.
// @early-stop
// regalloc + frameless-CString-temp wall (~61%): the saved-name scan (GetName +
// inline strcmp via a `bool match` local) and the first-focusable fallback are
// byte-aligned, but `this` lives to the tail (SetFocus) forcing a 3rd callee-saved
// reg, and the per-iteration CString temp's teardown threads differently than
// retail. Logic complete; deferred (same family as FocusNext/FindByName).
RVA(0x001839d0, 0xff)
i32 CMenuPage::RestoreFocus() {
    if (!m_10.IsEmpty()) {
        CMenuNode* node = (CMenuNode*)m_14.GetHeadPosition();
        while (node) {
            CMenuNode* cur = node;
            node = node->pNext;
            CMenuItem* item = cur->data;
            if (item) {
                CString name = item->GetName();
                bool match = strcmp(name, m_10) == 0;
                if (match) {
                    i32 k = item->m_24;
                    if (k == 1 || k == 2) {
                        if (SetFocus(item, 0)) {
                            return 1;
                        }
                    }
                }
            }
        }
    }
    CMenuNode* node = (CMenuNode*)m_14.GetHeadPosition();
    while (node) {
        CMenuNode* cur = node;
        node = node->pNext;
        CMenuItem* item = cur->data;
        if (item) {
            i32 k = item->m_24;
            if (k == 1 || k == 2) {
                if (SetFocus(item, 0)) {
                    return 1;
                }
            }
        }
    }
    return 0;
}

// 0x183ad0 - make `item` the focused item (release the prior, Configure/notify the new).
RVA(0x00183ad0, 0x57)
i32 CMenuPage::SetFocus(CMenuItem* item, i32 notify) {
    if (!item) {
        return 0;
    }
    i32 kind = item->m_24;
    if (kind == 2) {
        return 1;
    }
    if (kind != 1) {
        return 0;
    }
    if (m_64) {
        m_64->Release();
    }
    m_64 = item;
    return item->Configure((void*)notify) != 0;
}

// 0x183b30 - notify every child item.
RVA(0x00183b30, 0x2c)
i32 CMenuPage::NotifyAll(void* arg) {
    CMenuNode* node = (CMenuNode*)m_14.GetHeadPosition();
    while (node) {
        CMenuNode* cur = node;
        node = node->pNext;
        CMenuItem* item = cur->data;
        if (item) {
            item->Notify(arg);
        }
    }
    return 1;
}

// 0x183c50 - move focus to the next focusable item, wrapping if allowed.
// @early-stop
// regalloc wall (~75%): the two focusable-scan loops + the wrap + SetFocus are
// byte-aligned, but `this` stays live to the tail (SetFocus/CanWrap) so the
// recompile pins it in a 3rd callee-saved reg (ebx) where retail folds it into
// edi and pushes only esi/edi; the extra push/reg-pairing diverges the prologue
// and the per-loop node pointer. Logic complete; deferred to the final sweep.
RVA(0x00183c50, 0xbc)
i32 CMenuPage::FocusNext() {
    if (!m_64) {
        return 0;
    }
    CMenuNode* pos = (CMenuNode*)m_64->m_2c;
    if (!pos) {
        return 0;
    }
    CMenuItem* found = 0;
    CMenuNode* node = pos->pPrev;
    while (node) {
        CMenuNode* cur = node;
        node = node->pPrev;
        found = cur->data;
        if (found) {
            i32 k = found->m_24;
            if (k == 1 || k == 2) {
                break;
            }
        }
        found = 0;
    }
    if (!found) {
        if (!MenuPage_CanWrap(this)) {
            return 0;
        }
        CMenuNode* p2 = (CMenuNode*)m_64->m_2c;
        if (!p2) {
            return 0;
        }
        CMenuNode* n2 = p2->pNext;
        while (n2) {
            CMenuNode* cur = n2;
            n2 = n2->pNext;
            CMenuItem* it = cur->data;
            if (it) {
                i32 k = it->m_24;
                if (k == 1 || k == 2) {
                    found = it;
                }
            }
        }
        if (!found) {
            return 0;
        }
    }
    i32 kind = found->m_24;
    if (kind != 1 && kind != 2) {
        return 0;
    }
    if (found == m_64) {
        return 0;
    }
    return SetFocus(found, 1) != 0;
}

// 0x183d10 - move focus to the previous focusable item, wrapping if allowed.
// @early-stop
// same regalloc wall as FocusNext (~75%): mirror walk (pNext then pPrev), logic
// complete, deferred.
RVA(0x00183d10, 0xbc)
i32 CMenuPage::FocusPrev() {
    if (!m_64) {
        return 0;
    }
    CMenuNode* pos = (CMenuNode*)m_64->m_2c;
    if (!pos) {
        return 0;
    }
    CMenuItem* found = 0;
    CMenuNode* node = pos->pNext;
    while (node) {
        CMenuNode* cur = node;
        node = node->pNext;
        found = cur->data;
        if (found) {
            i32 k = found->m_24;
            if (k == 1 || k == 2) {
                break;
            }
        }
        found = 0;
    }
    if (!found) {
        if (!MenuPage_CanWrap(this)) {
            return 0;
        }
        CMenuNode* p2 = (CMenuNode*)m_64->m_2c;
        if (!p2) {
            return 0;
        }
        CMenuNode* n2 = p2->pPrev;
        while (n2) {
            CMenuNode* cur = n2;
            n2 = n2->pPrev;
            CMenuItem* it = cur->data;
            if (it) {
                i32 k = it->m_24;
                if (k == 1 || k == 2) {
                    found = it;
                }
            }
        }
        if (!found) {
            return 0;
        }
    }
    i32 kind = found->m_24;
    if (kind != 1 && kind != 2) {
        return 0;
    }
    if (found == m_64) {
        return 0;
    }
    return SetFocus(found, 1) != 0;
}

// 0x183b60 - lay out / draw the page: center the child items in the page rect,
// place each via its vtable Place, render selected ones through the host, and
// accumulate the running y.
// @early-stop
// scheduling tail (~99.98%, 231/232 B): the whole body is byte-identical except
// MSVC swaps which of two interchangeable registers (edx/edi) holds the two
// commutative `m_5c + m_38` operands at the y-init; not source-steerable (operand
// order, hoisting, and raw-vs-member access all canonicalize to the same pick).
RVA(0x00183b60, 0xe8)
i32 CMenuPage::Layout(i32 ctx) {
    if (m_30 & 4) {
        return LayoutOne(ctx);
    }
    i32 x0 = *(i32*)((char*)this + 0x34);
    i32 x1 = *(i32*)((char*)this + 0x3c);
    i32 x = (((x1 - x0 + 1) / 2)) + m_58 + x0;
    i32 y = m_5c + *(i32*)((char*)this + 0x38);
    CMenuPage* sub = m_60;
    if (sub) {
        i32 idx = *(i32*)((char*)sub + 0x64);
        CMenuItem** tab = *(CMenuItem***)((char*)sub + 0x14);
        CMenuItem* head = tab[idx];
        if (head) {
            y += head->m_1c;
            ((CMenuPlacer*)head)->Place(ctx, x, y, 0);
            y += *(i32*)((char*)this + 0x44) + head->m_1c;
        }
    }
    CMenuNode* node = (CMenuNode*)m_14.GetHeadPosition();
    while (node) {
        CMenuNode* cur = node;
        node = node->pNext;
        CMenuItem* item = cur->data;
        if (item) {
            y += item->GetWidth() / 2;
            item->Place((void*)ctx, x, y);
            if (item->m_24 == 2 && !(m_30 & 8)) {
                ((CMenuRenderHost*)m_4)->Draw(ctx, item, x, y);
            }
            y += item->GetWidth() / 2;
            y += *(i32*)((char*)this + 0x48);
        }
    }
    return 1;
}

// 0x183dd0 - activate (trigger) the focused item.
RVA(0x00183dd0, 0x16)
i32 CMenuPage::Activate() {
    if (!m_64) {
        return 0;
    }
    return m_64->Trigger() != 0;
}

// 0x184070 - hit-test at (a0,a1) and focus the item there.
RVA(0x00184070, 0x30)
i32 CMenuPage::FocusAndSelect(i32 a0, i32 a1) {
    CMenuItem* hit = HitTest(a0, a1);
    if (!hit) {
        return 0;
    }
    return SetFocus(hit, 1) != 0;
}

// 0x1840a0 - click at (a0,a1): hit-test, focus, activate, then re-focus.
RVA(0x001840a0, 0x57)
i32 CMenuPage::Click(i32 a0, i32 a1) {
    CMenuItem* hit = HitTest(a0, a1);
    if (!hit) {
        return 0;
    }
    if (!SetFocus(hit, 0)) {
        return 0;
    }
    if (!Activate()) {
        return 0;
    }
    FocusAndSelect(a0, a1);
    return 1;
}

// 0x184100 - hit-test: first child item whose own Hit(x,y) returns true.
RVA(0x00184100, 0x4a)
CMenuItem* CMenuPage::HitTest(i32 x, i32 y) {
    CMenuNode* node = (CMenuNode*)m_14.GetHeadPosition();
    while (node) {
        CMenuNode* cur = node;
        node = node->pNext;
        CMenuItem* item = cur->data;
        if (item) {
            if (item->Hit(x, y)) {
                return item;
            }
        }
    }
    return 0;
}

// 0x184150 - find the child item whose name matches `s` (linear scan + strcmp).
// @early-stop
// /GX EH-state wall (~77%): the cur-first walk, the GetName()-by-value temp, the
// inline strcmp via a `bool match` local (setcc form) and the per-iteration
// CString teardown all align; the residual is the __try state index MSVC stamps
// (entry push $8 vs $0) + the funclet cleanup scheduling. See docs/seh-eh.md and
// docs/patterns/eh-dtor-vptr-stamp-vs-trylevel-order.md. Logic complete; deferred.
RVA(0x00184150, 0xe0)
CMenuItem* CMenuPage::FindByName(const char* s) {
    if (!s) {
        return 0;
    }
    CString key(s);
    CMenuNode* node = (CMenuNode*)m_14.GetHeadPosition();
    while (node) {
        CMenuNode* cur = node;
        node = node->pNext;
        CMenuItem* item = cur->data;
        if (item) {
            CString name = item->GetName();
            bool match = strcmp(key, name) == 0;
            if (match) {
                return item;
            }
        }
    }
    return 0;
}

// 0x1843f0 - focus the item named by the forward key, else step focus forward.
// @early-stop
// /GX EH-state wall (~61%): KeyFwd -> FindByName -> SetFocus/FocusNext sequence
// is byte-aligned, residual is the __try state index (push $8 vs $0) + the
// per-return CString-temp teardown threading. Logic complete; deferred.
RVA(0x001843f0, 0xd2)
i32 CMenuPage::SelectForward() {
    if (!m_64) {
        return 0;
    }
    CString key;
    CMenuItem* item = FindByName(*MenuPage_KeyFwd(this, &key));
    if (item) {
        i32 k = item->m_24;
        if (k != 1 && k != 2) {
            return 0;
        }
        if (item == m_64) {
            return 0;
        }
        return SetFocus(item, 1);
    }
    return FocusNext();
}

// 0x1844d0 - focus the item named by the backward key, else step focus backward.
// @early-stop
// same /GX EH-state wall as SelectForward (~61%): KeyBack -> FindByName ->
// SetFocus/FocusPrev. Logic complete; deferred.
RVA(0x001844d0, 0xd2)
i32 CMenuPage::SelectBackward() {
    if (!m_64) {
        return 0;
    }
    CString key;
    CMenuItem* item = FindByName(*MenuPage_KeyBack(this, &key));
    if (item) {
        i32 k = item->m_24;
        if (k != 1 && k != 2) {
            return 0;
        }
        if (item == m_64) {
            return 0;
        }
        return SetFocus(item, 1);
    }
    return FocusPrev();
}

// 0x183460 - allocate (RezAlloc 0x5c) + placement-construct a child item, Init it,
// and append on success (else delete).
// @early-stop
// /GX placement-new wall (~34%): the allocator (RezAlloc), the Init dispatch
// (vtable+0x4), the success Append + the failure deleting-dtor are correct, but
// retail INLINES the 6-CString child ctor into the body, raising a /GX EH frame
// (push -1/push 0xb/fs:0) with descending trylevel writes [esp+0x20]=0..6 around
// each CString construct; the recompile keeps the ctor out-of-line (Construct())
// so it emits no frame. This is the documented rezalloc-placement-new EH wall
// (docs/patterns/rezalloc-placement-new-no-eh-frame.md,
// eh-dtor-vptr-stamp-vs-trylevel-order.md; topic:eh/topic:wall). Logic complete;
// deferred to the final sweep (match the child ctor as a leaf, then inline it).
RVA(0x00183460, 0x13d)
CMenuItem* CMenuPage::AddItem(i32 a0, i32 a1, i32 a2, i32 a3, i32 a4, i32 a5) {
    CMenuItem* item = (CMenuItem*)RezAlloc(0x5c);
    if (item) {
        item->Construct();
    }
    if (item->Init(a0, a1, a2, a3, a4, a5) == 0) {
        if (item) {
            delete item;
        }
        return 0;
    }
    return Append(item) ? item : 0;
}

// 0x1835a0 - like AddItem, but also links the new item to its parent context.
// @early-stop
// same /GX placement-new wall as AddItem (the inlined child ctor + EH trylevel).
RVA(0x001835a0, 0x14b)
CMenuItem* CMenuPage::AddSubItem(i32 a0, i32 a1, i32 a2, i32 a3, i32 a4, i32 a5) {
    CMenuItem* item = (CMenuItem*)RezAlloc(0x5c);
    if (item) {
        item->Construct();
    }
    if (item->Init(a0, a1, a2, a3, a4, a5) == 0) {
        if (item) {
            delete item;
        }
        return 0;
    }
    item->m_1c = a4;
    *(i32*)((char*)item + 0x30) = a5;
    return Append(item) ? item : 0;
}

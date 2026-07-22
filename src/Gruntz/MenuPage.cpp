#include <rva.h>
#include <Rez/RezAlloc.h> // RezAlloc/RezFree
#include <Gruntz/ChatBox.h>
#include <Image/CImage.h>
#include <DDrawMgr/DDrawWorker.h> // CDDrawWorker - the image-registry strip m_subPage caches

#include <Gruntz/MenuPage.h>
#include <Gruntz/GameRegistry.h> // CDDrawSurfaceMgr (m_owner) - its m_10 CDDrawWorkerRegistry
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawWorkerRegistry.h> // m_imageRegistry (full def)
#include <Gruntz/Sprite.h>                // CDDrawWorker (fold: ex via ResMgr.h)
#include <DDrawMgr/DDrawSubMgrPages.h> // the m_drawTarget pages (fold: ex ResMgr.h CDrawTarget)       // CDDrawWorkerRegistry (== CDDrawWorkerRegistry): its m_10map catalog

inline void* operator new(size_t, void* p) {
    return p;
}

RVA(0x001832d0, 0x20)
CString CMenuPage::GetKey() {
    return m_key;
}

RVA(0x001832f0, 0xa5)
i32 CMenuPage::Configure(
    CChatBox* host,
    const char* label,
    const char* key,
    const char* parent,
    i32 flags
) {
    if (!host) {
        return 0;
    }
    m_owner = host->m_page;
    m_host = host;
    m_key = label;
    m_switchKey = parent;
    m_rowSpacing = host->m_rowSpacing; // host+0x1c
    m_headGap = host->m_headGap;       // host+0x18
    m_flags = flags;
    m_rect = host->m_rect8; // 16-byte block copy host+0x8 -> this+0x34
    m_offsetX = 0;
    m_offsetY = 0;
    CObject* slot_ob = 0;
    m_owner->m_imageRegistry->m_10map.Lookup(key, slot_ob);
    void* slot = static_cast<void*>(slot_ob);
    m_subPage = static_cast<CDDrawWorker*>(slot_ob);
    return slot != 0;
}

RVA(0x001833a0, 0x1a)
void CMenuPage::InitDefaults() {
    Clear();
    m_owner = 0;
    m_host = 0;
    m_subPage = 0;
    m_focus = 0;
    m_flags = 0;
}

RVA(0x001833c0, 0x2b)
void CMenuPage::Clear() {
    CMenuListNode* node = reinterpret_cast<CMenuListNode*>(m_items.GetHeadPosition());
    while (node) {
        CMenuListNode* cur = node;
        node = node->pNext;
        CMenuItem* item = cur->data;
        if (item) {
            delete item;
        }
    }
    m_items.RemoveAll();
}

// ResolveSubPage: look `key` up in the owner's catalog map, cache the resolved
// entry in m_subPage, and return whether it was found.
// @early-stop
// ~90.5% - schedule coin-flip (topic:scheduling): body byte-exact and identical in
// shape to CMenuPage::Configure's catalog Lookup; the sole residual is MSVC5
// emitting the `mov [slot],0` init AFTER both call-arg pushes (retail) vs between
// them (cl). Permuter confirmed no source spelling reorders it (90.238 -> 90.238).
RVA(0x001833f0, 0x38)
i32 CMenuPage::ResolveSubPage(const char* key) {
    CObject* slot_ob = 0;
    m_owner->m_imageRegistry->m_10map.Lookup(key, slot_ob);
    void* slot = static_cast<void*>(slot_ob);
    m_subPage = static_cast<CDDrawWorker*>(slot_ob);
    return slot != 0;
}

RVA(0x00183430, 0x24)
void* CMenuPage::Append(CMenuItem* item) {
    if (!item) {
        return 0;
    }
    item->m_listPos = m_items.AddTail(item);
    return reinterpret_cast<void*>(1);
}

RVA(0x00183990, 0x38)
i32 CMenuPage::ReleaseAll() {
    if (m_focus) {
        m_focus->Release();
        m_focus = 0;
    }
    CMenuListNode* node = reinterpret_cast<CMenuListNode*>(m_items.GetHeadPosition());
    while (node) {
        CMenuListNode* cur = node;
        node = node->pNext;
        CMenuItem* item = cur->data;
        if (item) {
            item->Detach();
        }
    }
    return 1;
}

// restore focus: if a name (m_focusName) was saved, focus the item matching
// it; otherwise (or if not found) focus the first focusable item.
// @early-stop
// regalloc + frameless-CString-temp wall (~61%): the saved-name scan (GetName +
// inline strcmp via a `bool match` local) and the first-focusable fallback are
// byte-aligned, but `this` lives to the tail (SetFocus) forcing a 3rd callee-saved
// reg, and the per-iteration CString temp's teardown threads differently than
// retail. Logic complete; deferred (same family as FocusNext/FindByName).
RVA(0x001839d0, 0xff)
i32 CMenuPage::RestoreFocus() {
    if (!m_focusName.IsEmpty()) {
        CMenuListNode* node = reinterpret_cast<CMenuListNode*>(m_items.GetHeadPosition());
        while (node) {
            CMenuListNode* cur = node;
            node = node->pNext;
            CMenuItem* item = cur->data;
            if (item) {
                CString name = item->GetName();
                bool match = strcmp(name, m_focusName) == 0;
                if (match) {
                    i32 k = item->m_state;
                    if (k == 1 || k == 2) {
                        if (SetFocus(item, 0)) {
                            return 1;
                        }
                    }
                }
            }
        }
    }
    CMenuListNode* node = reinterpret_cast<CMenuListNode*>(m_items.GetHeadPosition());
    while (node) {
        CMenuListNode* cur = node;
        node = node->pNext;
        CMenuItem* item = cur->data;
        if (item) {
            i32 k = item->m_state;
            if (k == 1 || k == 2) {
                if (SetFocus(item, 0)) {
                    return 1;
                }
            }
        }
    }
    return 0;
}

RVA(0x00183ad0, 0x57)
i32 CMenuPage::SetFocus(CMenuItem* item, i32 notify) {
    if (!item) {
        return 0;
    }
    i32 kind = item->m_state;
    if (kind == 2) {
        return 1;
    }
    if (kind != 1) {
        return 0;
    }
    if (m_focus) {
        m_focus->Release();
    }
    m_focus = item;
    return item->Configure(reinterpret_cast<void*>(notify)) != 0;
}

RVA(0x00183b30, 0x2c)
i32 CMenuPage::NotifyAll(void* arg) {
    CMenuListNode* node = reinterpret_cast<CMenuListNode*>(m_items.GetHeadPosition());
    while (node) {
        CMenuListNode* cur = node;
        node = node->pNext;
        CMenuItem* item = cur->data;
        if (item) {
            item->Notify(arg);
        }
    }
    return 1;
}

// move focus to the next focusable item, wrapping if allowed.
// @early-stop
// regalloc wall (~75%): the two focusable-scan loops + the wrap + SetFocus are
// byte-aligned, but `this` stays live to the tail (SetFocus/CanWrap) so the
// recompile pins it in a 3rd callee-saved reg (ebx) where retail folds it into
// edi and pushes only esi/edi; the extra push/reg-pairing diverges the prologue
// and the per-loop node pointer. Logic complete; deferred to the final sweep.
RVA(0x00183c50, 0xbc)
i32 CMenuPage::FocusNext() {
    if (!m_focus) {
        return 0;
    }
    CMenuListNode* pos = reinterpret_cast<CMenuListNode*>(m_focus->m_listPos);
    if (!pos) {
        return 0;
    }
    CMenuItem* found = 0;
    CMenuListNode* node = pos->pPrev;
    while (node) {
        CMenuListNode* cur = node;
        node = node->pPrev;
        found = cur->data;
        if (found) {
            i32 k = found->m_state;
            if (k == 1 || k == 2) {
                break;
            }
        }
        found = 0;
    }
    if (!found) {
        if (!CanWrap()) {
            return 0;
        }
        CMenuListNode* p2 = reinterpret_cast<CMenuListNode*>(m_focus->m_listPos);
        if (!p2) {
            return 0;
        }
        CMenuListNode* n2 = p2->pNext;
        while (n2) {
            CMenuListNode* cur = n2;
            n2 = n2->pNext;
            CMenuItem* it = cur->data;
            if (it) {
                i32 k = it->m_state;
                if (k == 1 || k == 2) {
                    found = it;
                }
            }
        }
        if (!found) {
            return 0;
        }
    }
    i32 kind = found->m_state;
    if (kind != 1 && kind != 2) {
        return 0;
    }
    if (found == m_focus) {
        return 0;
    }
    return SetFocus(found, 1) != 0;
}

// move focus to the previous focusable item, wrapping if allowed.
// @early-stop
// same regalloc wall as FocusNext (~75%): mirror walk (pNext then pPrev), logic
// complete, deferred.
RVA(0x00183d10, 0xbc)
i32 CMenuPage::FocusPrev() {
    if (!m_focus) {
        return 0;
    }
    CMenuListNode* pos = reinterpret_cast<CMenuListNode*>(m_focus->m_listPos);
    if (!pos) {
        return 0;
    }
    CMenuItem* found = 0;
    CMenuListNode* node = pos->pNext;
    while (node) {
        CMenuListNode* cur = node;
        node = node->pNext;
        found = cur->data;
        if (found) {
            i32 k = found->m_state;
            if (k == 1 || k == 2) {
                break;
            }
        }
        found = 0;
    }
    if (!found) {
        if (!CanWrap()) {
            return 0;
        }
        CMenuListNode* p2 = reinterpret_cast<CMenuListNode*>(m_focus->m_listPos);
        if (!p2) {
            return 0;
        }
        CMenuListNode* n2 = p2->pPrev;
        while (n2) {
            CMenuListNode* cur = n2;
            n2 = n2->pPrev;
            CMenuItem* it = cur->data;
            if (it) {
                i32 k = it->m_state;
                if (k == 1 || k == 2) {
                    found = it;
                }
            }
        }
        if (!found) {
            return 0;
        }
    }
    i32 kind = found->m_state;
    if (kind != 1 && kind != 2) {
        return 0;
    }
    if (found == m_focus) {
        return 0;
    }
    return SetFocus(found, 1) != 0;
}

// lay out / draw the page: center the child items in the page rect,
// place each via its vtable Place, render selected ones through the host, and
// accumulate the running y.
// @early-stop
// scheduling tail (~99.98%, 231/232 B): the whole body is byte-identical except
// MSVC swaps which of two interchangeable registers (edx/edi) holds the two
// commutative `m_offsetY + m_rectTop` operands at the y-init; not source-steerable (operand
// order, hoisting, and raw-vs-member access all canonicalize to the same pick).
RVA(0x00183b60, 0xe8)
i32 CMenuPage::Layout(i32 ctx) {
    if (m_flags & 4) {
        return LayoutOne(ctx);
    }
    i32 x0 = m_rect.left;
    i32 x1 = m_rect.right;
    i32 x = (((x1 - x0 + 1) / 2)) + m_offsetX + x0;
    i32 y = m_offsetY + m_rect.top;
    CDDrawWorker* sub = m_subPage;
    if (sub) {
        CImage* head = static_cast<CImage*>(sub->m_items.GetAt(sub->m_minIndex));
        if (head) {
            y += head->m_anchorY;
            head->RenderFrame(
                reinterpret_cast<void*>(ctx),
                reinterpret_cast<void*>(x),
                reinterpret_cast<void*>(y),
                static_cast<void*>(0)
            );
            y += m_headGap + head->m_anchorY;
        }
    }
    CMenuListNode* node = reinterpret_cast<CMenuListNode*>(m_items.GetHeadPosition());
    while (node) {
        CMenuListNode* cur = node;
        node = node->pNext;
        CMenuItem* item = cur->data;
        if (item) {
            y += item->GetWidth() / 2;
            item->Place(ctx, x, y);
            if (item->m_state == 2 && !(m_flags & 8)) {
                m_host->Draw(ctx, reinterpret_cast<i32>(item), x, y);
            }
            y += item->GetWidth() / 2;
            y += m_rowSpacing;
        }
    }
    return 1;
}

RVA(0x00183dd0, 0x16)
i32 CMenuPage::Activate() {
    if (!m_focus) {
        return 0;
    }
    return m_focus->Trigger() != 0;
}

RVA(0x00183df0, 0x3d)
i32 CMenuPage::Switch(i32 refocus) {
    if (m_switchKey.GetLength() == 0) {
        return 0;
    }
    if (!m_host->ReplaceNode(const_cast<char*>(static_cast<const char*>(m_switchKey)))) {
        return 0;
    }
    if (refocus) {
        m_host->ScrollRow1();
    }
    return 1;
}

// whether focus may wrap at the page ends: never if hidden (0x2),
// always if enabled (0x1), else defer to the host's wrap flag.
// @early-stop
// @early-stop
// movb-vs-movsx peephole wall (~95%, 1 instruction): retail sign-extends the LOW
// BYTE of the host's i32 field (`movsx eax, byte [eax+0x20]; and eax,1`; the field
// IS i32 - Init 0x182ab0 stores the whole DWORD). MSVC5 /O2 narrows to
// `mov al,[...]; and eax,1` for every spelling incl. the (char)-of-i32 cast arm
// (tested 2026-07-13). See docs/patterns/char-and1-movb-vs-movsx.md.
RVA(0x00183e30, 0x1f)
i32 CMenuPage::CanWrap() {
    i32 f = m_flags;
    if (f & 2) {
        return 0;
    }
    if (f & 1) {
        return 1;
    }
    return static_cast<char>(m_host->m_wrapFlag) & 1; // m_host is the owning CChatBox (i32 @+0x20)
}

// single-list grid layout: center each child in the page rect, place
// it (vtable +0x24), render the selected one (host Draw) and advance x/y, wrapping
// to a new column every m_rowsPerCol rows.
// @early-stop
// scheduling tail (~99.98%, 1 B): the whole body incl. the now byte-exact `sub
// esp,0xc` prologue matches; the residual is the same edx/edi commutative
// operand-order pick as the sibling Layout (0x183b60) at the y-init - not source-
// steerable (canonicalizes to the same register pick). Logic complete.
RVA(0x00183e50, 0x11c)
i32 CMenuPage::LayoutOne(i32 ctx) {
    i32 x0 = m_rect.left;
    i32 x1 = m_rect.right;
    i32 x = (((x1 - x0 + 1) / 2)) + m_offsetX + x0;
    i32 y = m_offsetY + m_rect.top;
    CDDrawWorker* sub = m_subPage;
    if (sub) {
        CImage* head = static_cast<CImage*>(sub->m_items.GetAt(sub->m_minIndex));
        if (head) {
            y += head->m_anchorY;
            head->RenderFrame(
                reinterpret_cast<void*>(ctx),
                reinterpret_cast<void*>(x),
                reinterpret_cast<void*>(y),
                static_cast<void*>(0)
            );
            y += m_headGap + head->m_anchorY;
        }
    }
    i32 col = ((m_colWidth / 2)) + m_rect.left + m_colOffset;
    i32 ytop = y;
    i32 row = 0;
    CMenuListNode* node = reinterpret_cast<CMenuListNode*>(m_items.GetHeadPosition());
    while (node) {
        CMenuListNode* cur = node;
        node = node->pNext;
        CMenuItem* item = cur->data;
        if (item) {
            y += item->GetWidth() / 2;
            item->Place(ctx, col, y);
            if (item->m_state == 2 && !(m_flags & 8)) {
                m_host->Draw(ctx, reinterpret_cast<i32>(item), col, y);
            }
            y += item->GetWidth() / 2;
            y += m_rowSpacing;
        }
        if (++row < m_rowsPerCol) {
            // same column
        } else {
            col += m_colWidth;
            y = ytop;
            row = 0;
        }
    }
    return 1;
}

RVA(0x00183f70, 0x74)
i32 CMenuPage::FocusForwardN() {
    CMenuItem* cur = m_focus;
    if (!cur) {
        return 0;
    }
    if (!(m_flags & 4)) {
        return 0;
    }
    CMenuListNode* pos = reinterpret_cast<CMenuListNode*>(cur->m_listPos);
    if (!pos) {
        return 0;
    }
    i32 n = m_rowsPerCol;
    CMenuItem* found = 0;
    if (n >= 0) {
        n++;
        do {
            if (pos) {
                CMenuListNode* node = pos;
                pos = node->pNext;
                found = node->data;
            } else {
                found = 0;
            }
        } while (--n);
    }
    if (!found) {
        return 0;
    }
    i32 k = found->m_state;
    if (k != 1 && k != 2) {
        return 0;
    }
    if (found == cur) {
        return 0;
    }
    return SetFocus(found, 1) != 0;
}

RVA(0x00183ff0, 0x75)
i32 CMenuPage::FocusBackwardN() {
    CMenuItem* cur = m_focus;
    if (!cur) {
        return 0;
    }
    if (!(m_flags & 4)) {
        return 0;
    }
    CMenuListNode* pos = reinterpret_cast<CMenuListNode*>(cur->m_listPos);
    if (!pos) {
        return 0;
    }
    i32 n = m_rowsPerCol;
    CMenuItem* found = 0;
    if (n >= 0) {
        n++;
        do {
            if (pos) {
                CMenuListNode* node = pos;
                pos = node->pPrev;
                found = node->data;
            } else {
                found = 0;
            }
        } while (--n);
    }
    if (!found) {
        return 0;
    }
    i32 k = found->m_state;
    if (k != 1 && k != 2) {
        return 0;
    }
    if (found == cur) {
        return 0;
    }
    return SetFocus(found, 1) != 0;
}

RVA(0x00184070, 0x30)
i32 CMenuPage::FocusAndSelect(i32 a0, i32 a1) {
    CMenuItem* hit = HitTest(a0, a1);
    if (!hit) {
        return 0;
    }
    return SetFocus(hit, 1) != 0;
}

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

RVA(0x00184100, 0x4a)
CMenuItem* CMenuPage::HitTest(i32 x, i32 y) {
    CMenuListNode* node = reinterpret_cast<CMenuListNode*>(m_items.GetHeadPosition());
    while (node) {
        CMenuListNode* cur = node;
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

// find the child item whose name matches `s` (linear scan + strcmp).
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
    CMenuListNode* node = reinterpret_cast<CMenuListNode*>(m_items.GetHeadPosition());
    while (node) {
        CMenuListNode* cur = node;
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

// focus the item named by the focused item's forward key (GetKey1),
// else step focus backward by m_rowsPerCol nodes.
// @early-stop
// /GX EH-state wall (~61%, same family as SelectForward): m_focus->GetKey1() ->
// FindByName -> SetFocus / FocusBackwardN is byte-aligned; the residual is the
// __try state index (push $8 vs $0) + the per-return CString-temp teardown
// threading. See docs/seh-eh.md + eh-dtor-vptr-stamp-vs-trylevel-order.md.
RVA(0x00184230, 0xd2)
i32 CMenuPage::SelectFwd2() {
    if (!m_focus) {
        return 0;
    }
    CString key = m_focus->GetNavFwdName();
    CMenuItem* item = FindByName(key);
    if (item) {
        i32 k = item->m_state;
        if (k != 1 && k != 2) {
            return 0;
        }
        if (item == m_focus) {
            return 0;
        }
        return SetFocus(item, 1);
    }
    return FocusBackwardN();
}

// mirror: focused item's backward key (GetKey2), else FocusForwardN.
// @early-stop
// same /GX EH-state wall as SelectFwd2 (~61%). Logic complete; deferred.
RVA(0x00184310, 0xd2)
i32 CMenuPage::SelectBack2() {
    if (!m_focus) {
        return 0;
    }
    CString key = m_focus->GetNavBackName();
    CMenuItem* item = FindByName(key);
    if (item) {
        i32 k = item->m_state;
        if (k != 1 && k != 2) {
            return 0;
        }
        if (item == m_focus) {
            return 0;
        }
        return SetFocus(item, 1);
    }
    return FocusForwardN();
}

// focus the item named by the forward key (m_focus->GetField54), else FocusNext.
// @early-stop
// Shape fixed 2026-07-21 (58->62%): the fwd key is m_focus->GetField54() (NRV thiscall
// on m_focus, 0x184610) - the ex `MenuPage_KeyFwd(this,&key)` cdecl free-function alias
// was the wrong call shape. Residual (~38%, same as the SelectFwd2/Back2 siblings) is
// the genuine /GX EH-state wall: the __try state index (push $8 vs $0) + the per-return
// CString-temp teardown threading. Deferred.
RVA(0x001843f0, 0xd2)
i32 CMenuPage::SelectForward() {
    if (!m_focus) {
        return 0;
    }
    CString key = m_focus->GetField54();
    CMenuItem* item = FindByName(key);
    if (item) {
        i32 k = item->m_state;
        if (k != 1 && k != 2) {
            return 0;
        }
        if (item == m_focus) {
            return 0;
        }
        return SetFocus(item, 1);
    }
    return FocusNext();
}

// focus the item named by the backward key (m_focus->GetField58), else FocusPrev.
// @early-stop
// Shape fixed 2026-07-21 (58->62%): the back key is m_focus->GetField58() (NRV thiscall
// on m_focus, 0x184630); the ex `MenuPage_KeyBack(this,&key)` cdecl alias was wrong.
// Residual is the same /GX EH-state wall as SelectForward/the SelectBack2 sibling. Deferred.
RVA(0x001844d0, 0xd2)
i32 CMenuPage::SelectBackward() {
    if (!m_focus) {
        return 0;
    }
    CString key = m_focus->GetField58();
    CMenuItem* item = FindByName(key);
    if (item) {
        i32 k = item->m_state;
        if (k != 1 && k != 2) {
            return 0;
        }
        if (item == m_focus) {
            return 0;
        }
        return SetFocus(item, 1);
    }
    return FocusPrev();
}

RVA(0x00183460, 0x13d)
CMenuItem* CMenuPage::AddItem(
    const char* label,
    const char* spriteKey,
    i32 cmdId,
    const char* key,
    i32 flags
) {
    CMenuItem* item = new CMenuItem();
    // Init keeps its mangling-pinned i32 slots (virtual); the string args cast at
    // the forward (same 4-byte pushes).
    if (item->Init(
            reinterpret_cast<i32>(this),
            reinterpret_cast<i32>(label),
            reinterpret_cast<i32>(spriteKey),
            cmdId,
            reinterpret_cast<i32>(key),
            flags
        )
        == 0) {
        if (item) {
            delete item;
        }
        return 0;
    }
    return Append(item) ? item : 0;
}

RVA(0x001835a0, 0x14b)
CMenuItem* CMenuPage::AddSubItem(
    const char* label,
    const char* spriteKey,
    i32 cmdId,
    i32 cmdParam,
    i32 tag,
    const char* key,
    i32 flags
) {
    CMenuItem* item = new CMenuItem();
    if (item->Init(
            reinterpret_cast<i32>(this),
            reinterpret_cast<i32>(label),
            reinterpret_cast<i32>(spriteKey),
            cmdId,
            reinterpret_cast<i32>(key),
            flags
        )
        == 0) {
        if (item) {
            delete item;
        }
        return 0;
    }
    item->m_1c = tag;
    item->m_cmdParam = cmdParam;
    return Append(item) ? item : 0;
}

// `new CMenuItem2()` folded from the split RezAlloc(0x74)+placement-new (the derived
// ctor runs the base ctor -> derived vptr stamp -> seeds +0x5c..+0x70), Init it
// (vtable +0x4), then on success run its slot-14 setter (SetFrame) and append (else
// delete). The fold recovers the /GX EH frame + inlined base ctor (34%->94%).
// @early-stop
// 93.9%: residual is the EH trylevel scheduling around the inlined 6-CString base
// ctor (docs/patterns/rezalloc-placement-new-no-eh-frame.md); logic byte-exact.
RVA(0x001836f0, 0x160)
CMenuItem2* CMenuPage::AddItem2(i32 a0, i32 a1, i32 a2, i32 a3, i32 a4) {
    CMenuItem2* item = new CMenuItem2();
    if (item->Init(a4, a3, a2, a1, a0, reinterpret_cast<i32>(this)) == 0) {
        if (item) {
            delete item;
        }
        return 0;
    }
    item->SetFrame(a0);
    return Append(item) ? item : 0;
}

// like AddItem2, but the new item links its parent context (item+0x30/m_1c) on
// success. `new CMenuItem2()` folded from the split RezAlloc+placement-new (49%->58%).
// @early-stop
// 58%: same inlined-base-ctor EH-trylevel-scheduling residual as AddItem2, amplified
// by the extra parent-link stores (docs/patterns/rezalloc-placement-new-no-eh-frame.md).
RVA(0x00183850, 0x13b)
CMenuItem2* CMenuPage::AddSubItem2(i32 a0, i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6, i32 a7) {
    CMenuItem2* item = new CMenuItem2();
    if (item->Init(a6, a4, a2, a1, a0, reinterpret_cast<i32>(this)) == 0) {
        if (item) {
            delete item;
        }
        return 0;
    }
    item->SetFrame(a7);
    item->m_cmdParam = a2;
    item->m_1c = a3;
    return Append(item) ? item : 0;
}

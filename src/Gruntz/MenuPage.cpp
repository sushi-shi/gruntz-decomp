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
    m_subPage = static_cast<CDDrawWorker*>(slot_ob);
    return slot_ob != 0;
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
    POSITION node = m_items.GetHeadPosition();
    while (node) {
        CMenuItem* item = NextItem(node);
        if (item) {
            delete item;
        }
    }
    m_items.RemoveAll();
}

// ResolveSubPage: look `key` up in the owner's catalog map, cache the resolved
// entry in m_subPage, and return whether it was found.
// @early-stop
RVA(0x001833f0, 0x38)
i32 CMenuPage::ResolveSubPage(const char* key) {
    CObject* slot_ob = 0;
    m_owner->m_imageRegistry->m_10map.Lookup(key, slot_ob);
    m_subPage = static_cast<CDDrawWorker*>(slot_ob);
    return slot_ob != 0;
}

RVA(0x00183430, 0x24)
i32 CMenuPage::Append(CMenuItem* item) {
    if (!item) {
        return 0;
    }
    item->m_listPos = m_items.AddTail(item);
    return 1;
}

RVA(0x00183990, 0x38)
i32 CMenuPage::ReleaseAll() {
    if (m_focus) {
        m_focus->Release();
        m_focus = 0;
    }
    POSITION node = m_items.GetHeadPosition();
    while (node) {
        CMenuItem* item = NextItem(node);
        if (item) {
            item->Detach();
        }
    }
    return 1;
}

// restore focus: if a name (m_focusName) was saved, focus the item matching
// it; otherwise (or if not found) focus the first focusable item.
RVA(0x001839d0, 0xff)
i32 CMenuPage::RestoreFocus() {
    if (!m_focusName.IsEmpty()) {
        POSITION node = m_items.GetHeadPosition();
        while (node) {
            CMenuItem* item = NextItem(node);
            if (item) {
                bool match = item->GetName() == m_focusName;
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
    POSITION node = m_items.GetHeadPosition();
    while (node) {
        CMenuItem* item = NextItem(node);
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
    return item->Configure(notify) != 0;
}

RVA(0x00183b30, 0x2c)
i32 CMenuPage::NotifyAll(u32 dt) {
    POSITION node = m_items.GetHeadPosition();
    while (node) {
        CMenuItem* item = NextItem(node);
        if (item) {
            item->Notify(dt);
        }
    }
    return 1;
}

// move focus to the next focusable item, wrapping if allowed.
// EXACT. Two shape fixes, both once filed as a "regalloc wall": (1) the no-wrap gate is
// not its own exit - written positively (`if (CanWrap()) { ... }` with one trailing
// `if (!found) return 0;`) it branches into the SHARED post-wrap return as retail's
// 0x183dbc does (77.13 -> 97.18, 2026-07-27); (2) the scan loop ends by NULLING the
// cursor, not `break` (97.18 -> 100.00, 2026-07-28, found by jcc_sieve).
RVA(0x00183c50, 0xbc)
i32 CMenuPage::FocusNext() {
    if (!m_focus) {
        return 0;
    }
    POSITION pos = m_focus->m_listPos;
    if (!pos) {
        return 0;
    }
    CMenuItem* found = 0;
    POSITION node = pos;

    PrevItem(node);
    while (node) {
        found = PrevItem(node);
        if (found) {
            i32 k = found->m_state;
            if (k == 1 || k == 2) {
                // NOT `break`: retail ends the scan by NULLING the cursor
                // (`xor eax,eax / jmp <bottom test>`, 0x183972) and lets the while
                // condition fall out - a `break` jumps straight past the bottom test.
                node = 0;
                continue;
            }
        }
        found = 0;
    }
    if (!found) {
        // the no-wrap gate is NOT its own exit: retail 0x183dbc/0x183e7c branches it
        // into the shared post-wrap `found == 0` return
        if (CanWrap()) {
            POSITION cur = m_focus->m_listPos;
            if (!cur) {
                return 0;
            }
            POSITION n2 = cur;

            NextItem(n2);
            while (n2) {
                CMenuItem* it = NextItem(n2);
                if (it) {
                    i32 k = it->m_state;
                    if (k == 1 || k == 2) {
                        found = it;
                    }
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
// EXACT. Mirror of FocusNext (pNext then pPrev) and fixed by the same two shape changes;
// see its note.
RVA(0x00183d10, 0xbc)
i32 CMenuPage::FocusPrev() {
    if (!m_focus) {
        return 0;
    }
    POSITION pos = m_focus->m_listPos;
    if (!pos) {
        return 0;
    }
    CMenuItem* found = 0;
    POSITION node = pos;

    NextItem(node);
    while (node) {
        found = NextItem(node);
        if (found) {
            i32 k = found->m_state;
            if (k == 1 || k == 2) {
                // NOT `break`: retail ends the scan by NULLING the cursor
                // (`xor eax,eax / jmp <bottom test>`, 0x183972) and lets the while
                // condition fall out - a `break` jumps straight past the bottom test.
                node = 0;
                continue;
            }
        }
        found = 0;
    }
    if (!found) {
        // the no-wrap gate is NOT its own exit: retail 0x183dbc/0x183e7c branches it
        // into the shared post-wrap `found == 0` return
        if (CanWrap()) {
            POSITION cur = m_focus->m_listPos;
            if (!cur) {
                return 0;
            }
            POSITION n2 = cur;

            PrevItem(n2);
            while (n2) {
                CMenuItem* it = PrevItem(n2);
                if (it) {
                    i32 k = it->m_state;
                    if (k == 1 || k == 2) {
                        found = it;
                    }
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
RVA(0x00183b60, 0xe8)
i32 CMenuPage::Layout(CDDrawSurfacePair* target) {
    if (m_flags & 4) {
        return LayoutOne(target);
    }
    i32 x0 = m_rect.left;
    i32 x1 = m_rect.right;
    i32 y = m_offsetY + m_rect.top;
    i32 x = (((x1 - x0 + 1) / 2)) + m_offsetX + x0;
    CDDrawWorker* sub = m_subPage;
    if (sub) {
        CImage* head = static_cast<CImage*>(sub->m_items.GetAt(sub->m_minIndex));
        if (head) {
            y += head->m_anchorY;
            head->RenderFrame(target, x, y, 0);
            y += m_headGap + head->m_anchorY;
        }
    }
    POSITION node = m_items.GetHeadPosition();
    while (node) {
        CMenuItem* item = NextItem(node);
        if (item) {
            y += item->GetWidth() / 2;
            item->Place(target, x, y);
            if (item->m_state == 2 && !(m_flags & 8)) {
                m_host->Draw(target, item, x, y);
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
    if (!m_host->ReplaceNode(m_switchKey)) {
        return 0;
    }
    if (refocus) {
        m_host->ScrollRow1();
    }
    return 1;
}

// whether focus may wrap at the page ends: never if hidden (0x2),
// always if enabled (0x1), else defer to the host's wrap flag.
// The last arm is a BRANCH in the source, not a bare `& 1` expression: written as
// `if (w & 1) return 1; return 0;` cl folds the pair back to `and eax,1` but no
// longer runs the byte-narrowing peephole, so the load stays `movsx eax,byte`
// - which is retail. Spelled as a returned expression, every form (incl. the
// (char) cast, a char local and a ?: ) narrows to `mov al,[..]`.
// See docs/patterns/char-and1-movb-vs-movsx.md.
RVA(0x00183e30, 0x1f)
i32 CMenuPage::CanWrap() {
    i32 f = m_flags;
    if (f & 2) {
        return 0;
    }
    if (f & 1) {
        return 1;
    }
    i32 w = static_cast<char>(m_host->m_wrapFlag); // m_host is the owning CChatBox (i32 @+0x20)
    if (w & 1) {
        return 1;
    }
    return 0;
}

// single-list grid layout: center each child in the page rect, place
// it (vtable +0x24), render the selected one (host Draw) and advance x/y, wrapping
// to a new column every m_rowsPerCol rows.
// @early-stop
RVA(0x00183e50, 0x11c)
i32 CMenuPage::LayoutOne(CDDrawSurfacePair* target) {
    i32 x0 = m_rect.left;
    i32 x1 = m_rect.right;
    i32 x = (((x1 - x0 + 1) / 2)) + m_offsetX + x0;
    i32 y = m_offsetY + m_rect.top;
    CDDrawWorker* sub = m_subPage;
    if (sub) {
        CImage* head = static_cast<CImage*>(sub->m_items.GetAt(sub->m_minIndex));
        if (head) {
            y += head->m_anchorY;
            head->RenderFrame(target, x, y, 0);
            y += m_headGap + head->m_anchorY;
        }
    }
    i32 col = ((m_colWidth / 2)) + m_rect.left + m_colOffset;
    i32 ytop = y;
    i32 row = 0;
    POSITION node = m_items.GetHeadPosition();
    while (node) {
        CMenuItem* item = NextItem(node);
        if (item) {
            y += item->GetWidth() / 2;
            item->Place(target, col, y);
            if (item->m_state == 2 && !(m_flags & 8)) {
                m_host->Draw(target, item, col, y);
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
    // MFC's CPtrList::GetNext IS this walk inlined (`node = pos; pos = node->pNext;
    // return node->data`), so the ex-CMenuListNode raw-node view is just the
    // accessor spelled out - same two loads, no pun.
    POSITION pos = cur->m_listPos;
    if (!pos) {
        return 0;
    }
    i32 n = m_rowsPerCol;
    CMenuItem* found = 0;
    if (n >= 0) {
        n++;
        do {
            if (pos != 0) {
                found = static_cast<CMenuItem*>(m_items.GetNext(pos));
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
    // MFC's CPtrList::GetPrev IS this walk inlined (`node = pos; pos = node->pPrev;
    // return node->data`), so the ex-CMenuListNode raw-node view is just the
    // accessor spelled out - same two loads, no pun.
    POSITION pos = cur->m_listPos;
    if (!pos) {
        return 0;
    }
    i32 n = m_rowsPerCol;
    CMenuItem* found = 0;
    if (n >= 0) {
        n++;
        do {
            if (pos != 0) {
                found = static_cast<CMenuItem*>(m_items.GetPrev(pos));
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
i32 CMenuPage::FocusAndSelect(i32 x, i32 y) {
    CMenuItem* hit = HitTest(x, y);
    if (!hit) {
        return 0;
    }
    return SetFocus(hit, 1) != 0;
}

RVA(0x001840a0, 0x57)
i32 CMenuPage::Click(i32 x, i32 y) {
    CMenuItem* hit = HitTest(x, y);
    if (!hit) {
        return 0;
    }
    if (!SetFocus(hit, 0)) {
        return 0;
    }
    if (!Activate()) {
        return 0;
    }
    FocusAndSelect(x, y);
    return 1;
}

RVA(0x00184100, 0x4a)
CMenuItem* CMenuPage::HitTest(i32 x, i32 y) {
    POSITION node = m_items.GetHeadPosition();
    while (node) {
        CMenuItem* item = NextItem(node);
        if (item) {
            if (item->Hit(x, y)) {
                return item;
            }
        }
    }
    return 0;
}

// find the child item whose name matches `s` (linear scan + strcmp).
RVA(0x00184150, 0xe0)
CMenuItem* CMenuPage::FindByName(const char* s) {
    if (!s) {
        return 0;
    }
    CString key(s);
    POSITION node = m_items.GetHeadPosition();
    while (node) {
        CMenuItem* item = NextItem(node);
        if (item) {
            bool match = strcmp(key, item->GetName()) == 0;
            if (match) {
                return item;
            }
        }
    }
    return 0;
}

// focus the item named by the focused item's forward key (GetKey1),
// else step focus backward by m_rowsPerCol nodes.
RVA(0x00184230, 0xd2)
i32 CMenuPage::SelectFwd2() {
    if (!m_focus) {
        return 0;
    }
    CMenuItem* item = FindByName(m_focus->GetNavFwdName());
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
RVA(0x00184310, 0xd2)
i32 CMenuPage::SelectBack2() {
    if (!m_focus) {
        return 0;
    }
    CMenuItem* item = FindByName(m_focus->GetNavBackName());
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
RVA(0x001843f0, 0xd2)
i32 CMenuPage::SelectForward() {
    if (!m_focus) {
        return 0;
    }
    CMenuItem* item = FindByName(m_focus->GetField54());
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
RVA(0x001844d0, 0xd2)
i32 CMenuPage::SelectBackward() {
    if (!m_focus) {
        return 0;
    }
    CMenuItem* item = FindByName(m_focus->GetField58());
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
    if (item->Init(this, label, spriteKey, cmdId, key, flags) == 0) {
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
    if (item->Init(this, label, spriteKey, cmdId, key, flags) == 0) {
        if (item) {
            delete item;
        }
        return 0;
    }
    item->m_cmdParam = cmdParam;
    item->m_1c = tag;
    return Append(item) ? item : 0;
}

// `new CMenuItem2()` folded from the split RezAlloc(0x74)+placement-new (the derived
// ctor runs the base ctor -> derived vptr stamp -> seeds +0x5c..+0x70), Init it
// (vtable +0x4), then on success run its slot-14 setter (SetFrame) and append (else
// delete). The fold recovers the /GX EH frame + inlined base ctor. 100%: the
// ex "EH trylevel wall" was the 5-arg mis-signature (retail ret 0x18 = 6 args;
// the Init arg list was reversed and SetFrame took a0 instead of a5).
RVA(0x001836f0, 0x160)
CMenuItem2* CMenuPage::AddItem2(
    const char* name,
    const char* spriteKey,
    i32 cmdId,
    const char* key,
    i32 flags,
    i32 frame
) {
    CMenuItem2* item = new CMenuItem2();
    if (item->Init(this, name, spriteKey, cmdId, key, flags) == 0) {
        if (item) {
            delete item;
        }
        return 0;
    }
    item->SetFrame(frame);
    return Append(item) ? item : 0;
}

// like AddItem2, but the new item links its parent context (item+0x30/m_1c) on
// success. `new CMenuItem2()` folded from the split RezAlloc+placement-new (49%->58%).
// @early-stop
RVA(0x00183850, 0x13b)
CMenuItem2* CMenuPage::AddSubItem2(
    const char* name,
    const char* spriteKey,
    i32 cmdId,
    i32 cmdParam,
    i32 parentCtx,
    const char* key,
    i32 flags,
    i32 frame
) {
    CMenuItem2* item = new CMenuItem2();
    if (item->Init(this, name, spriteKey, cmdId, key, flags) == 0) {
        if (item) {
            delete item;
        }
        return 0;
    }
    item->SetFrame(frame);
    item->m_cmdParam = cmdParam;
    item->m_1c = parentCtx;
    return Append(item) ? item : 0;
}

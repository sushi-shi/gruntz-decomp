#include <rva.h>

#include <Gruntz/MenuPage.h>

#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawWorker.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <Enums.h>
#include <Gruntz/ChatBox.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/MenuItemState.h>
#include <Gruntz/Sprite.h>
#include <Image/CImage.h>

#include <new>
#include <stddef.h>

RVA(0x001832d0, 0x20)
CString CMenuPage::GetKey() {
    return m_key;
}

static inline CDDrawWorker* LookupWorker(CMapStringToOb& map, LPCTSTR name) {
    CObject* found = NULL;
    map.Lookup(name, found);
    return static_cast<CDDrawWorker*>(found);
}

#define RESOLVE_MENU_SUB_PAGE(key, found)                                                          \
    CDDrawWorker* found = LookupWorker(m_owner->m_imageRegistry->m_workersByName, key);            \
    m_subPage = found;                                                                             \
    return found != NULL

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
    m_rowSpacing = host->m_rowSpacing;
    m_headGap = host->m_headGap;
    m_flags = flags;
    m_rect = host->m_rect8;
    m_offsetX = 0;
    m_offsetY = 0;
    RESOLVE_MENU_SUB_PAGE(key, slot_ob);
}

RVA(0x001833a0, 0x1a)
void CMenuPage::InitDefaults() {
    Clear();
    m_owner = NULL;
    m_host = NULL;
    m_subPage = NULL;
    m_focus = NULL;
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

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x001833f0, 0x38)
i32 CMenuPage::ResolveSubPage(const char* key) {
    RESOLVE_MENU_SUB_PAGE(key, found);
}

RVA(0x00183430, 0x24)
i32 CMenuPage::Append(CMenuItem* item) {
    if (!item) {
        return 0;
    }
    item->m_listPos = m_items.AddTail(item);
    return 1;
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

    if (item->Init(this, label, spriteKey, cmdId, key, flags) == 0) {
        if (item) {
            delete item;
        }
        return NULL;
    }
    return Append(item) ? item : NULL;
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
        return NULL;
    }
    item->SetCommandParam(cmdParam);
    item->SetSecondaryCommandId(tag);
    return Append(item) ? item : NULL;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
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
        return NULL;
    }
    item->SetFrame(frame);
    return Append(item) ? item : NULL;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
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
        return NULL;
    }
    item->SetFrame(frame);
    item->SetCommandParam(cmdParam);
    item->SetSecondaryCommandId(parentCtx);
    return Append(item) ? item : NULL;
}
RVA(0x00183990, 0x38)
i32 CMenuPage::ReleaseAll() {
    if (m_focus) {
        m_focus->Release();
        m_focus = NULL;
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

RVA(0x001839d0, 0xff)
i32 CMenuPage::RestoreFocus() {
    if (!m_focusName.IsEmpty()) {
        POSITION node = m_items.GetHeadPosition();
        while (node) {
            CMenuItem* item = NextItem(node);
            if (item) {
                bool match = item->GetName() == m_focusName;
                if (match) {
                    MenuItemState k = item->m_state;
                    if (k == MENUSTATE_NORMAL || k == MENUSTATE_SELECTED) {
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
            MenuItemState k = item->m_state;
            if (k == MENUSTATE_NORMAL || k == MENUSTATE_SELECTED) {
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
    MenuItemState kind = item->m_state;
    if (kind == MENUSTATE_SELECTED) {
        return 1;
    }
    if (kind != MENUSTATE_NORMAL) {
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

RVA(0x00183b60, 0xe8)
i32 CMenuPage::Layout(CDDrawSurfacePair* target) {
    if (m_flags & 4) {
        return LayoutOne(target);
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
            if (item->m_state == MENUSTATE_SELECTED && !(m_flags & 8)) {
                m_host->Draw(target, item, x, y);
            }
            y += item->GetWidth() / 2;
            y += m_rowSpacing;
        }
    }
    return 1;
}

RVA(0x00183c50, 0xbc)
i32 CMenuPage::FocusNext() {
    if (!m_focus) {
        return 0;
    }
    POSITION pos = m_focus->m_listPos;
    if (!pos) {
        return 0;
    }
    CMenuItem* found = NULL;
    POSITION node = pos;

    PrevItem(node);
    while (node) {
        found = PrevItem(node);
        if (found) {
            MenuItemState k = found->m_state;
            if (k == MENUSTATE_NORMAL || k == MENUSTATE_SELECTED) {

                node = NULL;
                continue;
            }
        }
        found = NULL;
    }
    if (!found) {

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
                    MenuItemState k = it->m_state;
                    if (k == MENUSTATE_NORMAL || k == MENUSTATE_SELECTED) {
                        found = it;
                    }
                }
            }
        }
        if (!found) {
            return 0;
        }
    }
    MenuItemState kind = found->m_state;
    if (kind != MENUSTATE_NORMAL && kind != MENUSTATE_SELECTED) {
        return 0;
    }
    if (found == m_focus) {
        return 0;
    }
    return SetFocus(found, 1) != 0;
}

RVA(0x00183d10, 0xbc)
i32 CMenuPage::FocusPrev() {
    if (!m_focus) {
        return 0;
    }
    POSITION pos = m_focus->m_listPos;
    if (!pos) {
        return 0;
    }
    CMenuItem* found = NULL;
    POSITION node = pos;

    NextItem(node);
    while (node) {
        found = NextItem(node);
        if (found) {
            MenuItemState k = found->m_state;
            if (k == MENUSTATE_NORMAL || k == MENUSTATE_SELECTED) {

                node = NULL;
                continue;
            }
        }
        found = NULL;
    }
    if (!found) {

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
                    MenuItemState k = it->m_state;
                    if (k == MENUSTATE_NORMAL || k == MENUSTATE_SELECTED) {
                        found = it;
                    }
                }
            }
        }
        if (!found) {
            return 0;
        }
    }
    MenuItemState kind = found->m_state;
    if (kind != MENUSTATE_NORMAL && kind != MENUSTATE_SELECTED) {
        return 0;
    }
    if (found == m_focus) {
        return 0;
    }
    return SetFocus(found, 1) != 0;
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
        m_host->PlayActivationSound();
    }
    return 1;
}

RVA(0x00183e30, 0x1f)
i32 CMenuPage::CanWrap() {
    i32 f = m_flags;
    if (f & 2) {
        return 0;
    }
    if (f & 1) {
        return 1;
    }
    i32 w = static_cast<char>(m_host->m_wrapFlag);
    if (w & 1) {
        return 1;
    }
    return 0;
}

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
            if (item->m_state == MENUSTATE_SELECTED && !(m_flags & 8)) {
                m_host->Draw(target, item, col, y);
            }
            y += item->GetWidth() / 2;
            y += m_rowSpacing;
        }
        if (++row < m_rowsPerCol) {

        } else {
            col += m_colWidth;
            y = ytop;
            row = 0;
        }
    }
    return 1;
}

RVA(0x00183f70, 0x74)
i32 CMenuPage::MoveFocusRightColumn() {
    CMenuItem* cur = m_focus;
    if (!cur) {
        return 0;
    }
    if (!(m_flags & 4)) {
        return 0;
    }

    POSITION pos = cur->m_listPos;
    if (!pos) {
        return 0;
    }
    i32 n = m_rowsPerCol;
    CMenuItem* found = NULL;
    if (n >= 0) {
        n++;
        do {
            if (pos != NULL) {
                found = static_cast<CMenuItem*>(m_items.GetNext(pos));
            } else {
                found = NULL;
            }
        } while (--n);
    }
    if (!found) {
        return 0;
    }
    MenuItemState k = found->m_state;
    if (k != MENUSTATE_NORMAL && k != MENUSTATE_SELECTED) {
        return 0;
    }
    if (found == cur) {
        return 0;
    }
    return SetFocus(found, 1) != 0;
}

RVA(0x00183ff0, 0x75)
i32 CMenuPage::MoveFocusLeftColumn() {
    CMenuItem* cur = m_focus;
    if (!cur) {
        return 0;
    }
    if (!(m_flags & 4)) {
        return 0;
    }

    POSITION pos = cur->m_listPos;
    if (!pos) {
        return 0;
    }
    i32 n = m_rowsPerCol;
    CMenuItem* found = NULL;
    if (n >= 0) {
        n++;
        do {
            if (pos != NULL) {
                found = static_cast<CMenuItem*>(m_items.GetPrev(pos));
            } else {
                found = NULL;
            }
        } while (--n);
    }
    if (!found) {
        return 0;
    }
    MenuItemState k = found->m_state;
    if (k != MENUSTATE_NORMAL && k != MENUSTATE_SELECTED) {
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
    return NULL;
}

RVA(0x00184150, 0xe0)
CMenuItem* CMenuPage::FindByName(const char* s) {
    if (!s) {
        return NULL;
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
    return NULL;
}

RVA(0x00184230, 0xd2)
i32 CMenuPage::MoveFocusLeft() {
    if (!m_focus) {
        return 0;
    }
    CMenuItem* item = FindByName(m_focus->GetLeftName());
    if (item) {
        MenuItemState k = item->m_state;
        if (k != MENUSTATE_NORMAL && k != MENUSTATE_SELECTED) {
            return 0;
        }
        if (item == m_focus) {
            return 0;
        }
        return SetFocus(item, 1);
    }
    return MoveFocusLeftColumn();
}

RVA(0x00184310, 0xd2)
i32 CMenuPage::MoveFocusRight() {
    if (!m_focus) {
        return 0;
    }
    CMenuItem* item = FindByName(m_focus->GetRightName());
    if (item) {
        MenuItemState k = item->m_state;
        if (k != MENUSTATE_NORMAL && k != MENUSTATE_SELECTED) {
            return 0;
        }
        if (item == m_focus) {
            return 0;
        }
        return SetFocus(item, 1);
    }
    return MoveFocusRightColumn();
}

RVA(0x001843f0, 0xd2)
i32 CMenuPage::MoveFocusUp() {
    if (!m_focus) {
        return 0;
    }
    CMenuItem* item = FindByName(m_focus->GetUpName());
    if (item) {
        MenuItemState k = item->m_state;
        if (k != MENUSTATE_NORMAL && k != MENUSTATE_SELECTED) {
            return 0;
        }
        if (item == m_focus) {
            return 0;
        }
        return SetFocus(item, 1);
    }
    return FocusNext();
}

RVA(0x001844d0, 0xd2)
i32 CMenuPage::MoveFocusDown() {
    if (!m_focus) {
        return 0;
    }
    CMenuItem* item = FindByName(m_focus->GetDownName());
    if (item) {
        MenuItemState k = item->m_state;
        if (k != MENUSTATE_NORMAL && k != MENUSTATE_SELECTED) {
            return 0;
        }
        if (item == m_focus) {
            return 0;
        }
        return SetFocus(item, 1);
    }
    return FocusPrev();
}

// CMenuItem/CMenuItem2 header inlines this TU materializes: link.exe kept the
// tail group 0x1845b0-0x1848aa from menupage.obj (the first defining obj).
RVA_COMPGEN(0x00184670, 0x1e, ??_GCMenuItem@@UAEPAXI@Z)
RVA_COMPGEN(0x00184690, 0x91, ??1CMenuItem@@UAE@XZ)
RVA_COMPGEN(0x00184730, 0x41, ?Reset@CMenuItem@@UAEXXZ)
RVA_COMPGEN(0x001847c0, 0x1e, ??_GCMenuItem2@@UAEPAXI@Z)
RVA_COMPGEN(0x001847e0, 0xa6, ??1CMenuItem2@@UAE@XZ)

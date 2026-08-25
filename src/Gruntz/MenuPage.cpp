#include <rva.h>

#include <Gruntz/MenuPage.h>

#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawWorker.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <Enums.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/MenuItemState.h>
#include <Gruntz/MenuTree.h>
#include <Gruntz/Sprite.h>
#include <Image/CImage.h>

#include <new>
#include <stddef.h>

RVA(0x001832d0, 0x20)
CString CMenuPage::GetPageKey() {
    return m_pageKey;
}

static inline CDDrawWorker* LookupWorker(CMapStringToOb& map, LPCTSTR name) {
    CObject* foundObject = NULL;
    map.Lookup(name, foundObject);
    return static_cast<CDDrawWorker*>(foundObject);
}

#define RESOLVE_MENU_HEADER_ANIMATION(animationKey, animation)                                     \
    CDDrawWorker* animation =                                                                      \
        LookupWorker(m_world->m_imageRegistry->m_workersByName, animationKey);                     \
    m_headerAnimation = animation;                                                                 \
    return animation != NULL

RVA(0x001832f0, 0xa5)
i32 CMenuPage::Configure(
    CMenuTree* menuTree,
    const char* pageKey,
    const char* headerAnimationKey,
    const char* parentPageKey,
    GZ_ENUM_PARAM(MenuPageFlags, i32) flags
) {
    if (!menuTree) {
        return 0;
    }
    m_world = menuTree->m_world;
    m_menuTree = menuTree;
    m_pageKey = pageKey;
    m_parentPageKey = parentPageKey;
    m_rowSpacing = menuTree->m_rowSpacing;
    m_headerGap = menuTree->m_headerGap;
    m_flags = flags;
    m_bounds = menuTree->m_bounds;
    m_contentOffsetX = 0;
    m_contentOffsetY = 0;
    RESOLVE_MENU_HEADER_ANIMATION(headerAnimationKey, headerAnimation);
}

RVA(0x001833a0, 0x1a)
void CMenuPage::Reset() {
    ClearItems();
    m_world = NULL;
    m_menuTree = NULL;
    m_headerAnimation = NULL;
    m_focusedItem = NULL;
    m_flags = MENU_PAGE_FLAGS_NONE;
}

RVA(0x001833c0, 0x2b)
void CMenuPage::ClearItems() {
    POSITION position = m_items.GetHeadPosition();
    while (position) {
        CMenuItem* item = NextItem(position);
        if (item) {
            delete item;
        }
    }
    m_items.RemoveAll();
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x001833f0, 0x38)
i32 CMenuPage::ResolveHeaderAnimation(const char* animationKey) {
    RESOLVE_MENU_HEADER_ANIMATION(animationKey, headerAnimation);
}

RVA(0x00183430, 0x24)
i32 CMenuPage::AppendItem(CMenuItem* item) {
    if (!item) {
        return 0;
    }
    item->m_listPosition = m_items.AddTail(item);
    return 1;
}

RVA(0x00183460, 0x13d)
CMenuItem* CMenuPage::AddItem(
    const char* name,
    const char* animationKey,
    i32 commandId,
    const char* targetPageKey,
    GZ_ENUM_PARAM(MenuItemFlags, i32) flags
) {
    CMenuItem* item = new CMenuItem();

    if (item->Init(this, name, animationKey, commandId, targetPageKey, flags) == 0) {
        if (item) {
            delete item;
        }
        return NULL;
    }
    return AppendItem(item) ? item : NULL;
}

RVA(0x001835a0, 0x14b)
CMenuItem* CMenuPage::AddItem(
    const char* name,
    const char* animationKey,
    i32 commandId,
    i32 commandParam,
    i32 secondaryCommandId,
    const char* targetPageKey,
    GZ_ENUM_PARAM(MenuItemFlags, i32) flags
) {
    CMenuItem* item = new CMenuItem();
    if (item->Init(this, name, animationKey, commandId, targetPageKey, flags) == 0) {
        if (item) {
            delete item;
        }
        return NULL;
    }
    item->SetCommandParam(commandParam);
    item->SetSecondaryCommandId(secondaryCommandId);
    return AppendItem(item) ? item : NULL;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x001836f0, 0x160)
CAnimatedMenuItem* CMenuPage::AddAnimatedItem(
    const char* name,
    const char* animationKey,
    i32 commandId,
    const char* targetPageKey,
    GZ_ENUM_PARAM(MenuItemFlags, i32) flags,
    i32 framePeriodMs
) {
    CAnimatedMenuItem* item = new CAnimatedMenuItem();
    if (item->Init(this, name, animationKey, commandId, targetPageKey, flags) == 0) {
        if (item) {
            delete item;
        }
        return NULL;
    }
    item->SetFramePeriod(framePeriodMs);
    return AppendItem(item) ? item : NULL;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00183850, 0x13b)
CAnimatedMenuItem* CMenuPage::AddAnimatedItem(
    const char* name,
    const char* animationKey,
    i32 commandId,
    i32 commandParam,
    i32 secondaryCommandId,
    const char* targetPageKey,
    GZ_ENUM_PARAM(MenuItemFlags, i32) flags,
    i32 framePeriodMs
) {
    CAnimatedMenuItem* item = new CAnimatedMenuItem();
    if (item->Init(this, name, animationKey, commandId, targetPageKey, flags) == 0) {
        if (item) {
            delete item;
        }
        return NULL;
    }
    item->SetFramePeriod(framePeriodMs);
    item->SetCommandParam(commandParam);
    item->SetSecondaryCommandId(secondaryCommandId);
    return AppendItem(item) ? item : NULL;
}
RVA(0x00183990, 0x38)
i32 CMenuPage::PrepareForActivation() {
    if (m_focusedItem) {
        m_focusedItem->Deselect();
        m_focusedItem = NULL;
    }
    POSITION position = m_items.GetHeadPosition();
    while (position) {
        CMenuItem* item = NextItem(position);
        if (item) {
            item->OnPageActivated();
        }
    }
    return 1;
}

RVA(0x001839d0, 0xff)
i32 CMenuPage::FocusInitialItem() {
    if (!m_initialFocusItemName.IsEmpty()) {
        POSITION position = m_items.GetHeadPosition();
        while (position) {
            CMenuItem* item = NextItem(position);
            if (item) {
                bool matches = item->GetItemName() == m_initialFocusItemName;
                if (matches) {
                    MenuItemState state = item->m_state;
                    if (state == MENUSTATE_NORMAL || state == MENUSTATE_SELECTED) {
                        if (SetFocusedItem(item, 0)) {
                            return 1;
                        }
                    }
                }
            }
        }
    }
    POSITION position = m_items.GetHeadPosition();
    while (position) {
        CMenuItem* item = NextItem(position);
        if (item) {
            MenuItemState state = item->m_state;
            if (state == MENUSTATE_NORMAL || state == MENUSTATE_SELECTED) {
                if (SetFocusedItem(item, 0)) {
                    return 1;
                }
            }
        }
    }
    return 0;
}

RVA(0x00183ad0, 0x57)
i32 CMenuPage::SetFocusedItem(CMenuItem* item, i32 playFocusSound) {
    if (!item) {
        return 0;
    }
    MenuItemState state = item->m_state;
    if (state == MENUSTATE_SELECTED) {
        return 1;
    }
    if (state != MENUSTATE_NORMAL) {
        return 0;
    }
    if (m_focusedItem) {
        m_focusedItem->Deselect();
    }
    m_focusedItem = item;
    return item->Select(playFocusSound) != 0;
}

RVA(0x00183b30, 0x2c)
i32 CMenuPage::UpdateItems(u32 deltaMs) {
    POSITION position = m_items.GetHeadPosition();
    while (position) {
        CMenuItem* item = NextItem(position);
        if (item) {
            item->Update(deltaMs);
        }
    }
    return 1;
}

RVA(0x00183b60, 0xe8)
i32 CMenuPage::Draw(CDDrawSurfacePair* target) {
    if (HAS(m_flags, MENU_PAGE_MULTI_COLUMN)) {
        return DrawMultiColumn(target);
    }
    i32 left = m_bounds.left;
    i32 right = m_bounds.right;
    i32 centerX = (((right - left + 1) / 2)) + m_contentOffsetX + left;
    i32 drawY = m_contentOffsetY + m_bounds.top;
    CDDrawWorker* headerAnimation = m_headerAnimation;
    if (headerAnimation) {
        CImage* headerFrame =
            static_cast<CImage*>(headerAnimation->m_items.GetAt(headerAnimation->m_minIndex));
        if (headerFrame) {
            drawY += headerFrame->m_anchorY;
            headerFrame->RenderFrame(target, centerX, drawY, 0);
            drawY += m_headerGap + headerFrame->m_anchorY;
        }
    }
    POSITION position = m_items.GetHeadPosition();
    while (position) {
        CMenuItem* item = NextItem(position);
        if (item) {
            drawY += item->GetFrameHeight() / 2;
            item->DrawAt(target, centerX, drawY);
            if (item->m_state == MENUSTATE_SELECTED
                && !HAS(m_flags, MENU_PAGE_HIDE_FOCUS_CURSORS)) {
                m_menuTree->DrawFocusCursors(target, item, centerX, drawY);
            }
            drawY += item->GetFrameHeight() / 2;
            drawY += m_rowSpacing;
        }
    }
    return 1;
}

RVA(0x00183c50, 0xbc)
i32 CMenuPage::MoveFocusUpSequential() {
    if (!m_focusedItem) {
        return 0;
    }
    POSITION currentPosition = m_focusedItem->m_listPosition;
    if (!currentPosition) {
        return 0;
    }
    CMenuItem* candidateItem = NULL;
    POSITION scanPosition = currentPosition;

    PrevItem(scanPosition);
    while (scanPosition) {
        candidateItem = PrevItem(scanPosition);
        if (candidateItem) {
            MenuItemState state = candidateItem->m_state;
            if (state == MENUSTATE_NORMAL || state == MENUSTATE_SELECTED) {

                scanPosition = NULL;
                continue;
            }
        }
        candidateItem = NULL;
    }
    if (!candidateItem) {

        if (CanWrap()) {
            POSITION wrapStartPosition = m_focusedItem->m_listPosition;
            if (!wrapStartPosition) {
                return 0;
            }
            POSITION wrapPosition = wrapStartPosition;

            NextItem(wrapPosition);
            while (wrapPosition) {
                CMenuItem* wrapCandidate = NextItem(wrapPosition);
                if (wrapCandidate) {
                    MenuItemState state = wrapCandidate->m_state;
                    if (state == MENUSTATE_NORMAL || state == MENUSTATE_SELECTED) {
                        candidateItem = wrapCandidate;
                    }
                }
            }
        }
        if (!candidateItem) {
            return 0;
        }
    }
    MenuItemState state = candidateItem->m_state;
    if (state != MENUSTATE_NORMAL && state != MENUSTATE_SELECTED) {
        return 0;
    }
    if (candidateItem == m_focusedItem) {
        return 0;
    }
    return SetFocusedItem(candidateItem, 1) != 0;
}

RVA(0x00183d10, 0xbc)
i32 CMenuPage::MoveFocusDownSequential() {
    if (!m_focusedItem) {
        return 0;
    }
    POSITION currentPosition = m_focusedItem->m_listPosition;
    if (!currentPosition) {
        return 0;
    }
    CMenuItem* candidateItem = NULL;
    POSITION scanPosition = currentPosition;

    NextItem(scanPosition);
    while (scanPosition) {
        candidateItem = NextItem(scanPosition);
        if (candidateItem) {
            MenuItemState state = candidateItem->m_state;
            if (state == MENUSTATE_NORMAL || state == MENUSTATE_SELECTED) {

                scanPosition = NULL;
                continue;
            }
        }
        candidateItem = NULL;
    }
    if (!candidateItem) {

        if (CanWrap()) {
            POSITION wrapStartPosition = m_focusedItem->m_listPosition;
            if (!wrapStartPosition) {
                return 0;
            }
            POSITION wrapPosition = wrapStartPosition;

            PrevItem(wrapPosition);
            while (wrapPosition) {
                CMenuItem* wrapCandidate = PrevItem(wrapPosition);
                if (wrapCandidate) {
                    MenuItemState state = wrapCandidate->m_state;
                    if (state == MENUSTATE_NORMAL || state == MENUSTATE_SELECTED) {
                        candidateItem = wrapCandidate;
                    }
                }
            }
        }
        if (!candidateItem) {
            return 0;
        }
    }
    MenuItemState state = candidateItem->m_state;
    if (state != MENUSTATE_NORMAL && state != MENUSTATE_SELECTED) {
        return 0;
    }
    if (candidateItem == m_focusedItem) {
        return 0;
    }
    return SetFocusedItem(candidateItem, 1) != 0;
}

RVA(0x00183dd0, 0x16)
i32 CMenuPage::ActivateFocusedItem() {
    if (!m_focusedItem) {
        return 0;
    }
    return m_focusedItem->Activate() != 0;
}

RVA(0x00183df0, 0x3d)
i32 CMenuPage::ReturnToParentPage(i32 playActivationSound) {
    if (m_parentPageKey.GetLength() == 0) {
        return 0;
    }
    if (!m_menuTree->SetActivePageByKey(m_parentPageKey)) {
        return 0;
    }
    if (playActivationSound) {
        m_menuTree->PlayActivationSound();
    }
    return 1;
}

RVA(0x00183e30, 0x1f)
i32 CMenuPage::CanWrap() {
    MenuPageFlags pageFlags = m_flags;
    if (HAS(pageFlags, MENU_PAGE_DISABLE_WRAP)) {
        return 0;
    }
    if (HAS(pageFlags, MENU_PAGE_FORCE_WRAP)) {
        return 1;
    }
    i32 treeWrapFlags = static_cast<char>(m_menuTree->m_wrapFlags);
    if (treeWrapFlags & 1) {
        return 1;
    }
    return 0;
}

RVA(0x00183e50, 0x11c)
i32 CMenuPage::DrawMultiColumn(CDDrawSurfacePair* target) {
    i32 left = m_bounds.left;
    i32 right = m_bounds.right;
    i32 centerX = (((right - left + 1) / 2)) + m_contentOffsetX + left;
    i32 drawY = m_contentOffsetY + m_bounds.top;
    CDDrawWorker* headerAnimation = m_headerAnimation;
    if (headerAnimation) {
        CImage* headerFrame =
            static_cast<CImage*>(headerAnimation->m_items.GetAt(headerAnimation->m_minIndex));
        if (headerFrame) {
            drawY += headerFrame->m_anchorY;
            headerFrame->RenderFrame(target, centerX, drawY, 0);
            drawY += m_headerGap + headerFrame->m_anchorY;
        }
    }
    i32 columnX = ((m_columnWidth / 2)) + m_bounds.left + m_columnOffsetX;
    i32 firstRowY = drawY;
    i32 rowInColumn = 0;
    POSITION position = m_items.GetHeadPosition();
    while (position) {
        CMenuItem* item = NextItem(position);
        if (item) {
            drawY += item->GetFrameHeight() / 2;
            item->DrawAt(target, columnX, drawY);
            if (item->m_state == MENUSTATE_SELECTED
                && !HAS(m_flags, MENU_PAGE_HIDE_FOCUS_CURSORS)) {
                m_menuTree->DrawFocusCursors(target, item, columnX, drawY);
            }
            drawY += item->GetFrameHeight() / 2;
            drawY += m_rowSpacing;
        }
        if (++rowInColumn < m_rowsPerColumn) {

        } else {
            columnX += m_columnWidth;
            drawY = firstRowY;
            rowInColumn = 0;
        }
    }
    return 1;
}

RVA(0x00183f70, 0x74)
i32 CMenuPage::MoveFocusRightColumn() {
    CMenuItem* currentItem = m_focusedItem;
    if (!currentItem) {
        return 0;
    }
    if (!HAS(m_flags, MENU_PAGE_MULTI_COLUMN)) {
        return 0;
    }

    POSITION position = currentItem->m_listPosition;
    if (!position) {
        return 0;
    }
    i32 stepsRemaining = m_rowsPerColumn;
    CMenuItem* candidateItem = NULL;
    if (stepsRemaining >= 0) {
        stepsRemaining++;
        do {
            if (position != NULL) {
                candidateItem = static_cast<CMenuItem*>(m_items.GetNext(position));
            } else {
                candidateItem = NULL;
            }
        } while (--stepsRemaining);
    }
    if (!candidateItem) {
        return 0;
    }
    MenuItemState state = candidateItem->m_state;
    if (state != MENUSTATE_NORMAL && state != MENUSTATE_SELECTED) {
        return 0;
    }
    if (candidateItem == currentItem) {
        return 0;
    }
    return SetFocusedItem(candidateItem, 1) != 0;
}

RVA(0x00183ff0, 0x75)
i32 CMenuPage::MoveFocusLeftColumn() {
    CMenuItem* currentItem = m_focusedItem;
    if (!currentItem) {
        return 0;
    }
    if (!HAS(m_flags, MENU_PAGE_MULTI_COLUMN)) {
        return 0;
    }

    POSITION position = currentItem->m_listPosition;
    if (!position) {
        return 0;
    }
    i32 stepsRemaining = m_rowsPerColumn;
    CMenuItem* candidateItem = NULL;
    if (stepsRemaining >= 0) {
        stepsRemaining++;
        do {
            if (position != NULL) {
                candidateItem = static_cast<CMenuItem*>(m_items.GetPrev(position));
            } else {
                candidateItem = NULL;
            }
        } while (--stepsRemaining);
    }
    if (!candidateItem) {
        return 0;
    }
    MenuItemState state = candidateItem->m_state;
    if (state != MENUSTATE_NORMAL && state != MENUSTATE_SELECTED) {
        return 0;
    }
    if (candidateItem == currentItem) {
        return 0;
    }
    return SetFocusedItem(candidateItem, 1) != 0;
}

RVA(0x00184070, 0x30)
i32 CMenuPage::FocusItemAt(i32 screenX, i32 screenY) {
    CMenuItem* hitItem = HitTest(screenX, screenY);
    if (!hitItem) {
        return 0;
    }
    return SetFocusedItem(hitItem, 1) != 0;
}

RVA(0x001840a0, 0x57)
i32 CMenuPage::ClickAt(i32 screenX, i32 screenY) {
    CMenuItem* hitItem = HitTest(screenX, screenY);
    if (!hitItem) {
        return 0;
    }
    if (!SetFocusedItem(hitItem, 0)) {
        return 0;
    }
    if (!ActivateFocusedItem()) {
        return 0;
    }
    FocusItemAt(screenX, screenY);
    return 1;
}

RVA(0x00184100, 0x4a)
CMenuItem* CMenuPage::HitTest(i32 screenX, i32 screenY) {
    POSITION position = m_items.GetHeadPosition();
    while (position) {
        CMenuItem* item = NextItem(position);
        if (item) {
            if (item->HitTest(screenX, screenY)) {
                return item;
            }
        }
    }
    return NULL;
}

RVA(0x00184150, 0xe0)
CMenuItem* CMenuPage::FindItemByName(const char* name) {
    if (!name) {
        return NULL;
    }
    CString requestedName(name);
    POSITION position = m_items.GetHeadPosition();
    while (position) {
        CMenuItem* item = NextItem(position);
        if (item) {
            bool matches = strcmp(requestedName, item->GetItemName()) == 0;
            if (matches) {
                return item;
            }
        }
    }
    return NULL;
}

RVA(0x00184230, 0xd2)
i32 CMenuPage::MoveFocusLeft() {
    if (!m_focusedItem) {
        return 0;
    }
    CMenuItem* item = FindItemByName(m_focusedItem->GetLeftItemName());
    if (item) {
        MenuItemState state = item->m_state;
        if (state != MENUSTATE_NORMAL && state != MENUSTATE_SELECTED) {
            return 0;
        }
        if (item == m_focusedItem) {
            return 0;
        }
        return SetFocusedItem(item, 1);
    }
    return MoveFocusLeftColumn();
}

RVA(0x00184310, 0xd2)
i32 CMenuPage::MoveFocusRight() {
    if (!m_focusedItem) {
        return 0;
    }
    CMenuItem* item = FindItemByName(m_focusedItem->GetRightItemName());
    if (item) {
        MenuItemState state = item->m_state;
        if (state != MENUSTATE_NORMAL && state != MENUSTATE_SELECTED) {
            return 0;
        }
        if (item == m_focusedItem) {
            return 0;
        }
        return SetFocusedItem(item, 1);
    }
    return MoveFocusRightColumn();
}

RVA(0x001843f0, 0xd2)
i32 CMenuPage::MoveFocusUp() {
    if (!m_focusedItem) {
        return 0;
    }
    CMenuItem* item = FindItemByName(m_focusedItem->GetUpItemName());
    if (item) {
        MenuItemState state = item->m_state;
        if (state != MENUSTATE_NORMAL && state != MENUSTATE_SELECTED) {
            return 0;
        }
        if (item == m_focusedItem) {
            return 0;
        }
        return SetFocusedItem(item, 1);
    }
    return MoveFocusUpSequential();
}

RVA(0x001844d0, 0xd2)
i32 CMenuPage::MoveFocusDown() {
    if (!m_focusedItem) {
        return 0;
    }
    CMenuItem* item = FindItemByName(m_focusedItem->GetDownItemName());
    if (item) {
        MenuItemState state = item->m_state;
        if (state != MENUSTATE_NORMAL && state != MENUSTATE_SELECTED) {
            return 0;
        }
        if (item == m_focusedItem) {
            return 0;
        }
        return SetFocusedItem(item, 1);
    }
    return MoveFocusDownSequential();
}

// CMenuItem/CAnimatedMenuItem header inlines this TU materializes: link.exe kept the
// tail group 0x1845b0-0x1848aa from menupage.obj (the first defining obj).
RVA_COMPGEN(0x00184670, 0x1e, ??_GCMenuItem@@UAEPAXI@Z)
RVA_COMPGEN(0x00184690, 0x91, ??1CMenuItem@@UAE@XZ)
RVA_COMPGEN(0x00184730, 0x41, ?Reset@CMenuItem@@UAEXXZ)
RVA_COMPGEN(0x001847c0, 0x1e, ??_GCAnimatedMenuItem@@UAEPAXI@Z)
RVA_COMPGEN(0x001847e0, 0xa6, ??1CAnimatedMenuItem@@UAE@XZ)

#ifndef GRUNTZ_MENUPAGE_H
#define GRUNTZ_MENUPAGE_H

#include <list>
class CMenuItem;

#include <string>

#include <Ints.h>

#include <Enums.h>
#include <Gruntz/AnimatedMenuItem.h>
#include <Gruntz/MenuItem.h>
#include <Gruntz/MenuItemState.h>
#include <Ints.h>

class CDDrawSurfaceMgr;
class CDDrawSurfacePair;
class CDDrawWorker;
class CMenuTree;
class CAnimatedMenuItem;

GZ_ENUM_FLAGS_BEGIN(MenuPageFlags, i32)
    MENU_PAGE_FLAGS_NONE = 0,
    MENU_PAGE_FORCE_WRAP = 0x1,
    MENU_PAGE_DISABLE_WRAP = 0x2,
    MENU_PAGE_MULTI_COLUMN = 0x4,
    MENU_PAGE_HIDE_FOCUS_CURSORS = 0x8
GZ_ENUM_FLAGS_END(MenuPageFlags, i32)
GZ_ENUM_FLAGS_OPS(MenuPageFlags)

class CMenuPage {
public:
    CMenuPage() {
        m_world = NULL;
        m_menuTree = NULL;
        m_headerAnimation = NULL;
        m_focusedItem = NULL;
        m_flags = MENU_PAGE_FLAGS_NONE;
    }

    ~CMenuPage() {
        Reset();
    }
    std::string GetPageKey();

    i32 Configure(
        CMenuTree* menuTree,
        const std::string& pageKey,
        const std::string& headerAnimationKey,
        const std::string& parentPageKey,
        GZ_ENUM_PARAM(MenuPageFlags, i32) flags
    );
    void Reset();
    void ClearItems();
    i32 ResolveHeaderAnimation(const std::string& animationKey);
    i32 AppendItem(CMenuItem* item);

    CMenuItem* AddItem(
        const std::string& name,
        const std::string& animationKey,
        i32 commandId,
        const std::string& targetPageKey,
        GZ_ENUM_PARAM(MenuItemFlags, i32) flags
    );

    CMenuItem* AddItem(
        const std::string& name,
        const std::string& animationKey,
        i32 commandId,
        i32 commandParam,
        i32 secondaryCommandId,
        const std::string& targetPageKey,
        GZ_ENUM_PARAM(MenuItemFlags, i32) flags
    );
    i32 PrepareForActivation();
    i32 FocusInitialItem();
    i32 SetFocusedItem(CMenuItem* item, i32 playFocusSound);
    i32 UpdateItems(u32 deltaMs);
    i32 Draw(CDDrawSurfacePair* target);
    i32 MoveFocusUpSequential();
    i32 MoveFocusDownSequential();
    i32 ActivateFocusedItem();
    i32 FocusItemAt(i32 screenX, i32 screenY);
    i32 ClickAt(i32 screenX, i32 screenY);
    CMenuItem* HitTest(i32 screenX, i32 screenY);
    CMenuItem* FindItemByName(const std::string& name);
    i32 MoveFocusUp();
    i32 MoveFocusDown();
    i32 DrawMultiColumn(CDDrawSurfacePair* target);

    CAnimatedMenuItem* AddAnimatedItem(
        const std::string& name,
        const std::string& animationKey,
        i32 commandId,
        const std::string& targetPageKey,
        GZ_ENUM_PARAM(MenuItemFlags, i32) flags,
        i32 framePeriodMs
    );

    CAnimatedMenuItem* AddAnimatedItem(
        const std::string& name,
        const std::string& animationKey,
        i32 commandId,
        i32 commandParam,
        i32 secondaryCommandId,
        const std::string& targetPageKey,
        GZ_ENUM_PARAM(MenuItemFlags, i32) flags,
        i32 framePeriodMs
    );
    i32 ReturnToParentPage(i32 playActivationSound);
    i32 CanWrap();
    i32 MoveFocusRightColumn();
    i32 MoveFocusLeftColumn();
    i32 MoveFocusLeft();
    i32 MoveFocusRight();

    CDDrawSurfaceMgr* m_world;
    CMenuTree* m_menuTree;
    std::string m_parentPageKey;
    std::string m_pageKey;
    std::string m_initialFocusItemName;
    std::list<CMenuItem*> m_items;

    CMenuItem* NextItem(std::list<CMenuItem*>::iterator& position) {
        return static_cast<CMenuItem*>(*(position++));
    }
    CMenuItem* PrevItem(std::list<CMenuItem*>::iterator& position) {
        CMenuItem* item = *position;
        position = position == m_items.begin() ? m_items.end() : --position;
        return item;
    }
    MenuPageFlags m_flags;
    RECT m_bounds;

    i32 m_headerGap;
    i32 m_rowSpacing;
    i32 m_columnWidth;
    i32 m_rowsPerColumn;
    i32 m_columnOffsetX;
    i32 m_contentOffsetX;
    i32 m_contentOffsetY;
    CDDrawWorker* m_headerAnimation;

    CMenuItem* m_focusedItem;
};

#define RESOLVE_MENU_HEADER_ANIMATION(animationKey, animation)                                         CDDrawWorker* animation = m_world->FindWorker(animationKey);                                       m_headerAnimation = animation;                                                                     return animation != NULL

inline b32 CMenuItem::IsSelectable() {
    return m_state == MENUSTATE_NORMAL || m_state == MENUSTATE_SELECTED;
}

#endif

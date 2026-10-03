#ifndef GRUNTZ_MENUITEM_H
#define GRUNTZ_MENUITEM_H

#include <list>
class CMenuItem;

#include <string>

#include <Ints.h>

#include <Enums.h>
#include <Gruntz/CoordNode.h>
#include <Ints.h>
#include <Wap32/CoordUnset.h>

GZ_ENUM_FORWARD(MenuItemState);

GZ_ENUM_FLAGS_BEGIN(MenuItemFlags, i32)
    MENU_ITEM_FLAGS_NONE = 0,
    MENU_ITEM_INITIAL_DISABLED = 0x1,
    MENU_ITEM_HOLD_FINAL_ANIMATION_FRAME = 0x10000
GZ_ENUM_FLAGS_END(MenuItemFlags, i32)
GZ_ENUM_FLAGS_OPS(MenuItemFlags)

class CMenuPage;
class CMenuItem;
class CMenuTree;
class CDDrawSurfacePair;
class CDDrawWorker;

class CDDrawSurfaceMgr;

class CMenuItem {
public:
    CMenuItem();
    virtual ~CMenuItem();

    virtual i32 Init(
        CMenuPage* page,
        const char* name,
        const char* animationKey,
        i32 commandId,
        const char* targetPageKey,
        GZ_ENUM_PARAM(MenuItemFlags, i32) flags
    );

    virtual void Cleanup();
    virtual void Reset();
    virtual i32 GetFrameHeight();
    virtual i32 GetFrameWidth();
    inline b32 IsSelectable();

    virtual void SetState(MenuItemState state) {
        m_state = state;
    }

    virtual i32 OnPageActivated();

    virtual i32 Update(u32 deltaMs);
    virtual i32 DrawAt(CDDrawSurfacePair* target, i32 centerX, i32 centerY);

    virtual i32 Select(i32 playFocusSound);
    virtual i32 Deselect();
    virtual i32 Activate();

    virtual i32 UsesStateAnimations() {
        return 0;
    }

    std::string GetItemName() {
        return m_itemName;
    }

    std::string GetLeftItemName() {
        return m_leftItemName;
    }

    std::string GetRightItemName() {
        return m_rightItemName;
    }

    std::string GetUpItemName() {
        return m_upItemName;
    }

    std::string GetDownItemName() {
        return m_downItemName;
    }
    i32 PostCommands();
    i32 HitTest(i32 screenX, i32 screenY);
    void SetCommandParam(i32 commandParam) {
        m_commandParam = commandParam;
    }
    void SetSecondaryCommandId(i32 secondaryCommandId) {
        m_secondaryCommandId = secondaryCommandId;
    }

    CDDrawSurfaceMgr* m_world;

    CMenuTree* m_menuTree;
    CMenuPage* m_page;
    std::string m_itemName;
    std::string m_targetPageKey;
    i32 m_commandId;
    i32 m_secondaryCommandId;
    MenuItemFlags m_flags;
    MenuItemState m_state;
    CDDrawWorker* m_animation;


    i32 m_commandParam;
    i32 m_hitLeft;
    i32 m_hitTop;
    i32 m_hitRight;
    i32 m_hitBottom;
    Coord m_fixedCenter;
    std::string m_leftItemName;
    std::string m_rightItemName;
    std::string m_upItemName;
    std::string m_downItemName;
};

inline CMenuItem::~CMenuItem() {
    Cleanup();
}

inline void CMenuItem::Reset() {
    m_menuTree = NULL;
    m_page = NULL;
    m_animation = NULL;
    m_world = NULL;

    m_hitLeft = UNINIT_FILL;
    m_fixedCenter.m_x = UNINIT_FILL;
    (m_leftItemName).erase();
    (m_rightItemName).erase();
    (m_upItemName).erase();
    (m_downItemName).erase();
}

inline CMenuItem::CMenuItem() {
    Reset();
}

#endif

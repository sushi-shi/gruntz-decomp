#define GRUNTZ_MENUITEM_TU

#include <rva.h>

#include <Gruntz/MenuItem.h>

#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawWorker.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <Enums.h>
#include <Gruntz/AnimatedMenuItem.h>
#include <Gruntz/ChatBoxOwner.h>
#include <Gruntz/MenuItemState.h>
#include <Gruntz/MenuPage.h>
#include <Gruntz/MenuTree.h>
#include <Image/CImage.h>
#include <Wap32/CoordUnset.h>

#include <stdio.h>

static inline CDDrawWorker* LookupWorker(CMapStringToOb& map, LPCTSTR name) {
    CObject* foundObject = NULL;
    map.Lookup(name, foundObject);
    return static_cast<CDDrawWorker*>(foundObject);
}

RVA(0x00185740, 0xa9)
i32 CMenuItem::Init(
    CMenuPage* page,
    const char* name,
    const char* animationKey,
    i32 commandId,
    const char* targetPageKey,
    GZ_ENUM_PARAM(MenuItemFlags, i32) flags
) {
    if (!page) {
        return 0;
    }
    m_flags = flags;
    m_world = page->m_world;
    m_menuTree = page->m_menuTree;
    m_page = page;
    m_itemName = name;
    m_targetPageKey = targetPageKey;
    m_commandId = commandId;
    m_secondaryCommandId = 0;
    m_commandParam = 0;
    if (HAS(m_flags, MENU_ITEM_INITIAL_DISABLED)) {
        m_state = MENUSTATE_DISABLED;
    } else {
        m_state = MENUSTATE_NORMAL;
    }
    if (!UsesStateAnimations()) {
        CObject* animationObject = NULL;

        m_world->m_imageRegistry->m_workersByName.Lookup(animationKey, animationObject);
        m_animation = animationObject;
        if (!animationObject) {
            return 0;
        }
    }
    return 1;
}
RVA(0x001857f0, 0x5)
void CMenuItem::Cleanup() {
    Reset();
}

RVA(0x00185800, 0x2c)
i32 CMenuItem::GetFrameWidth() {
    CDDrawWorker* animation = static_cast<CDDrawWorker*>(m_animation);
    if (!animation) {
        return 0;
    }
    CImage* frame = animation->GetAt(2);
    if (!frame) {
        return 0;
    }
    return frame->m_width;
}
RVA(0x00185830, 0x2c)
i32 CMenuItem::GetFrameHeight() {
    CDDrawWorker* animation = static_cast<CDDrawWorker*>(m_animation);
    if (!animation) {
        return 0;
    }
    CImage* frame = animation->GetAt(2);
    if (!frame) {
        return 0;
    }
    return frame->m_height;
}
RVA(0x00185860, 0x4a)
i32 CMenuItem::PostCommands() {
    i32 commandId = m_commandId;
    if (!commandId) {
        return commandId;
    }
    HWND windowHandle = m_menuTree->m_windowHandle;
    if (windowHandle) {
        PostMessageA(windowHandle, WM_COMMAND, commandId, m_commandParam);
    }
    if (m_secondaryCommandId && windowHandle) {
        PostMessageA(windowHandle, WM_COMMAND, m_secondaryCommandId, 0);
    }
    return 1;
}

RVA(0x001858b0, 0x6)
i32 CMenuItem::OnPageActivated() {
    return 1;
}

RVA(0x001858c0, 0x8)
i32 CMenuItem::Update(u32) {
    return 1;
}

RVA(0x001858d0, 0x94)
i32 CMenuItem::DrawAt(CDDrawSurfacePair* target, i32 centerX, i32 centerY) {
    CDDrawWorker* animation = static_cast<CDDrawWorker*>(m_animation);
    if (!animation) {
        return 0;
    }

    if (m_fixedCenterX != UNINIT_FILL) {
        centerX = m_fixedCenterX;
        centerY = m_fixedCenterY;
    }
    MenuItemState state = m_state;
    CImage* frame = animation->GetAt(IDX(state));
    if (!frame) {
        return 0;
    }
    frame->RenderFrame(target, centerX, centerY, 0);
    m_hitLeft = centerX - frame->m_anchorX;
    m_hitRight = centerX + frame->m_anchorX;
    m_hitTop = centerY - frame->m_anchorY;
    m_hitBottom = centerY + frame->m_anchorY;
    return 1;
}
RVA(0x00185970, 0x25)
i32 CMenuItem::Select(i32 playFocusSound) {
    if (playFocusSound) {
        m_menuTree->PlayFocusSound();
    }
    SetState(MENUSTATE_SELECTED);
    return 1;
}
RVA(0x001859a0, 0xd)
i32 CMenuItem::Deselect() {
    SetState(MENUSTATE_NORMAL);
    return 1;
}

RVA(0x001859b0, 0x25)
i32 CMenuItem::Activate() {
    m_menuTree->PlayActivationSound();
    PostCommands();
    m_menuTree->SetActivePageByKey(m_targetPageKey);
    return 1;
}
RVA(0x001859e0, 0x4b)
i32 CMenuItem::HitTest(i32 screenX, i32 screenY) {
    if (m_hitLeft == UNINIT_FILL) {
        return 0;
    }
    if (screenX < m_hitLeft) {
        return 0;
    }
    if (screenX > m_hitRight) {
        return 0;
    }
    if (screenY < m_hitTop) {
        return 0;
    }
    return screenY <= m_hitBottom;
}

RVA(0x00185a30, 0x123)
i32 CAnimatedMenuItem::Init(
    CMenuPage* page,
    const char* name,
    const char* animationKey,
    i32 commandId,
    const char* targetPageKey,
    GZ_ENUM_PARAM(MenuItemFlags, i32) flags
) {
    if (!page) {
        return 0;
    }
    if (!CMenuItem::Init(page, name, animationKey, commandId, targetPageKey, flags)) {
        return 0;
    }
    m_frameIndex = 0;
    m_frameTimerMs = 0;
    SET_ANIMATED_MENU_ITEM_FRAME_PERIOD_INLINE(0x64);

    char animationName[0x80];

    sprintf(animationName, "%s_NORMAL", animationKey);
    m_normalAnimation = LookupWorker(m_world->m_imageRegistry->m_workersByName, animationName);

    sprintf(animationName, "%s_SELECTED", animationKey);
    m_selectedAnimation = LookupWorker(m_world->m_imageRegistry->m_workersByName, animationName);

    sprintf(animationName, "%s_DISABLED", animationKey);
    m_disabledAnimation = LookupWorker(m_world->m_imageRegistry->m_workersByName, animationName);

    return 1;
}
RVA(0x00185b60, 0xe)
i32 CAnimatedMenuItem::GetFrameWidth() {
    CImage* frame = GetCurrentFrame();
    if (!frame) {
        return 0;
    }
    return frame->m_width;
}

RVA(0x00185b70, 0xe)
i32 CAnimatedMenuItem::GetFrameHeight() {
    CImage* frame = GetCurrentFrame();
    if (!frame) {
        return 0;
    }
    return frame->m_height;
}

RVA(0x00185b80, 0x2b)
i32 CAnimatedMenuItem::Update(u32 deltaMs) {
    if (deltaMs >= static_cast<u32>(m_frameTimerMs)) {
        m_frameTimerMs = m_framePeriodMs;
        AdvanceFrame();
        return 1;
    }
    m_frameTimerMs = m_frameTimerMs - deltaMs;
    return 1;
}

RVA(0x00185bb0, 0x72)
i32 CAnimatedMenuItem::DrawAt(CDDrawSurfacePair* target, i32 centerX, i32 centerY) {

    if (m_fixedCenterX != UNINIT_FILL) {
        centerX = m_fixedCenterX;
        centerY = m_fixedCenterY;
    }
    CImage* frame = GetCurrentFrame();
    if (!frame) {
        return 0;
    }
    frame->RenderFrame(target, centerX, centerY, 0);
    m_hitLeft = centerX - frame->m_anchorX;
    m_hitRight = centerX + frame->m_anchorX;
    m_hitTop = centerY - frame->m_anchorY;
    m_hitBottom = centerY + frame->m_anchorY;
    return 1;
}
RVA(0x00185c30, 0x1b)
CDDrawWorker* CAnimatedMenuItem::GetStateAnimation() {
    switch (m_state) {
        case MENUSTATE_NORMAL:
            return m_normalAnimation;
        case MENUSTATE_SELECTED:
            return m_selectedAnimation;
        case MENUSTATE_DISABLED:
            return m_disabledAnimation;
    }
    return NULL;
}

RVA(0x00185c50, 0x4d)
CImage* CAnimatedMenuItem::GetCurrentFrame() {
    CDDrawWorker* animation = GetStateAnimation();
    if (!animation) {
        return NULL;
    }

    CImage* frame = animation->GetAt(m_frameIndex);
    if (frame == NULL) {
        m_frameIndex = animation->m_minIndex;
        frame = animation->GetAt(m_frameIndex);
    }
    return frame;
}
RVA(0x00185ca0, 0x4e)
i32 CAnimatedMenuItem::AdvanceFrame() {
    if (!GetCurrentFrame()) {
        return 0;
    }
    m_frameIndex = m_frameIndex + 1;
    if (HAS(m_flags, MENU_ITEM_HOLD_FINAL_ANIMATION_FRAME)) {
        CDDrawWorker* animation = GetStateAnimation();
        if (animation) {
            if (m_frameIndex > animation->m_maxIndex) {
                m_frameIndex = m_frameIndex - 1;
                return 1;
            }
        }
    }
    return GetCurrentFrame() != NULL;
}

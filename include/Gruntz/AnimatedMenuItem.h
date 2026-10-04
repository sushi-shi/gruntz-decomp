#ifndef GRUNTZ_ANIMATED_MENU_ITEM_H
#define GRUNTZ_ANIMATED_MENU_ITEM_H

#include <string>

#include <Ints.h>

#include <Enums.h>
#include <Gruntz/MenuItem.h>
#include <Image/CImage.h>
#include <Image/ImageSet.h>
#include <Ints.h>

GZ_ENUM_FORWARD(MenuItemState);

class CMenuPage;

class CAnimatedMenuItem : public CMenuItem {
public:
    CAnimatedMenuItem();
    virtual ~CAnimatedMenuItem()   {
        Cleanup();
    }
    virtual i32 Init(
        CMenuPage* page,
        const std::string& name,
        const std::string& animationKey,
        i32 commandId,
        const std::string& targetPageKey,
        GZ_ENUM_PARAM(MenuItemFlags, i32) flags
    )  ;

    virtual void Reset()   {
        m_framePeriodMs = 0x64;
        m_normalAnimation = NULL;
        m_selectedAnimation = NULL;
        m_disabledAnimation = NULL;
        m_frameIndex = 0;
        m_frameTimerMs = 0;
    }
    virtual i32 GetFrameHeight()  ;
    virtual i32 GetFrameWidth()  ;

    virtual void SetState(MenuItemState state)   {
        i32 framePeriodMs = m_framePeriodMs;
        m_state = state;
        m_frameIndex = 0;
        m_frameTimerMs = framePeriodMs;
    }
    virtual i32 Update(u32 deltaMs)  ;
    virtual i32 DrawAt(CDDrawSurfacePair* target, i32 centerX, i32 centerY)  ;

    virtual i32 UsesStateAnimations()   {
        return 1;
    }
    virtual void SetFramePeriod(i32 framePeriodMs);

    CDDrawWorker* GetStateAnimation();
    CImage* GetCurrentFrame();
    i32 AdvanceFrame();

    CDDrawWorker* m_normalAnimation;
    CDDrawWorker* m_selectedAnimation;
    CDDrawWorker* m_disabledAnimation;
    i32 m_frameIndex;
    i32 m_frameTimerMs;
    i32 m_framePeriodMs;
};

inline void CAnimatedMenuItem::SetFramePeriod(i32 framePeriodMs) {
    m_framePeriodMs = framePeriodMs;
}

inline CAnimatedMenuItem::CAnimatedMenuItem() {
    m_normalAnimation = NULL;
    m_selectedAnimation = NULL;
    m_disabledAnimation = NULL;
    m_frameIndex = 0;
    m_frameTimerMs = 0;
    SetFramePeriod(0x64);
}

#endif

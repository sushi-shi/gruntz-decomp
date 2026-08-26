#include <rva.h>

#include <Gruntz/MenuTree.h>

#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSubMgrPagesInline.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <DDrawMgr/DirectDrawMgr.h>
#include <Dsndmgr/SoundBuffer.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/MenuPage.h>
#include <Gruntz/SoundCueRegistry.h>
#include <Gruntz/SoundState.h>
#include <Image/CImage.h>
#include <Image/ImageSet.h>
#include <Rez/FrameClock.h>
#include <Utils/MapTyped.h>
#include <Wap32/CoordUnset.h>

#include <stddef.h>

RVA(0x00182ab0, 0x7b)
i32 CMenuTree::Configure(
    CDDrawSurfaceMgr* world,
    HWND windowHandle,
    RECT* bounds,
    i32 headerGap,
    i32 rowSpacing,
    i32 wrapFlags
) {
    if (!world) {
        return 0;
    }
    m_world = world;
    m_windowHandle = windowHandle;
    m_wrapFlags = wrapFlags;
    m_headerGap = headerGap;
    m_rowSpacing = rowSpacing;
    m_activePage = NULL;
    if (bounds) {
        CopyRect(&m_bounds, bounds);
        return 1;
    }
    m_bounds.left = 0;
    m_bounds.top = 0;
    m_bounds.right = world->m_drawTarget->m_frontSurface->m_width - 1;
    m_bounds.bottom = world->m_drawTarget->m_frontSurface->m_height - 1;
    return 1;
}

RVA(0x00182b30, 0x30)
void CMenuTree::Reset() {
    ClearPages();
    INITIALIZE_MENU_TREE_MEMBERS;
}

RVA(0x00182b60, 0x3e)
void CMenuTree::ClearPages() {
    POSITION position = m_pages.GetHeadPosition();
    while (position) {
        CMenuPage* page = static_cast<CMenuPage*>(m_pages.GetNext(position));
        delete page;
    }
    m_pages.RemoveAll();
    m_activePage = NULL;
}

RVA(0x00182ba0, 0x35)
i32 CMenuTree::AddPage(CMenuPage* page) {
    if (!page) {
        return 0;
    }
    m_pages.AddTail(page);
    if (!m_activePage) {
        SetActivePage(page);
    }
    return 1;
}

RVA(0x00182be0, 0x8d)
CMenuPage* CMenuTree::FindPage(const char* pageKey) {
    POSITION position = m_pages.GetHeadPosition();
    while (position) {
        CMenuPage* page = static_cast<CMenuPage*>(m_pages.GetNext(position));
        if (page) {
            if (strcmp(page->GetPageKey(), pageKey) == 0) {
                return page;
            }
        }
    }
    return NULL;
}

RVA(0x00182c70, 0x38)
i32 CMenuTree::Update(u32 deltaMs) {
    if (!m_activePage) {
        return 0;
    }
    if (!m_activePage->UpdateItems(deltaMs)) {
        return 0;
    }
    return UpdateCursorAnimations(static_cast<i32>(deltaMs)) != 0;
}

RVA(0x00182cb0, 0x26)
i32 CMenuTree::DrawActivePage() {
    if (!m_activePage) {
        return 0;
    }
    CDDrawSurfacePair* backBuffer = m_world->m_drawTarget->m_backPair;
    if (!backBuffer) {
        return 0;
    }
    return m_activePage->Draw(backBuffer) != 0;
}

RVA(0x00182ce0, 0x36)
i32 CMenuTree::PresentFrame() {
    CDDrawSubMgrPages* drawTarget = m_world->m_drawTarget;
    FlipFrontAndRestoreOverlay(drawTarget);
    return 1;
}

RVA(0x00182d20, 0x16)
i32 CMenuTree::MoveFocusUp() {
    if (!m_activePage) {
        return 0;
    }
    return m_activePage->MoveFocusUpSequential() != 0;
}

RVA(0x00182d40, 0x16)
i32 CMenuTree::MoveFocusDown() {
    if (!m_activePage) {
        return 0;
    }
    return m_activePage->MoveFocusDownSequential() != 0;
}

RVA(0x00182d60, 0x16)
i32 CMenuTree::ActivateFocusedItem() {
    if (!m_activePage) {
        return 0;
    }
    return m_activePage->ActivateFocusedItem() != 0;
}

RVA(0x00182d80, 0x18)
i32 CMenuTree::ReturnToPreviousPage() {
    if (!m_activePage) {
        return 0;
    }
    return m_activePage->ReturnToParentPage(1) != 0;
}

RVA(0x00182da0, 0x2a)
i32 CMenuTree::SetActivePage(CMenuPage* page) {
    if (!page) {
        return 0;
    }
    m_activePage = page;
    page->PrepareForActivation();
    m_activePage->FocusInitialItem();
    return 1;
}

RVA(0x00182dd0, 0x19)
i32 CMenuTree::SetActivePageByKey(const char* pageKey) {
    return SetActivePage(FindPage(pageKey));
}

static inline CDDrawWorker* LookupWorker(CDDrawSurfaceMgr* world, LPCTSTR name) {
    CObject* foundObject = NULL;
    world->m_imageRegistry->m_workersByName.Lookup(name, foundObject);
    return static_cast<CDDrawWorker*>(foundObject);
}

RVA(0x00182df0, 0x69)
i32 CMenuTree::ConfigureLeftCursorAnimation(
    const char* animationKey,
    i32 framePeriodMs,
    i32 offsetX
) {
    if (!m_world) {
        return 0;
    }
    CDDrawWorker* animation = LookupWorker(m_world, animationKey);
    m_leftCursorAnimation = animation;
    if (!animation) {
        return 0;
    }
    m_leftCursorFrame = static_cast<CImage*>(animation->m_items.GetAt(animation->m_minIndex));
    m_leftCursorFrameIndex = animation->m_minIndex;
    m_leftCursorFramePeriodMs = framePeriodMs;
    m_leftCursorFrameTimerMs = framePeriodMs;
    m_leftCursorOffsetX = offsetX;
    return 1;
}

RVA(0x00182e60, 0x69)
i32 CMenuTree::ConfigureRightCursorAnimation(
    const char* animationKey,
    i32 framePeriodMs,
    i32 offsetX
) {
    if (!m_world) {
        return 0;
    }
    CDDrawWorker* animation = LookupWorker(m_world, animationKey);
    m_rightCursorAnimation = animation;
    if (!animation) {
        return 0;
    }
    m_rightCursorFrame = static_cast<CImage*>(animation->m_items.GetAt(animation->m_minIndex));
    m_rightCursorFrameIndex = animation->m_minIndex;
    m_rightCursorFramePeriodMs = framePeriodMs;
    m_rightCursorFrameTimerMs = framePeriodMs;
    m_rightCursorOffsetX = offsetX;
    return 1;
}

// @early-stop
RVA(0x00182ed0, 0xbc)
i32 CMenuTree::UpdateCursorAnimations(i32 deltaMs) {
    CDDrawWorker* leftAnimation = m_leftCursorAnimation;
    if (leftAnimation) {
        if (static_cast<u32>(m_leftCursorFrameTimerMs) > static_cast<u32>(deltaMs)) {
            m_leftCursorFrameTimerMs -= deltaMs;
        } else {
            m_leftCursorFrameTimerMs = m_leftCursorFramePeriodMs;
            i32 frameIndex = m_leftCursorFrameIndex + 1;
            m_leftCursorFrameIndex = frameIndex;
            CImage* frame = leftAnimation->GetAt(frameIndex);
            m_leftCursorFrame = frame;
            if (frame == NULL) {
                m_leftCursorFrame =
                    static_cast<CImage*>(leftAnimation->m_items.GetAt(leftAnimation->m_minIndex));
                m_leftCursorFrameIndex = leftAnimation->m_minIndex;
            }
        }
    }
    CDDrawWorker* rightAnimation = m_rightCursorAnimation;
    if (rightAnimation) {
        if (static_cast<u32>(m_rightCursorFrameTimerMs) > static_cast<u32>(deltaMs)) {
            m_rightCursorFrameTimerMs -= deltaMs;
            return 1;
        }
        m_rightCursorFrameTimerMs = m_rightCursorFramePeriodMs;
        i32 frameIndex = m_rightCursorFrameIndex + 1;
        m_rightCursorFrameIndex = frameIndex;
        CImage* frame = rightAnimation->GetAt(frameIndex);
        m_rightCursorFrame = frame;
        if (frame == NULL) {
            m_rightCursorFrame =
                static_cast<CImage*>(rightAnimation->m_items.GetAt(rightAnimation->m_minIndex));
            m_rightCursorFrameIndex = rightAnimation->m_minIndex;
        }
    }
    return 1;
}

// @early-stop
RVA(0x00182f90, 0x92)
i32 CMenuTree::DrawFocusCursors(
    CDDrawSurfacePair* target,
    CMenuItem* item,
    i32 defaultCenterX,
    i32 defaultCenterY
) {
    if (!item) {
        return 0;
    }
    i32 itemCenterX, itemCenterY;
    if (item->m_fixedCenterX != UNINIT_FILL) {
        itemCenterY = item->m_fixedCenterY;
        itemCenterX = item->m_fixedCenterX;
    } else {
        itemCenterY = defaultCenterY;
        itemCenterX = defaultCenterX;
    }
    if (m_leftCursorFrame) {
        i32 cursorX = -(item->GetFrameWidth() / 2) - m_leftCursorOffsetX + itemCenterX;
        m_leftCursorFrame->RenderFrame(target, cursorX, itemCenterY, 0);
    }
    if (m_rightCursorFrame) {
        i32 cursorX = item->GetFrameWidth() / 2 + m_rightCursorOffsetX + itemCenterX;
        m_rightCursorFrame->RenderFrame(target, cursorX, itemCenterY, 0);
    }
    return 1;
}

static __inline i32 PlayMenuCue(SoundCueRegistry* soundRegistry, const char* cueKey) {
    if (!soundRegistry->m_silentMode) {
        SoundCue* foundCue = NULL;
        MapLookup(soundRegistry->m_cues, cueKey, foundCue);
        SoundCue* cue = foundCue;
        if (cue != NULL) {
            b32 soundEnabled = g_soundEnabled;
            i32 volumePercent = g_soundVolumePercent;
            if (soundEnabled != false) {
                i32 cueTimeMs = g_soundCueTimeMs;
                u32 elapsedMs =
                    static_cast<u32>(cueTimeMs) - static_cast<u32>(cue->m_lastPlayTimeMs);
                if (elapsedMs >= static_cast<u32>(cue->m_replayDelayMs)) {
                    cue->m_lastPlayTimeMs = cueTimeMs;
                    return cue->m_sound->AcquireAndPlay(volumePercent, 0, 0, false);
                }
            }
        }
    }
    return 0;
}

RVA(0x00183030, 0x7b)
i32 CMenuTree::PlayFocusSound() {
    if (m_focusSoundKey.GetLength() == 0) {
        return 0;
    }
    return PlayMenuCue(m_world->m_soundRegistry, m_focusSoundKey);
}

RVA(0x001830b0, 0x7b)
i32 CMenuTree::PlayActivationSound() {
    if (m_activationSoundKey.GetLength() == 0) {
        return 0;
    }
    return PlayMenuCue(m_world->m_soundRegistry, m_activationSoundKey);
}

RVA(0x00183130, 0x16)
i32 CMenuTree::MoveFocusLeft() {
    if (!m_activePage) {
        return 0;
    }
    return m_activePage->MoveFocusLeftColumn() != 0;
}

RVA(0x00183150, 0x16)
i32 CMenuTree::MoveFocusRight() {
    if (!m_activePage) {
        return 0;
    }
    return m_activePage->MoveFocusRightColumn() != 0;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00183170, 0x24)
i32 CMenuTree::FocusItemAt(i32 screenX, i32 screenY) {
    if (!m_activePage) {
        return 0;
    }
    return m_activePage->FocusItemAt(screenX, screenY) != 0;
}

RVA(0x001831a0, 0x24)
i32 CMenuTree::ClickAt(i32 screenX, i32 screenY) {
    CMenuPage* page = m_activePage;
    if (!page) {
        return 0;
    }
    return page->ClickAt(screenX, screenY) != 0;
}

RVA(0x001831d0, 0x16)
i32 CMenuTree::MoveFocusLeftFollowingLinks() {
    CMenuPage* page = m_activePage;
    if (!page) {
        return 0;
    }
    return page->MoveFocusLeft() != 0;
}

RVA(0x001831f0, 0x16)
i32 CMenuTree::MoveFocusRightFollowingLinks() {
    CMenuPage* page = m_activePage;
    if (!page) {
        return 0;
    }
    return page->MoveFocusRight() != 0;
}

RVA(0x00183210, 0x16)
i32 CMenuTree::MoveFocusUpFollowingLinks() {
    CMenuPage* page = m_activePage;
    if (!page) {
        return 0;
    }
    return page->MoveFocusUp() != 0;
}

RVA(0x00183230, 0x16)
i32 CMenuTree::MoveFocusDownFollowingLinks() {
    CMenuPage* page = m_activePage;
    if (!page) {
        return 0;
    }
    return page->MoveFocusDown() != 0;
}

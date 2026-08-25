#ifndef GRUNTZ_MENU_TREE_H
#define GRUNTZ_MENU_TREE_H

#include <rva.h>

#include <Mfc.h>

#include <Ints.h>

class CMenuPage;
class CMenuItem;

class CDDrawSurfaceMgr;
class CDDrawSurfacePair;

class CDDrawWorker;

class CImage;

class CMenuTree {
public:
    void InitializeMembers();

    i32 Configure(
        CDDrawSurfaceMgr* world,
        HWND windowHandle,
        RECT* bounds,
        i32 headerGap,
        i32 rowSpacing,
        i32 wrapFlags
    );
    void Reset();
    void ClearPages();

    CMenuPage* FindPage(const char* pageKey);
    CMenuTree() {
        InitializeMembers();
    }
    ~CMenuTree();
    i32 AddPage(CMenuPage* page);
    i32 SetActivePage(CMenuPage* page);
    i32 SetActivePageByKey(const char* pageKey);
    i32 ConfigureLeftCursorAnimation(const char* animationKey, i32 framePeriodMs, i32 offsetX);
    i32 ConfigureRightCursorAnimation(const char* animationKey, i32 framePeriodMs, i32 offsetX);
    i32 UpdateCursorAnimations(i32 deltaMs);
    i32 DrawFocusCursors(
        CDDrawSurfacePair* target,
        CMenuItem* item,
        i32 defaultCenterX,
        i32 defaultCenterY
    );
    i32 PlayFocusSound();
    i32 PlayActivationSound();
    i32 FocusItemAt(i32 screenX, i32 screenY);
    i32 ClickAt(i32 screenX, i32 screenY);
    i32 MoveFocusUpFollowingLinks();
    i32 MoveFocusDownFollowingLinks();
    i32 MoveFocusLeftFollowingLinks();
    i32 MoveFocusRightFollowingLinks();

    i32 Update(u32 deltaMs);
    i32 DrawActivePage();
    i32 PresentFrame();
    i32 MoveFocusDown();
    i32 MoveFocusUp();
    i32 ActivateFocusedItem();
    i32 ReturnToPreviousPage();
    i32 MoveFocusLeft();
    i32 MoveFocusRight();

    CDDrawSurfaceMgr* m_world;

    HWND m_windowHandle;

    RECT m_bounds;
    i32 m_headerGap;
    i32 m_rowSpacing;

    i32 m_wrapFlags;
    CPtrList m_pages;
    CMenuPage* m_activePage;
    CString m_focusSoundKey;
    CString m_activationSoundKey;
    CDDrawWorker* m_leftCursorAnimation;
    CImage* m_leftCursorFrame;
    i32 m_leftCursorFramePeriodMs;
    i32 m_leftCursorFrameTimerMs;
    i32 m_leftCursorOffsetX;
    i32 m_leftCursorFrameIndex;
    CDDrawWorker* m_rightCursorAnimation;
    CImage* m_rightCursorFrame;
    i32 m_rightCursorFramePeriodMs;
    i32 m_rightCursorFrameTimerMs;
    i32 m_rightCursorOffsetX;
    i32 m_rightCursorFrameIndex;
};

#define INITIALIZE_MENU_TREE_MEMBERS                                                               \
    m_world = NULL;                                                                                \
    m_windowHandle = NULL;                                                                         \
    m_activePage = NULL;                                                                           \
    m_leftCursorAnimation = NULL;                                                                  \
    m_rightCursorAnimation = NULL;                                                                 \
    m_leftCursorFrame = NULL;                                                                      \
    m_rightCursorFrame = NULL;                                                                     \
    m_focusSoundKey.Empty();                                                                       \
    m_activationSoundKey.Empty()

#endif // GRUNTZ_MENU_TREE_H

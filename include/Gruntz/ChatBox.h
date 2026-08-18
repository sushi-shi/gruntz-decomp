#ifndef GRUNTZ_CHATBOX_H
#define GRUNTZ_CHATBOX_H

#include <rva.h>

#include <Mfc.h>

#include <Ints.h>

class CMenuPage;
class CMenuItem;

class CDDrawSurfaceMgr;
class CDDrawSurfacePair;

class CDDrawWorker;

class CImage;

class CChatBox {
public:
    void Init();

    i32 InitRegion(CDDrawSurfaceMgr* src, HWND wnd, RECT* rc, i32 d, i32 e, i32 f);
    void Reset();
    void Clear();

    CMenuPage* Find(const char* s);
    // Retail's ctor body: `new CChatBox` at 0x9ff85 sets unwind state 3 after the
    // last CString member and then calls Init() inside the same protected region.
    CChatBox() {
        Init();
    }
    ~CChatBox();
    i32 AddNode(CMenuPage* node);
    i32 AttachNode(CMenuPage* n);
    i32 ReplaceNode(const char* key);
    i32 ConfigureLeftCursorAnimation(const char* key, i32 x, i32 y);
    i32 ConfigureRightCursorAnimation(const char* key, i32 x, i32 y);
    i32 Step(i32 dt);
    i32 Draw(CDDrawSurfacePair* target, CMenuItem* sprite, i32 x, i32 y);
    i32 PlayFocusSound();
    i32 PlayActivationSound();
    i32 FocusSelect(i32 x, i32 y);
    i32 ClickAt(i32 x, i32 y);
    i32 MoveFocusUpFollowingLinks();
    i32 MoveFocusDownFollowingLinks();
    i32 MoveFocusLeftFollowingLinks();
    i32 MoveFocusRightFollowingLinks();

    i32 Step(u32 dt);
    i32 Pre();
    i32 Post();
    i32 MoveFocusDown();
    i32 MoveFocusUp();
    i32 ActivateFocusedItem();
    i32 ReturnToPreviousPage();
    i32 MoveFocusLeft();
    i32 MoveFocusRight();

    CDDrawSurfaceMgr* m_page;

    HWND m_wnd;

    RECT m_rect8;
    i32 m_headGap;
    i32 m_rowSpacing;

    i32 m_wrapFlag;
    CPtrList m_nodeList;
    CMenuPage* m_activeNode;
    CString m_row0Key;
    CString m_row1Key;
    CDDrawWorker* m_row0Anim;
    CImage* m_row0Frame;
    i32 m_row0Period;
    i32 m_row0Timer;
    i32 m_row0Offset;
    i32 m_row0FrameIdx;
    CDDrawWorker* m_row1Anim;
    CImage* m_row1Frame;
    i32 m_row1Period;
    i32 m_row1Timer;
    i32 m_row1Offset;
    i32 m_row1FrameIdx;
};

#endif // GRUNTZ_CHATBOX_H

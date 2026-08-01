#ifndef GRUNTZ_CHATBOX_H
#define GRUNTZ_CHATBOX_H

#include <Ints.h>
#include <rva.h>

#include <Mfc.h>

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
    ~CChatBox();
    i32 AddNode(CMenuPage* node);
    i32 AttachNode(CMenuPage* n);
    i32 ReplaceNode(const char* key);
    i32 AdvanceRow0(void* key, i32 x, i32 y);
    i32 AdvanceRow1(void* key, i32 x, i32 y);
    i32 Step(i32 dt);
    i32 Draw(CDDrawSurfacePair* target, CMenuItem* sprite, i32 x, i32 y);
    i32 ScrollRow0();
    i32 ScrollRow1();
    i32 FocusSelect(i32 x, i32 y);
    i32 HitTest0(i32 x, i32 y);
    i32 HitTest1();
    i32 HitTest2();
    i32 HitTest3();
    i32 HitTest4();

    i32 Step(u32 dt);
    i32 Pre();
    i32 Post();
    i32 OnFlag40000000();
    i32 OnFlag80000000();
    i32 OnFlag00000003();
    i32 OnFlag00000100();
    i32 OnFlag10000000();
    i32 OnFlag20000000();

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
SIZE_UNKNOWN();

#endif // GRUNTZ_CHATBOX_H

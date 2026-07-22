#ifndef GRUNTZ_CHATBOX_H
#define GRUNTZ_CHATBOX_H

#include <Ints.h>
#include <rva.h>

#include <Mfc.h>

class CMenuPage;

class CDDrawSurfaceMgr;

class CDDrawWorker;             // CImageSet IS CDDrawWorker (<DDrawMgr/DDrawWorker.h>);
typedef CDDrawWorker CImageSet; // identical repeat of ImageSet.h's typedef - legal, and
class CImage; // Image/CImage.h

class CChatBox {
public:
    void Init(); // 0xa0280 - re-zero the rows (no list teardown)
    // 0x182ab0 (__thiscall, defined in MenuStateAssets.cpp): seed the box from the
    // resource holder + the game window's HWND, copy/derive the region rect. The ex
    // MenuRegion view of this same `this` is dissolved onto CChatBox.
    i32 InitRegion(CDDrawSurfaceMgr* src, HWND wnd, RECT* rc, i32 d, i32 e, i32 f); // 0x182ab0
    void Reset();                             // 0x182b30 - free the node list, re-zero the rows
    void Clear();                             // 0x182b60 - free node payloads, empty the list
    i32 Find(const char* s);                  // 0x182be0 - find a node by string key
    ~CChatBox();                              // 0x182bxx teardown
    i32 AddNode(void* node);                  // 0x182ba0
    i32 AttachNode(void* n);                  // 0x182da0
    i32 ReplaceNode(void* n);                 // 0x182dd0
    i32 AdvanceRow0(void* key, i32 x, i32 y); // 0x182df0
    i32 AdvanceRow1(void* key, i32 x, i32 y); // 0x182e60
    i32 Step(i32 dt);                         // 0x182ed0
    i32 Draw(i32 a0, i32 sprite, i32 arg2, i32 arg3); // 0x182f90
    i32 ScrollRow0();                                 // 0x183030
    i32 ScrollRow1();                                 // 0x1830b0
    i32 FocusSelect(i32 x, i32 y);                    // 0x183170 (page FocusAndSelect(x,y))
    i32 HitTest0(i32 x, i32 y);                       // 0x1831a0
    i32 HitTest1();                                   // 0x183210
    i32 HitTest2();                                   // 0x183230
    i32 HitTest3();                                   // 0x1831d0
    i32 HitTest4();                                   // 0x1831f0

    // The front-end menu drive (the menu state's m_1b4 object IS a CChatBox): each
    // per-frame entity-flag scan fires one of these, then Step/Pre/Post run the
    // tail. They forward to the owned menu page (m_activeNode, a CMenuPage)
    // navigation and the surface set hung off the owner (m_page). __thiscall.
    i32 Step(u32 dt);     // 0x182c70  NotifyAll(page) + inner Step(i32)
    i32 Pre();            // 0x182cb0  page Layout
    i32 Post();           // 0x182ce0  Flip + BltFast the menu surfaces
    i32 OnFlag40000000(); // 0x182d20  page FocusNext
    i32 OnFlag80000000(); // 0x182d40  page FocusPrev
    i32 OnFlag00000003(); // 0x182d60  page Activate
    i32 OnFlag00000100(); // 0x182d80  page Switch(1)
    i32 OnFlag10000000(); // 0x183130  page FocusBackwardN
    i32 OnFlag20000000(); // 0x183150  page FocusForwardN

    CDDrawSurfaceMgr* m_page; // +0x00 parent/page back pointer == the CState::m_c
                              //       resource holder (MenuRegion::Init @0x182ab0 stores
                              //       a CDDrawSurfaceMgr*): the box drives its +0x04
                              //       render set, +0x10 image registry, +0x28 cue host.
    HWND m_wnd;               // +0x04  the command-target window (InitRegion arg; ex the CMenuItemHost view's m_wnd)
    // +0x08..+0x1f: the region rect + 2 scalars (retail Init @0x182ab0 CopyRects the
    // rect arg into +0x08 and stores the two i32 args at +0x18/+0x1c).
    RECT m_rect8; // +0x08  region rect (left/top/right/bottom)
    i32 m_headGap;    // +0x18  page head-item gap (CMenuPage::Configure -> m_headGap)
    i32 m_rowSpacing; // +0x1c  page per-item row spacing (Configure -> m_rowSpacing)
    // +0x20: focus-wrap-enable. A 4-byte field: Init stores the whole DWORD arg;
    // CanWrap reads the low byte SIGNED (`movsx eax, byte [host+0x20]; and eax,1`).
    i32 m_wrapFlag;
    CPtrList m_nodeList;     // +0x24 message-node list (head at +0x28)
    CMenuPage* m_activeNode; // +0x40 queued/active node slot (a CMenuPage)
    CString m_row0Key;       // +0x44 row0 font/asset key
    CString m_row1Key;       // +0x48 row1 font/asset key
    CImageSet* m_row0Anim;   // +0x4c row0 current message/animation record (CImageSet)
    CImage* m_row0Frame;     // +0x50 row0 current frame drawable (Blit)
    i32 m_row0Period;        // +0x54 row0 frame-advance reload period
    i32 m_row0Timer;         // +0x58 row0 frame-advance countdown
    i32 m_row0Offset;        // +0x5c row0 horizontal draw offset
    i32 m_row0FrameIdx;      // +0x60 row0 index into the frame table
    CImageSet* m_row1Anim;   // +0x64 row1 current message/animation record (CImageSet)
    CImage* m_row1Frame;     // +0x68 row1 current frame drawable (Blit)
    i32 m_row1Period;        // +0x6c row1 frame-advance reload period
    i32 m_row1Timer;         // +0x70 row1 frame-advance countdown
    i32 m_row1Offset;        // +0x74 row1 horizontal draw offset
    i32 m_row1FrameIdx;      // +0x78 row1 index into the frame table
};
SIZE_UNKNOWN();

#endif // GRUNTZ_CHATBOX_H

// ChatBox.h - a two-row scrolling on-screen text/chat display (C:\Proj\Gruntz).
//
// Trace-discovered as Region_182ab0; the engine identity (LoadChatBoxSprite +
// the per-row frame-animation state + the message-node list) names it a
// chat/marquee box. Two text rows, each with: a CString font/asset key
// (m_row0Key / m_row1Key), a "current message node"/animation record pointer
// (m_row0Anim / m_row1Anim), and per-row frame-animation state (current frame
// handle + reload period + countdown timer + draw offset + frame index). Owns a
// CPtrList of message nodes at m_nodeList, plus a parent/page back pointer at
// m_page and a queued/active node slot at m_activeNode.
//
// Field names are recovered from the member writes in the ctor/methods; only the
// offsets + code bytes are load-bearing (rename is /O2 name-independent).
#ifndef GRUNTZ_CHATBOX_H
#define GRUNTZ_CHATBOX_H

#include <Ints.h>
#include <rva.h>

#include <Mfc.h>

// The active/queued node slot IS a CMenuPage (MenuPage.h): the node accessors this
// box dispatches into (dtor 0x183250, ReleaseAll 0x183990, RestoreFocus 0x1839d0,
// Click 0x1840a0, SelectForward 0x1843f0, SelectBackward 0x1844d0, SelectFwd2
// 0x184230, SelectBack2 0x184310, GetKey 0x1832d0) are the same RVAs as CMenuPage's.
class CMenuPage;

// The on-screen "page"/owner object reached through m_page (render surface set at
// +0x04, key->node catalog at +0x10, sprite roster at +0x28). Defined in ChatBox.cpp.
struct CChatPage;

// Per-row animation record (frame table + clamp range) and per-row frame drawable
// (Blit 0x153790); both defined in ChatBox.cpp.
struct CChatAnim;
struct CChatFrame;

// ---------------------------------------------------------------------------
// CChatBox
// ---------------------------------------------------------------------------
class CChatBox {
public:
    void Init();                              // 0xa0280 - re-zero the rows (no list teardown)
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

    CChatPage* m_page; // +0x00 parent/page back pointer (render set / catalog / roster)
    i32 m_4;           // +0x04  (only ever zeroed; role unproven)
    char m_pad8[0x24 - 0x08];
    CPtrList m_nodeList;     // +0x24 message-node list (head at +0x28)
    CMenuPage* m_activeNode; // +0x40 queued/active node slot (a CMenuPage)
    CString m_row0Key;       // +0x44 row0 font/asset key
    CString m_row1Key;       // +0x48 row1 font/asset key
    CChatAnim* m_row0Anim;   // +0x4c row0 current message/animation record
    CChatFrame* m_row0Frame; // +0x50 row0 current frame drawable (Blit)
    i32 m_row0Period;        // +0x54 row0 frame-advance reload period
    i32 m_row0Timer;         // +0x58 row0 frame-advance countdown
    i32 m_row0Offset;        // +0x5c row0 horizontal draw offset
    i32 m_row0FrameIdx;      // +0x60 row0 index into the frame table
    CChatAnim* m_row1Anim;   // +0x64 row1 current message/animation record
    CChatFrame* m_row1Frame; // +0x68 row1 current frame drawable (Blit)
    i32 m_row1Period;        // +0x6c row1 frame-advance reload period
    i32 m_row1Timer;         // +0x70 row1 frame-advance countdown
    i32 m_row1Offset;        // +0x74 row1 horizontal draw offset
    i32 m_row1FrameIdx;      // +0x78 row1 index into the frame table
};
SIZE_UNKNOWN(CChatBox);

#endif // GRUNTZ_CHATBOX_H

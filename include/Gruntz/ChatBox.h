// ChatBox.h - a two-row scrolling on-screen text/chat display (C:\Proj\Gruntz).
//
// Trace-discovered as Region_182ab0; the engine identity (LoadChatBoxSprite +
// the per-row scroll state + the message-node list) names it a chat/marquee box.
// Two text rows, each with: a CString font/asset key (m_44 / m_48), a "current
// message node" pointer (m_4c / m_64), and per-row horizontal-scroll counters
// (m_50..m_60 row0, m_68..m_78 row1). Owns a CObList of message nodes at m_24
// (head cached in the node-walk loops as m_28 = list+4), plus a parent/page back
// pointer at m_0 and a queued-node slot at m_40.
//
// Only offsets + code bytes are load-bearing; field names are placeholders
// recovered from the member writes in the ctor/methods.
#ifndef GRUNTZ_CHATBOX_H
#define GRUNTZ_CHATBOX_H

#include <Ints.h>
#include <rva.h>

#include <Mfc.h>

// ---------------------------------------------------------------------------
// The message-node class (defined in another TU - its ctor/dtor/accessors at
// 0x1833a0 / 0x183990 / 0x1839d0 / 0x1840a0 / 0x1843f0 / 0x1844d0 / ...). Modeled
// here only enough to name the methods our ChatBox rows dispatch into, so their
// `mov ecx,this+0x40; call rel32` is reloc-masked. __thiscall by default.
// ---------------------------------------------------------------------------
class CChatNode;

// The on-screen "page"/owner object reached through m_0 (its message-text table
// is read in AdvanceRow0/AdvanceRow1 via m_0->m_28). External - opaque view.
struct CChatPage;

// ---------------------------------------------------------------------------
// CChatBox
// ---------------------------------------------------------------------------
class CChatNode;

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
    // tail. They forward to the owned menu page (m_40, a CMenuPage) navigation and
    // the surface set hung off the owner (m_0). __thiscall.
    i32 Step(u32 dt);     // 0x182c70  NotifyAll(page) + inner Step(i32)
    i32 Pre();            // 0x182cb0  page Layout
    i32 Post();           // 0x182ce0  Flip + BltFast the menu surfaces
    i32 OnFlag40000000(); // 0x182d20  page FocusNext
    i32 OnFlag80000000(); // 0x182d40  page FocusPrev
    i32 OnFlag00000003(); // 0x182d60  page Activate
    i32 OnFlag00000100(); // 0x182d80  page Switch(1)
    i32 OnFlag10000000(); // 0x183130  page FocusBackwardN
    i32 OnFlag20000000(); // 0x183150  page FocusForwardN

    void* m_0; // +0x00 parent/page back pointer
    i32 m_4;   // +0x04
    char m_pad8[0x24 - 0x08];
    CPtrList m_24; // +0x24 message-node list (vptr+6 fields = 0x1c, head at +0x28)
    void* m_40;    // +0x40 queued/active node slot
    CString m_44;  // +0x44 row0 font/asset key
    CString m_48;  // +0x48 row1 font/asset key
    void* m_4c;    // +0x4c row0 current node
    i32 m_50;      // +0x50
    i32 m_54;      // +0x54
    i32 m_58;      // +0x58
    i32 m_5c;      // +0x5c
    i32 m_60;      // +0x60
    void* m_64;    // +0x64 row1 current node
    i32 m_68;      // +0x68
    i32 m_6c;      // +0x6c
    i32 m_70;      // +0x70
    i32 m_74;      // +0x74
    i32 m_78;      // +0x78
};
SIZE_UNKNOWN(CChatBox);

#endif // GRUNTZ_CHATBOX_H
